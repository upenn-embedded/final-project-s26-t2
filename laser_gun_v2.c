/*
Blaster V2
this code integrates the SPI LCD into the code

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
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdint.h>

#include "ST7735.h"
#include "LCD_GFX.h"

// button stuff
#define BTN_NONE    0
#define BTN_TRIGGER 1
#define BTN_RELOAD  2

// move IR LED off PB2 because PB2 is used by LCD chip select
#define IR_LED_DDR   DDRC
#define IR_LED_PORT  PORTC
#define IR_LED_PIN   PORTC0

#define MAX_AMMO 12

volatile uint8_t button_event = BTN_NONE;
volatile uint8_t ammo = MAX_AMMO;
volatile uint8_t reloading = 0;
volatile uint8_t can_fire = 1;

// functions
void gpio_init(void);
void lcd_draw_status(const char *status_text);
void initial_LCD_screen(void);
void trigger_action(void);
void reload_action(void);
void fire_ir_pulse(void);


int main(void)
{
    lcd_init();
    gpio_init();
    sei();

    initial_LCD_screen();

    while (1)
    {
        if (button_event == BTN_TRIGGER)
        {
            button_event = BTN_NONE;

            if (reloading)
            {
                lcd_draw_status("RELOADING");
            }
            else if (!can_fire)
            {
                lcd_draw_status("COOLDOWN");
            }
            else if (ammo > 0)
            {
                trigger_action();
            }
            else
            {
                lcd_draw_status("EMPTY");
            }
        }
        else if (button_event == BTN_RELOAD)
        {
            button_event = BTN_NONE;

            if (!reloading)
            {
                reload_action();
            }
        }
    }
}

void gpio_init(void)
{
    // buttons on PD2 and PD3 as inputs
    DDRD &= ~(1 << DDD2);
    DDRD &= ~(1 << DDD3);

    // internal pullups enabled
    PORTD |= (1 << PORTD2);
    PORTD |= (1 << PORTD3);

    // pin change interrupt for port D
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);

    // IR LED output on PC0
    IR_LED_DDR |= (1 << DDC0);
    IR_LED_PORT &= ~(1 << IR_LED_PIN);
}

void initial_LCD_screen(void)
{
    // displays Ammo count and the status of the blaster, currently in READY state  
    lcd_draw_status("READY");
}

void lcd_draw_status(const char *status_text)
{
    char buf[20];

    LCD_setScreen(BLACK);

    snprintf(buf, sizeof(buf), "Ammo: %u", ammo);
    LCD_drawString(10, 10, buf, WHITE, BLACK);

    LCD_drawString(10, 30, "Status:", CYAN, BLACK);
    LCD_drawString(10, 45, (char *)status_text, YELLOW, BLACK);
}

void fire_ir_pulse(void)
{
    // placeholder for now
    // later this should become the 38kHz modulated burst
    IR_LED_PORT |= (1 << IR_LED_PIN);
    _delay_ms(80);
    IR_LED_PORT &= ~(1 << IR_LED_PIN);
}

void trigger_action(void)
{
    can_fire = 0;

    fire_ir_pulse();

    if (ammo > 0)
    {
        ammo--;
    }

    if (ammo == 0)
    {
        lcd_draw_status("EMPTY");
    }
    else
    {
        lcd_draw_status("FIRED");
    }

    // simple MVP cooldown
    _delay_ms(500);
    can_fire = 1;

    if (ammo > 0)
    {
        lcd_draw_status("READY");
    }
    else
    {
        lcd_draw_status("EMPTY");
    }
}

void reload_action(void)
{
    reloading = 1;
    lcd_draw_status("RELOADING");

    // placeholder reload behavior
    // proposal says hold-to-reload is 10s, but this is fine for current LCD bringup
    _delay_ms(1500);

    ammo = MAX_AMMO;
    reloading = 0;
    can_fire = 1;

    lcd_draw_status("READY");
}

ISR(PCINT2_vect)
{
    if (!(PIND & (1 << PIND2)))
    {
        button_event = BTN_TRIGGER;
    }
    else if (!(PIND & (1 << PIND3)))
    {
        button_event = BTN_RELOAD;
    }
}