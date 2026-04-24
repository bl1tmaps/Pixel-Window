#ifndef PIXEL_WINDOW_H
#define PIXEL_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PixelWindow PixelWindow;

// Creates a window with the specified dimensions.
PixelWindow* pw_create_window(int width, int height, const char* title, bool resizable);

// Processes OS events (like closing the window). Returns false if the window is closed.
bool pw_process_events(PixelWindow* win);

// Takes a tightly packed 8-bit RGB array (size: width * height * 3) and draws it.
void pw_update_window(PixelWindow* win, const uint8_t* rgb_buffer);

// Cleans up resources.
void pw_destroy_window(PixelWindow* win);

#ifdef __cplusplus
}
#endif

#endif // PIXEL_WINDOW_H

// ============================================================================
// IMPLEMENTATION
// Define PIXEL_WINDOW_IMPLEMENTATION in exactly ONE source file.
// ============================================================================

#ifdef PIXEL_WINDOW_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// WINDOWS (Win32)
// ----------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

struct PixelWindow {
    int width;
    int height;
    bool is_running;
    HWND hwnd;
    uint8_t* bgra_buffer; // Win32 standard is BGRA
    BITMAPINFO bmi;
};

static LRESULT CALLBACK pw__window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PixelWindow* win = (PixelWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (uMsg == WM_CLOSE || uMsg == WM_DESTROY) {
        if (win) win->is_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

PixelWindow* pw_create_window(int width, int height, const char* title, bool resizable) {
    PixelWindow* win = (PixelWindow*)calloc(1, sizeof(PixelWindow));
    win->width = width;
    win->height = height;
    win->is_running = true;
    win->bgra_buffer = (uint8_t*)malloc(width * height * 4);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = pw__window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "PixelWindowClass";
    RegisterClass(&wc);

    // Set up dynamic window styles based on the flag
    DWORD window_style = WS_OVERLAPPEDWINDOW;
    if (!resizable) {
        window_style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, window_style, FALSE);

    win->hwnd = CreateWindowEx(0, "PixelWindowClass", title,
        window_style | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);

    SetWindowLongPtr(win->hwnd, GWLP_USERDATA, (LONG_PTR)win);

    win->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    win->bmi.bmiHeader.biWidth = width;
    win->bmi.bmiHeader.biHeight = -height; // Top-down
    win->bmi.bmiHeader.biPlanes = 1;
    win->bmi.bmiHeader.biBitCount = 32;
    win->bmi.bmiHeader.biCompression = BI_RGB;

    return win;
}

bool pw_process_events(PixelWindow* win) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return win->is_running;
}

void pw_update_window(PixelWindow* win, const uint8_t* rgb_buffer) {
    int pixels = win->width * win->height;
    // Swizzle RGB to BGRA
    for (int i = 0; i < pixels; ++i) {
        win->bgra_buffer[i*4 + 0] = rgb_buffer[i*3 + 2]; // B
        win->bgra_buffer[i*4 + 1] = rgb_buffer[i*3 + 1]; // G
        win->bgra_buffer[i*4 + 2] = rgb_buffer[i*3 + 0]; // R
        win->bgra_buffer[i*4 + 3] = 255;                 // A
    }

    HDC hdc = GetDC(win->hwnd);
    StretchDIBits(hdc, 0, 0, win->width, win->height,
                  0, 0, win->width, win->height,
                  win->bgra_buffer, &win->bmi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(win->hwnd, hdc);
}

void pw_destroy_window(PixelWindow* win) {
    if (!win) return;
    DestroyWindow(win->hwnd);
    free(win->bgra_buffer);
    free(win);
}

// ----------------------------------------------------------------------------
// LINUX (X11)
// ----------------------------------------------------------------------------
#elif defined(__linux__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct PixelWindow {
    int width;
    int height;
    bool is_running;
    Display* display;
    Window window;
    GC gc;
    XImage* ximage;
    uint8_t* bgra_buffer; // X11 24-depth usually implies 32-bit BGRA (ZPixmap)
    Atom wm_delete_window;
};

PixelWindow* pw_create_window(int width, int height, const char* title, bool resizable) {
    PixelWindow* win = (PixelWindow*)calloc(1, sizeof(PixelWindow));
    win->width = width;
    win->height = height;
    win->is_running = true;
    win->bgra_buffer = (uint8_t*)malloc(width * height * 4);

    win->display = XOpenDisplay(NULL);
    int screen = DefaultScreen(win->display);

    win->window = XCreateSimpleWindow(win->display, RootWindow(win->display, screen),
        0, 0, width, height, 0,
        BlackPixel(win->display, screen),
        BlackPixel(win->display, screen));

    XStoreName(win->display, win->window, title);

    // --- NEW X11 RESIZING LOGIC ---
    if (!resizable) {
        XSizeHints* hints = XAllocSizeHints();
        if (hints) {
            hints->flags = PMinSize | PMaxSize;
            hints->min_width = hints->max_width = width;
            hints->min_height = hints->max_height = height;
            XSetWMNormalHints(win->display, win->window, hints);
            XFree(hints);
        }
    }
    // ------------------------------

    win->wm_delete_window = XInternAtom(win->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win->display, win->window, &win->wm_delete_window, 1);

    XSelectInput(win->display, win->window, ExposureMask | KeyPressMask);
    XMapWindow(win->display, win->window);

    win->gc = DefaultGC(win->display, screen);

    win->ximage = XCreateImage(win->display, DefaultVisual(win->display, screen),
        24, ZPixmap, 0, (char*)win->bgra_buffer,
        width, height, 32, 0);

    return win;
}

bool pw_process_events(PixelWindow* win) {
    XEvent event;
    while (XPending(win->display) > 0) {
        XNextEvent(win->display, &event);
        if (event.type == ClientMessage) {
            if ((Atom)event.xclient.data.l[0] == win->wm_delete_window) {
                win->is_running = false;
            }
        }
    }
    return win->is_running;
}

void pw_update_window(PixelWindow* win, const uint8_t* rgb_buffer) {
    int pixels = win->width * win->height;
    // Swizzle RGB to BGRA for XImage
    for (int i = 0; i < pixels; ++i) {
        win->bgra_buffer[i*4 + 0] = rgb_buffer[i*3 + 2]; // B
        win->bgra_buffer[i*4 + 1] = rgb_buffer[i*3 + 1]; // G
        win->bgra_buffer[i*4 + 2] = rgb_buffer[i*3 + 0]; // R
        win->bgra_buffer[i*4 + 3] = 255;
    }

    XPutImage(win->display, win->window, win->gc, win->ximage, 0, 0, 0, 0, win->width, win->height);
    XFlush(win->display);
}

void pw_destroy_window(PixelWindow* win) {
    if (!win) return;
    win->ximage->data = NULL; // Prevent XDestroyImage from freeing our buffer, we do it
    XDestroyImage(win->ximage);
    XDestroyWindow(win->display, win->window);
    XCloseDisplay(win->display);
    free(win->bgra_buffer);
    free(win);
}

// ----------------------------------------------------------------------------
// macOS (Metal)
// ----------------------------------------------------------------------------
#elif defined(__APPLE__)

// IMPORTANT: On macOS, the file defining PIXEL_WINDOW_IMPLEMENTATION must 
// be compiled as Objective-C++ (.mm) and use ARC (-fobjc-arc).
#ifdef __OBJC__

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct PixelWindow {
    int width;
    int height;
    bool is_running;
    uint8_t* rgba_buffer;
    
    NSWindow* window;
    CAMetalLayer* metal_layer;
    id<MTLDevice> device;
    id<MTLCommandQueue> command_queue;
    id<MTLRenderPipelineState> pipeline_state;
    id<MTLTexture> texture;
};

// Minimal MSL Shader to draw a fullscreen texture
static const char* msl_source = R"(
#include <metal_stdlib>
using namespace metal;
struct VertexOut { float4 pos [[position]]; float2 uv; };
vertex VertexOut vs(uint vid [[vertex_id]]) {
    float2 pos[] = { float2(-1, -1), float2(1, -1), float2(-1, 1), float2(1, 1) };
    float2 uv[]  = { float2(0, 1),   float2(1, 1),  float2(0, 0),  float2(1, 0) };
    return { float4(pos[vid], 0.0, 1.0), uv[vid] };
}
fragment float4 fs(VertexOut in [[stage_in]], texture2d<float> tex [[texture(0)]]) {
    constexpr sampler s(filter::nearest);
    return tex.sample(s, in.uv);
}
)";

