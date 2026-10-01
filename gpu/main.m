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
#import <ImageIO/ImageIO.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include "scene_export.h"
#include "shader_src.h"

enum { KEY_A = 0, KEY_S = 1, KEY_D = 2, KEY_Q = 12, KEY_W = 13, KEY_E = 14,
	KEY_1 = 18, KEY_2 = 19, KEY_3 = 20, KEY_4 = 21, KEY_ESC = 53,
	KEY_LEFT = 123, KEY_RIGHT = 124 };

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
- (instancetype)initWithScene:(t_gpu_scene *)scene;
- (void)encodeTo:(id<MTLTexture>)texture buffer:(id<MTLCommandBuffer>)cmd;
- (id<MTLTexture>)offscreenTexture;
- (void)useCamera:(int)index;
- (void)keyDown:(unsigned short)key;
- (void)keyUp:(unsigned short)key;
- (void)lookBy:(float)dx :(float)dy;
- (void)setFast:(bool)fast;
@end

@implementation Renderer
{
	t_gpu_scene					*_scene;
	id<MTLDevice>				_device;
	id<MTLCommandQueue>			_queue;
	id<MTLComputePipelineState>	_pipeline;
	id<MTLBuffer>				_objects;
	id<MTLBuffer>				_lights;
	simd_float3					_position;
	float						_yaw;
	float						_pitch;
	float						_fov;
	int							_camera;
	bool						_keys[256];
	bool						_fast;
	CFTimeInterval				_last;
	CFTimeInterval				_fpsStart;
	int							_frames;
}

- (instancetype)initWithScene:(t_gpu_scene *)scene
{
	NSError				*error = nil;
	id<MTLLibrary>		library;
	MTLCompileOptions	*options = [MTLCompileOptions new];

	self = [super init];
	_scene = scene;
	_samples = 3;
	_device = MTLCreateSystemDefaultDevice();
	_queue = [_device newCommandQueue];
	options.mathMode = MTLMathModeFast;
	library = [_device newLibraryWithSource:@(g_shader_src) options:options
		error:&error];
	if (!library)
	{
		fprintf(stderr, "error compilando el shader:\n%s\n",
			error.localizedDescription.UTF8String);
		exit(1);
	}
	_pipeline = [_device newComputePipelineStateWithFunction:
		[library newFunctionWithName:@"render"] error:&error];
	_objects = [_device newBufferWithBytes:scene->objects
		length:sizeof(t_gpu_object) * (scene->nobjects + 1)
		options:MTLResourceStorageModeShared];
	_lights = [_device newBufferWithBytes:scene->lights
		length:sizeof(t_gpu_light) * (scene->nlights + 1)
		options:MTLResourceStorageModeShared];
	[self useCamera:0];
	return (self);
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
	return (f);
}

- (void)encodeTo:(id<MTLTexture>)texture buffer:(id<MTLCommandBuffer>)cmd
{
	id<MTLComputeCommandEncoder>	enc = [cmd computeCommandEncoder];
	t_gpu_frame						f = [self frame];
	NSUInteger						w = _pipeline.threadExecutionWidth;
	NSUInteger						h;

	h = _pipeline.maxTotalThreadsPerThreadgroup / w;
	[enc setComputePipelineState:_pipeline];
	[enc setTexture:texture atIndex:0];
	[enc setBytes:&f length:sizeof(f) atIndex:0];
	[enc setBuffer:_objects offset:0 atIndex:1];
	[enc setBuffer:_lights offset:0 atIndex:2];
	[enc dispatchThreads:MTLSizeMake(texture.width, texture.height, 1)
		threadsPerThreadgroup:MTLSizeMake(w, h, 1)];
	[enc endEncoding];
}

- (id<MTLTexture>)offscreenTexture
{
	MTLTextureDescriptor	*desc;

	desc = [MTLTextureDescriptor
		texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
		width:_scene->width height:_scene->height mipmapped:NO];
	desc.usage = MTLTextureUsageShaderWrite;
	desc.storageMode = MTLStorageModeShared;
	return ([_device newTextureWithDescriptor:desc]);
}

- (double)renderOnce:(id<MTLTexture>)texture
{
	id<MTLCommandBuffer>	cmd = [_queue commandBuffer];

	[self encodeTo:texture buffer:cmd];
	[cmd commit];
	[cmd waitUntilCompleted];
	return ((cmd.GPUEndTime - cmd.GPUStartTime) * 1000);
}

- (void)keyDown:(unsigned short)key
{
	if (key == KEY_ESC)
		exit(0);
	if (key >= KEY_1 && key <= KEY_4)
		_samples = key - KEY_1 + 1;
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
	float		speed = dt * (_fast ? 60 : 20);

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
	[self encodeTo:drawable.texture buffer:cmd];
	[cmd presentDrawable:drawable];
	[cmd commit];
	if (++_frames, now - _fpsStart >= 0.5)
	{
		view.window.title = [NSString stringWithFormat:
			@"raytracing majestuoso (GPU) - %.0f fps - %dx%d - AA %dx%d",
			_frames / (now - _fpsStart), (int)drawable.texture.width,
			(int)drawable.texture.height, _samples, _samples];
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

static void		bench(Renderer *renderer)
{
	id<MTLTexture>	texture = [renderer offscreenTexture];
	double			total;
	int				aa;
	int				i;

	[renderer renderOnce:texture];
	for (aa = 1; aa <= 4; aa++)
	{
		renderer.samples = aa;
		total = 0;
		for (i = 0; i < 20; i++)
			total += [renderer renderOnce:texture];
		printf("%dx%d AA %dx%d: %.2f ms/frame (%.0f fps)\n",
			(int)texture.width, (int)texture.height, aa, aa,
			total / 20, 20000 / total);
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

	renderer.samples = 2;
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
		export_scene(argv[1], &scene);
		renderer = [[Renderer alloc] initWithScene:&scene];
		if (argc >= 3 && strcmp(argv[2], "--save") == 0)
		{
			id<MTLTexture> texture = [renderer offscreenTexture];
			printf("render: %.2f ms\n", [renderer renderOnce:texture]);
			save_png(texture, argc >= 4 ? argv[3] : "scene");
		}
		else if (argc >= 3 && strcmp(argv[2], "--bench") == 0)
			bench(renderer);
		else
			run_window(renderer, &scene);
	}
	return (0);
}
