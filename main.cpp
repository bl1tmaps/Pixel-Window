#define PIXEL_WINDOW_IMPLEMENTATION
#include "pixel_window.h"
#include <stdio.h> // Needed for printf

// Generate a simple RGB test pattern (our background)
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

// Draw a solid rectangle into the pixel buffer
void draw_rectangle(uint8_t* buffer, int win_width, int win_height, int cx, int cy, int size, uint8_t r, uint8_t g, uint8_t b) {
    int half_size = size / 2;

    for (int y = cy - half_size; y <= cy + half_size; ++y) {
        for (int x = cx - half_size; x <= cx + half_size; ++x) {
            // BOUNDS CHECK: Extremely important so we don't crash when the mouse hits the edge
            if (x >= 0 && x < win_width && y >= 0 && y < win_height) {
                int i = (y * win_width + x) * 3;
                buffer[i + 0] = r;
                buffer[i + 1] = g;
                buffer[i + 2] = b;
            }
        }
    }
}

int main() {
    int width = 800, height = 600;
    uint8_t* rgb_buffer = (uint8_t*)malloc(width * height * 3);

    // Remember to pass 'false' for the resizable flag we added earlier
    PixelWindow* win = pw_create_window(width, height, "Interactive Pixel Window", false);

    int offset = 0;
    while (pw_process_events(win)) {
        // 1. Draw the animated background pattern
        generate_pattern(rgb_buffer, width, height, offset++);

        // 2. Fetch the current input state
        const PwInputState* input = pw_get_input(win);

        // 3. Print any keys pressed this frame to the terminal
        for (int i = 0; i < 512; ++i) {
            if (input->keys_pressed[i]) {
                // If it's a printable ASCII character, show the letter too
                if (i >= 32 && i <= 126) {
                    printf("Key pressed: %c (Code: %d)\n", (char)i, i);
                }
                else {
                    printf("Key pressed: Code %d\n", i);
                }
            }
        }

        // 4. Calculate mouse pixel coordinates from normalized coords
        int mouse_px = (int)(input->mouse_x * width);
        int mouse_py = (int)(input->mouse_y * height);

        // 5. Determine rectangle color (Green if ANY button is down, else Red)
        bool any_button_down = input->mouse_down[0] || input->mouse_down[1] || input->mouse_down[2];
        uint8_t r = any_button_down ? 0 : 255;
        uint8_t g = any_button_down ? 255 : 0;
        uint8_t b = 0;

        // 6. Draw a 20x20 rectangle at the mouse cursor
        draw_rectangle(rgb_buffer, width, height, mouse_px, mouse_py, 20, r, g, b);

        // 7. Push the buffer to the screen
        pw_update_window(win, rgb_buffer);
    }

    pw_destroy_window(win);
    free(rgb_buffer);
    return 0;
}