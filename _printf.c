/* _printf.c */
#include "main.h"

/**
 * _puts - write a C string to stdout
 * @s: string (prints "(null)" if s is NULL)
 *
 * Return: number of bytes written
 */
static int _puts(const char *s)
{
	int n = 0;

	if (!s)
		s = "(null)";
	while (s[n])
		n++;
	if (n)
		write(1, s, n);
	return (n);
}

/**
 * _putc - write one character to stdout
 * @c: character
 *
 * Return: 1 on success
 */
static int _putc(char c)
{
	return (write(1, &c, 1) == 1 ? 1 : 0);
}

/**
 * print_conv - handle one conversion specifier
 * @sp: specifier character ('c', 's', or '%')
 * @ap: address of the variadic list
 *
 * Return: number of chars printed for this specifier
 */
static int print_conv(char sp, va_list *ap)
{
	if (sp == 'c')
		return (_putc((char)va_arg(*ap, int)));
	if (sp == 's')
		return (_puts(va_arg(*ap, char *)));
	if (sp == '%')
		return (_putc('%'));

	/* unknown specifier: print '%' then the char */
	return (_putc('%') + _putc(sp));
}

/**
 * _printf - produces output according to a format
 * @format: format string (supports %c, %s, %%)
 *
 * Return: number of characters printed, or -1 on error
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
			i++;
			if (!format[i])
			{
				va_end(ap);
				return (-1);
			}
			count += print_conv(format[i++], &ap);
		}
	}
	va_end(ap);
	return (count);
}
