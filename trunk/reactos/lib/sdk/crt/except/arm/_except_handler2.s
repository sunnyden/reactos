/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Implementation of _except_handler2
 * FILE:              lib/sdk/crt/except/arm/_except_handler2.s
 * PROGRAMMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <kxarm.h>

/* CODE **********************************************************************/
    TEXTAREA

    LEAF_ENTRY _except_handler2
    DCD 0xdefc // __assertfail
    bx lr
    LEAF_END _except_handler2

    END
/* EOF */
