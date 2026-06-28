# Pixel Window

A minimal, cross platform window-spawning library. 
Provides a buffer of RGB pixels, mouse, and realtime keyboard, and Sprite Text on Mac OS, 
Linux X11/Wayland, and Windows (Win32). All CPU rendered.

## Examples

The core premise:
```
    // ...
    PixelWindow* win = pw_create_window(width, height, "Interactive Pixel Window", false);
    
    // ...
    while (something) {
        // ...
        pw_update_window(win, rgb_buffer);
        // ...
    }
```
see main.cpp for an example.

[MacOS](assets/Macos.png)
