// tiny_printf.h
#ifndef TINY_PRINTF_H
#define TINY_PRINTF_H

#include <stdarg.h>

// Declare the tiny_printf family of functions
int printf(const char *format, ...);
int sprintf(char *out, const char *format, ...);
int snprintf(char *buf, unsigned int count, const char *format, ...);

#endif // TINY_PRINTF_H
