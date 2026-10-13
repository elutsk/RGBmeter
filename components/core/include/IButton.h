#pragma once

// Hardware-independent push button.
class IButton {
public:
    virtual ~IButton() = default;

    // Returns true once per press: the first call after a press returns
    // true, the following calls return false until the next press.
    virtual bool wasPressed() = 0;
};