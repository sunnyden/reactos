#include <msvcrti.h>


unsigned char * _mbsinc(const unsigned char *s)
{
	unsigned char *c = (unsigned char *)s;
	if (_ismbblead(*s) ) 
		c++;
	c++;
	return c;
}
