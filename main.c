/*
 *
 *
 * Full Calculator with working PA0 compute button.
 */

#define F_CPU 8000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "avr.h"          // avr_wait()
#include "keypad.h"       // keypad_init(), keypad_getchar()
#include "lcd.h"          // lcd_init(), lcd_clr(), lcd_puts2(), lcd_pos()
#include "calc_minimal.h" // parse_and_eval(), format_to_three_decimals(), button_pressed_PA0()

#define MAX_BUF   32
#define LCD_WIDTH 16

static char    buf[MAX_BUF];
static uint8_t len = 0;
static bool    op_entered = false;
static bool       decimal_entered = false;

//clear both LCD lines and reset buffer + operator flag
static void clear_all(void) {
    lcd_clr();
    len = 0;
    buf[0] = '\0';
    op_entered = false;
}

static void hardware_init(void) {
    lcd_init();
    clear_all();
    keypad_init();
    // configure PA0 as input with pull-up
    CLR_BIT(DDRA, PA0);
    SET_BIT(PORTA, PA0);
	
	// configuration for the green LED
	SET_BIT(DDRB, PB3);
	CLR_BIT(PORTB, PB3);
	
	// configuring for the red LED (error) on PB4
	SET_BIT(DDRB, PB4);
	CLR_BIT(PORTB, PB4);
}

int main(void) {
    hardware_init();

    while (1) {		
        // compute on PA0 (once per debounced press)
		if (button_pressed_PA0() && op_entered) {
			char *op_ptr = strpbrk(buf, "+-*/");
			if (op_ptr) {
				uint8_t idx = (uint8_t)(op_ptr - buf);
				// must have something after the operator
				if (idx > 0 && idx < (len - 1)) {
					// look at the first char of the second operand
					char c1 = buf[idx + 1];
					bool valid = false;
					// case A: it's a digit
					if (c1 >= '0' && c1 <= '9') {
						valid = true;
					}
					// case B: it's a decimal point, and there's a digit after it
					else if (c1 == '.' 
						  && (idx + 2 < len) 
						  && (buf[idx + 2] >= '0' && buf[idx + 2] <= '9'))
					{
						valid = true;
					}

					if (valid) {
						double ans;
						bool ok = parse_and_eval(buf, &ans);
						clear_all();
						lcd_pos(1, 0);
						if (ok) {
							SET_BIT(PORTB, PB3);
							avr_wait(1000);
							CLR_BIT(PORTB, PB3);

							char out[16];
							format_to_three_decimals(ans, out);
							lcd_puts2(out);
						}
						else {
						    SET_BIT(PORTB, PB4);
						    avr_wait(1000);
						    CLR_BIT(PORTB, PB4);		
							lcd_puts2("Err");
						}
					}
				}
			}
		}

        // non-blocking keypad scan
        char raw = keypad_poll();
        char k   = 0;

        // clear on '#'
        if (raw == '#') {
            clear_all();
            continue;
        }
        // digit 0–9
        else if (raw >= '0' && raw <= '9') {
            k = raw;
        }
        // decimal point (*) ? '.', allowed anywhere once
        else if (raw == '*' && !decimal_entered) {
            k = '.';
            decimal_entered = true;
        }
        // operator A–D ? + - * /, only if at least one digit & no op yet
        else if ((raw == 'A' || raw == 'B' || raw == 'C' || raw == 'D')
                 && (len > 0) && (!op_entered))
        {
            switch (raw) {
                case 'A': k = '+'; break;
                case 'B': k = '-'; break;
                case 'C': k = '*'; break;
                case 'D': k = '/'; break;
            }
            op_entered       = true;
            decimal_entered  = false;  // reset for second operand
        }

        // append & redraw row 0 if we got something valid
        if (k) {
            if (len < MAX_BUF - 1) {
                buf[len++] = k;
                buf[len]   = '\0';
            }
            lcd_pos(0, 0);
            lcd_puts2(buf);
        }

        _delay_ms(50);
    }
}

