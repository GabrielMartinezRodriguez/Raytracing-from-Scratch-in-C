/*
** rt_gpu: el mismo raytracer, ejecutado en la GPU con Metal.
**
**   ./rt_gpu escena.rt                 ventana en tiempo real
**   ./rt_gpu escena.rt --save nombre   guarda nombre.png y sale
**   ./rt_gpu escena.rt --bench         mide el tiempo por frame y sale
**
** Controles: WASD mover, Q/E bajar/subir, arrastrar raton mirar,
** shift correr, flechas izq/der cambiar de camara, 1-4 antialiasing,
** ESC salir.
*/

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <MetalFX/MetalFX.h>
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "scene_export.h"
#include "bvh.h"
#include "obj_loader.h"
#include "shader_src.h"

enum { KEY_A = 0, KEY_S = 1, KEY_D = 2, KEY_Q = 12, KEY_W = 13, KEY_E = 14,
	KEY_1 = 18, KEY_2 = 19, KEY_3 = 20, KEY_4 = 21, KEY_ESC = 53,
	KEY_LEFT = 123, KEY_RIGHT = 124, KEY_T = 17, KEY_G = 5 };

static simd_float3	xyz(t_vec4 v)
{
	return (simd_make_float3(v.x, v.y, v.z));
}

static t_vec4		vec4(simd_float3 v)
{
	return (simd_make_float4(v.x, v.y, v.z, 0));
}

@interface Renderer : NSObject <MTKViewDelegate>
@property (nonatomic) int samples;
@property (nonatomic) float moveSpeed;
@property (nonatomic) bool temporalOn;
@property (nonatomic) bool giOn;
- (instancetype)initWithScene:(t_gpu_scene *)scene mesh:(t_gpu_mesh *)mesh;
- (void)buildMesh:(t_gpu_mesh *)mesh alpha:(id<MTLFunction>)alpha_fn;
- (void)buildInstances:(t_gpu_mesh *)mesh;
- (void)encodeTo:(id<MTLTexture>)texture buffer:(id<MTLCommandBuffer>)cmd;
- (id<MTLTexture>)offscreenTexture;
- (void)useCamera:(int)index;
- (void)keyDown:(unsigned short)key;
- (void)keyUp:(unsigned short)key;
- (void)lookBy:(float)dx :(float)dy;
- (void)setFast:(bool)fast;
- (void)pathAt:(float)t frame:(unsigned int)frame;
- (double)renderOnce:(id<MTLTexture>)texture;
- (void)encodeTemporalTo:(id<MTLTexture>)texture scale:(float)scale buffer:(id<MTLCommandBuffer>)cmd;
- (id<MTLTexture>)privateTexture:(MTLPixelFormat)fmt w:(NSUInteger)w h:(NSUInteger)h usage:(MTLTextureUsage)usage;
- (void)encodeGITo:(id<MTLTexture>)texture scale:(float)scale buffer:(id<MTLCommandBuffer>)cmd;
- (void)giReferenceAt:(float)t samples:(int)spp size:(MTLSize)size name:(const char *)name;
- (void)encodePass:(int)pass to:(id<MTLTexture>)texture base:(id<MTLTexture>)base buffer:(id<MTLCommandBuffer>)cmd;
- (void)cameraMatrices:(simd_float4x4 *)view proj:(simd_float4x4 *)proj aspect:(float)aspect;
- (t_gpu_frame)frame;
- (void)buildAccel:(id<MTLFunction>)shape_fn;
@end

static void		save_png(id<MTLTexture> texture, const char *name);

static float	aces(float x)
{
	float v = (x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f);

	return (v < 0 ? 0 : v > 1 ? 1 : v);
}

@implementation Renderer
{
	t_gpu_scene					*_scene;
	id<MTLDevice>				_device;
	id<MTLCommandQueue>			_queue;
	id<MTLComputePipelineState>	_pipeline;
	id<MTLBuffer>				_objects;
	id<MTLBuffer>				_lights;
	id<MTLBuffer>				_nodes;
	id<MTLTexture>				_base;
	id<MTLFXSpatialScaler>		_scaler;
	id<MTLFXTemporalScaler>		_temporal;
	id<MTLFXTemporalDenoisedScaler>	_denoiser;
	id<MTLTexture>				_gColor;
	id<MTLTexture>				_gDepth;
	id<MTLTexture>				_gMotion;
	id<MTLTexture>				_gAlbedo;
	id<MTLTexture>				_gNormal;
	id<MTLTexture>				_gRough;
	id<MTLTexture>				_gSpec;
	id<MTLTexture>				_gOut;
	id<MTLTexture>				_tColor;
	id<MTLTexture>				_tDepth;
	id<MTLTexture>				_tMotion;
	id<MTLTexture>				_tOut;
	t_gpu_frame					_prev;
	simd_float2					_lastJitter;
	bool						_hasPrev;
	id<MTLTexture>				_lowres;
	int							_nplanes;
	id<MTLAccelerationStructure>	_accel;
	id<MTLIntersectionFunctionTable>	_table;
	id<MTLAccelerationStructure>	_meshAccel;
	id<MTLAccelerationStructure>	_meshPrim;
	id<MTLBuffer>				_insts;
	int							_ninsts;
	id<MTLIntersectionFunctionTable>	_meshTable;
	id<MTLBuffer>				_meshPos;
	id<MTLBuffer>				_tris;
	id<MTLBuffer>				_mats;
	id<MTLBuffer>				_meshBase;
	NSArray						*_textures;
	int							_ntris;
	int							_nopaque;
	simd_float3					_position;
	float						_yaw;
	float						_pitch;
	float						_fov;
	int							_camera;
	bool						_keys[256];
	bool						_fast;
	bool						_hardware;
	bool						_city;
	CFTimeInterval				_last;
	CFTimeInterval				_fpsStart;
	int							_frames;
	unsigned int				_frame;
	simd_float3					_pathStart;
	float						_pathYaw;
	simd_float4					_sceneMin;
	simd_float4					_sceneMax;
	double						_gpuMs;
}

