#include "main.h"

/**
 * main - simple local tests for _printf
 *
 * Return: Always 0
 */
int main(void)
{
	_printf("u=%u\n", 4294967295U);
	_printf("o=%o\n", 0755);
	_printf("x=%x\n", 0xBEEF);
	_printf("X=%X\n", 0xBEEF);
	return (0);
}
