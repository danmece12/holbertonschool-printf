#include "main.h"

/**
 * main - simple local tests for _printf
 *
 * Return: Always 0
 */
int main(void)
{
	_printf("d: %d, i: %i\n", -2147483648, 2147483647);
	_printf("mix: %s %c %d %% %i\n", "hello", 'X', -42, 42);
	return (0);
}