- (instancetype)initWithScene:(t_gpu_scene *)scene mesh:(t_gpu_mesh *)mesh
{
	NSError				*error = nil;
	id<MTLLibrary>		library;
	MTLCompileOptions	*options = [MTLCompileOptions new];

	self = [super init];
	_scene = scene;
	_samples = 3;
	_moveSpeed = 20;
	_device = MTLCreateSystemDefaultDevice();
	_queue = [_device newCommandQueue];
	options.mathMode = MTLMathModeFast;
	_hardware = _device.supportsRaytracing && !(getenv("RT_HW")
		&& strcmp(getenv("RT_HW"), "0") == 0);
	_city = getenv("RT_CITY") && atoi(getenv("RT_CITY")) > 1;
	library = [_device newLibraryWithSource:[NSString stringWithFormat:@"%s%s%s",
		_hardware ? "#define HW_RT 1\n" : "", _city ? "#define MESH_INST 1\n"
		: "", g_shader_src] options:options error:&error];
	if (!library)
	{
		fprintf(stderr, "error compilando el shader:\n%s\n",
			error.localizedDescription.UTF8String);
		exit(1);
	}
	MTLComputePipelineDescriptor *pdesc = [MTLComputePipelineDescriptor new];
	pdesc.computeFunction = [library newFunctionWithName:@"render"];
	id<MTLFunction> shape_fn = nil;
	id<MTLFunction> alpha_fn = nil;
	if (mesh && mesh->ntris > 0 && !_hardware)
	{
		fprintf(stderr, "error: los modelos .obj necesitan ray tracing por "
			"hardware (no uses RT_HW=0)\n");
		exit(1);
	}
	if (_hardware)
	{
		shape_fn = [library newFunctionWithName:@"shape_hit"];
		alpha_fn = [library newFunctionWithName:@"alpha_test"];
		MTLLinkedFunctions *linked = [MTLLinkedFunctions linkedFunctions];
		linked.functions = @[shape_fn, alpha_fn];
		pdesc.linkedFunctions = linked;
	}
	_pipeline = [_device newComputePipelineStateWithDescriptor:pdesc
		options:0 reflection:nil error:&error];
	if (!_pipeline)
	{
		fprintf(stderr, "error creando el pipeline: %s\n",
			error.localizedDescription.UTF8String);
		exit(1);
	}
	printf("modo: %s\n", _hardware ? "ray tracing por hardware"
		: "BVH por software");
	/*
	** Con hardware el arbol por software no se usa: solo se separan los
	** planos (antes costaba 6 s con San Miguel).
	*/
	if (_hardware)
	{
		_nplanes = partition_planes(scene);
		_nodes = [_device newBufferWithLength:sizeof(t_gpu_node)
			options:MTLResourceStorageModeShared];
	}
	else
	{
		int nnodes;
		CFTimeInterval t0 = CACurrentMediaTime();
		t_gpu_node *nodes = build_bvh(scene, &nnodes, &_nplanes);
		printf("BVH: %d nodos en %.1f ms\n", nnodes,
			(CACurrentMediaTime() - t0) * 1000);
		_nodes = [_device newBufferWithBytes:nodes
			length:sizeof(t_gpu_node) * (nnodes + 1)
			options:MTLResourceStorageModeShared];
		free(nodes);
	}
	_objects = [_device newBufferWithBytes:scene->objects
		length:sizeof(t_gpu_object) * (scene->nobjects + 1)
		options:MTLResourceStorageModeShared];
	if (_hardware && scene->nobjects > _nplanes)
		[self buildAccel:shape_fn];
	if (_hardware && mesh && mesh->ntris > 0)
		[self buildMesh:mesh alpha:alpha_fn];
	_lights = [_device newBufferWithBytes:scene->lights
		length:sizeof(t_gpu_light) * (scene->nlights + 1)
		options:MTLResourceStorageModeShared];
	[self useCamera:0];
	return (self);
}

/*
** Una caja (AABB) por objeto no plano; la GPU construye su propio arbol
** sobre ellas y lo recorre con las unidades de ray tracing.
*/

- (void)buildAccel:(id<MTLFunction>)shape_fn
{
	int			n = _scene->nobjects - _nplanes;
	id<MTLBuffer>	boxes = [_device newBufferWithLength:
		sizeof(MTLAxisAlignedBoundingBox) * n
		options:MTLResourceStorageModeShared];
	MTLAxisAlignedBoundingBox	*b = boxes.contents;
	CFTimeInterval	t0 = CACurrentMediaTime();
	float		lo[3];
	float		hi[3];

	_sceneMin = simd_make_float4(INFINITY, INFINITY, INFINITY, 0);
	_sceneMax = simd_make_float4(-INFINITY, -INFINITY, -INFINITY, 0);
	for (int i = 0; i < n; i++)
	{
		object_bounds(&_scene->objects[_nplanes + i], lo, hi);
		_sceneMin = simd_min(_sceneMin, simd_make_float4(lo[0], lo[1], lo[2], 0));
		_sceneMax = simd_max(_sceneMax, simd_make_float4(hi[0], hi[1], hi[2], 0));
		b[i].min = MTLPackedFloat3Make(lo[0], lo[1], lo[2]);
		b[i].max = MTLPackedFloat3Make(hi[0], hi[1], hi[2]);
	}
	MTLAccelerationStructureBoundingBoxGeometryDescriptor *geo =
		[MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];
	geo.boundingBoxBuffer = boxes;
	geo.boundingBoxCount = n;
	geo.boundingBoxStride = sizeof(MTLAxisAlignedBoundingBox);
	geo.intersectionFunctionTableOffset = 0;
	geo.opaque = YES;
	MTLPrimitiveAccelerationStructureDescriptor *desc =
		[MTLPrimitiveAccelerationStructureDescriptor descriptor];
	desc.geometryDescriptors = @[geo];
	MTLAccelerationStructureSizes sizes =
		[_device accelerationStructureSizesWithDescriptor:desc];
	_accel = [_device newAccelerationStructureWithSize:
		sizes.accelerationStructureSize];
	id<MTLBuffer> scratch = [_device newBufferWithLength:
		sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
	id<MTLCommandBuffer> cmd = [_queue commandBuffer];
	id<MTLAccelerationStructureCommandEncoder> enc =
		[cmd accelerationStructureCommandEncoder];
	[enc buildAccelerationStructure:_accel descriptor:desc
		scratchBuffer:scratch scratchBufferOffset:0];
	[enc endEncoding];
	[cmd commit];
	[cmd waitUntilCompleted];
	MTLIntersectionFunctionTableDescriptor *tdesc =
		[MTLIntersectionFunctionTableDescriptor new];
	tdesc.functionCount = 1;
	_table = [_pipeline newIntersectionFunctionTableWithDescriptor:tdesc];
	[_table setFunction:[_pipeline functionHandleWithFunction:shape_fn]
		atIndex:0];
	[_table setBuffer:_objects offset:sizeof(t_gpu_object) * _nplanes
		atIndex:0];
	printf("estructura de aceleracion: %d cajas en %.1f ms\n", n,
		(CACurrentMediaTime() - t0) * 1000);
}

static uint32_t	hash_int(uint32_t x)
{
	x ^= x >> 16;
	x *= 0x7feb352dU;
	x ^= x >> 15;
	x *= 0x846ca68bU;
	return (x ^ (x >> 16));
}

static float	halton(int i, int b)
{
	float	f = 1;
	float	r = 0;

	while (i > 0)
	{
		f /= b;
		r += f * (i % b);
		i /= b;
	}
	return (r);
}

/*
** Material tal como lo lee la GPU (struct GpuMaterial del shader).
*/

typedef struct	s_host_material
{
	simd_float4		kd;
	MTLResourceID	diffuse;
	uint32_t		flags;
	uint32_t		pad;
}				t_host_material;

/*
** true si la imagen tiene algun pixel con transparencia real.
*/

static bool		has_transparency(CGImageRef img)
{
	CGImageAlphaInfo	info = CGImageGetAlphaInfo(img);
	size_t				w = CGImageGetWidth(img);
	size_t				h = CGImageGetHeight(img);
	bool				found = false;

	if (info == kCGImageAlphaNone || info == kCGImageAlphaNoneSkipLast
		|| info == kCGImageAlphaNoneSkipFirst)
		return (false);
	uint8_t *px = malloc(w * h * 4);
	CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(px, w, h, 8, w * 4, cs,
		(CGBitmapInfo)kCGImageAlphaPremultipliedLast);
	CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), img);
	for (size_t i = 3; i < w * h * 4 && !found; i += 4)
		found = px[i] < 128;
	CGContextRelease(ctx);
	CGColorSpaceRelease(cs);
	free(px);
	return (found);
}

/*
** Malla de triangulos para las unidades RT: texturas (en paralelo, con
** mipmaps), triangulos ordenados en opacos + con transparencia (estos
** llaman a alpha_test) y estructura de aceleracion de triangulos nativos.
*/

