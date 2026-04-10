#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LED_PIN    PB0
#define LED_DDR    DDRB
#define LED_PORT   PORTB

volatile uint8_t hit_flag = 0;

// INT0 ISR — fires on falling edge (TSOP38238 output goes LOW = IR detected)
ISR(INT0_vect) {
    hit_flag = 1;
}

void init_ir_interrupt(void) {
    // PD2 (INT0) as input, internal pull-up enabled
    DDRD  &= ~(1 << PD2);
    PORTD |=  (1 << PD2);

    // Trigger INT0 on falling edge
    EICRA |= (1 << ISC01);   // ISC01=1, ISC00=0 → falling edge
    EICRA &= ~(1 << ISC00);

    // Enable INT0
    EIMSK |= (1 << INT0);
}

int main(void) {
    // LED output
    LED_DDR  |= (1 << LED_PIN);
    LED_PORT &= ~(1 << LED_PIN);  // start off

    init_ir_interrupt();
    sei();

    while (1) {
        if (hit_flag) {
            LED_PORT |= (1 << LED_PIN);   // LED on
            _delay_ms(200);                // visible flash
            LED_PORT &= ~(1 << LED_PIN);  // LED off
            hit_flag = 0;
        }
    }
}
