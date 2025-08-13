#include <stdarg.h>
#include <unistd.h>
#include "main.h"

#define BUF_CAP 1024

/* -------- buffered output helpers -------- */

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

static int buf_putc(char *buf, int *len, char c)
{
	if (*len == BUF_CAP && buf_flush(buf, len) == -1)
		return (-1);
	buf[(*len)++] = c;
	return (1);
}

/* append a C string; if s == NULL, print "(null)" */
static int buf_puts(char *buf, int *len, const char *s)
{
	int n = 0;

	if (!s)
		s = "(null)";
	while (*s)
	{
		if (buf_putc(buf, len, *s++) == -1)
			return (-1);
		n++;
	}
	return (n);
}

/* print "\xHH" for a byte value v (uppercase hex, always 2 digits) */
static int buf_put_hex_escape(char *buf, int *len, unsigned int v)
{
	static const char HEX[] = "0123456789ABCDEF";

	if (buf_putc(buf, len, '\\') == -1) return (-1);
	if (buf_putc(buf, len, 'x')  == -1) return (-1);
	if (buf_putc(buf, len, HEX[(v >> 4) & 0xF]) == -1) return (-1);
	if (buf_putc(buf, len, HEX[v & 0xF])        == -1) return (-1);
	return (4);
}

/* %S : like %s but non-printables become \xHH */
static int buf_putS(char *buf, int *len, const char *s)
{
	int n = 0;

	if (!s)
		return (buf_puts(buf, len, "(null)"));

	for (; *s; s++)
	{
		unsigned char c = (unsigned char)*s;

		if (c < 32 || c >= 127)
		{
			if (buf_put_hex_escape(buf, len, c) == -1)
				return (-1);
			n += 4;
		}
		else
		{
			if (buf_putc(buf, len, c) == -1)
				return (-1);
			n += 1;
		}
	}
	return (n);
}

/* small int printer for %d/%i (no malloc, still buffered) */
static int buf_putint(char *buf, int *len, int v)
{
	char tmp[12];
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

/* unsigned in any base (used for %b) */
static int buf_putuint_base(char *buf, int *len,
	unsigned int u, unsigned int base, const char *digits)
{
	char tmp[33];
	int i = 0, n = 0;

	do {
		tmp[i++] = digits[u % base];
		u /= base;
	} while (u);

	while (i--)
	{
		if (buf_putc(buf, len, tmp[i]) == -1)
			return (-1);
		n++;
	}
	return (n);
}

/* unsigned long in any base (for %p on 64-bit) */
static int buf_putulong_base(char *buf, int *len,
	unsigned long u, unsigned int base, const char *digits)
{
	char tmp[65];
	int i = 0, n = 0;

	do {
		tmp[i++] = digits[u % base];
		u /= base;
	} while (u);

	while (i--)
	{
		if (buf_putc(buf, len, tmp[i]) == -1)
			return (-1);
		n++;
	}
	return (n);
}

/* -------- dispatcher for specifiers -------- */

static int handle_conv(char sp, va_list ap, char *buf, int *len)
{
	if (sp == 'c')
		return (buf_putc(buf, len, (char)va_arg(ap, int)));

	if (sp == 's')
		return (buf_puts(buf, len, va_arg(ap, char *)));

	if (sp == '%')
		return (buf_putc(buf, len, '%'));

	if (sp == 'd' || sp == 'i')
		return (buf_putint(buf, len, va_arg(ap, int)));

	if (sp == 'b')
		return (buf_putuint_base(buf, len,
			va_arg(ap, unsigned int), 2, "01"));

	if (sp == 'S')
		return (buf_putS(buf, len, va_arg(ap, char *)));

	if (sp == 'p')
	{
		void *ptr = va_arg(ap, void *);
		int n = 0, m;

		if (!ptr)
			return (buf_puts(buf, len, "(nil)"));

		if (buf_putc(buf, len, '0') == -1) return (-1);
		if (buf_putc(buf, len, 'x') == -1) return (-1);
		n += 2;

		m = buf_putulong_base(buf, len,
			(unsigned long)ptr, 16, "0123456789abcdef");
		return (m == -1 ? -1 : n + m);
	}

	/* unknown specifier: print it literally */
	if (buf_putc(buf, len, '%') == -1)
		return (-1);
	if (buf_putc(buf, len, sp) == -1)
		return (-1);
	return (2);
}

/* -------- public API -------- */

int _printf(const char *format, ...)
{
	char buf[BUF_CAP];
	int len = 0, out = 0, ret;
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
			continue;
		}

		format++;        /* skip '%' */
		if (!*format)    /* trailing '%' is an error */
		{ out = -1; break; }

		ret = handle_conv(*format++, ap, buf, &len);
		if (ret == -1)
		{ out = -1; break; }
		out += ret;
	}
	if (out != -1 && buf_flush(buf, &len) == -1)
		out = -1;
	va_end(ap);
	return (out);
}
