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

static int buf_puts_raw(char *buf, int *len, const char *s)
{
	int n = 0;

	while (*s)
	{
		if (buf_putc(buf, len, *s++) == -1)
			return (-1);
		n++;
	}
	return (n);
}

static int buf_pad_spaces(char *buf, int *len, int count)
{
	int n = 0;

	while (count-- > 0)
	{
		if (buf_putc(buf, len, ' ') == -1)
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
		return (buf_puts_raw(buf, len, "(null)"));

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

/* ----- helpers that compute lengths (no output) ----- */

static int ulen_base(unsigned long u, unsigned int base)
{
	int d = 1;

	while (u >= base)
	{
		u /= base;
		d++;
	}
	return (d);
}

static int sdec_len(long v, int plus, int space)
{
	unsigned long u;
	int sign = 0;

	if (v < 0)
	{
		sign = 1;
		u = (unsigned long)(0 - (unsigned long)v);
	}
	else
	{
		u = (unsigned long)v;
		if (plus) sign = 1;
		else if (space) sign = 1;
	}
	return (sign + ulen_base(u, 10));
}

/* ----- number emitters ----- */

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

static int buf_putlong_flags(char *buf, int *len, long v, int plus, int space)
{
	unsigned long u;
	int n = 0;

	if (v < 0)
	{
		if (buf_putc(buf, len, '-') == -1)
			return (-1);
		n++;
		u = (unsigned long)(0 - (unsigned long)v);
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

/* ================= core dispatcher (flags + width + length) ================= */
/*
 * lenmod: 0 = none, 1 = 'h', 2 = 'l'
 * width : >=0 (right-justified with spaces if > produced length)
 */
static int handle_conv(char sp, va_list ap, char *buf, int *len,
	int f_plus, int f_space, int f_hash, int width, int lenmod)
{
	int need = 0, pad = 0, out = 0;

	/* %c and %% handled later; %s here with width */
	if (sp == 's')
	{
		const char *s = va_arg(ap, char *);
		int L = 0;

		if (!s) s = "(null)";
		while (s[L]) L++;
		need = L;
		pad = (width > need) ? (width - need) : 0;

		out += buf_pad_spaces(buf, len, pad);
		if (out == -1) return (-1);
		return (out + buf_puts_raw(buf, len, s));
	}
	else if (sp == 'S')
	{
		/* custom: ignore width */
		return (buf_putS(buf, len, va_arg(ap, char *)));
	}
	else if (sp == 'b')
	{
		/* custom: ignore width */
		return (buf_putuint_base(buf, len,
			(unsigned long)va_arg(ap, unsigned int), 2, "01"));
	}
	else if (sp == 'p')
	{
		void *ptr = va_arg(ap, void *);
		int m = 0;

		if (!ptr)
		{
			need = 5; /* "(nil)" */
			pad = (width > need) ? (width - need) : 0;
			out += buf_pad_spaces(buf, len, pad);
			if (out == -1) return (-1);
			return (out + buf_puts_raw(buf, len, "(nil)"));
		}

		need = 2 + ulen_base((unsigned long)ptr, 16);
		pad = (width > need) ? (width - need) : 0;

		out += buf_pad_spaces(buf, len, pad);
		if (out == -1) return (-1);

		if (buf_putc(buf, len, '0') == -1) return (-1);
		if (buf_putc(buf, len, 'x') == -1) return (-1);
		m += 2;
		m += buf_putuint_base(buf, len, (unsigned long)ptr, 16,
			"0123456789abcdef");
		return (out + m);
	}
	else if (sp == 'd' || sp == 'i')
	{
		long v;

		if (lenmod == 2) v = va_arg(ap, long);
		else if (lenmod == 1) v = (long)(short)va_arg(ap, int);
		else v = (long)va_arg(ap, int);

		need = sdec_len(v, f_plus, f_space);
		pad = (width > need) ? (width - need) : 0;

		out += buf_pad_spaces(buf, len, pad);
		if (out == -1) return (-1);
		return (out + buf_putlong_flags(buf, len, v, f_plus, f_space));
	}
	else if (sp == 'u' || sp == 'o' || sp == 'x' || sp == 'X')
	{
		unsigned long u;
		int base;
		const char *digits;
		int prefix = 0;
		int m;

		if (lenmod == 2) u = va_arg(ap, unsigned long);
		else if (lenmod == 1) u = (unsigned long)((unsigned short)va_arg(ap, unsigned int));
		else u = (unsigned long)va_arg(ap, unsigned int);

		base = (sp == 'o') ? 8 : (sp == 'u' ? 10 : 16);
		digits = (sp == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";

		if (f_hash && u != 0)
		{
			if (sp == 'o') prefix = 1;
			else prefix = 2; /* 0x or 0X */
		}

		need = prefix + ulen_base(u, base);
		pad = (width > need) ? (width - need) : 0;

		out += buf_pad_spaces(buf, len, pad);
		if (out == -1) return (-1);

		if (prefix)
		{
			if (sp == 'o')
			{ if (buf_putc(buf, len, '0') == -1) return (-1); out++; }
			else
			{
				if (buf_putc(buf, len, '0') == -1) return (-1);
				if (buf_putc(buf, len, (sp == 'X') ? 'X' : 'x') == -1) return (-1);
				out += 2;
			}
		}
		m = buf_putuint_base(buf, len, u, base, digits);
		return (m == -1 ? -1 : out + m);
	}

	/* %c and %% (with width) or unknown (print literally) */
	if (sp == 'c' || sp == '%')
	{
		need = 1;
		pad  = (width > need) ? (width - need) : 0;
		out += buf_pad_spaces(buf, len, pad);
		if (out == -1) return (-1);
		if (sp == 'c') return (out + buf_putc(buf, len, (char)va_arg(ap, int)));
		return (out + buf_putc(buf, len, '%'));
	}

	/* unknown specifier: print literally "%X" without width semantics */
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
			int width = 0;    /* field width (right-justify) */
			int lenmod = 0;   /* 0 none, 1 'h', 2 'l' */
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

			/* width: '*' or digits */
			if (*format == '*')
			{
				width = va_arg(ap, int);
				if (width < 0) width = 0;
				format++;
			}
			else
			{
				while (*format >= '0' && *format <= '9')
				{
					width = width * 10 + (*format - '0');
					format++;
				}
			}

			/* length modifiers */
			if (*format == 'h') { lenmod = 1; format++; }
			else if (*format == 'l') { lenmod = 2; format++; }

			if (!*format) { out = -1; break; }

			ret = handle_conv(*format++, ap, buf, &len,
				f_plus, f_space, f_hash, width, lenmod);
			if (ret == -1) { out = -1; break; }
			out += ret;
		}
	}
	if (out != -1 && buf_flush(buf, &len) == -1)
		out = (-1);
	va_end(ap);
	return (out);
}