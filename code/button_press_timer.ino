#include <avr/io.h>
#include <avr/interrupt.h>

#define BUTTON_PIN PORTB0 // pin d8
#define LED_PIN PORTB5 // pin 13
#define TIMER_PIN PORTB1 // Using D9 as our timer pin

#define BAUD 9600
#define MY_UBRR F_CPU/16/BAUD-1

volatile uint32_t timer_count = 0;
volatile bool button_pressed = false;
volatile uint8_t button_debounce = 0;

void uart_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
} // Initializes UART for serial communication

void uart_transmit(unsigned char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
} // Transmits a single character over UART

void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
} // Prints a string over UART

void uart_print_float(float value, int decimal_places) {
    char buffer[20];
    dtostrf(value, 0, decimal_places, buffer);
    uart_print(buffer);
} // Prints a float value over UART with specified decimal places

void timer1_init() {
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10); // CTC mode, prescaler 64
    OCR1A = 249; // 1ms interrupt at 16MHz
    TIMSK1 |= (1 << OCIE1A);
} // Initializes Timer1 for 1ms interrupts

void timer2_init() {
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A = 249;
    TIMSK2 |= (1 << OCIE2A);
} // Initializes Timer2 for button debouncing

void enable_global_interrupts() {
    sei();
} // Enables global interrupts

void button_init() {
    DDRB &= ~(1 << BUTTON_PIN);
    PORTB |= (1 << BUTTON_PIN);
} // Initializes the button pin as input with pull-up

void timer_pin_init() {
    DDRB |= (1 << TIMER_PIN); // Set TIMER_PIN as output
    PORTB &= ~(1 << TIMER_PIN); // Initially set it low
} // Initializes the timer pin as output and sets it low

bool is_button_pressed() {
    return (PINB & (1 << BUTTON_PIN)) == 0;
} // Checks if the button is currently pressed

void led_init() {
    DDRB |= (1 << LED_PIN);
} // Initializes the LED pin as output

void led_on() {
    PORTB |= (1 << LED_PIN);
} // Turns the LED on

void led_off() {
    PORTB &= ~(1 << LED_PIN);
} // Turns the LED off

ISR(TIMER1_COMPA_vect) {
    if (PINB & (1 << TIMER_PIN)) {
        timer_count++;
    }
} // Timer1 ISR: Increments timer_count when TIMER_PIN is high

ISR(TIMER2_COMPA_vect) {
    if (button_debounce > 0) {
        button_debounce--;
    }
} // Timer2 ISR: Handles button debouncing

void setup() {
    uart_init(MY_UBRR);
    timer1_init();
    timer2_init();
    button_init();
    timer_pin_init();
    led_init();
    enable_global_interrupts();
    uart_print("Button Press Duration Tracker\n");
} // Initializes all components and prints a welcome message

void loop() {
    bool current_button_state = is_button_pressed();

    if (current_button_state && !button_pressed && button_debounce == 0) {
        timer_count = 0;
        button_pressed = true;
        PORTB |= (1 << TIMER_PIN); // Set timer pin high
        uart_print("Button pressed!\n");
        button_debounce = 20;
        led_on();
    } else if (!current_button_state && button_pressed) {
        button_pressed = false;
        PORTB &= ~(1 << TIMER_PIN); // Set timer pin low
        button_debounce = 20;
        led_off();
        float elapsed_time = timer_count / 1000.0;
        uart_print("Total press time: ");
        uart_print_float(elapsed_time, 3);
        uart_print(" seconds\n");
    }
} // Main loop: Handles button press/release and calculates press duration