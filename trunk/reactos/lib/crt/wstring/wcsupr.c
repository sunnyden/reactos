#include <ctype.h>
#include <string.h>

/*
 * @implemented
 */
wchar_t *_wcsupr(wchar_t *x)
{
	wchar_t  *y = x;

	while (*y) {
		*y = towupper(*y);
		y++;
	}
	return x;
}
