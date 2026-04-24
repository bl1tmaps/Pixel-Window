#define PIXEL_WINDOW_IMPLEMENTATION
#include "pixel_window.h"

// Generate a simple RGB test pattern
void generate_pattern(uint8_t* buffer, int width, int height, int offset) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = (y * width + x) * 3;
            buffer[i + 0] = (uint8_t)(x + offset); // R
            buffer[i + 1] = (uint8_t)(y + offset); // G
            buffer[i + 2] = 128;                   // B
        }
    }
}

int main() {
    int width = 800, height = 600;
    uint8_t* rgb_buffer = (uint8_t*)malloc(width * height * 3);
    
    PixelWindow* win = pw_create_window(width, height, "Cross-Platform Pixel Buffer", false);
    
    int offset = 0;
    while (pw_process_events(win)) {
        generate_pattern(rgb_buffer, width, height, offset++);
        pw_update_window(win, rgb_buffer);
    }
    
    pw_destroy_window(win);
    free(rgb_buffer);
    return 0;
}