- (void)buildMesh:(t_gpu_mesh *)mesh alpha:(id<MTLFunction>)alpha_fn
{
	CFTimeInterval		t0 = CACurrentMediaTime();
	int					nm = mesh->nmats;
	NSMutableArray		*texs = [NSMutableArray arrayWithCapacity:nm];
	bool				*alpha = calloc(nm, sizeof(bool));
	id<MTLDevice>		device = _device;

	NSMutableArray *slots = [NSMutableArray arrayWithCapacity:nm];
	for (int i = 0; i < nm; i++)
		[slots addObject:[NSNull null]];
	dispatch_apply(nm, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
		^(size_t i) {
		if (!mesh->mats[i].texture[0])
			return ;
		NSURL *url = [NSURL fileURLWithPath:@(mesh->mats[i].texture)];
		CGImageSourceRef src = CGImageSourceCreateWithURL(
			(__bridge CFURLRef)url, NULL);
		if (!src)
			return ;
		CGImageRef img = CGImageSourceCreateImageAtIndex(src, 0, NULL);
		CFRelease(src);
		if (!img)
			return ;
		alpha[i] = has_transparency(img);
		MTKTextureLoader *loader = [[MTKTextureLoader alloc]
			initWithDevice:device];
		id<MTLTexture> tex = [loader newTextureWithCGImage:img options:@{
			MTKTextureLoaderOptionSRGB: @YES,
			MTKTextureLoaderOptionGenerateMipmaps: @YES,
			MTKTextureLoaderOptionTextureStorageMode: @(MTLStorageModePrivate)}
			error:nil];
		CGImageRelease(img);
		if (tex)
			@synchronized (slots) { slots[i] = tex; }
	});
	id<MTLBuffer> mats = [_device newBufferWithLength:sizeof(t_host_material)
		* nm options:MTLResourceStorageModeShared];
	t_host_material *hm = mats.contents;
	int ntex = 0;
	int nalpha = 0;
	for (int i = 0; i < nm; i++)
	{
		memset(&hm[i], 0, sizeof(hm[i]));
		hm[i].kd = mesh->mats[i].kd;
		if (slots[i] != [NSNull null])
		{
			id<MTLTexture> tex = slots[i];
			hm[i].diffuse = tex.gpuResourceID;
			hm[i].flags = MAT_TEXTURE | (alpha[i] ? MAT_ALPHA : 0);
			[texs addObject:tex];
			ntex++;
			nalpha += alpha[i];
		}
		else
			alpha[i] = false;
	}
	_mats = mats;
	_textures = texs;
	printf("texturas: %d cargadas (%d con transparencia) en %.0f ms\n", ntex,
		nalpha, (CACurrentMediaTime() - t0) * 1000);

	/*
	** Opacos primero y transparentes despues: dos geometrias del mismo
	** buffer, y solo la segunda paga la llamada a alpha_test.
	*/
	t0 = CACurrentMediaTime();
	size_t n = mesh->ntris;
	size_t nop = 0;
	for (size_t i = 0; i < n; i++)
		nop += !alpha[mesh->tris[i].material];
	_meshPos = [_device newBufferWithLength:sizeof(float) * 9 * n
		options:MTLResourceStorageModeShared];
	_tris = [_device newBufferWithLength:sizeof(t_gpu_tri) * n
		options:MTLResourceStorageModeShared];
	float *pos = _meshPos.contents;
	t_gpu_tri *tris = _tris.contents;
	size_t io = 0;
	size_t ia = nop;
	for (size_t i = 0; i < n; i++)
	{
		size_t k = alpha[mesh->tris[i].material] ? ia++ : io++;
		memcpy(&pos[k * 9], &mesh->pos[i * 9], sizeof(float) * 9);
		tris[k] = mesh->tris[i];
	}
	free(alpha);
	free_mesh_data(mesh);
	_ntris = (int)n;
	_nopaque = (int)nop;
	NSMutableArray *geos = [NSMutableArray array];
	for (int k = 0; k < 2; k++)
	{
		size_t first = k == 0 ? 0 : nop;
		size_t count = k == 0 ? nop : n - nop;
		if (count == 0)
			continue ;
		MTLAccelerationStructureTriangleGeometryDescriptor *g =
			[MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
		g.vertexBuffer = _meshPos;
		g.vertexBufferOffset = first * 9 * sizeof(float);
		g.vertexStride = 3 * sizeof(float);
		g.triangleCount = count;
		g.opaque = (k == 0);
		g.intersectionFunctionTableOffset = 0;
		[geos addObject:g];
	}
	MTLPrimitiveAccelerationStructureDescriptor *desc =
		[MTLPrimitiveAccelerationStructureDescriptor descriptor];
	desc.geometryDescriptors = geos;
	MTLAccelerationStructureSizes sizes =
		[_device accelerationStructureSizesWithDescriptor:desc];
	_meshPrim = [_device newAccelerationStructureWithSize:
		sizes.accelerationStructureSize];
	id<MTLBuffer> scratch = [_device newBufferWithLength:
		sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
	id<MTLCommandBuffer> cmd = [_queue commandBuffer];
	id<MTLAccelerationStructureCommandEncoder> enc =
		[cmd accelerationStructureCommandEncoder];
	[enc buildAccelerationStructure:_meshPrim descriptor:desc
		scratchBuffer:scratch scratchBufferOffset:0];
	[enc endEncoding];
	[cmd commit];
	[cmd waitUntilCompleted];
	[self buildInstances:mesh];
	uint32_t base = (uint32_t)nop;
	_meshBase = [_device newBufferWithBytes:&base length:sizeof(base)
		options:MTLResourceStorageModeShared];
	MTLIntersectionFunctionTableDescriptor *tdesc =
		[MTLIntersectionFunctionTableDescriptor new];
	tdesc.functionCount = 1;
	_meshTable = [_pipeline newIntersectionFunctionTableWithDescriptor:tdesc];
	[_meshTable setFunction:[_pipeline functionHandleWithFunction:alpha_fn]
		atIndex:0];
	[_meshTable setBuffer:_tris offset:0 atIndex:0];
	[_meshTable setBuffer:_mats offset:0 atIndex:1];
	[_meshTable setBuffer:_meshBase offset:0 atIndex:2];
	printf("malla: %zu triangulos (%zu opacos, %zu con transparencia), "
		"estructura de %.0f MB en %.0f ms\n", n, nop, n - nop,
		sizes.accelerationStructureSize / 1048576.0,
		(CACurrentMediaTime() - t0) * 1000);
}

/*
** Recorrido de prueba: desde la camara 1 avanza 2,5 m y gira 0,25 rad.
** Deterministico por t (0..1) para poder comparar con las referencias.
*/

- (void)pathAt:(float)t frame:(unsigned int)frame
{
	if (getenv("RT_STATIC"))
		t = 0.5f;
	float tm = getenv("RT_ONLY_YAW") ? 0.5f : t;
	float ty = getenv("RT_ONLY_MOVE") ? 0.5f : t;
	float len = getenv("RT_PATH_LEN") ? atof(getenv("RT_PATH_LEN")) : 1;
	_position = _pathStart + simd_make_float3(-2.5f, 0, 0.3f) * tm * len;
	_yaw = _pathYaw + 0.25f * ty * len;
	_frame = frame;
}

/*
** Instancias del modelo. Sin RT_CITY hay una sola (el modelo tal cual);
** con RT_CITY=N, una cuadricula de N x N copias separadas por el tamano del
** modelo y giradas de 90 en 90 grados para que no se vean todas iguales.
*/

- (void)buildInstances:(t_gpu_mesh *)mesh
{
	int				n = getenv("RT_CITY") ? atoi(getenv("RT_CITY")) : 1;
	float			sx = mesh->size.x * 1.02f;
	float			sz = mesh->size.z * 1.02f;
	float			step = fmaxf(sx, sz);

	n = (n < 1 || !_city) ? 1 : n;
	_ninsts = n * n;
	_insts = [_device newBufferWithLength:sizeof(simd_float4) * _ninsts
		options:MTLResourceStorageModeShared];
	id<MTLBuffer> descs = [_device newBufferWithLength:
		sizeof(MTLAccelerationStructureInstanceDescriptor) * _ninsts
		options:MTLResourceStorageModeShared];
	simd_float4 *it = _insts.contents;
	MTLAccelerationStructureInstanceDescriptor *d = descs.contents;
	for (int i = 0; i < _ninsts; i++)
	{
		int gx = i % n;
		int gz = i / n;
		int rot = (i == 0) ? 0 : (int)(hash_int(i) % 4);
		float c = (float[]){1, 0, -1, 0}[rot];
		float s = (float[]){0, 1, 0, -1}[rot];
		/*
		** Cada copia gira alrededor del centro del modelo y se coloca en su
		** celda de la cuadricula.
		*/
		simd_float3 ctr = mesh->center;
		simd_float3 rc = simd_make_float3(c * ctr.x + s * ctr.z, ctr.y,
			-s * ctr.x + c * ctr.z);
		float tx = gx * step + ctr.x - rc.x;
		float tz = gz * step + ctr.z - rc.z;
		it[i] = simd_make_float4(c, s, tx, tz);
		memset(&d[i], 0, sizeof(d[i]));
		d[i].accelerationStructureIndex = 0;
		d[i].mask = 0xFF;
		d[i].transformationMatrix.columns[0] = MTLPackedFloat3Make(c, 0, -s);
		d[i].transformationMatrix.columns[1] = MTLPackedFloat3Make(0, 1, 0);
		d[i].transformationMatrix.columns[2] = MTLPackedFloat3Make(s, 0, c);
		d[i].transformationMatrix.columns[3] = MTLPackedFloat3Make(tx, 0, tz);
	}
	if (!_city)
	{
		_meshAccel = _meshPrim;
		return ;
	}
	MTLInstanceAccelerationStructureDescriptor *idesc =
		[MTLInstanceAccelerationStructureDescriptor descriptor];
	idesc.instancedAccelerationStructures = @[_meshPrim];
	idesc.instanceCount = _ninsts;
	idesc.instanceDescriptorBuffer = descs;
	MTLAccelerationStructureSizes sizes =
		[_device accelerationStructureSizesWithDescriptor:idesc];
	_meshAccel = [_device newAccelerationStructureWithSize:
		sizes.accelerationStructureSize];
	id<MTLBuffer> scratch = [_device newBufferWithLength:
		sizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];
	id<MTLCommandBuffer> cmd = [_queue commandBuffer];
	id<MTLAccelerationStructureCommandEncoder> enc =
		[cmd accelerationStructureCommandEncoder];
	[enc buildAccelerationStructure:_meshAccel descriptor:idesc
		scratchBuffer:scratch scratchBufferOffset:0];
	[enc endEncoding];
	[cmd commit];
	[cmd waitUntilCompleted];
	printf("instancias: %d (%.0f millones de triangulos en escena)\n",
		_ninsts, (double)_ninsts * _ntris / 1e6);
}

- (void)useCamera:(int)index
{
	t_gpu_camera	*camera = &_scene->cameras[index];
	simd_float3		dir = simd_normalize(xyz(camera->direction));

	_camera = index;
	_position = xyz(camera->origin);
	_yaw = atan2f(dir.x, dir.z);
	_pitch = asinf(dir.y);
	_fov = camera->fov;
	_pathStart = _position;
	_pathYaw = _yaw;
	printf("camara %d/%d\n", index + 1, _scene->ncameras);
}

- (simd_float3)forward
{
	return (simd_make_float3(cosf(_pitch) * sinf(_yaw), sinf(_pitch),
		cosf(_pitch) * cosf(_yaw)));
}

/*
** Misma base que inicamera() en la version CPU: x = up ^ dir, y = dir ^ x
*/

- (t_gpu_frame)frame
{
	t_gpu_frame	f;
	simd_float3	fwd = [self forward];
	simd_float3	right = simd_normalize(simd_cross(
		simd_make_float3(0, 1, 0), fwd));

	f.origin = vec4(_position);
	f.forward = vec4(fwd);
	f.right = vec4(right);
	f.up = vec4(simd_cross(fwd, right));
	f.ambient = _scene->ambient;
	f.fov = _fov;
	f.nobjects = _scene->nobjects;
	f.nlights = _scene->nlights;
	f.samples = _samples;
	f.nplanes = _nplanes;
	f.ntris = _ntris;
	f.ao_rays = getenv("RT_AO") ? atoi(getenv("RT_AO")) : 8;
	f.frame = _frame;
	f.nopaque = _nopaque;
	f.scene_min = _sceneMin - 0.01f;
	f.scene_max = _sceneMax + 0.01f;
	return (f);
}

- (void)encodePass:(int)pass to:(id<MTLTexture>)texture
	base:(id<MTLTexture>)base buffer:(id<MTLCommandBuffer>)cmd
{
	id<MTLComputeCommandEncoder>	enc = [cmd computeCommandEncoder];
	t_gpu_frame						f = [self frame];

	f.pass = pass;
	f.spread = 2 * tanf(f.fov * M_PI / 360) / texture.width;
	/*
	** Con reescalado, las texturas se muestrean al detalle de la resolucion
	** final, no al de la de render.
	*/
	if (pass == 3)
		f.spread *= (float)texture.width / _temporal.outputWidth;
	if (pass == 4)
		f.spread *= (float)texture.width / _denoiser.outputWidth;
	[enc setComputePipelineState:_pipeline];
	[enc setTexture:texture atIndex:0];
	[enc setTexture:base atIndex:1];
	bool gi = (pass == 4);
	[enc setTexture:(pass == 3 ? _tDepth : gi ? _gDepth : base) atIndex:2];
	[enc setTexture:(pass == 3 ? _tMotion : gi ? _gMotion : base) atIndex:3];
	[enc setTexture:(gi ? _gAlbedo : base) atIndex:4];
	[enc setTexture:(gi ? _gNormal : base) atIndex:5];
	[enc setTexture:(gi ? _gRough : base) atIndex:6];
	[enc setTexture:(gi ? _gSpec : base) atIndex:7];
	if (pass == 3 || pass == 4)
	{
		f.prev_origin = _hasPrev ? _prev.origin : f.origin;
		f.prev_forward = _hasPrev ? _prev.forward : f.forward;
		f.prev_right = _hasPrev ? _prev.right : f.right;
		f.prev_up = _hasPrev ? _prev.up : f.up;
		f.jitter = simd_make_float4(halton(_frame % 32 + 1, 2) - 0.5f,
			halton(_frame % 32 + 1, 3) - 0.5f, 0, 0);
		float mj = getenv("RT_MVJ") ? atof(getenv("RT_MVJ")) : 0;
		f.jitter.z = mj * f.jitter.x;
		f.jitter.w = mj * f.jitter.y;
		_prev = f;
		_lastJitter = simd_make_float2(f.jitter.x, f.jitter.y);
	}
	[enc setBytes:&f length:sizeof(f) atIndex:0];
	[enc setBuffer:_objects offset:0 atIndex:1];
	[enc setBuffer:_lights offset:0 atIndex:2];
	if (_accel)
	{
		[enc setAccelerationStructure:_accel atBufferIndex:3];
		[enc setIntersectionFunctionTable:_table atBufferIndex:4];
		[enc useResource:_accel usage:MTLResourceUsageRead];
	}
	else
		[enc setBuffer:_nodes offset:0 atIndex:3];
	if (_meshAccel)
	{
		[enc setAccelerationStructure:_meshAccel atBufferIndex:8];
		[enc setIntersectionFunctionTable:_meshTable atBufferIndex:9];
		[enc setBuffer:_tris offset:0 atIndex:10];
		[enc setBuffer:_mats offset:0 atIndex:11];
		[enc setBuffer:_meshPos offset:0 atIndex:12];
		[enc useResource:_meshAccel usage:MTLResourceUsageRead];
		[enc useResource:_meshPrim usage:MTLResourceUsageRead];
		[enc setBuffer:_insts offset:0 atIndex:13];
		[enc useResource:_meshBase usage:MTLResourceUsageRead];
		for (id<MTLTexture> t in _textures)
			[enc useResource:t usage:MTLResourceUsageRead];
	}
	[enc dispatchThreads:MTLSizeMake(texture.width, texture.height, 1)
		threadsPerThreadgroup:MTLSizeMake(8, 8, 1)];
	[enc endEncoding];
}

/*
** Con antialiasing (samples > 1) se usa el modo adaptativo salvo
** RT_ADAPTIVE=0: primero 1 rayo por pixel y luego solo se refinan los bordes.
*/

- (void)encodeTo:(id<MTLTexture>)texture buffer:(id<MTLCommandBuffer>)cmd
{
	bool	adaptive = _samples > 1 && !(getenv("RT_ADAPTIVE")
		&& strcmp(getenv("RT_ADAPTIVE"), "0") == 0);

	if (!adaptive)
	{
		[self encodePass:0 to:texture base:texture buffer:cmd];
		return ;
	}
	if (!_base || _base.width != texture.width
		|| _base.height != texture.height)
	{
		MTLTextureDescriptor *td = [MTLTextureDescriptor
			texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
			width:texture.width height:texture.height mipmapped:NO];
		td.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
		td.storageMode = MTLStorageModePrivate;
		_base = [_device newTextureWithDescriptor:td];
	}
	[self encodePass:1 to:_base base:_base buffer:cmd];
	[self encodePass:2 to:texture base:_base buffer:cmd];
}

- (id<MTLTexture>)offscreenTexture
{
	MTLTextureDescriptor	*desc;

	desc = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
		width:_scene->width height:_scene->height mipmapped:NO];
	desc.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead
		| MTLTextureUsageRenderTarget;
	desc.storageMode = MTLStorageModeShared;
	return ([_device newTextureWithDescriptor:desc]);
}

/*
** RT_SCALE < 1: se renderiza a menor resolucion y MetalFX (el equivalente
** de Apple a DLSS/FSR) reescala a la resolucion final, como en los juegos.
*/

- (void)encodeScaledTo:(id<MTLTexture>)texture buffer:(id<MTLCommandBuffer>)cmd
{
	float	scale = getenv("RT_SCALE") ? atof(getenv("RT_SCALE")) : 1;

	if (_giOn || getenv("RT_GI"))
	{
		[self encodeGITo:texture scale:(getenv("RT_SCALE") ? scale : 0.5f)
			buffer:cmd];
		return ;
	}
	if (_temporalOn || getenv("RT_TEMPORAL"))
	{
		[self encodeTemporalTo:texture scale:(scale < 1 ? scale : 0.5f)
			buffer:cmd];
		return ;
	}
	if (scale >= 1)
	{
		[self encodeTo:texture buffer:cmd];
		return ;
	}
	if (!_scaler || _scaler.outputWidth != texture.width)
	{
		MTLFXSpatialScalerDescriptor *desc = [MTLFXSpatialScalerDescriptor new];
		desc.inputWidth = (NSUInteger)(texture.width * scale);
		desc.inputHeight = (NSUInteger)(texture.height * scale);
		desc.outputWidth = texture.width;
		desc.outputHeight = texture.height;
		desc.colorTextureFormat = MTLPixelFormatBGRA8Unorm;
		desc.outputTextureFormat = MTLPixelFormatBGRA8Unorm;
		desc.colorProcessingMode = MTLFXSpatialScalerColorProcessingModePerceptual;
		_scaler = [desc newSpatialScalerWithDevice:_device];
		MTLTextureDescriptor *td = [MTLTextureDescriptor
			texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
			width:desc.inputWidth height:desc.inputHeight mipmapped:NO];
		td.usage = MTLTextureUsageShaderWrite | _scaler.colorTextureUsage;
		td.storageMode = MTLStorageModePrivate;
		_lowres = [_device newTextureWithDescriptor:td];
	}
	[self encodeTo:_lowres buffer:cmd];
	_scaler.colorTexture = _lowres;
	_scaler.outputTexture = texture;
	[_scaler encodeToCommandBuffer:cmd];
}

/*
** Reescalado temporal (como DLSS): render a scale * resolucion con 1 rayo
** desplazado por pixel; MetalFX acumula las imagenes anteriores usando
** profundidad y movimiento y reconstruye la resolucion final.
*/

- (void)encodeTemporalTo:(id<MTLTexture>)texture scale:(float)scale
	buffer:(id<MTLCommandBuffer>)cmd
{
	NSUInteger	iw = (NSUInteger)(texture.width * scale);
	NSUInteger	ih = (NSUInteger)(texture.height * scale);

	if (!_temporal || _temporal.outputWidth != texture.width)
	{
		MTLFXTemporalScalerDescriptor *d = [MTLFXTemporalScalerDescriptor new];
		d.inputWidth = iw;
		d.inputHeight = ih;
		d.outputWidth = texture.width;
		d.outputHeight = texture.height;
		d.colorTextureFormat = MTLPixelFormatBGRA8Unorm;
		d.outputTextureFormat = MTLPixelFormatBGRA8Unorm;
		d.depthTextureFormat = MTLPixelFormatR32Float;
		d.motionTextureFormat = MTLPixelFormatRG16Float;
		_temporal = [d newTemporalScalerWithDevice:_device];
		_temporal.depthReversed = YES;
		_temporal.motionVectorScaleX = iw;
		_temporal.motionVectorScaleY = ih;
		_tColor = [self privateTexture:MTLPixelFormatBGRA8Unorm w:iw h:ih
			usage:MTLTextureUsageShaderWrite | _temporal.colorTextureUsage];
		_tDepth = [self privateTexture:MTLPixelFormatR32Float w:iw h:ih
			usage:MTLTextureUsageShaderWrite | _temporal.depthTextureUsage];
		_tMotion = [self privateTexture:MTLPixelFormatRG16Float w:iw h:ih
			usage:MTLTextureUsageShaderWrite | _temporal.motionTextureUsage];
		_tOut = [self privateTexture:MTLPixelFormatBGRA8Unorm w:texture.width
			h:texture.height usage:_temporal.outputTextureUsage];
		_hasPrev = false;
	}
	[self encodePass:3 to:_tColor base:_tColor buffer:cmd];
	_temporal.colorTexture = _tColor;
	_temporal.inputContentWidth = _tColor.width;
	_temporal.inputContentHeight = _tColor.height;
	_temporal.depthTexture = _tDepth;
	_temporal.motionTexture = _tMotion;
	_temporal.outputTexture = _tOut;
	float js = getenv("RT_JS") ? atof(getenv("RT_JS")) : -1;
	_temporal.jitterOffsetX = js * _lastJitter.x;
	_temporal.jitterOffsetY = js * _lastJitter.y;
	float ms = getenv("RT_MS") ? atof(getenv("RT_MS")) : 1;
	_temporal.motionVectorScaleX = ms * _tColor.width;
	_temporal.motionVectorScaleY = ms * _tColor.height;
	_temporal.reset = !_hasPrev;
	[_temporal encodeToCommandBuffer:cmd];
	_hasPrev = true;
	id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
	[blit copyFromTexture:_tOut toTexture:texture];
	[blit endEncoding];
}

/*
** Matrices de camara que pide el denoiser: mundo -> vista (la camara mira
** hacia -Z) y vista -> recorte (perspectiva con profundidad 0..1, la misma
** que escribe el kernel en la textura de profundidad).
*/

- (void)cameraMatrices:(simd_float4x4 *)view proj:(simd_float4x4 *)proj
	aspect:(float)aspect
{
	t_gpu_frame	f = [self frame];
	simd_float3	r = xyz(f.right);
	simd_float3	u = xyz(f.up);
	simd_float3	b = -xyz(f.forward);
	simd_float3	o = xyz(f.origin);
	float		n = 0.05f;
	float		fa = 1000.0f;
	float		xs = 1 / tanf(f.fov * M_PI / 360);
	float		ys = xs * aspect;

	*view = (simd_float4x4){{
		{r.x, u.x, b.x, 0}, {r.y, u.y, b.y, 0}, {r.z, u.z, b.z, 0},
		{-simd_dot(r, o), -simd_dot(u, o), -simd_dot(b, o), 1}}};
	*proj = (simd_float4x4){{
		{xs, 0, 0, 0}, {0, ys, 0, 0}, {0, 0, fa / (n - fa), -1},
		{0, 0, n * fa / (n - fa), 0}}};
}

/*
** Iluminacion global con el denoiser temporal de MetalFX: render con grano a
** scale * resolucion, MetalFX limpia y reescala, y un ultimo paso aplica el
** tonemapping.
*/

- (void)encodeGITo:(id<MTLTexture>)texture scale:(float)scale
	buffer:(id<MTLCommandBuffer>)cmd
{
	NSUInteger	iw = (NSUInteger)(texture.width * scale);
	NSUInteger	ih = (NSUInteger)(texture.height * scale);

	if (!_denoiser || _denoiser.outputWidth != texture.width)
	{
		MTLFXTemporalDenoisedScalerDescriptor *d =
			[MTLFXTemporalDenoisedScalerDescriptor new];
		d.inputWidth = iw;
		d.inputHeight = ih;
		d.outputWidth = texture.width;
		d.outputHeight = texture.height;
		d.colorTextureFormat = MTLPixelFormatRGBA16Float;
		d.depthTextureFormat = MTLPixelFormatR32Float;
		d.motionTextureFormat = MTLPixelFormatRG16Float;
		d.diffuseAlbedoTextureFormat = MTLPixelFormatRGBA16Float;
		d.specularAlbedoTextureFormat = MTLPixelFormatRGBA16Float;
		d.normalTextureFormat = MTLPixelFormatRGBA16Float;
		d.roughnessTextureFormat = MTLPixelFormatR16Float;
		d.outputTextureFormat = MTLPixelFormatRGBA16Float;
		_denoiser = [d newTemporalDenoisedScalerWithDevice:_device];
		if (!_denoiser)
		{
			fprintf(stderr, "error: no se pudo crear el denoiser de MetalFX\n");
			exit(1);
		}
		MTLTextureUsage w = MTLTextureUsageShaderWrite;
		_gColor = [self privateTexture:MTLPixelFormatRGBA16Float w:iw h:ih
			usage:w | _denoiser.colorTextureUsage];
		_gDepth = [self privateTexture:MTLPixelFormatR32Float w:iw h:ih
			usage:w | _denoiser.depthTextureUsage];
		_gMotion = [self privateTexture:MTLPixelFormatRG16Float w:iw h:ih
			usage:w | _denoiser.motionTextureUsage];
		_gAlbedo = [self privateTexture:MTLPixelFormatRGBA16Float w:iw h:ih
			usage:w | _denoiser.diffuseAlbedoTextureUsage];
		_gSpec = [self privateTexture:MTLPixelFormatRGBA16Float w:iw h:ih
			usage:w | _denoiser.specularAlbedoTextureUsage];
		_gNormal = [self privateTexture:MTLPixelFormatRGBA16Float w:iw h:ih
			usage:w | _denoiser.normalTextureUsage];
		_gRough = [self privateTexture:MTLPixelFormatR16Float w:iw h:ih
			usage:w | _denoiser.roughnessTextureUsage];
		_gOut = [self privateTexture:MTLPixelFormatRGBA16Float
			w:texture.width h:texture.height
			usage:_denoiser.outputTextureUsage | MTLTextureUsageShaderRead];
		_hasPrev = false;
	}
	[self encodePass:4 to:_gColor base:_gColor buffer:cmd];
	simd_float4x4 view;
	simd_float4x4 proj;
	[self cameraMatrices:&view proj:&proj aspect:(float)iw / ih];
	_denoiser.colorTexture = _gColor;
	_denoiser.depthTexture = _gDepth;
	_denoiser.motionTexture = _gMotion;
	_denoiser.diffuseAlbedoTexture = _gAlbedo;
	_denoiser.specularAlbedoTexture = _gSpec;
	_denoiser.normalTexture = _gNormal;
	_denoiser.roughnessTexture = _gRough;
	_denoiser.outputTexture = _gOut;
	_denoiser.jitterOffsetX = -_lastJitter.x;
	_denoiser.jitterOffsetY = -_lastJitter.y;
	_denoiser.motionVectorScaleX = iw;
	_denoiser.motionVectorScaleY = ih;
	_denoiser.worldToViewMatrix = view;
	_denoiser.viewToClipMatrix = proj;
	_denoiser.shouldResetHistory = !_hasPrev;
	[_denoiser encodeToCommandBuffer:cmd];
	_hasPrev = true;
	[self encodePass:5 to:texture base:_gOut buffer:cmd];
}

- (void)giReferenceAt:(float)t samples:(int)spp size:(MTLSize)size
	name:(const char *)name
{
	size_t			w = size.width;
	size_t			h = size.height;
	MTLTextureDescriptor *td = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
		width:w height:h mipmapped:NO];
	td.usage = MTLTextureUsageShaderWrite | MTLTextureUsageShaderRead;
	td.storageMode = MTLStorageModeShared;
	id<MTLTexture>	acc = [_device newTextureWithDescriptor:td];
	id<MTLTexture>	dummy = [self offscreenTexture];
	float			*sum = calloc(w * h * 4, sizeof(float));
	float			*px = malloc(w * h * 4 * sizeof(float));
	CFTimeInterval	t0 = CACurrentMediaTime();

	if (!_denoiser || _denoiser.outputWidth != w)
	{
		id<MTLCommandBuffer> cmd = [_queue commandBuffer];
		[self encodeGITo:dummy scale:1 buffer:cmd];
		[cmd commit];
		[cmd waitUntilCompleted];
	}
	for (int s = 0; s < spp; s++)
	{
		[self pathAt:t frame:1000 + s];
		id<MTLCommandBuffer> cmd = [_queue commandBuffer];
		[self encodePass:4 to:acc base:acc buffer:cmd];
		[cmd commit];
		[cmd waitUntilCompleted];
		[acc getBytes:px bytesPerRow:w * 16 fromRegion:MTLRegionMake2D(0, 0,
			w, h) mipmapLevel:0];
		for (size_t i = 0; i < w * h * 4; i++)
			sum[i] += px[i];
	}
	uint8_t *out = malloc(w * h * 4);
	for (size_t i = 0; i < w * h; i++)
	{
		for (int c = 0; c < 3; c++)
			out[i * 4 + 2 - c] = (uint8_t)(powf(aces(sum[i * 4 + c] / spp),
				1 / 2.2f) * 255 + 0.5f);
		out[i * 4 + 3] = 255;
	}
	id<MTLTexture> img = [self offscreenTexture];
	[img replaceRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0
		withBytes:out bytesPerRow:w * 4];
	save_png(img, name);
	printf("referencia %s: %d muestras en %.1f s\n", name, spp,
		CACurrentMediaTime() - t0);
	free(sum);
	free(px);
	free(out);
}

