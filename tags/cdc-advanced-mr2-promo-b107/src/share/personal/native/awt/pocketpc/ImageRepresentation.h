/*
 * @(#)ImageRepresentation.h	1.6 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */
#ifndef _IMAGE_REPRESENTATION_H_
#define _IMAGE_REPRESENTATION_H_

#include "java_awt_image_ImageObserver.h"

class ImageRepresentation
{
public:
    ImageRepresentation();
    ~ImageRepresentation(void);
    
private:
    /* The pixmap that represents this image. A pixmap is an offscreen image
       on the server side. */

    /* The mask that determines which pixels are transparent. */
	
    /* The width and height of the pixmap. */
    jint width, height;
	
    /* This is a temporary buffer used to store the RGB values for pixels
       that are set using the setPixels() method. It is used to reduce the 
       number of times memory is allocated and freed as the set pixels method 
       is usually called many times (once for each line of image read in).
       The pixel values are stored in this buffer as a sequence of red, green, 
       blue values as bytes. A pixbuf is then created from this buffer to be 
       drawn into the pixmap. 
    */
    
    /* The width and height of the buffer allocated. */
    jint rgbBufferSize;
	
    /* The number of lines that have been stored in the pixbuf so far. */
    int numLines;

};


/* Defines for ImageRepresentation.availInfo field to describe parts of the 
   image that have been retrieved. */

#define IMAGE_SIZE_INFO (java_awt_image_ImageObserver_WIDTH |	\
					    java_awt_image_ImageObserver_HEIGHT)
					    
#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)


#endif	/* _IMAGE_REPRESENTATION_H_ */
