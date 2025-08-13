#include "main.h"

/**
 * main - simple local tests for _printf
 *
 * Return: Always 0
 */
int main(void)
{
	_printf("%S\n", "Best\nSchool");   /* -> Best\x0ASchool */
    _printf("%S\n", "\tHi\x7F!");      /* -> \x09Hi\x7F!  (127 becomes \x7F) */
    return 0;
}
