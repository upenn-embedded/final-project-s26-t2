#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define MAX_LIVES 5
volatile uint8_t hit_flag = 0;
uint8_t lives = MAX_LIVES;

ISR(INT0_vect) {
    hit_flag = 1;
}

void init_ir_interrupt(void) {
    DDRD  &= ~(1 << PD2);
    PORTD |=  (1 << PD2);
    EICRA |=  (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |=  (1 << INT0);
}

// color: 0=off, 1=green, 2=red
void set_led(uint8_t idx, uint8_t color) {
    uint8_t r = (color == 2) ? 1 : 0;
    uint8_t g = (color == 1) ? 1 : 0;

    switch (idx) {
        case 0: // PB1=R, PB2=G
            if (r) PORTB |=  (1<<PB1); else PORTB &= ~(1<<PB1);
            if (g) PORTB |=  (1<<PB2); else PORTB &= ~(1<<PB2);
            break;
        case 1: // PB3=R, PB4=G
            if (r) PORTB |=  (1<<PB3); else PORTB &= ~(1<<PB3);
            if (g) PORTB |=  (1<<PB4); else PORTB &= ~(1<<PB4);
            break;
        case 2: // PB5=R, PC0=G
            if (r) PORTB |=  (1<<PB5); else PORTB &= ~(1<<PB5);
            if (g) PORTC |=  (1<<PC0); else PORTC &= ~(1<<PC0);
            break;
        case 3: // PC1=R, PC2=G
            if (r) PORTC |=  (1<<PC1); else PORTC &= ~(1<<PC1);
            if (g) PORTC |=  (1<<PC2); else PORTC &= ~(1<<PC2);
            break;
        case 4: // PC3=R, PC4=G
            if (r) PORTC |=  (1<<PC3); else PORTC &= ~(1<<PC3);
            if (g) PORTC |=  (1<<PC4); else PORTC &= ~(1<<PC4);
            break;
    }
}

void init_leds(void) {
    DDRB |= (1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)|(1<<PB5);
    DDRC |= (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4);
}

void refresh_leds(void) {
    for (uint8_t i = 0; i < MAX_LIVES; i++)
        set_led(i, i < lives ? 1 : 2);
}

void flash_hit(void) {
    // All red+green on = yellow flash
    PORTB |= (1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)|(1<<PB5);
    PORTC |= (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4);
    _delay_ms(120);
    PORTB &= ~((1<<PB1)|(1<<PB2)|(1<<PB3)|(1<<PB4)|(1<<PB5));
    PORTC &= ~((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3)|(1<<PC4));
    _delay_ms(60);
}

void dead_sequence(void) {
    for (uint8_t f = 0; f < 6; f++) {
        for (uint8_t i = 0; i < MAX_LIVES; i++) set_led(i, 2);
        _delay_ms(100);
        for (uint8_t i = 0; i < MAX_LIVES; i++) set_led(i, 0);
        _delay_ms(100);
    }
    for (uint8_t i = 0; i < MAX_LIVES; i++) set_led(i, 2);
}

int main(void) {
    init_leds();
    init_ir_interrupt();
    sei();
    refresh_leds();

    while (1) {
        if (hit_flag) {
            if (lives > 0) {
                flash_hit();
                lives--;
                if (lives == 0)
                    dead_sequence();
                else
                    refresh_leds();
            }
            hit_flag = 0;
        }
    }
}
