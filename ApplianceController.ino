#include <Wire.h>
#include "Controller.h"


namespace Board {
constexpr uint8_t Start = 2, Door = 3, Cancel = 4, Buzzer = 5;
constexpr uint8_t Motor = 6, Light = 7, HeaterLed = 8;
constexpr uint8_t JoyX = A1, JoyY = A0, Display = 0x70;

constexpr uint8_t DoorOpenLevel = LOW;
constexpr bool PassiveBuzzer = false;
}

//class object creation
Controller controller;
Debouncer startButton, cancelButton, door;
bool displayPresent = false, speedMenu = false;
uint32_t nextDisplay = 0, beepStarted = 0, joyChanged = 0;
uint16_t beepDuration = 0;
int8_t previousX = 0, previousY = 0;

bool rawDoorOpen() {
     return digitalRead(Board::Door) == Board::DoorOpenLevel; 
     }


void stopActuators() { 
    analogWrite(Board::Motor, 0); 
    digitalWrite(Board::HeaterLed, LOW);
    }

void beep(uint32_t now, uint16_t duration) {
    beepStarted = now;
    beepDuration = duration;
    if (Board::PassiveBuzzer) tone(Board::Buzzer, 2000);
    else digitalWrite(Board::Buzzer, HIGH);
}

void command(uint8_t value) {
    Wire.beginTransmission(Board::Display);
    Wire.write(value);
    Wire.endTransmission();
}

// Segment patterns for the board's HT16K33 display.
uint16_t glyph(char ch) {
    const uint16_t digits[] = {0x3f, 0x06, 0xdb, 0xcf, 0xe6, 0xed, 0xfd, 0x07, 0xff, 0xef};
    if (ch >= '0' && ch <= '9') return digits[ch - '0'];
    switch (ch) {
    case 'O': return 0x3f; case 'P': return 0xf3; case 'E': return 0xf9;
    case 'N': return 0x2136; case 'D': return 0x120f; case 'I': return 0x1200;
    case 'L': return 0x38; case 'A': return 0xf7; case 'U': return 0x3e;
    case 'S': return 0xed; case ' ': return 0; default: return 0xc0;
    }
}

void render(uint32_t now, bool open) {
    char text[5];
    uint16_t seconds = (controller.remainingMs + 999) / 1000;
    uint8_t minutes = seconds / 60;
    bool statusPhase = (now / 1000) % 3 == 0;
    const char* label = nullptr;
    if (open) label = "OPEN";
    else if (controller.state == State::Complete) label = "DONE";
    else if (controller.state == State::Idle) label = "IDLE";
    else if (controller.state == State::Paused && statusPhase) label = "PAUS";
    if (label) { 
        for (uint8_t i=0; i<4; ++i) text[i] = label[i];
        
         }
    else if (speedMenu && controller.state != State::Running) {
        text[0]='P'; text[1]='0'+controller.speed/100;
        text[2]='0'+controller.speed/10%10; text[3]='0'+controller.speed%10;
    } else {
        text[0]='0'+minutes/10; text[1]='0'+minutes%10;
        text[2]='0'+seconds%60/10; text[3]='0'+seconds%10;
    }
    text[4]=0;
    // Stop updating the display if it stops responding.
    if (displayPresent) {
        Wire.beginTransmission(Board::Display); Wire.write(0);
        for (uint8_t i=0; i<8; ++i) {
            uint16_t bits = i<4 ? glyph(text[i]) : 0;
            Wire.write(static_cast<uint8_t>(bits)); Wire.write(static_cast<uint8_t>(bits >> 8));
        }
        if (Wire.endTransmission() != 0) displayPresent = false;
    }
    if (Serial.availableForWrite() >= 35) {
        Serial.print(F("state=")); Serial.print(static_cast<uint8_t>(controller.state));
        Serial.print(F(" display=")); Serial.print(text);
        Serial.print(F(" pwm=")); Serial.println(controller.active(open) ? controller.speed : 0);
    }
}

int8_t direction(int value) { return value < 350 ? -1 : value > 674 ? 1 : 0; }

void setup() {


    pinMode(Board::Motor, OUTPUT); 
    pinMode(Board::HeaterLed, OUTPUT); 
    stopActuators();
    pinMode(Board::Light, OUTPUT); 
    pinMode(Board::Buzzer, OUTPUT);
    digitalWrite(Board::Light, LOW); 
    digitalWrite(Board::Buzzer, LOW);
    pinMode(Board::Start, INPUT_PULLUP);
    pinMode(Board::Cancel, INPUT_PULLUP);
    pinMode(Board::Door, INPUT_PULLUP);
    uint32_t now = millis();
    startButton.begin(digitalRead(Board::Start)==LOW, now);
    cancelButton.begin(digitalRead(Board::Cancel)==LOW, now);
    door.begin(rawDoorOpen(), now);
    Serial.begin(9600);
    Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
    Wire.setWireTimeout(3000, true);
#endif
    Wire.beginTransmission(Board::Display);
    displayPresent = Wire.endTransmission()==0;
    if (displayPresent) { command(0x21); command(0x81); command(0xe5); }
}

void loop() {
    uint32_t now = millis();
    bool open = rawDoorOpen();
    if (open) stopActuators();
    door.update(open, now);
    bool start = startButton.update(digitalRead(Board::Start)==LOW, now);
    bool cancel = cancelButton.update(digitalRead(Board::Cancel)==LOW, now);
    State previous = controller.state;
    // Stop on the first open reading; debounce the door closing.
    controller.tick(now, open || door.value, start, cancel);
    if (controller.state != previous || cancel) beep(now, controller.state==State::Complete ? 700 : 70);
    if (beepDuration && now - beepStarted >= beepDuration) {
        noTone(Board::Buzzer); digitalWrite(Board::Buzzer, LOW); beepDuration=0;
    }
    int8_t x = direction(analogRead(Board::JoyX));
    int8_t y = direction(analogRead(Board::JoyY));
    if (!cancel && controller.state != State::Running && controller.state != State::Paused) {
        if (y && !previousY) { speedMenu = !speedMenu; beep(now, 40); }
        if (x && (x != previousX || now - joyChanged >= 250)) {
            joyChanged = now;
            if (speedMenu) {
                int speed = controller.speed + x * 10;
                controller.speed = constrain(speed, 0, 255);
            } else controller.adjustTime(x);
        }
    }
    previousX=x; previousY=y;
    bool enabled = controller.active(rawDoorOpen());
    analogWrite(Board::Motor, enabled ? controller.speed : 0);
    digitalWrite(Board::HeaterLed, enabled ? HIGH : LOW);
    digitalWrite(Board::Light, open || enabled ? HIGH : LOW);
    if (now - nextDisplay >= 200) { nextDisplay=now; render(now, open); }
}
