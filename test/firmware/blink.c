/*
 * DiyIrTower
 * Copyright (C) 2024 maehw
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This is just a classic "blinky" example that toggles the activity LED at 1 Hz (no infrared communication involved).

#ifndef F_CPU
    #define F_CPU 9600000UL
#elif F_CPU != 9600000UL
    #error "F_CPU must be set to 9.6 MHz"
#endif

#include <avr/io.h>
#include <util/delay.h>

/*
 * Pin Assignment Table for the ATtiny13A:
 *
 * IC Pin | Pin  | Purpose               | Direction | Side of IC
 * ----------------------------------------------------------------
 * 1      | PB5  | Reset                 | Unused    | Left
 * 2      | PB3  | Unused                | Unused    | Left
 * 3      | PB4  | Unused                | Unused    | Left
 * 4      | GND  | Ground                | -         | Left
 * 5      | PB0  | Unused                | Unused    | Right
 * 6      | PB1  | Unused                | Unused    | Right
 * 7      | PB2  | IR_ACTIVITY_LED_PIN   | Output    | Right
 * 8      | VCC  | Supply Voltage (VCC)  | -         | Right
 */

/*
 * Make sure to burn the fuses so that
 * - the calibrated internal 9.6 MHz oscillator is selected
 * - clock pre-scaler set to 1 (device is shipped with CKDIV8 programmed; which is not what we want).
 *
 * Reading fuses with avrdude (and in this case an ehajo-isp):
 *   avrdude -c ehajo-isp -p t13a -U hfuse:r:-:h -U lfuse:r:-:h
 * The default is: 0xff (high), 0x6a (low)
 *
 * The required setting is: 0xff (high), 0x7a (low)
 * Writing fuses with avrdude:
 *   avrdude -c ehajo-isp -p t13a -U hfuse:w:0xff:m -U lfuse:w:0x7a:m
 *
 * Compile and link it like this:
 *   avr-gcc -g -Wall -Os -mmcu=attiny13a -o blink.elf blink.c
 * Flash it:
 *   avrdude -c ehajo-isp -p t13a -U flash:w:blink.elf:e
 */

#define IR_ACTIVITY_LED_PIN  PB2 // GPIO output pin for activity LED (in production firmware used to indicate IR activity)

void setup() {
    // Set LED pin as output
    DDRB |= (1 << IR_ACTIVITY_LED_PIN);

    // Initially it on
    PORTB |= (1 << IR_ACTIVITY_LED_PIN);

    _delay_ms(100);
}

int main() {
    setup();

    while (1) {
        // Toggle IR_ACTIVITY_LED_PIN
        PORTB ^= (1 << IR_ACTIVITY_LED_PIN);

        _delay_ms(500);
    }

    return 0;
}
