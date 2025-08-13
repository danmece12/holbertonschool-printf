#include <stdarg.h>
#include <unistd.h>
#include "main.h"

#define BUF_CAP 1024

/* ================= buffered output helpers ================= */

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

/* \xHH escape (uppercase) used by %S */
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

/* decimal unsigned: prints u in base 10 */
static int buf_putulong10(char *buf, int *len, unsigned long u)
{
	char tmp[21];
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

/* %d / %i with + / space flags, long version for both 32/64-bit */
static int buf_putlong_flags(char *buf, int *len, long v, int plus, int space)
{
	unsigned long u;
	int n = 0;

	if (v < 0)
	{
		if (buf_putc(buf, len, '-') == -1)
			return (-1);
		n++;
		u = (unsigned long)(0 - (unsigned long)v); /* abs without overflow */
	}
	else
	{
		if (plus)
		{ if (buf_putc(buf, len, '+') == -1) return (-1); n++; }
		else if (space)
		{ if (buf_putc(buf, len, ' ') == -1) return (-1); n++; }
		u = (unsigned long)v;
	}
	n += buf_putulong10(buf, len, u);
	return (n);
}

/* generic unsigned in any base (supports up to unsigned long) */
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

/* ================= core dispatcher (with flags + length) ================= */

/*
 * lenmod: 0 = none, 1 = 'h', 2 = 'l'
 */
static int handle_conv(char sp, va_list ap, char *buf, int *len,
	int f_plus, int f_space, int f_hash, int lenmod)
{
	/* simple specs without length */
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
		m = buf_putuint_base(buf, len, (unsigned long)ptr, 16,
			"0123456789abcdef");
		return (m == -1 ? -1 : n + m);
	}

	/* signed decimal (%d, %i) with length */
	if (sp == 'd' || sp == 'i')
	{
		if (lenmod == 2) /* 'l' */
			return (buf_putlong_flags(buf, len, va_arg(ap, long),
				f_plus, f_space));
		else if (lenmod == 1) /* 'h' */
		{
			int x = va_arg(ap, int);
			return (buf_putlong_flags(buf, len, (long)(short)x,
				f_plus, f_space));
		}
		else /* default int */
			return (buf_putlong_flags(buf, len, (long)va_arg(ap, int),
				f_plus, f_space));
	}

	/* unsigned family (%u, %o, %x, %X) with length + # */
	if (sp == 'u' || sp == 'o' || sp == 'x' || sp == 'X')
	{
		unsigned long u;
		int n = 0;

		if (lenmod == 2) /* 'l' */
			u = va_arg(ap, unsigned long);
		else if (lenmod == 1) /* 'h' */
		{
			unsigned int t = va_arg(ap, unsigned int);
			u = (unsigned long)((unsigned short)t);
		}
		else
			u = (unsigned long)va_arg(ap, unsigned int);

		if (f_hash && u != 0)
		{
			if (sp == 'o')
			{ if (buf_putc(buf, len, '0') == -1) return (-1); n++; }
			else if (sp == 'x')
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				if (buf_putc(buf, len, 'x') == -1) return (-1);
				n += 2;
			}
			else if (sp == 'X')
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				if (buf_putc(buf, len, 'X') == -1) return (-1);
				n += 2;
			}
		}

		if (sp == 'u')
			return (n + buf_putuint_base(buf, len, u, 10, "0123456789"));
		if (sp == 'o')
			return (n + buf_putuint_base(buf, len, u, 8,  "01234567"));
		if (sp == 'x')
			return (n + buf_putuint_base(buf, len, u, 16, "0123456789abcdef"));
		/* 'X' */
		return (n + buf_putuint_base(buf, len, u, 16, "0123456789ABCDEF"));
	}

	/* unknown specifier: print literally */
	if (buf_putc(buf, len, '%') == -1) return (-1);
	if (buf_putc(buf, len, sp) == -1) return (-1);
	return (2);
}

/* ================= public API ================= */

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
			int f_plus = 0, f_space = 0, f_hash = 0;
			int lenmod = 0; /* 0 none, 1 'h', 2 'l' */
			int ret;

			format++; /* skip '%' */

			/* flags */
			while (*format == '+' || *format == ' ' || *format == '#')
			{
				if (*format == '+') f_plus = 1;
				else if (*format == ' ') f_space = 1;
				else f_hash = 1;
				format++;
			}
			/* length modifiers (single, no stacking) */
			if (*format == 'h') { lenmod = 1; format++; }
			else if (*format == 'l') { lenmod = 2; format++; }

			if (!*format) { out = -1; break; }

			ret = handle_conv(*format++, ap, buf, &len,
				f_plus, f_space, f_hash, lenmod);
			if (ret == -1) { out = -1; break; }
			out += ret;
		}
	}

	if (out != -1 && buf_flush(buf, &len) == -1)
		out = -1;
	va_end(ap);
	return (out);
}
