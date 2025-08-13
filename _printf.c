#include "main.h"

/* write a C string (prints "(nil)" if s == NULL) */
static int _puts(const char *s)
{
	int n = 0;

	if (!s)
		s = "(nil)";
	while (s[n])
		n++;
	if (n)
		write(1, s, n);
	return (n);
}

/* write one char */
static int _putc(char c)
{
	return (write(1, &c, 1) == 1 ? 1 : 0);
}

/* write a signed int in decimal */
static int _putint(int n)
{
	long v = n;         /* handle INT_MIN safely */
	char buf[20];
	int i = 0, cnt = 0;

	if (v < 0)
	{
		cnt += _putc('-');
		v = -v;
	}
	if (v == 0)
		return (cnt + _putc('0'));
	while (v > 0)
	{
		buf[i++] = (char)('0' + (v % 10));
		v /= 10;
	}
	while (i--)
		cnt += _putc(buf[i]);
	return (cnt);
}

/* write an unsigned int in an arbitrary base (2..16) */
static int _putuint_base(unsigned int v, unsigned int base, const char *digits)
{
	char buf[32];
	int i = 0, cnt = 0;

	if (v == 0)
		return (_putc('0'));
	while (v)
	{
		buf[i++] = digits[v % base];
		v /= base;
	}
	while (i--)
		cnt += _putc(buf[i]);
	return (cnt);
}

/* write an unsigned int in binary */
static int _putbin(unsigned int x)
{
	return (_putuint_base(x, 2, "01"));
}

/* handle one specifier */
static int print_conv(char sp, va_list *ap)
{
	if (sp == 'c')
		return (_putc((char)va_arg(*ap, int)));
	if (sp == 's')
		return (_puts(va_arg(*ap, char *)));
	if (sp == '%')
		return (_putc('%')));
	if (sp == 'd' || sp == 'i')
		return (_putint(va_arg(*ap, int)));
	if (sp == 'b') /* custom: unsigned int to binary */
		return (_putbin(va_arg(*ap, unsigned int)));
	if (sp == 'u')
		return (_putuint_base(va_arg(*ap, unsigned int), 10, "0123456789"));
	if (sp == 'o')
		return (_putuint_base(va_arg(*ap, unsigned int), 8, "01234567"));
	if (sp == 'x')
		return (_putuint_base(va_arg(*ap, unsigned int), 16, "0123456789abcdef"));
	if (sp == 'X')
		return (_putuint_base(va_arg(*ap, unsigned int), 16, "0123456789ABCDEF"));

	/* Unknown: print '%' then the char */
	return (_putc('%') + _putc(sp));
}

/**
 * _printf - prints according to a format
 * @format: supports %c, %s, %%, %d, %i, %b, %u, %o, %x, %X
 * Return: number of chars printed, or -1 on error
 */
int _printf(const char *format, ...)
{
	va_list ap;
	int count = 0, i = 0;

	if (!format)
		return (-1);

	va_start(ap, format);
	while (format[i])
	{
		if (format[i] != '%')
			count += _putc(format[i++]);
		else
		{
			if (!format[++i]) /* stray '%' at end */
				return (va_end(ap), -1);
			count += print_conv(format[i++], &ap);
		}
	}
	va_end(ap);
	return (count);
}
