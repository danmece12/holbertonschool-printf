/* _printf.c */
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
	long v = n;         /* use long to handle INT_MIN */
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

/* handle one specifier */
static int print_conv(char sp, va_list *ap)
{
	if (sp == 'c')
		return (_putc((char)va_arg(*ap, int)));
	if (sp == 's')
		return (_puts(va_arg(*ap, char *)));
	if (sp == '%' )
		return (_putc('%'));
	if (sp == 'd' || sp == 'i')
		return (_putint(va_arg(*ap, int)));
	/* Unknown: print '%' then the char */
	return (_putc('%') + _putc(sp));
}

/**
 * _printf - prints according to a format
 * @format: supports %c, %s, %%, %d, %i
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