@interface PWWindowDelegate : NSObject <NSWindowDelegate>
@property (nonatomic, assign) bool* is_running_ptr;
@end
@implementation PWWindowDelegate
- (BOOL)windowShouldClose:(id)sender {
    if (_is_running_ptr) *_is_running_ptr = false;
    return YES;
}
@end

PixelWindow* pw_create_window(int width, int height, const char* title, bool resizable) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    
    PixelWindow* win = (PixelWindow*)calloc(1, sizeof(PixelWindow));
    win->width = width;
    win->height = height;
    win->is_running = true;
    win->rgba_buffer = (uint8_t*)malloc(width * height * 4);

    // Set up dynamic window styles
    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;
    if (resizable) {
        style |= NSWindowStyleMaskResizable;
    }

    NSRect rect = NSMakeRect(0, 0, width, height);
    win->window = [[NSWindow alloc] initWithContentRect:rect
                                              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [win->window setTitle:[NSString stringWithUTF8String:title]];
    [win->window center];
    
    PWWindowDelegate* delegate = [[PWWindowDelegate alloc] init];
    delegate.is_running_ptr = &win->is_running;
    [win->window setDelegate:delegate];
    
    // Metal Setup
    win->device = MTLCreateSystemDefaultDevice();
    win->command_queue = [win->device newCommandQueue];
    
    NSView* view = [win->window contentView];
    [view setWantsLayer:YES];
    win->metal_layer = [CAMetalLayer layer];
    win->metal_layer.device = win->device;
    win->metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    win->metal_layer.framebufferOnly = YES;
    win->metal_layer.drawableSize = CGSizeMake(width, height);
    [view setLayer:win->metal_layer];
    
    // Texture Setup
    MTLTextureDescriptor* texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                                                                                       width:width
                                                                                      height:height
                                                                                   mipmapped:NO];
    win->texture = [win->device newTextureWithDescriptor:texDesc];
    
    // Pipeline Setup
    NSError* error = nil;
    NSString* sourceString = [NSString stringWithUTF8String:msl_source];
    id<MTLLibrary> library = [win->device newLibraryWithSource:sourceString options:nil error:&error];
    
    MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDesc.vertexFunction = [library newFunctionWithName:@"vs"];
    pipelineDesc.fragmentFunction = [library newFunctionWithName:@"fs"];
    pipelineDesc.colorAttachments[0].pixelFormat = win->metal_layer.pixelFormat;
    
    win->pipeline_state = [win->device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];

    [win->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    
    return win;
}

