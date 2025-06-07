/*
 * keypad.c
 *
 * Fixed scanning logic so that columns are driven low (as outputs)
 * instead of being turned into inputs. This ensures reliable reads.
 */

#include "keypad.h"
#include <util/delay.h>
#include <avr/io.h>

// ----------------------------------------------------------------
// Initialize the keypad pins (PC4–PC7 as outputs driving HIGH,
// PC0–PC3 as inputs with pull-ups).
// ----------------------------------------------------------------
void keypad_init(void) {
	// Disable JTAG so PC2..PC5 become GPIO
	MCUCSR |= (1 << JTD);
	MCUCSR |= (1 << JTD);

	// 1) All column pins (PC4..PC7) = outputs
	// 2) All row pins    (PC0..PC3) = inputs
	DDRC = 0xF0;    // 1111 0000: columns=outputs, rows=inputs

	// 1) Drive all columns HIGH (idle state)
	// 2) Enable pull-ups on all rows
	// Writing ‘1’ to PORTC bit drives HIGH if DDRC=1 (column)
	// or enables pull-up if DDRC=0 (row).
	PORTC = 0xF0 | 0x0F;  // 1111 0000 | 0000 1111 = 1111 1111
}

// ----------------------------------------------------------------
// Helper: drive a single column low, read a single row, then restore.
//   - col: 0..3  corresponds to PC4..PC7
//   - row: 0..3  corresponds to PC0..PC3
// Returns 1 if that (row,col) key is pressed; else 0.
// ----------------------------------------------------------------
static uint8_t is_pressed(uint8_t row, uint8_t col) {
	// Drive only PC(col+4) low; leave other columns HIGH
	PORTC &= ~(1 << (col + 4));  // pull PC(col+4) LOW
	_delay_us(10);               // let lines settle

	// If (PINC & (1<<row)) == 0, that row pin is being pulled low ? pressed
	uint8_t pressed = !(PINC & (1 << row));

	// Restore PC(col+4) HIGH
	PORTC |= (1 << (col + 4));

	return pressed;
}

// ----------------------------------------------------------------
// Blocking routine: returns exactly one character when pressed+released.
// Includes a 50 ms debounce.
// ----------------------------------------------------------------
char keypad_getchar(void) {
	// FINAL MAPPING: replace letters A–P with actual key labels
    const char keys[4][4] = {
	    { '1','2','3','A' },  // row 0
	    { '4','5','6','B' },  // row 1
	    { '7','8','9','C' },  // row 2
	    { '*','0','#','D' }   // row 3
    };

	while (1) {
		// Scan columns 0..3
		for (uint8_t col = 0; col < 4; col++) {
			// Check each row in this column
			for (uint8_t row = 0; row < 4; row++) {
				if (is_pressed(row, col)) {
					_delay_ms(50);                 // 50 ms debounce
					// Wait until the user lifts the key
					while (is_pressed(row, col));
					return keys[row][col];
				}
			}
		}
		// If no key was pressed, loop again
	}
}


/*
 * Non-blocking 4×4 scan: returns a key label or 0 if none pressed.
 * You must call this regularly in your main loop.
 */
char keypad_poll(void) {
    // same mapping as getchar:
    static const char keys[4][4] = {
        { '1','2','3','A' },
        { '4','5','6','B' },
        { '7','8','9','C' },
        { '*','0','#','D' }
    };

    // drive each column low in turn and check rows
    for (uint8_t col = 0; col < 4; col++) {
        // drive only this column low
        PORTC &= ~(1 << (col + 4));
        _delay_us(10);

        // scan rows
        for (uint8_t row = 0; row < 4; row++) {
            if (!(PINC & (1 << row))) {
                // key is down—debounce and wait for release
                _delay_ms(50);
                while (!(PINC & (1 << row)));
                // restore column high before returning
                PORTC |= (1 << (col + 4));
                return keys[row][col];
            }
        }
        // restore this column to high
        PORTC |= (1 << (col + 4));
    }

    // none pressed
    return 0;
}
