/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Keith Whitwell <keith@tungstengraphics.com>
 *   Kevin E. Martin <martin@valinux.com>
 */

#include "radeon_lock.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "r300_context.h"
#include "r300_state.h"

#include "framebuffer.h"

#include "drirenderbuffer.h"

#if DEBUG_LOCKING
char *prevLockFile = NULL;
int prevLockLine = 0;
#endif

/* Turn on/off page flipping according to the flags in the sarea:
 */
void radeonUpdatePageFlipping(radeonContextPtr rmesa)
{
	int use_back;

	rmesa->doPageFlip = rmesa->sarea->pfState;
	if (rmesa->glCtx->WinSysDrawBuffer) {
		driFlipRenderbuffers(rmesa->glCtx->WinSysDrawBuffer,
				     rmesa->sarea->pfCurrentPage);
		r300UpdateDrawBuffer(rmesa->glCtx);
	}

	use_back = rmesa->glCtx->DrawBuffer ?
	    (rmesa->glCtx->DrawBuffer->_ColorDrawBufferMask[0] ==
	     BUFFER_BIT_BACK_LEFT) : 1;
	use_back ^= (rmesa->sarea->pfCurrentPage == 1);

	if (use_back) {
		rmesa->state.color.drawOffset =
		    rmesa->radeonScreen->backOffset;
		rmesa->state.color.drawPitch = rmesa->radeonScreen->backPitch;
	} else {
		rmesa->state.color.drawOffset =
		    rmesa->radeonScreen->frontOffset;
		rmesa->state.color.drawPitch =
		    rmesa->radeonScreen->frontPitch;
	}
}

/* Update the hardware state.  This is called if another context has
 * grabbed the hardware lock, which includes the X server.  This
 * function also updates the driver's window state after the X server
 * moves, resizes or restacks a window -- the change will be reflected
 * in the drawable position and clip rects.  Since the X server grabs
 * the hardware lock when it changes the window state, this routine will
 * automatically be called after such a change.
 */
void radeonGetLock(radeonContextPtr rmesa, GLuint flags)
{
	__DRIdrawablePrivate *const drawable = rmesa->dri.drawable;
	__DRIdrawablePrivate *const readable = rmesa->dri.readable;
	__DRIscreenPrivate *sPriv = rmesa->dri.screen;
	drm_radeon_sarea_t *sarea = rmesa->sarea;
	r300ContextPtr r300 = (r300ContextPtr) rmesa;

	assert(drawable != NULL);

	drmGetLock(rmesa->dri.fd, rmesa->dri.hwContext, flags);

	/* The window might have moved, so we might need to get new clip
	 * rects.
	 *
	 * NOTE: This releases and regrabs the hw lock to allow the X server
	 * to respond to the DRI protocol request for new drawable info.
	 * Since the hardware state depends on having the latest drawable
	 * clip rects, all state checking must be done _after_ this call.
	 */
	DRI_VALIDATE_DRAWABLE_INFO(sPriv, drawable);
	if (drawable != readable) {
		DRI_VALIDATE_DRAWABLE_INFO(sPriv, readable);
	}

	if (rmesa->lastStamp != drawable->lastStamp) {
		radeonUpdatePageFlipping(rmesa);
		radeonSetCliprects(rmesa);
		r300UpdateViewportOffset(rmesa->glCtx);
		driUpdateFramebufferSize(rmesa->glCtx, drawable);
	}

	if (sarea->ctx_owner != rmesa->dri.hwContext) {
		int i;

		sarea->ctx_owner = rmesa->dri.hwContext;
		for (i = 0; i < r300->nr_heaps; i++) {
			DRI_AGE_TEXTURES(r300->texture_heaps[i]);
		}
	}

	rmesa->lost_context = GL_TRUE;
}
