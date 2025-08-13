#include <unistd.h>
#include <stdarg.h>
#include "main.h"

#define BUF_CAP 1024

/* ---------- small buffered-output helpers ---------- */

/**
 * buf_flush - write out buffered bytes
 * @buf: buffer
 * @len: pointer to current length
 * Return: 0 on success, -1 on error
 */
static int buf_flush(char *buf, int *len)
{
	if (*len > 0)
	{
		if (write(1, buf, *len) == -1)
			return (-1);
		*len = 0;
	}
	return (0);
}

/**
 * buf_putc - append one char to buffer (flushing if full)
 * Return: 1 on success, -1 on error
 */
static int buf_putc(char *buf, int *len, char c)
{
	if (*len == BUF_CAP && buf_flush(buf, len) == -1)
		return (-1);
	buf[(*len)++] = c;
	return (1);
}

/**
 * buf_puts - append a C string (or "(nil)" if NULL)
 * Return: number of chars appended, or -1 on error
 */
static int buf_puts(char *buf, int *len, const char *s)
{
	int n = 0;

	if (!s)
		s = "(nil)";
	while (*s)
	{
		if (buf_putc(buf, len, *s++) == -1)
			return (-1);
		n++;
	}
	return (n);
}

/* A tiny integer helper (for %d/%i). No malloc, still buffered. */
static int buf_putint(char *buf, int *len, int v)
{
	char tmp[12];  /* enough for -2147483648\0 */
	unsigned int u;
	int i = 0, n = 0;

	if (v < 0)
	{
		if (buf_putc(buf, len, '-') == -1)
			return (-1);
		n++;
		u = (unsigned int)(-(long)v);
	}
	else
		u = (unsigned int)v;

	/* build digits in reverse */
	do {
		tmp[i++] = (char)('0' + (u % 10));
		u /= 10;
	} while (u);

	while (i--)
	{
		if (buf_putc(buf, len, tmp[i]) == -1)
			return (-1);
		n++;
	}
	return (n);
}

/* add near your other helpers */
static int buf_putuint_base(char *buf, int *len,
                            unsigned int u, unsigned int base,
                            const char *digits)
{
    char tmp[33];             /* enough for 32 bits in base 2 */
    int i = 0, n = 0;

    /* build digits in reverse */
    do {
        tmp[i++] = digits[u % base];
        u /= base;
    } while (u);

    while (i--) {
        if (buf_putc(buf, len, tmp[i]) == -1)
            return (-1);
        n++;
    }
    return (n);
}

/* ---------- minimal dispatcher: %c, %s, %%, %d, %i ---------- */

static int handle_conv(char sp, va_list ap, char *buf, int *len)
{
    if (sp == 'c') return buf_putc(buf, len, (char)va_arg(ap, int));
    if (sp == 's') return buf_puts(buf, len, va_arg(ap, char *));
    if (sp == '%') return buf_putc(buf, len, '%');
    if (sp == 'd' || sp == 'i') return buf_putint(buf, len, va_arg(ap, int));
    if (sp == 'b')  /* <<<<<< add this */
        return buf_putuint_base(buf, len,
                                va_arg(ap, unsigned int), 2, "01");

    /* unknown specifier: print literally */
    if (buf_putc(buf, len, '%') == -1) return (-1);
    if (buf_putc(buf, len, sp) == -1) return (-1);
    return (2);
}

/**
 * _printf - minimal printf with 1KB local buffering
 * @format: format string
 * Return: number of characters printed, or -1 on error
 */
int _printf(const char *format, ...)
{
	char buf[BUF_CAP];
	int len = 0, out = 0;
	va_list ap;

	if (!format)
		return (-1);

	va_start(ap, format);
	while (*format)
	{
		if (*format != '%')
		{
			if (buf_putc(buf, &len, *format++) == -1)
			{ out = -1; break; }
			out++;
		}
		else
		{
			int ret;

			format++; /* skip '%' */
			if (!*format) { out = -1; break; } /* trailing '%' */
			ret = handle_conv(*format++, ap, buf, &len);
			if (ret == -1) { out = -1; break; }
			out += ret;
		}
	}
	if (out != -1 && buf_flush(buf, &len) == -1)
		out = -1;
	va_end(ap);
	return (out);
}
