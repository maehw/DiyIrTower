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

// This is the productive firmware.

#ifndef F_CPU
    #define F_CPU 9600000UL
#elif F_CPU != 9600000UL
    #error "F_CPU must be set to 9.6 MHz"
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

/*
 * Pin Assignment Table for the ATtiny13A:
 *
 * IC Pin | Pin  | Purpose               | Direction | Side of IC
 * --------------------------------------------------------------
 * 1      | PB5  | Reset                 | Unused    | Left
 * 2      | PB3  | IR_LED_TX_PIN         | Output    | Left
 * 3      | PB4  | PHOTO_RX_PIN          | Input     | Left
 * 4      | GND  | Ground                | -         | Left
 * 5      | PB0  | UART_RX_PIN           | Output    | Right
 * 6      | PB1  | UART_TX_PIN           | Input     | Right
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
 *   avr-gcc -g -Wall -Os -mmcu=attiny13a -o main.elf main.c
 * Flash it:
 *   avrdude -c ehajo-isp -p t13a -U flash:w:main.elf:e
 */

#define IR_LED_TX_PIN        PB3 // GPIO output pin for IR LED
#define UART_TX_PIN          PB1 // GPIO input pin for enabling modulation
#define PHOTO_RX_PIN         PB4 // GPIO input pin for the demodulated signal from photo-transistor
#define IR_ACTIVITY_LED_PIN  PB2 // GPIO output pin for indicating IR TX modulation or IR RX signal detection from photo-transistor
#define UART_RX_PIN          PB0 // GPIO output pin that mirrors the input from the photo-transistor

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
volatile uint8_t activity_led_counter = 0;
volatile uint8_t modulation_counter = 0;
volatile uint8_t prev_photo_rx_state = 0;
volatile uint8_t prev_uart_tx_state = 0;

void pin_change(bool on_init);

void setup() {
    // Set IR LED pin, activity status LED pin and UART RX pin as outputs
    DDRB |= (1 << IR_LED_TX_PIN) | (1 << IR_ACTIVITY_LED_PIN) | (1 << UART_RX_PIN);

    // Set enable pin as input
    DDRB &= ~(1 << UART_TX_PIN);

    // Enable internal pull-up resistor for enable pin and photo-transistor input pin
    PORTB |= (1 << UART_TX_PIN) | (1 << PHOTO_RX_PIN);

    // Enable Pin Change Interrupt for PHOTO_RX_PIN (PB4) and UART_TX_PIN (PB1)
    GIMSK |= (1 << PCIE); // Enable Pin Change Interrupt
    PCMSK |= (1 << PCINT4) | (1 << PCINT1); // Enable Pin Change Interrupt on PHOTO_RX_PIN (PB4) and UART_TX_PIN (PB1)

    // Configure Timer0 for 38 kHz modulation frequency
    TCCR0A = (1 << WGM01); // CTC mode
    OCR0A = MODULATION_HALF_PERIOD - 1; // Set the compare value for the desired modulation frequency
    TIMSK0 = (1 << OCIE0A); // Enable Timer0 compare match A interrupt

    // Initialize to correct state
    pin_change(true);

    TCCR0B = (1 << CS00); // Start Timer0 without pre-scaler (i.e. runs with F_CPU)
}

void pin_change(bool on_init) {
    // Get current states of pins PHOTO_RX_PIN and UART_TX_PIN for determining interrupt source and for edge detection
    uint8_t current_photo_rx_state = PINB & (1 << PHOTO_RX_PIN);
    uint8_t current_uart_tx_state = PINB & (1 << UART_TX_PIN);

    if ((current_uart_tx_state != prev_uart_tx_state) || on_init) {
        // UART_TX_PIN state has changed
        if (current_uart_tx_state) {
            // Rising edge detected on UART_TX_PIN

            // Disable modulation
            modulation_enabled = 0;

            // Set IR LED pin to LOW as modulation is disabled
            PORTB &= ~(1 << IR_LED_TX_PIN);
        } else {
            // Falling edge detected on UART_TX_PIN

            // Enable modulation
            modulation_enabled = 1;

            // Re-set duration for activity status LED to be on
            activity_led_counter = (TX_ACTIVITY_LED_ON_DURATION > activity_led_counter) ? TX_ACTIVITY_LED_ON_DURATION : activity_led_counter;

            // Turn on activity status LED
            PORTB |= (1 << IR_ACTIVITY_LED_PIN);
        }

        // Reset modulation counter
        modulation_counter = 0;
    }

    if ((current_photo_rx_state != prev_photo_rx_state) || on_init) {
        // PHOTO_RX_PIN state has changed; it is an active-low signal

        if (current_photo_rx_state) {
            // Rising edge detected on PHOTO_RX_PIN

            // Also set UART_RX_PIN high
            PORTB |= (1 << UART_RX_PIN);
        } else {
            // Falling edge detected on PHOTO_RX_PIN

            // Also set UART_RX_PIN low
            PORTB &= ~(1 << UART_RX_PIN);

            // Re-set duration for activity status LED to be on
            activity_led_counter = (RX_ACTIVITY_LED_ON_DURATION > activity_led_counter) ? RX_ACTIVITY_LED_ON_DURATION : activity_led_counter;

            // Turn on activity status LED
            PORTB |= (1 << IR_ACTIVITY_LED_PIN);
        }
    }

    // Update previous state variables
    prev_photo_rx_state = current_photo_rx_state;
    prev_uart_tx_state = current_uart_tx_state;
}

ISR(PCINT0_vect) {
    pin_change(false);
}

ISR(TIM0_COMPA_vect) {
    // Take care of IR modulation
    if (modulation_enabled) {
        // Toggle IR LED pin every two half periods
        PORTB ^= (1 << IR_LED_TX_PIN);
    }

    // Check if it's time to update LED counters
    static uint8_t led_counter = 0;
    led_counter++;
    if (led_counter >= ((2 * MODULATION_FREQ)/LED_UPDATE_FREQ)) {
        // Reset the counter
        led_counter = 0;

        // Update LED counters: check if activity status LED needs to be switched off again (should be on right now)
        if (activity_led_counter > 0) {
            activity_led_counter--;
            if (activity_led_counter == 0) {
                PORTB &= ~(1 << IR_ACTIVITY_LED_PIN);
            }
        }
    }
}

int main() {
    setup();
    sei(); // Enable global interrupts

    while (1) {
        // Main loop remains idle
    }

    return 0;
}
