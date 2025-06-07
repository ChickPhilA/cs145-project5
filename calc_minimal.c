/*
 * calc_minimal.c
 *
 * Created: 6/2/2025 11:02:45 PM
 *  Author: Philmig
 */ 

// calc_minimal.c

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>   // for atoi (or you can parse manually)
#include <stdio.h>    // for sprintf
#include <ctype.h>
#include "calc_minimal.h";
#include <avr/io.h>
#include "avr.h"

// maximum length of the input expression (including digits/operators)
#define MAX_EXPR_LEN  32

// maximum depth of operator or value stacks
#define MAX_STACK     16

// returns precedence: '*' and '/' = 2; '+' and '-' = 1; otherwise 0
static int8_t precedence(char op) {
	switch (op) {
		case '*': case '/': return 2;
		case '+': case '-': return 1;
		default:            return 0;
	}
}

// applies a single operator to two integer operands a and b.
// if operator is '/', does integer division (b must not be zero).
// returns result. If b==0 during '/', we set *error=true.
static double apply_op(double a, double b, char op, bool *err) {
	double res = 0.0;
	switch (op) {
		case '+': res = a + b; break;
		case '-': res = a - b; break;
		case '*': res = a * b; break;
		case '/':
		if (b == 0.0) {
			*err = true;
			return 0.0;
		}
		res = a / b;
		break;
		default:
		*err = true;
		return 0.0;
	}
	return res;
}

// Parses and evaluates an integer?only infix expression in "expr".
// - expr: a null terminated C string with digits and + - * / only
// - out_val: on success, *out_val = result as a double (no fractional part)
// returns true if parsing + evaluation succeed; false on any error (overflow,
// divide by zero, malformed, too many tokens, etc.).

bool parse_and_eval(const char *expr, double *out_val) {
 #define MAX_STACK 32
    double val_stack[MAX_STACK];
    char   op_stack [MAX_STACK];
    int    vt = -1, ot = -1;
    bool   error = false;
    const char *p = expr;

    while (*p) {
        // skip whitespace
        if (*p == ' ' || *p == '\t') { p++; continue; }

        // number: integer part (if any), then optional fraction
        if (isdigit((unsigned char)*p) ||
           (*p == '.' && isdigit((unsigned char)*(p+1)))) 
        {
            double num = 0.0;
            // integer portion
            while (isdigit((unsigned char)*p)) {
                num = num*10.0 + (*p - '0');
                p++;
            }
            // fractional portion
            if (*p == '.') {
                p++;
                double place = 0.1;
                while (isdigit((unsigned char)*p)) {
                    num += (*p - '0') * place;
                    place *= 0.1;
                    p++;
                }
            }
            if (vt >= MAX_STACK - 1) { error = true; break; }
            val_stack[++vt] = num;
        }
        // operator
        else if (*p=='+'||*p=='-'||*p=='*'||*p=='/') {
            char op = *p++;
            // pop higher?or?equal precedence
            while (ot >= 0 && precedence(op_stack[ot]) >= precedence(op)) {
                if (vt < 1) { error = true; break; }
                double b = val_stack[vt--], a = val_stack[vt--];
                double r = apply_op(a, b, op_stack[ot--], &error);
                if (error) break;
                val_stack[++vt] = r;
            }
            if (error) break;
            if (ot >= MAX_STACK - 1) { error = true; break; }
            op_stack[++ot] = op;
        }
        else {
            // invalid character
            error = true;
            break;
        }
    }

    // apply any remaining operators
    while (!error && ot >= 0) {
        if (vt < 1) { error = true; break; }
        double b = val_stack[vt--], a = val_stack[vt--];
        double r = apply_op(a, b, op_stack[ot--], &error);
        if (error) break;
        val_stack[++vt] = r;
    }

    if (error || vt != 0) {
        #undef MAX_STACK
        return false;
    }
    *out_val = val_stack[0];
    #undef MAX_STACK
    return true;
}



// formats a “double” that actually holds an integer value into a C-string
// with exactly three decimal places. for example, 123.00 will be "123.000".
// uf buf is too small, this will overflow; make sure buf is 16 bytes.
void format_to_three_decimals(double val, char *buf) {
    bool neg = false;
    if (val < 0.0) {
        neg = true;
        val = -val;
    }
    // round to nearest thousandth:
    int32_t scaled = (int32_t)floor(val * 1000.0 + 0.5);
    int32_t integer = scaled / 1000;
    int32_t frac    = scaled % 1000;   // 0..999

    if (neg) {
        sprintf(buf, "-%ld.%03ld", (long)integer, (long)frac);
    } else {
        sprintf(buf,  "%ld.%03ld", (long)integer, (long)frac);
    }
}

bool button_pressed_PA0(void) {
	static bool last_pressed = false;
	bool curr = !(PINA & (1<<PA0));  // true if button is down

	if (curr && !last_pressed) {
		// potential new press ? debounce
		avr_wait(20);
		curr = !(PINA & (1<<PA0));
		if (curr) {
			last_pressed = true;
			return true;
		}
	}
	else if (!curr) {
		// released ? reset
		last_pressed = false;
	}
	return false;
}