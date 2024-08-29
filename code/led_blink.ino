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


// See Wokwi for layout and test run
// https://wokwi.com/projects/407487610175451137

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer1.h"

#define LED_PIN 13
#define TOGGLE_PIN 8 // in real life we'd see a square wave here.

volatile uint32_t seconds = 0;
volatile uint16_t fast_toggle_count = 0;

void configure_timer1() {
    TIMER1_CONTROL_A = 0;
    TIMER1_CONTROL_B = 0;
    
    TIMER1_CONTROL_B |= (1 << WGM12);
    TIMER1_CONTROL_B |= (1 << CS11) | (1 << CS10);
    
    TIMER1_COMPARE_A = 250;
    TIMER1_INT_MASK |= (1 << OCIE1A);
    
    TIMER1_COUNTER = 0;
} // This function sets up Timer1 for 1ms interrupts

ISR(TIMER1_COMPA_vect) {
    static uint16_t ms_count = 0;
    ms_count++;
    
    if (fast_toggle_count++ == 10) {
        toggle_pin(PORTB0); 
        fast_toggle_count = 0;
    }
    
    if (ms_count == 1000) {
        seconds++;
        toggle_pin(PORTB5); 
        ms_count = 0;
    }
} // This ISR handles the Timer1 compare match events

void initialize_system() {
    DDRB |= (1 << DDB5) | (1 << DDB0);
    configure_timer1();
    sei();
} // This function initializes I/O pins and Timer1

void toggle_pin(uint8_t pin) {  PORTB ^= (1 << pin); } // This function toggles a specified pin
void loop() { } // This function is an empty loop as all tasks are handled in the ISR

int main(void) {
    initialize_system();
    while(1) {
        loop();
    }
    return 0;
} // This function starts the program and continuously calls the loop
