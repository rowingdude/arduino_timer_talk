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
// https://wokwi.com/projects/407493956117131265


#include <avr/io.h>
#include <avr/interrupt.h>

#define RED_PIN 2
#define YELLOW_PIN 4
#define GREEN_PIN 7
#define PED_RED_PIN 5
#define PED_GREEN_PIN 6
#define BUTTON_PIN 3 

volatile uint8_t traffic_state = 0;
volatile uint16_t timer_count = 0;

void traffic_red_on() { PORTD |= (1 << RED_PIN); }         
void traffic_red_off() { PORTD &= ~(1 << RED_PIN); }       
void traffic_yellow_on() { PORTD |= (1 << YELLOW_PIN); }   
void traffic_yellow_off() { PORTD &= ~(1 << YELLOW_PIN); } 
void traffic_green_on() { PORTD |= (1 << GREEN_PIN); }   
void traffic_green_off() { PORTD &= ~(1 << GREEN_PIN); } 

void ped_red_on() { PORTD |= (1 << PED_RED_PIN); }      
void ped_red_off() { PORTD &= ~(1 << PED_RED_PIN); }    
void ped_green_on() { PORTD |= (1 << PED_GREEN_PIN); }  
void ped_green_off() { PORTD &= ~(1 << PED_GREEN_PIN); }

void all_lights_off() {
    traffic_red_off();
    traffic_yellow_off();
    traffic_green_off();
    ped_red_off();
    ped_green_off();
} // Turn off all lights

void set_traffic_green() {
    all_lights_off();
    traffic_green_on();
    ped_red_on();
} // Set traffic light to green, pedestrian to red

void set_traffic_yellow() {
    all_lights_off();
    traffic_yellow_on();
    ped_red_on();
} // Set traffic light to yellow, pedestrian to red

void set_traffic_red() {
    all_lights_off();
    traffic_red_on();
    ped_green_on();
} // Set traffic light to red, pedestrian to green

void update_lights() {
    switch(traffic_state) {
        case 0: set_traffic_green(); break;
        case 1: set_traffic_yellow(); break;
        case 2: set_traffic_red(); break;
    }
} // Update lights based on current state

void advance_traffic_state() {
    traffic_state = (traffic_state + 1) % 3;
    update_lights();
} // Advance to the next traffic state and update lights

void handle_pedestrian_button() {
    traffic_state = 2;  
    timer_count = 0;    
    update_lights();
} // Handle button press by setting state to RED and resetting timer

ISR(TIMER1_COMPA_vect) {
    timer_count++;
    if (timer_count >= 400) { 
        timer_count = 0;
        advance_traffic_state();
    }
} // Timer interrupt to advance traffic state

ISR(INT1_vect) {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 200) {  
        handle_pedestrian_button();
    } // This is debounce

    last_interrupt_time = interrupt_time;
} // Interrupt to handle pedestrian button press

void timer1_init() {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); 
    OCR1A = 2499;  
    TIMSK1 = (1 << OCIE1A);  
} // Initialize Timer1 for periodic interrupts

void button_interrupt_init() {
    EICRA |= (1 << ISC10) | (1 << ISC11);
    EIMSK |= (1 << INT1);   
} // Initialize external interrupt for button

void setup_pins() {
    DDRD |= (1 << RED_PIN) | (1 << YELLOW_PIN) | (1 << GREEN_PIN) | (1 << PED_RED_PIN) | (1 << PED_GREEN_PIN);
    DDRD &= ~(1 << BUTTON_PIN);
    PORTD |= (1 << BUTTON_PIN);
} // Set up pins for LEDs and button

void setup() {
    setup_pins();
    timer1_init();
    button_interrupt_init();
    sei();  
    update_lights();
    Serial.begin(9600);
} // Initialize everything and start serial communication

void print_debug_info() {
    Serial.print("State: ");
    Serial.print(traffic_state);
    Serial.print(", Time: ");
    Serial.print(timer_count);
    Serial.print(", Button: ");
    Serial.println(digitalRead(BUTTON_PIN) ? "Released" : "Pressed");
} // Print debug information to serial monitor

void loop() {
    print_debug_info();
    delay(100); 
} // Main loop to print debug info and delay
