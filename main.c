#include "main.h"
#include <limits.h>

/**
 * main - simple local tests for _printf
 *
 * Return: Always 0
 */
int main(void)
{
	_printf("|%5c|\n", 'A');                 /* |    A| */
	_printf("|%10s|\n", "hi");               /* |        hi| */
	_printf("|%8d|\n", 123);                 /* |     123| */
	_printf("|%8i|\n", -123);                /* |    -123| */
	_printf("|%#8x|\n", 0x2a);               /* |    0x2a| */
	_printf("|%#8o|\n", 0732);               /* |    0732| */
	_printf("|%20p|\n", (void*)0xabc);       /* padded 0xabc */
	_printf("|%5%|\n");                      /* |    %| */
	return 0;
}
