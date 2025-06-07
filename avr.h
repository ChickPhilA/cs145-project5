// avr.h
#ifndef AVR_H
#define AVR_H

#include <avr/io.h>
#include <util/delay.h>

#define SET_BIT(sfr, bit)   ((sfr) |=  (1 << (bit)))
#define CLR_BIT(sfr, bit)   ((sfr) &= ~(1 << (bit)))
#define GET_BIT(sfr, bit)   (((sfr) >> (bit)) & 1)
#define NOP()               __asm__ __volatile__("nop")

static inline void avr_wait(uint16_t ms) {
	while (ms--) _delay_ms(1);
}

#endif // AVR_H
