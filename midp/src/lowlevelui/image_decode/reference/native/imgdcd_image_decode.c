/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <string.h>

#include <kni.h>
#include <midpMalloc.h>
#include "imgdcd_intern_image_decode.h"

typedef struct _rids {
    imageSrcData super;
    uchar **buf;
    int offset;
    int len, remaining;
} resourceImageSrcData, *resourceImageSrcPtr;

static int
getByte(imageSrcPtr self)
{
    resourceImageSrcPtr r = (resourceImageSrcPtr) self->ptr;

    if (r->remaining < 0) return 0;

    --(r->remaining);
    return (*(r->buf))[r->offset++];
}

static bool
endOfData(imageSrcPtr self)
{
    resourceImageSrcPtr r = (resourceImageSrcPtr) self->ptr;
    if (r->remaining < 0) return TRUE;
    else return FALSE;
}

static int
getBytes(imageSrcPtr self, uchar *buf, int len)
{
    resourceImageSrcPtr r = (resourceImageSrcPtr) self->ptr;

    if (r->remaining < 0) return 0;

    if (len > r->remaining) len = r->remaining;

    if (buf != NULL) {
        memcpy(buf, &((*(r->buf))[r->offset]), len);
    }

    r->remaining -= len;
    r->offset += len;

    return len;
}

/* The done function is called just before the structure is
 * freed.  It may be called before the image data is completely
 * initialized due to a initialization failure, so the structure may
 * not be cleanly initialized.
 */
static void
done(imageSrcPtr self) {
  /* suppress compiler warnings */
  (void)self;
}

static void
skip(imageSrcPtr self, int numBytes)
{
    getBytes(self, NULL, numBytes);
}

static void
reset(imageSrcPtr self)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    p->offset = 0;
    p->remaining = p->len;
}
static long
getPos(imageSrcPtr self)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    return p->offset;
}

static void
seek(imageSrcPtr self, long pos)
{
    resourceImageSrcPtr p = (resourceImageSrcPtr) self->ptr;

    p->offset = pos;
    p->remaining = p->len - pos;
}


/*
 * WARNING: We need to pass in a pointer to a pointer for the source
 * data. The source data is actually allocated on the Java heap and
 * is protected by a TEMPORARY_ROOT prior to this call. Because of
 * this, (*data) will be updated every time the data object moves in
 * the Java heap.
 */
imageSrcPtr
create_imagesrc_from_data(char **data, int len) {
    resourceImageSrcData *p = midpMalloc(sizeof(resourceImageSrcData));

    if (p == NULL) {
	return NULL;
    }

    p->super.ptr       = p;
    p->super.done      = done;
    p->super.reset     = reset;
    p->super.getByte   = getByte;
    p->super.getBytes  = getBytes;
    p->super.skip      = skip;
    p->super.seek      = seek;
    p->super.getpos    = getPos;
    p->super.endOfData = endOfData;

    p->buf = (uchar **)data;
    p->len = len;
    p->super.reset((imageSrcPtr)p);

    return &(p->super);
}
