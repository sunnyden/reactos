#include "precomp.h"
#include <ctype.h>
#include <direct.h>
#include <stdlib.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented 
 *
 *    _chdrive (MSVCRT.@)
 *
 * Change the current drive.
 *
 * PARAMS
 *  newdrive [I] Drive number to change to (1 = 'A', 2 = 'B', ...)
 *
 * RETURNS
 *  Success: 0. The current drive is set to newdrive.
 *  Failure: -1. errno indicates the error.
 *
 * NOTES
 *  See SetCurrentDirectoryA.
 */
int _chdrive(int newdrive)
{
  WCHAR buffer[] = L"A:";

  buffer[0] += newdrive - 1;
  if (!SetCurrentDirectoryW( buffer ))
  {
    _dosmaperr(GetLastError());
    if (newdrive <= 0)
    {
      __set_errno(EACCES);
    }
    return -1;
  }
  return 0;
}