- (id<MTLTexture>)privateTexture:(MTLPixelFormat)fmt w:(NSUInteger)w
	h:(NSUInteger)h usage:(MTLTextureUsage)usage
{
	MTLTextureDescriptor *td = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:fmt width:w height:h mipmapped:NO];
	td.usage = usage;
	td.storageMode = MTLStorageModePrivate;
	return ([_device newTextureWithDescriptor:td]);
}

- (double)renderOnce:(id<MTLTexture>)texture
{
	id<MTLCommandBuffer>	cmd = [_queue commandBuffer];

	[self encodeScaledTo:texture buffer:cmd];
	[cmd commit];
	[cmd waitUntilCompleted];
	if (cmd.error)
		fprintf(stderr, "error GPU: %s\n", cmd.error.localizedDescription.UTF8String);
	return ((cmd.GPUEndTime - cmd.GPUStartTime) * 1000);
}

- (void)keyDown:(unsigned short)key
{
	if (key == KEY_ESC)
		exit(0);
	if (key >= KEY_1 && key <= KEY_4)
		_samples = key - KEY_1 + 1;
	if (key == KEY_T)
		_temporalOn = !_temporalOn;
	if (key == KEY_G)
		_giOn = !_giOn;
	if (key == KEY_RIGHT && _camera + 1 < _scene->ncameras)
		[self useCamera:_camera + 1];
	if (key == KEY_LEFT && _camera > 0)
		[self useCamera:_camera - 1];
	if (key < 256)
		_keys[key] = true;
}

