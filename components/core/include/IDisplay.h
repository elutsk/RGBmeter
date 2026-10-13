#pragma once

// Hardware-independent monochrome text display.
// Drawing calls only change an in-memory framebuffer;
// flush() sends the framebuffer to the panel.
class IDisplay {
public:
    virtual ~IDisplay() = default;

    virtual void clear() = 0;

    // Draws ASCII text with its top-left corner at (x, y), in pixels.
    // scale = 1 is the native font size, 2 doubles it, and so on.
    virtual void drawText(int x, int y, const char* text, int scale) = 0;

    // Returns false if the panel did not accept the data.
    virtual bool flush() = 0;
};