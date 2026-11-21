#include "Controller.h"

void Controller::tick(uint32_t now, bool doorOpen, bool start, bool cancel) {
        uint32_t elapsed = now - last;
        last = now;
        if (cancel) { 
            remainingMs = 0;
            state = State::Idle; 
            return; }
        if (state == State::Running) {
            if (elapsed >= remainingMs) { 
            remainingMs = 0; state = State::Complete; }
            else remainingMs -= elapsed;
        }
        if (doorOpen) {
            if (state == State::Running) state = State::Paused;
            return;
        }
        if (start) {
            if (state == State::Running) state = State::Paused;
            else if (remainingMs > 0 && (state == State::Ready || state == State::Paused))
                state = State::Running;
        }
    }

void Controller::adjustTime(int8_t direction) {
        if (state == State::Running || state == State::Paused) return;
        int32_t value = static_cast<int32_t>(remainingMs) + direction * 10000L;
        remainingMs = value < 0 ? 0 : value > 5990000L ? 5990000L : value;
        state = remainingMs ? State::Ready : State::Idle;
    }

bool Controller::active(bool rawDoorOpen) const { return state == State::Running && !rawDoorOpen; }

void Debouncer::begin(bool initial, uint32_t now) { 
        value = candidate = initial; changed = now; 
        }

bool Debouncer::update(bool raw, uint32_t now) {
        if (raw != candidate) { 
            candidate = raw; changed = now;
             }
        if (candidate != value && now - changed >= 30) {
            value = candidate;
            return value;
        }
        return false;
    }