- (void)keyUp:(unsigned short)key
{
	if (key < 256)
		_keys[key] = false;
}

- (void)setFast:(bool)fast
{
	_fast = fast;
}

- (void)lookBy:(float)dx :(float)dy
{
	_yaw += dx * 0.004f;
	_pitch = fminf(fmaxf(_pitch - dy * 0.004f, -1.55f), 1.55f);
}

- (void)move:(float)dt
{
	simd_float3	fwd = [self forward];
	simd_float3	right = simd_normalize(simd_cross(
		simd_make_float3(0, 1, 0), fwd));
	float		speed = dt * _moveSpeed * (_fast ? 3 : 1);

	_position += fwd * speed * (_keys[KEY_W] - _keys[KEY_S]);
	_position += right * speed * (_keys[KEY_D] - _keys[KEY_A]);
	_position.y += speed * (_keys[KEY_E] - _keys[KEY_Q]);
}

- (void)drawInMTKView:(MTKView *)view
{
	CFTimeInterval			now = CACurrentMediaTime();
	id<CAMetalDrawable>		drawable = view.currentDrawable;
	id<MTLCommandBuffer>	cmd;

	if (_last > 0)
		[self move:now - _last];
	_last = now;
	if (!drawable)
		return ;
	cmd = [_queue commandBuffer];
	[self encodeScaledTo:drawable.texture buffer:cmd];
	[cmd presentDrawable:drawable];
	[cmd addCompletedHandler:^(id<MTLCommandBuffer> done) {
		self->_gpuMs = (done.GPUEndTime - done.GPUStartTime) * 1000;
	}];
	[cmd commit];
	if (++_frames, now - _fpsStart >= 0.5)
	{
		view.window.title = [NSString stringWithFormat:
			@"raytracing majestuoso (GPU) - %.0f fps"
			" - GPU %.2f ms/frame - %dx%d - %@",
			_frames / (now - _fpsStart), _gpuMs, (int)drawable.texture.width,
			(int)drawable.texture.height, _temporalOn
			? @"temporal 1/2 res (T)" : [NSString stringWithFormat:
			@"AA %dx%d (T: temporal)", _samples, _samples]];
		_frames = 0;
		_fpsStart = now;
	}
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size
{
}

@end

@interface RTView : MTKView
@property (nonatomic, weak) Renderer *renderer;
@end

@implementation RTView

- (BOOL)acceptsFirstResponder
{
	return (YES);
}

- (void)keyDown:(NSEvent *)event
{
	[self.renderer keyDown:event.keyCode];
}

- (void)keyUp:(NSEvent *)event
{
	[self.renderer keyUp:event.keyCode];
}

- (void)flagsChanged:(NSEvent *)event
{
	[self.renderer setFast:(event.modifierFlags & NSEventModifierFlagShift)];
}

- (void)mouseDragged:(NSEvent *)event
{
	[self.renderer lookBy:event.deltaX :event.deltaY];
}

@end

static void		save_png(id<MTLTexture> texture, const char *name)
{
	size_t			w = texture.width;
	size_t			h = texture.height;
	NSMutableData	*data = [NSMutableData dataWithLength:w * h * 4];
	CGColorSpaceRef	space = CGColorSpaceCreateDeviceRGB();
	CGContextRef	ctx;
	CGImageRef		image;
	NSURL			*url;
	CGImageDestinationRef	dest;

	[texture getBytes:data.mutableBytes bytesPerRow:w * 4
		fromRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0];
	ctx = CGBitmapContextCreate(data.mutableBytes, w, h, 8, w * 4, space,
		kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
	image = CGBitmapContextCreateImage(ctx);
	url = [NSURL fileURLWithPath:[NSString stringWithFormat:@"%s.png", name]];
	dest = CGImageDestinationCreateWithURL((__bridge CFURLRef)url,
		(__bridge CFStringRef)UTTypePNG.identifier, 1, NULL);
	CGImageDestinationAddImage(dest, image, NULL);
	CGImageDestinationFinalize(dest);
	CFRelease(dest);
	CGImageRelease(image);
	CGContextRelease(ctx);
	CGColorSpaceRelease(space);
	printf("guardado %s\n", url.path.UTF8String);
}

static int		cmp_double(const void *a, const void *b)
{
	double x = *(const double *)a;
	double y = *(const double *)b;

	return ((x > y) - (x < y));
}

/*
** --bench [aa]: mediana de 15 frames con el antialiasing indicado (1 por
** defecto). Imprime solo el numero para poder automatizar el registro.
*/

static void		bench(Renderer *renderer, int aa)
{
	id<MTLTexture>	texture = [renderer offscreenTexture];
	double			times[15];
	int				i;

	renderer.samples = aa;
	[renderer renderOnce:texture];
	for (i = 0; i < 15; i++)
		times[i] = [renderer renderOnce:texture];
	qsort(times, 15, sizeof(double), cmp_double);
	printf("bench %dx%d AA %dx%d: %.3f ms\n", (int)texture.width,
		(int)texture.height, aa, aa, times[7]);
}

/*
** --path N [prefijo] [aa]: N imagenes con la camara en movimiento. Imprime la
** mediana y el p95 del tiempo de GPU por imagen; con prefijo guarda 3
** imagenes de control (a 1/3, 2/3 y al final) para medir la calidad.
*/

static void		path_bench(Renderer *renderer, int n, const char *prefix, int aa)
{
	id<MTLTexture>	texture = [renderer offscreenTexture];
	double			*times = malloc(sizeof(double) * n);
	char			name[1100];

	renderer.samples = aa;
	[renderer pathAt:0 frame:0];
	[renderer renderOnce:texture];
	for (int i = 0; i < n; i++)
	{
		bool check = (i == n / 3 || i == 2 * n / 3 || i == n - 1);
		if (getenv("RT_CHECKPOINTS_ONLY") && !check)
		{
			times[i] = 0;
			continue ;
		}
		[renderer pathAt:(float)i / (n - 1) frame:i];
		times[i] = [renderer renderOnce:texture];
		if (prefix && check)
		{
			snprintf(name, sizeof(name), "%s_f%03d", prefix, i);
			save_png(texture, name);
		}
	}
	qsort(times, n, sizeof(double), cmp_double);
	printf("path %dx%d AA %dx%d, %d imagenes: mediana %.3f ms, p95 %.3f ms\n",
		(int)texture.width, (int)texture.height, aa, aa, n, times[n / 2],
		times[(int)(n * 0.95)]);
	free(times);
}

/*
** --video N salida.mp4 [fps]: renderiza N imagenes del recorrido y las pasa
** en crudo a ffmpeg (H.264, calidad alta) sin escribir imagenes a disco.
*/

static void		render_video(Renderer *renderer, int n, const char *out, int fps)
{
	id<MTLTexture>	texture = [renderer offscreenTexture];
	size_t			w = texture.width;
	size_t			h = texture.height;
	char			cmd[2048];
	double			total = 0;
	CFTimeInterval	t0 = CACurrentMediaTime();

	snprintf(cmd, sizeof(cmd), "ffmpeg -loglevel error -y -f rawvideo "
		"-pix_fmt bgra -s %zux%zu -r %d -i - -c:v libx264 -preset slow "
		"-crf 16 -pix_fmt yuv420p '%s'", w, h, fps, out);
	FILE *pipe = popen(cmd, "w");
	if (!pipe)
	{
		fprintf(stderr, "error: no se pudo lanzar ffmpeg\n");
		exit(1);
	}
	void *pixels = malloc(w * h * 4);
	renderer.samples = 1;
	for (int i = 0; i < n; i++)
	{
		[renderer pathAt:(float)i / (n - 1) frame:i];
		total += [renderer renderOnce:texture];
		[texture getBytes:pixels bytesPerRow:w * 4
			fromRegion:MTLRegionMake2D(0, 0, w, h) mipmapLevel:0];
		fwrite(pixels, 1, w * h * 4, pipe);
		if (i % 30 == 0)
			fprintf(stderr, "\rimagen %d/%d", i + 1, n);
	}
	pclose(pipe);
	free(pixels);
	printf("\nvideo %s: %d imagenes %zux%zu, GPU %.1f ms/imagen de media, "
		"%.1f s en total\n", out, n, w, h, total / n,
		CACurrentMediaTime() - t0);
}

/*
** --giref N prefijo muestras: referencia "perfecta" de la iluminacion
** global en las 3 imagenes de control del recorrido de N imagenes. Acumula
** muestras sin denoiser (cada una con rayos y jitter distintos) y aplica el
** mismo tonemapping que la version real.
*/

static void		gi_reference(Renderer *renderer, int n, const char *prefix,
					int spp)
{
	id<MTLTexture>	texture = [renderer offscreenTexture];
	int				checks[3] = {n / 3, 2 * n / 3, n - 1};
	char			name[1100];

	for (int k = 0; k < 3; k++)
	{
		snprintf(name, sizeof(name), "%s_f%03d", prefix, checks[k]);
		[renderer giReferenceAt:(float)checks[k] / (n - 1) samples:spp
			size:MTLSizeMake(texture.width, texture.height, 1) name:name];
	}
}

static void		run_window(Renderer *renderer, t_gpu_scene *scene)
{
	NSApplication	*app = [NSApplication sharedApplication];
	NSRect			screen = [NSScreen mainScreen].visibleFrame;
	CGFloat			scale = fmin(1, fmin(screen.size.width * 0.9 / scene->width,
		screen.size.height * 0.9 / scene->height));
	NSRect			rect = NSMakeRect(0, 0, scene->width * scale,
		scene->height * scale);
	NSWindow		*window;
	RTView			*view;

	renderer.samples = getenv("RT_AA") ? atoi(getenv("RT_AA")) : 2;
	[app setActivationPolicy:NSApplicationActivationPolicyRegular];
	window = [[NSWindow alloc] initWithContentRect:rect
		styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
		| NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable
		backing:NSBackingStoreBuffered defer:NO];
	view = [[RTView alloc] initWithFrame:rect
		device:MTLCreateSystemDefaultDevice()];
	view.renderer = renderer;
	view.delegate = renderer;
	view.framebufferOnly = NO;
	view.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
	view.preferredFramesPerSecond = 120;
	window.contentView = view;
	window.title = @"raytracing majestuoso (GPU)";
	[window center];
	[window makeKeyAndOrderFront:nil];
	[window makeFirstResponder:view];
	[[NSNotificationCenter defaultCenter]
		addObserverForName:NSWindowWillCloseNotification object:window
		queue:nil usingBlock:^(NSNotification *n) { (void)n; exit(0); }];
	[app activateIgnoringOtherApps:YES];
	[app run];
}

int				main(int argc, char **argv)
{
	t_gpu_scene	scene;
	Renderer	*renderer;

	@autoreleasepool
	{
		if (argc < 2)
		{
			fprintf(stderr, "uso: %s escena.rt [--save nombre | --bench]\n",
				argv[0]);
			return (1);
		}
		/*
		** --obj modelo.obj (en cualquier posicion): anade sus triangulos a
		** la escena del .rt, que sigue aportando camara, luces y resolucion.
		*/
		const char *obj_path = NULL;
		for (int i = 1; i + 1 < argc; i++)
			if (strcmp(argv[i], "--obj") == 0)
			{
				obj_path = argv[i + 1];
				for (int j = i; j + 2 <= argc; j++)
					argv[j] = argv[j + 2];
				argc -= 2;
				break ;
			}
		export_scene(argv[1], &scene);
		float model_size = 0;
		t_gpu_mesh mesh;
		memset(&mesh, 0, sizeof(mesh));
		if (obj_path)
		{
			CFTimeInterval t0 = CACurrentMediaTime();
			if (load_obj_cached(&mesh, obj_path) < 0)
				return (1);
			simd_float3 lo = simd_make_float3(INFINITY, INFINITY, INFINITY);
			simd_float3 hi = -lo;
			for (size_t i = 0; i < mesh.ntris * 3; i++)
			{
				simd_float3 v = simd_make_float3(mesh.pos[i * 3],
					mesh.pos[i * 3 + 1], mesh.pos[i * 3 + 2]);
				lo = simd_min(lo, v);
				hi = simd_max(hi, v);
			}
			printf("modelo cargado en %.0f ms; limites (%.1f, %.1f, %.1f) a "
				"(%.1f, %.1f, %.1f)\n", (CACurrentMediaTime() - t0) * 1000,
				lo.x, lo.y, lo.z, hi.x, hi.y, hi.z);
			model_size = simd_length(hi - lo);
			mesh.center = (lo + hi) * 0.5f;
			mesh.size = hi - lo;
		}
		renderer = [[Renderer alloc] initWithScene:&scene
			mesh:obj_path ? &mesh : NULL];
		/*
		** Con modelo: velocidad de paseo proporcional a su tamano (~4 m/s en
		** San Miguel) y reescalado temporal activado en la ventana.
		*/
		if (model_size > 0)
		{
			renderer.moveSpeed = model_size * 0.05f;
			renderer.temporalOn = argc < 3;
		}
		if (argc >= 3 && strcmp(argv[2], "--save") == 0)
		{
			id<MTLTexture> texture = [renderer offscreenTexture];
			renderer.samples = argc >= 5 ? atoi(argv[4]) : 3;
			printf("render: %.2f ms\n", [renderer renderOnce:texture]);
			save_png(texture, argc >= 4 ? argv[3] : "scene");
		}
		else if (argc >= 6 && strcmp(argv[2], "--giref") == 0)
			gi_reference(renderer, atoi(argv[3]), argv[4], atoi(argv[5]));
		else if (argc >= 5 && strcmp(argv[2], "--video") == 0)
			render_video(renderer, atoi(argv[3]), argv[4],
				argc >= 6 ? atoi(argv[5]) : 30);
		else if (argc >= 4 && strcmp(argv[2], "--path") == 0)
			path_bench(renderer, atoi(argv[3]), argc >= 5 ? argv[4] : NULL,
				argc >= 6 ? atoi(argv[5]) : 1);
		else if (argc >= 3 && strcmp(argv[2], "--bench") == 0)
			bench(renderer, argc >= 4 ? atoi(argv[3]) : 1);
		else
			run_window(renderer, &scene);
	}
	return (0);
}
