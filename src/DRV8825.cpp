/*
 * Copyright (c) 2017 Jari Saukkonen, Samuli Vuorinen
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Arduino.h>
#include "DRV8825.h"

static int STEPTABLE[] = { 0b000, 0b001, 0b010, 0b011, 0b100, 0b111 };

DRV8825::DRV8825(int pin_dir, int pin_step, int pin_enable, int pin_m0, int pin_m1, int pin_m2) :
    pin_dir(pin_dir),
    pin_step(pin_step),
    pin_enable(pin_enable),
    pin_m0(pin_m0),
    pin_m1(pin_m1),
    pin_m2(pin_m2)
{ 
    setMicrostepping(1);
    disable();
}

void DRV8825::setMicrostepping(int microstepping)
{ 
    int index;
    switch (microstepping) {
        default:
        case 1: index = 0; break;
        case 2: index = 1; break;
        case 4: index = 2; break;
        case 8: index = 3; break;
        case 16: index = 4; break;
        case 32: index = 5; break;
    }

    int pins = STEPTABLE[index];
    digitalWrite(pin_m0, (pins & 1) ? HIGH : LOW);
    digitalWrite(pin_m1, (pins & 2) ? HIGH : LOW);
    digitalWrite(pin_m2, (pins & 4) ? HIGH : LOW);
}

void DRV8825::enable() { 
    digitalWrite(pin_enable, LOW);
    delayMicroseconds(1);
    enabled = true;
}

void DRV8825::disable()
{ 
    digitalWrite(pin_enable, HIGH);
    delayMicroseconds(1);
    enabled = false;
}

bool DRV8825::isEnabled()
{ 
    return enabled;
}

void DRV8825::step(int dir)
{ 
    digitalWrite(pin_dir, (dir > 0) ? HIGH : LOW);
    digitalWrite(pin_step, HIGH);
    delayMicroseconds(2);
    digitalWrite(pin_step, LOW);
}