/*
 * @(#)QPatchedPixmap.cpp	1.7 06/10/25
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
 */

#include "QPatchedPixmap.h"
#include "qgfx_qws.h"
#include "qbitmap.h"

QImage QPatchedPixmap::convertToImage() const
{
    QImage image;
    if ( isNull() ) {
#if defined(CHECK_NULL)
        qWarning( "QPixmap::convertToImage: Cannot convert a null pixmap" );
#if defined(NASTY)
        abort();
#endif
#endif
        return image;
    }
	
    int w  = qt_screen->mapToDevice( QSize(width(), height()) ).width();
    int h  = qt_screen->mapToDevice( QSize(width(), height()) ).height();
    int d  = depth();
    bool mono = d == 1;
	
	
    if( d == 15 || d == 16 ) {
#ifndef QT_NO_QWS_DEPTH_16
        d = 32;
        // Convert here because we may not have a 32bpp gfx
        image.create( w,h,d,0, QImage::IgnoreEndian );

        for ( int y=h-1; y>=0; y-- ) {     // for each scan line...
            register uint *p = (uint *)image.scanLine(y);
            ushort  *s = (ushort*)scanLine(y);
            for (int i=w;i>0;i--)
                *p++ = qt_conv16ToRgb( *s++ );
        }
		
		const QBitmap *mymask = mask();
		if(mymask!=NULL) {
			QImage maskedimage(width(), height(), 32);
			maskedimage.fill(0);

			QGfx * mygfx=maskedimage.graphicsContext();
			if(mygfx) {
				mygfx->setAlphaSource(mymask->scanLine(0), mymask->bytesPerLine());
				
				mygfx->setSource(&image);				
				mygfx->setAlphaType(QGfx::LittleEndianMask);
				mygfx->setLineStep(maskedimage.bytesPerLine());
				mygfx->blt(0,0,width(),height(),0,0);
			} else {
				qWarning("No image gfx for convertToImage!");
			}
			delete mygfx;
			
			maskedimage.setAlphaBuffer(TRUE);
			image.reset();
			image=maskedimage;
		}
		
#endif
    } else {
        // We can only create little-endian pixmaps
        if ( d == 4 )
            image.create(w,h,8,0, QImage::IgnoreEndian );
        else if ( d == 24 )
            image.create(w,h,32,0, QImage::IgnoreEndian );
        else
            image.create(w,h,d,0, mono ? QImage::LittleEndian : QImage::IgnoreEndian );//####### endianness
			
			QGfx * mygfx=image.graphicsContext();
			const QBitmap *mymask = mask();
			if(mygfx) {
				QGfx::AlphaType at = QGfx::IgnoreAlpha;
				if(mymask!=NULL) {
					at = QGfx::LittleEndianMask;
					mygfx->setAlphaSource(mymask->scanLine(0), mymask->bytesPerLine());
					image.fill(0);
				}
						
				mygfx->setSource(this);				
				mygfx->setAlphaType(at);
				mygfx->setLineStep(image.bytesPerLine());
				mygfx->blt(0,0,width(),height(),0,0);
			} else {
				qWarning("No image gfx for convertToImage!");
			}
			delete mygfx;
			image.setAlphaBuffer(mymask==NULL?data->hasAlpha:TRUE);
    }
	
    if ( mono ) {                               // bitmap
        image.setNumColors( 2 );
        image.setColor( 0, qRgb(255,255,255) );
        image.setColor( 1, qRgb(0,0,0) );
    } else if ( d <= 8 ) {
        image.setNumColors( numCols() );
        for ( int i = 0; i < numCols(); i++ )
            image.setColor( i, clut()[i] );
    }
	
    image = qt_screen->mapFromDevice( image );
	
    return image;
}


