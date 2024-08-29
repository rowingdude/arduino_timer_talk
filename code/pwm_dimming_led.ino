
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

#define LED_PIN 9  
#define MODE_PIN 7

volatile uint8_t mode = 0;

void configureTimers() {
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 1023;

    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A = 255;
    TIMSK2 = (1 << OCIE2A);
} // Configures Timer1 and Timer2 for PWM and interrupts

void configurePins() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(MODE_PIN, OUTPUT);
} // Configures LED_PIN and MODE_PIN as outputs

void setup() {
    configureTimers();
    configurePins();
    sei(); // Enables global interrupts
} // Sets up timers, pins, and interrupts

void updateMode() {
    mode = (mode + 1) % 3;
    digitalWrite(MODE_PIN, HIGH); 
    digitalWrite(MODE_PIN, LOW);
} // Updates mode and toggles MODE_PIN

void rampUp() {
    for (int i = 0; i < 1024; i++) {
        OCR1A = i;
        delay(1);
    }
} // Ramps up the PWM signal

void rampDown() {
    for (int i = 1023; i >= 0; i--) {
        OCR1A = i;
        delay(1);
    }
} // Ramps down the PWM signal

void pulse() {
    for (int i = 0; i < 1024; i++) {
        OCR1A = i;
        delay(1);
    }
    for (int i = 1023; i >= 0; i--) {
        OCR1A = i;
        delay(1);
    }
} // Pulses the PWM signal

ISR(TIMER2_COMPA_vect) {
    updateMode();
} // Timer2 interrupt service routine

void loop() {
    switch (mode) {
        case 0: 
            rampUp();
            break;
        case 1:
            rampDown();
            break;
        case 2:
            pulse();
            break;
    }
} // Executes mode-specific PWM patterns