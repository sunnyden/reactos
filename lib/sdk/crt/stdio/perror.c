/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>


/*
 * @implemented
 */
void _wperror(const wchar_t *s)
{
  fwprintf(stderr, L"%s: %S\n", s, _strerror(NULL));
}
