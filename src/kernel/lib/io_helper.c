#include "io_helper.h"
#include <stdarg.h>
#include "string.h"

void sprintf(char *str, char *format, ...) {
    va_list args;
    char *out = str;

    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            *out++ = *format++;
            continue;
        }

        format++; // skip '%'

        switch (*format) {
            case '%':
                *out++ = '%';
                break;

            case 'c':
                *out++ = (char) va_arg(args, int);
                break;

            case 's':
                out = append_string(out, va_arg(args, char *));
                break;

            case 'd':
                out = append_signed(out, va_arg(args, int));
                break;

            case 'u':
                out = append_unsigned(out, va_arg(args, unsigned int), 10);
                break;

            case 'x':
                out = append_unsigned(out, va_arg(args, unsigned int), 16);
                break;

            default:
                /*
                 * Unknown conversion.
                 * Could also return an error.
                 */
                *out++ = '%';
                *out++ = *format;
                break;
        }

        format++;
    }

    *out = '\0';

    va_end(args);
}
