/*
Blaster V2 
This code integrates the SPI LCD into the code. 

lcd requirements for current MVP:
- display ammo count on startup
- initialize ammo to 12
- decrement ammo after each valid trigger event
- block firing and display EMPTY when ammo reaches 0
- display RELOADING during reload sequence
- reset ammo to 12 after successful reload
- optionally show cooldown / fire status for user feedback

possible future lcd additions:
- reload progress indicator

*/



#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

// button stuff 
#define BTN_NONE    0
#define BTN_TRIGGER 1   // PD2
#define BTN_RELOAD  2   // PD3
// this variable is our flag for the interrupt 
volatile uint8_t button_event = 0;


// graphics
#include "ST7735.h"
#include "LCD_GFX.h"

#define SCREEN_W        160
#define SCREEN_H        128

// ammo count 
uint8_t ammo = 12; 


void initial_LCD_screen(void){
    // need an additional function to refresh based on changes to the states? 
    char buf[20];
    snprintf(buf, sizeof(buf), "Ammo: %d", ammo);
    LCD_drawString(10, 4, buf, WHITE, RED);
}

void trigger_action(void) {
    // IR LED ON
    PORTB |= (1 << PORTB2);
    // some sort of delay? we dont want people to spam shooting, but delay_ms stops the entire microcontroller? 
    _delay_ms(1000);
    


    // decrement ammo 
    ammo--;
}

void reload_action(void) {
    // clears the screen

    // shows big text saying "reloading"


}

int main(void) {
    // SETUP 
    
    // LCD setup
    lcd_init();


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
            button_event = NONE;
            // first make a check if there's enough ammo 
            if (ammo > 0) {
                // do trigger action 
                trigger_action();
            }
            else {
                // NO AMMO! update on the LCD indicating no ammo / EMPTY 

            }
        }
        else if (button_event == BTN_RELOAD) {
            button_event = BTN_NONE;
            // do reload action
            reload_action();
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
