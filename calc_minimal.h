#ifndef _CALC_MINIMAL_H
#define _CALC_MINIMAL_H

#include <stdbool.h>

bool parse_and_eval(const char *expr, double *out_val);
void format_to_three_decimals(double val, char *buf);
bool button_pressed_PA0(void);

#endif