/* 
 * File:   newmain.c
 * Author: kimhu
 *
 * Created on April 3, 2026, 3:46 PM
 * 
 * this code just demonstrates the interrupts with some basic IRLED capabilities... 
 */
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>


#define BTN_NONE    0
#define BTN_TRIGGER 1   // PD2
#define BTN_RELOAD  2   // PD3
// this variable is our flag for the interrupt 
volatile uint8_t button_event = 0;


int main(void) {
    // SETUP 
    
    // 1. button setup 
    DDRD &= ~(1 << DDD3); 
    DDRD &= ~(1<< DDD2); 
    
    // enable internal pullups, buttons will be on gnd 
    PORTD |= (1 << PORTD2);
    PORTD |= (1 << PORTD3);
    
    // enable pci for port d
    PCICR |= (1 << PCIE2);
    
    // enable intterupts on PD3 and PD2 
    PCMSK2 |= (1<<PCINT18) | (1<<PCINT19);
    
    sei();
    
    // 2. IR led setup 
    // Set PB2 as output
    DDRB |= (1 << DDB2);

    // Start OFF 
    PORTB &= ~(1 << PORTB2);
    
    
    // MAIN LOOP
    while (1) {
        
        // check the button events
        if (button_event == BTN_TRIGGER) {
            // do trigger action
            button_event = BTN_NONE;
            // LED ON
            PORTB |= (1 << PORTB2);

            _delay_ms(1000);

            // LED OFF
            PORTB &= ~(1 << PORTB2);
        }
        else if (button_event == BTN_RELOAD) {
            // do reload action
            button_event = BTN_NONE;
        }
    }
}

// the interrupts sets the flag events 
ISR(PCINT2_vect)
{
    // PD2 (trigger)
    if (!(PIND & (1 << PIND2))) {
        // PD2 went LOW
        button_event = BTN_TRIGGER;
    }
    
    // PD3 (reload)
    if (!(PIND & (1 << PIND3))) {
        // PD3 went LOW
        button_event = BTN_RELOAD;
    }
}
