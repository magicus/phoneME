/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _IMGDCD_INTERN_IMAGE_DECODE_H_
#define _IMGDCD_INTERN_IMAGE_DECODE_H_

/**
 * IMPL_NOTE:Document this typedef
 */
typedef unsigned char uchar;

/**
 * Don't depend on VM types, so this code can be stand alone.
 * This should be replace with PCSL types in the future.
 */
#ifndef TRUE
#define TRUE (-1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef bool
typedef int bool;
#endif

typedef struct _ids {
    void   *ptr;                  /* pointer to device specific image struct */
    void   (*reset)(struct _ids *self);
    long   (*getpos)(struct _ids *self);
    void   (*seek)(struct _ids *self, long pos);
    int    (*getByte)(struct _ids *self);
    int    (*getBytes)(struct _ids *self, uchar *buf, int bufsize);
    void   (*skip)(struct _ids *self, int numBytes);
    void   (*done)(struct _ids *self);
    bool   (*endOfData)(struct _ids *self);
} imageSrcData, *imageSrcPtr;

typedef struct _idd {
    int    depth;
    void   *ptr;                  /* pointer to device specific image struct */
    void   (*setColormap)(struct _idd *self, long *map, int length);
    void   (*setSize)(struct _idd *self, int width, int height);
    void   (*sendPixels)(struct _idd *self, int y, uchar *scanline, int rgb);
    void   (*setTransMap)(struct _idd *self, unsigned char *map, int length,
                          int palLength);
} imageDstData, *imageDstPtr;

extern imageSrcPtr
create_imagesrc_from_data(char **data, int len);

extern bool
get_decoded_png_imagesize(imageSrcPtr src, int* width, int *height);

extern bool
decode_png_image(imageSrcPtr src, imageDstPtr dst);

#endif /* _IMGDCD_INTERN_IMAGE_DECODE_H_ */
