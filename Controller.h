#pragma once
#include <stdint.h>

enum class State : uint8_t { Idle, Ready, Running, Paused, Complete };

// State transitions and countdown, independent of the board hardware.
class Controller {
public:
    State state = State::Idle;
    uint32_t remainingMs = 0;
    uint8_t speed = 140;

    void tick(uint32_t now, bool doorOpen, bool start, bool cancel);

    void adjustTime(int8_t direction);
    bool active(bool rawDoorOpen) const;
private:
    uint32_t last = 0;
};


// Accept a switch change after it stays stable for 30 ms.
class Debouncer {
public:
    bool value = false;
    void begin(bool initial, uint32_t now);
    bool update(bool raw, uint32_t now);
private:
    bool candidate = false;
    uint32_t changed = 0;
};
