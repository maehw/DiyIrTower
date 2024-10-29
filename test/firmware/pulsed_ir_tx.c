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

// This is just an example that permanently transmits infrared light modulated at 38 kHz.
// Warning: watch your eyes and skin! Do not directly look into the IR LED!

#ifndef F_CPU
    #define F_CPU 9600000UL
#elif F_CPU != 9600000UL
    #error "F_CPU must be set to 9.6 MHz"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <util/delay.h>

/*
 * Pin Assignment Table for the ATtiny13A:
 *
 * IC Pin | Pin  | Purpose               | Direction | Side of IC
 * --------------------------------------------------------------
 * 1      | PB5  | Reset                 | Unused    | Left
 * 2      | PB3  | IR_LED_TX_PIN         | Output    | Left
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
 *   avr-gcc -g -Wall -Os -mmcu=attiny13a -o pulsed_ir_tx.elf pulsed_ir_tx.c
 * Flash it:
 *   avrdude -c ehajo-isp -p t13a -U flash:w:pulsed_ir_tx.elf:e
 */

#define IR_ACTIVITY_LED_PIN  PB2 // GPIO output pin for activity LED (used to indicate IR activity)
#define IR_LED_TX_PIN        PB3 // GPIO output pin for IR LED

#define MODULATION_FREQ                 38000 // 38 kHz
#define MODULATION_HALF_PERIOD          (F_CPU / (2 * MODULATION_FREQ)) -2 // Calculate the half period in clock cycles; make sure that we really get this to ~2*38 kHz here
// Needed to add a small correction offset determined by measurement:
//  0: 37.5 kHz
// -1: 37.6 kHz
// -2: 38.1 kHz

#define LED_UPDATE_FREQ                 1000 // 1 kHz
#define RX_ACTIVITY_LED_ON_DURATION     160 // Duration in milliseconds for activity status LED to be ON for RX
#define TX_ACTIVITY_LED_ON_DURATION     100 // Duration in milliseconds for activity status LED to be ON for TX

volatile uint8_t modulation_enabled = 0;

void pin_change(bool on_init);

void setup() {
    // Set IR LED pin, activity status LED pin and UART RX pin as outputs
    DDRB |= (1 << IR_LED_TX_PIN) | (1 << IR_ACTIVITY_LED_PIN);

    // Configure Timer0 for 38 kHz modulation frequency
    TCCR0A = (1 << WGM01); // CTC mode
    OCR0A = MODULATION_HALF_PERIOD - 1; // Set the compare value for the desired modulation frequency
    TIMSK0 = (1 << OCIE0A); // Enable Timer0 compare match A interrupt

    TCCR0B = (1 << CS00); // Start Timer0 without pre-scaler (i.e. runs with F_CPU)
}

ISR(TIM0_COMPA_vect) {
    if(modulation_enabled) {
        // Toggle IR LED pin every two half periods
        PORTB ^= (1 << IR_LED_TX_PIN);
    }
    else
    {
        // Make sure IR LED is switched off
        PORTB &= ~(1 << IR_LED_TX_PIN);
    }
}

int main() {
    setup();
    sei(); // Enable global interrupts

    while (1) {
        // Toggle IR_ACTIVITY_LED_PIN
        PORTB ^= (1 << IR_ACTIVITY_LED_PIN);
        // Toggle IR LED modulation
        modulation_enabled = !modulation_enabled;

        _delay_ms(500);
    }

    return 0;
}
