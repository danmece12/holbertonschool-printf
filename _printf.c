#include <stdarg.h>
#include <unistd.h>
#include "main.h"

#define BUF_CAP 1024

/* ---------- buffered output helpers ---------- */
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

/* \xHH escape (uppercase hex) for %S */
static int buf_put_hex_escape(char *buf, int *len, unsigned int v)
{
	static const char HEX[] = "0123456789ABCDEF";

	if (buf_putc(buf, len, '\\') == -1) return (-1);
	if (buf_putc(buf, len, 'x')  == -1) return (-1);
	if (buf_putc(buf, len, HEX[(v >> 4) & 0xF]) == -1) return (-1);
	if (buf_putc(buf, len, HEX[v & 0xF])        == -1) return (-1);
	return (4);
}

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

/* decimal signed */
static int buf_putint_core(char *buf, int *len, unsigned int u)
{
	char tmp[12];
	int i = 0, n = 0;

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

/* %d / %i with + / space flags */
static int buf_putint_flags(char *buf, int *len, int v, int plus, int space)
{
	int n = 0;

	if (v < 0)
	{
		if (buf_putc(buf, len, '-') == -1) return (-1);
		n++;
		return (n + buf_putint_core(buf, len, (unsigned int)(-(long)v)));
	}

	/* positive: optional sign or leading space */
	if (plus)
	{ if (buf_putc(buf, len, '+') == -1) return (-1); n++; }
	else if (space)
	{ if (buf_putc(buf, len, ' ') == -1) return (-1); n++; }

	return (n + buf_putint_core(buf, len, (unsigned int)v));
}

/* generic unsigned (for %u, %o, %x, %X, %b) */
static int buf_putuint_base(char *buf, int *len,
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

/* ---------- dispatcher with flags ---------- */
static int handle_conv(char sp, va_list ap, char *buf, int *len,
	int f_plus, int f_space, int f_hash)
{
	/* simple ones first */
	if (sp == 'c')
		return (buf_putc(buf, len, (char)va_arg(ap, int)));
	if (sp == 's')
		return (buf_puts(buf, len, va_arg(ap, char *)));
	if (sp == '%')
		return (buf_putc(buf, len, '%'));
	if (sp == 'S')
		return (buf_putS(buf, len, va_arg(ap, char *)));
	if (sp == 'b')
		return (buf_putuint_base(buf, len,
			(unsigned int)va_arg(ap, unsigned int), 2, "01"));
	if (sp == 'p')
	{
		void *ptr = va_arg(ap, void *);
		int n = 0, m;

		if (!ptr)
			return (buf_puts(buf, len, "(nil)"));
		if (buf_putc(buf, len, '0') == -1) return (-1);
		if (buf_putc(buf, len, 'x') == -1) return (-1);
		n += 2;
		m = buf_putuint_base(buf, len,
			(unsigned long)ptr, 16, "0123456789abcdef");
		return (m == -1 ? -1 : n + m);
	}

	/* signed decimal with + / space */
	if (sp == 'd' || sp == 'i')
		return (buf_putint_flags(buf, len, va_arg(ap, int), f_plus, f_space));

	/* unsigned family (flags: only # for o/x/X) */
	if (sp == 'u')
		return (buf_putuint_base(buf, len,
			(unsigned int)va_arg(ap, unsigned int), 10, "0123456789"));

	if (sp == 'o' || sp == 'x' || sp == 'X')
	{
		unsigned int u = va_arg(ap, unsigned int);
		int n = 0;

		if (f_hash && u != 0)
		{
			if (sp == 'o')
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				n++;
			}
			else if (sp == 'x')
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				if (buf_putc(buf, len, 'x') == -1) return (-1);
				n += 2;
			}
			else /* 'X' */
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				if (buf_putc(buf, len, 'X') == -1) return (-1);
				n += 2;
			}
		}

		if (sp == 'o')
			return (n + buf_putuint_base(buf, len, u, 8, "01234567"));
		if (sp == 'x')
			return (n + buf_putuint_base(buf, len, u, 16, "0123456789abcdef"));
		/* 'X' */
		return (n + buf_putuint_base(buf, len, u, 16, "0123456789ABCDEF"));
	}

	/* unknown specifier: print it literally (%<char>) */
	if (buf_putc(buf, len, '%') == -1) return (-1);
	if (buf_putc(buf, len, sp) == -1) return (-1);
	return (2);
}

/* ---------- public API ---------- */
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
			continue;
		}

		/* skip '%' and parse flags (+, space, #) */
		{
			int f_plus = 0, f_space = 0, f_hash = 0, ret;

			format++;
			while (*format == '+' || *format == ' ' || *format == '#')
			{
				if (*format == '+') f_plus = 1;
				else if (*format == ' ') f_space = 1;
				else f_hash = 1;
				format++;
			}
			if (!*format) { out = -1; break; } /* lone '%' or '%' + flags at end */

			ret = handle_conv(*format++, ap, buf, &len, f_plus, f_space, f_hash);
			if (ret == -1) { out = -1; break; }
			out += ret;
		}
	}

	if (out != -1 && buf_flush(buf, &len) == -1)
		out = -1;
	va_end(ap);
	return (out);
}
