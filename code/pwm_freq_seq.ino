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
// https://wokwi.com/projects/407557035517917185



#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define FREQ_PIN 9
#define POT_PIN A0
#define LED_PIN 13
#define SWITCH_PIN1 2
#define SWITCH_PIN2 3

#define DOT_DURATION_MIN 240  // Corresponds to 5 wpm
#define DOT_DURATION_MAX 24   // Corresponds to 50 wpm

const float NOTES[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};
const uint8_t NOTE_COUNT = sizeof(NOTES) / sizeof(NOTES[0]);

volatile uint8_t current_note_index = 0;
volatile uint8_t mode = 0;
volatile uint16_t siren_counter = 0;
volatile uint8_t siren_direction = 0;

void serial_init(void) {
    UBRR0H = 0;
    UBRR0L = 103; // 9600 baud rate at 16 MHz
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
} // Initialize serial communication

void serial_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = data;
} // Transmit a character over serial

void serial_print(const char* str) {
    while (*str) {
        serial_transmit(*str++);
    }
} // Transmit a string over serial

void timer1_init(void) {
    DDRB |= (1 << PB1);
    TCCR1A = (1 << COM1A0);
    TCCR1B = (1 << WGM13) | (1 << CS10);
    TIMSK1 |= (1 << TOIE1);
} // Initialize timer1 for sound generation

void timer2_init(void) {
    TCCR2A = (1 << WGM21);
    TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
    OCR2A = 255;
    TIMSK2 |= (1 << OCIE2A);
} // Initialize timer2 for note duration

void set_frequency(float freq) {
    uint16_t icr = (F_CPU / (2 * freq)) - 1;
    ICR1 = icr;
    OCR1A = icr / 2;
} // Set sound frequency

float get_pot_value(void) {
    return (float)analogRead(POT_PIN);
} // Read potentiometer value

float get_pot_frequency(void) {
    return get_pot_value() * (900.0 / 1023.0) + 100.0;
} // Map potentiometer value to frequency

uint16_t get_pot_dot_duration(void) {
    return (uint16_t)((get_pot_value() / 1023.0) * (DOT_DURATION_MIN - DOT_DURATION_MAX) + DOT_DURATION_MAX);
} // Map potentiometer value to Morse code speed

void delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        _delay_us(1000);
    }
} // Delay for a given number of milliseconds

void play_note(float frequency) {
    set_frequency(frequency);
    PORTB |= (1 << PB5);
} // Play sound note and turn on LED

void stop_note(void) {
    TCCR1A &= ~(1 << COM1A0);
    PORTB &= ~(1 << PB5);
} // Stop sound note and turn off LED

uint8_t update_mode(void) {
    uint8_t switch1 = (PIND & (1 << PD2)) >> 2;
    uint8_t switch2 = (PIND & (1 << PD3)) >> 3;
    return (switch2 << 1) | switch1;
} // Update mode based on switch positions

void melody_mode(void) {
    float base_freq = NOTES[current_note_index];
    float adjusted_freq = base_freq + get_pot_frequency() - 440;
    play_note(adjusted_freq);
    serial_print("Mode: Melody, Frequency: ");
} // Melody mode functionality

void siren_mode(void) {
    TCCR1A |= (1 << COM1A0);
    serial_print("Mode: Siren\n");
} // Siren mode functionality

void theremin_mode(void) {
    float freq = get_pot_frequency();
    play_note(freq);
    serial_print("Mode: Theremin, Frequency: ");
} // Theremin mode functionality

void play_dot(void) {
    play_note(600);
    delay_ms(get_pot_dot_duration());
    stop_note();
    delay_ms(get_pot_dot_duration());
} // Play Morse code dot

void play_dash(void) {
    play_note(600);
    delay_ms(get_pot_dot_duration() * 3);
    stop_note();
    delay_ms(get_pot_dot_duration());
} // Play Morse code dash

void morse_code_mode(void) {
    uint8_t random_char = rand() % 26 + 'A';

    for (uint8_t i = 0; i < 8; i++) {
        if (random_char & (1 << (7 - i))) {
            play_dash();
        } else {
            play_dot();
        }
    }
    
    delay_ms(get_pot_dot_duration() * 3);
    serial_print("Mode: Morse Code\n");
} // Morse code mode functionality

ISR(TIMER1_OVF_vect) {
    if (mode == 1) {
        siren_counter++;
        if (siren_counter >= 1000) {
            siren_counter = 0;
            if (siren_direction == 0) {
                set_frequency(500);
                siren_direction = 1;
            } else {
                set_frequency(1000);
                siren_direction = 0;
            }
        }
    }
} // ISR for Timer1 overflow (siren mode)

ISR(TIMER2_COMPA_vect) {
    static uint8_t note_duration = 0;
    if (++note_duration >= 8) {
        note_duration = 0;
        current_note_index = (current_note_index + 1) % NOTE_COUNT;
    }
} // ISR for Timer2 compare match (melody mode)

int main(void) {
    serial_init();
    timer1_init();
    timer2_init();
    
    DDRB |= (1 << PB5);
    DDRD &= ~((1 << PD2) | (1 << PD3));
    PORTD |= (1 << PD2) | (1 << PD3);
    
    sei();
    
    while (1) {
        mode = update_mode();
        
        switch (mode) {
            case 0:
                melody_mode();
                break;
            case 1:
                siren_mode();
                break;
            case 2:
                theremin_mode();
                break;
            case 3:
                morse_code_mode();
                break;
        }
        
        if (mode != 1) {
            delay_ms(50);
            stop_note();
            delay_ms(10);
        }
    }

    return 0;
}
