// +--------------------------------------------------------------+
// | Benjamin Cance                                               |
// | Email: bjc@tdx.li                                            |
// +--------------------------------------------------------------+
// | Code Creation Date: 8/29/2024                                |
// +--------------------------------------------------------------+
// | Copyright: 2024                                              |
// | License: MIT License                                         |
// +--------------------------------------------------------------+
// | Contributions, corrections, and comments encouraged.         |
// +--------------------------------------------------------------+
// | DISCLAIMER: Use this code at your own risk. The author is    |
// | not responsible for any damage to hardware or software.      |
// +--------------------------------------------------------------+

// See wokwi link for layout and to test functionality
// https://wokwi.com/projects/407555272630754305

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED_PIN 9  // OC1A
#define MODE_PIN 7

volatile uint8_t mode = 0;

void setup() {
    // Set up Timer1 for PWM
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 1023;  // Top value for 10-bit PWM

    // Set up Timer2 for 1-second intervals
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A = 255;
    TIMSK2 = (1 << OCIE2A);

    pinMode(LED_PIN, OUTPUT);
    pinMode(MODE_PIN, OUTPUT);

    sei();
}

ISR(TIMER2_COMPA_vect) {
    mode = (mode + 1) % 3;  // Cycle through 3 modes
    digitalWrite(MODE_PIN, HIGH);  // Signal mode change
    digitalWrite(MODE_PIN, LOW);
}

void loop() {
    switch (mode) {
        case 0: // Ramp up
            for (int i = 0; i < 1024; i++) {
                OCR1A = i;
                delay(1);
            }
            break;
        case 1: // Ramp down
            for (int i = 1023; i >= 0; i--) {
                OCR1A = i;
                delay(1);
            }
            break;
        case 2: // Pulse
            for (int i = 0; i < 1024; i++) {
                OCR1A = i;
                delay(1);
            }
            for (int i = 1023; i >= 0; i--) {
                OCR1A = i;
                delay(1);
            }
            break;
    }
}