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

#ifndef DRV8825_H
#define DRV8825_H

class DRV8825 {
public:
    DRV8825(int pin_dir, int pin_step, int pin_enable, int pin_m0, int pin_m1, int pin_m2);

    void setMicrostepping(int microstepping);

    void enable();
    void disable();
    bool isEnabled();

    void step(int dir);

private:
    int pin_dir;
    int pin_step;
    int pin_enable;
    int pin_m0;
    int pin_m1;
    int pin_m2;

    bool enabled;
};

#endif