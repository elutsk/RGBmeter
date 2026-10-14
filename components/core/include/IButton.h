#pragma once

// Hardware-independent push button with two kinds of clicks:
// short (released before the long-click time) and long (held for it).
// Each method returns true once per event, then false until the next one.
class IButton {
public:
    virtual ~IButton() = default;

    // Reported on release, if the button was released before the long-click time.
    virtual bool wasShortClicked() = 0;

    // Reported once while the button is still held, as soon as the long-click
    // time has passed. Holding longer does not repeat it; releasing after a
    // long click does not produce a short click.
    virtual bool wasLongClicked() = 0;
};