bool pw_process_events(PixelWindow* win) {
    NSEvent* event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:nil
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES])) {
        [NSApp sendEvent:event];
    }
    return win->is_running;
}

void pw_update_window(PixelWindow* win, const uint8_t* rgb_buffer) {
    int pixels = win->width * win->height;
    // Pad RGB to RGBA
    for (int i = 0; i < pixels; ++i) {
        win->rgba_buffer[i*4 + 0] = rgb_buffer[i*3 + 0]; // R
        win->rgba_buffer[i*4 + 1] = rgb_buffer[i*3 + 1]; // G
        win->rgba_buffer[i*4 + 2] = rgb_buffer[i*3 + 2]; // B
        win->rgba_buffer[i*4 + 3] = 255;
    }

    MTLRegion region = MTLRegionMake2D(0, 0, win->width, win->height);
    [win->texture replaceRegion:region mipmapLevel:0 withBytes:win->rgba_buffer bytesPerRow:win->width * 4];

    id<CAMetalDrawable> drawable = [win->metal_layer nextDrawable];
    if (drawable) {
        id<MTLCommandBuffer> commandBuffer = [win->command_queue commandBuffer];
        
        MTLRenderPassDescriptor* passDesc = [MTLRenderPassDescriptor renderPassDescriptor];
        passDesc.colorAttachments[0].texture = drawable.texture;
        passDesc.colorAttachments[0].loadAction = MTLLoadActionClear;
        passDesc.colorAttachments[0].storeAction = MTLStoreActionStore;
        
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:passDesc];
        [encoder setRenderPipelineState:win->pipeline_state];
        [encoder setFragmentTexture:win->texture atIndex:0];
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
        [encoder endEncoding];
        
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

void pw_destroy_window(PixelWindow* win) {
    if (!win) return;
    free(win->rgba_buffer);
    free(win);
}

#endif // __OBJC__
#endif // __APPLE__

#endif // PIXEL_WINDOW_IMPLEMENTATION
