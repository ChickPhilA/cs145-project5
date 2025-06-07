#ifndef KEYPAD_H
#define KEYPAD_H

#include "avr.h"

#define KEYPAD_PORT PORTC
#define KEYPAD_PIN  PINC
#define KEYPAD_DDR  DDRC


void keypad_init(void);
char keypad_getchar(void);
char keypad_poll(void);


#endif