## Jonathan Oluchi Akabuike 
## ApplianceController

A C++ microwave controller for an Arduino Uno / ATmega328P training board, with joystick controls, a turntable output, switches, a buzzer, LEDs, and an HT16K33 display.


## Wiring


| Component | Pin |
| --- | --- |
| Start / pause | D2, active LOW |
| Door switch | D3, LOW means open |
| Cancel | D4, active LOW |
| Active buzzer | D5 |
| Turntable output | D6 |
| Chamber light | D7 |
| Heater LED | D8 |
| Joystick Y / X | A0 / A1 |
| HT16K33 display | SDA A4, SCL A5, address 0x70 |


## Controls

- Left/right changes time in 10-second steps, up to 99:50. Holding the joystick repeats every 250 ms.
- Up/down switches between time and motor PWM adjustment. `P140` means PWM 140 out of 255.
- Start begins, pauses, or resumes the countdown.
- Cancel clears the timer and turns off the motor and heater LED.
- Opening the door pauses the countdown and disables the outputs. Closing it does not restart the cycle; press Start to resume.
- When time runs out, the display shows `DONE` and the buzzer sounds.

The display shows `OPEN` when the door is open, `IDLE` with no time set, and remaining time as MMSS. While paused, it alternates between `PAUS` and the time.

## Code

`Controller.h` declares the state machine and debouncer; `Controller.cpp` implements them. `ApplianceController.ino` handles the hardware and serial output.

The states are Idle, Ready, Running, Paused, and Complete. Buttons are polled and debounced for 30 ms. The countdown and beeps use `millis()` so inputs can still be read while they run. Cancel takes priority over Start.

## Building

Open `ApplianceController.ino` in Arduino IDE and select Arduino Uno. The display uses the built-in Wire library. Serial output is 9600 baud.


## Testing

ControllerTests.cpp` checks transitions, cancellation, timer limits, completion, rollover, and debounce.

Bench checks:

- Outputs remain off at startup and when Start is pressed with no time set.
- Start, pause, and resume each happen once per button press.
- Opening the door stops the outputs; closing it alone does not restart them.
- Cancel clears the countdown, including when Start is pressed at the same time.
- Completion stops the outputs and sounds the buzzer.
- Time and PWM stay within their limits when the joystick is held.
- A disconnected display does not prevent door and button handling.
