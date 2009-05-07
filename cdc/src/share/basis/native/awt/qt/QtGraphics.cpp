/*
 * @(#)QtGraphics.cpp	1.9 06/10/25
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

#include "awt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "java_awt_QtGraphics.h"
#include "java_awt_AlphaComposite.h"
#include "java_awt_BasicStroke.h"

#include "QtBackEnd.h"
#include "QtApplication.h"

// Note : should be called with lock held
void setPainterClip(QPainter *painter, int desc) 
{
    if(QtGraphDescPool[desc].clipped){
        int *clip = QtGraphDescPool[desc].clip;
        painter->setClipRect(clip[0], clip[1], clip[2], clip[3]);
    }
    else {
        painter->setClipping(FALSE);
   }
}

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphics_pCloneGraphPSD (JNIEnv *env, jclass cls, jint qtGraphDesc, jboolean fresh)
{
    int i;
 
    AWT_QT_LOCK;
    for(i=NumGraphDescriptors-1; i>0; i--)
        if(QtGraphDescPool[i].used == 0) break;
    
    if(i==0) {
#ifndef QT_AWT_STATICPOOL
        if(fresh == JNI_TRUE) {
            QtGraphDescPool = (QtGraphDesc *)realloc(QtGraphDescPool, 
          (NumGraphDescriptors + NUM_GRAPH_DESCRIPTORS) * sizeof(QtGraphDesc));
			
            if(QtGraphDescPool != NULL) {
                memset(&QtGraphDescPool[NumGraphDescriptors], '\0', 
                       NUM_GRAPH_DESCRIPTORS * sizeof(QtGraphDesc));
                NumGraphDescriptors += NUM_GRAPH_DESCRIPTORS;
                i = NumGraphDescriptors-1;
            } else {
                env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                              "Dynamic Memory exhausted");
                AWT_QT_UNLOCK;
                return -1;
            }
        } else {
            env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                          "Graphic Descriptors Pool exhausted");
            AWT_QT_UNLOCK;
            return -1;
        }
		
#else			
        env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                      "Graphic Descriptors Pool exhausted");
        AWT_QT_UNLOCK;
        return -1;
#endif
    }

    memcpy(&QtGraphDescPool[i], &QtGraphDescPool[qtGraphDesc], 
           sizeof(QtGraphDesc));
				
    QtGraphDescPool[i].used++;
    QtGraphDescPool[i].qp = new QPen();
    QtGraphDescPool[i].qb = new QBrush(Qt::SolidPattern);
    QtGraphDescPool[i].extraalpha = 0xFF;
    QtGraphDescPool[i].rasterOp = Qt::CopyROP;
    QtGraphDescPool[i].blendmode = java_awt_AlphaComposite_SRC_OVER;
			
    QtImageDescPool[QtGraphDescPool[i].qid].count++;
    AWT_QT_UNLOCK;
	
    return (jint)i;
}

JNIEXPORT jint JNICALL
Java_java_awt_QtGraphics_pCloneImagePSD (JNIEnv *env, jclass cls, 
                                         jint qtImageDesc, jboolean fresh)
{
    int i;

    AWT_QT_LOCK;
    for(i=NumGraphDescriptors-1; i>0; i--)
        if(QtGraphDescPool[i].used == 0) break;

    if(i==0) {
#ifndef QT_AWT_STATICPOOL
        if(fresh == JNI_TRUE) {
            QtGraphDescPool = (QtGraphDesc *)realloc(QtGraphDescPool, 
         (NumGraphDescriptors + NUM_GRAPH_DESCRIPTORS) * sizeof(QtGraphDesc));
			
            if(QtGraphDescPool != NULL) {
                memset(&QtGraphDescPool[NumGraphDescriptors], '\0', 
                       NUM_GRAPH_DESCRIPTORS * sizeof(QtGraphDesc));
                NumGraphDescriptors += NUM_GRAPH_DESCRIPTORS;
                i = NumGraphDescriptors-1;
            } else {
                env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                              "Dynamic Memory exhausted");
                AWT_QT_UNLOCK;
                return -1;
            }
        } else {
            env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                          "Graphic Descriptors Pool exhausted");
            AWT_QT_UNLOCK;
            return -1;
        }
		
#else			
        env->ThrowNew(QtCachedIDs.OutOfMemoryError, 
                      "Graphic Descriptors Pool exhausted");
        AWT_QT_UNLOCK;
        return -1;
#endif
    }

    QtGraphDescPool[i].used++;
    QtGraphDescPool[i].qp = new QPen();
    QtGraphDescPool[i].qb = new QBrush(Qt::SolidPattern);
    QtGraphDescPool[i].extraalpha = 0xFF;
    QtGraphDescPool[i].rasterOp = Qt::CopyROP;
    QtGraphDescPool[i].blendmode = java_awt_AlphaComposite_SRC_OVER;
    
    QtGraphDescPool[i].qid = qtImageDesc;
    QtImageDescPool[QtGraphDescPool[i].qid].count++;
	
    AWT_QT_UNLOCK;
    return (jint)i;
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDisposePSD (JNIEnv *env, jclass cls, 
                                      jint qtGraphDesc)
{
    AWT_QT_LOCK;
    if(QtGraphDescPool[qtGraphDesc].qid >= 0)
        QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].count--;
    
    delete QtGraphDescPool[qtGraphDesc].qp;
    delete QtGraphDescPool[qtGraphDesc].qb;
		
    memset(&QtGraphDescPool[qtGraphDesc], '\0', sizeof(QtGraphDesc));
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetFont (JNIEnv * env, jclass cls, 
                                   jint qtGraphDesc, jint font)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].font = (QFont *)font;
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetStroke (JNIEnv * env, jclass cls, 
                                     jint qtGraphDesc, jint width, 
                                     jint cap, jint join, jint dash)
{	
    AWT_QT_LOCK;
	QPen *qp = QtGraphDescPool[qtGraphDesc].qp;
	
	qp->setWidth(width);

	switch(cap) {
    case java_awt_BasicStroke_CAP_BUTT: 
        qp->setCapStyle(QPen::FlatCap); 
        break;
    case java_awt_BasicStroke_CAP_ROUND: 
        qp->setCapStyle(QPen::RoundCap); 
        break;
    case java_awt_BasicStroke_CAP_SQUARE: 
        qp->setCapStyle(QPen::SquareCap); 
        break;
	}

	switch(join) {
    case java_awt_BasicStroke_JOIN_MITER: 
        qp->setJoinStyle(QPen::MiterJoin); 
        break;
    case java_awt_BasicStroke_JOIN_ROUND: 
        qp->setJoinStyle(QPen::RoundJoin); 
        break;
    case java_awt_BasicStroke_JOIN_BEVEL: 
        qp->setJoinStyle(QPen::BevelJoin); 
        break;
	}
		
	/* FIXME: have to add the dash styles */
    AWT_QT_UNLOCK;

}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetXORMode(JNIEnv *env, jclass cls, 
                                     jint qtGraphDesc, jint rgb)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].rasterOp = Qt::XorROP;
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetPaintMode(JNIEnv *env, jclass cls, 
                                       jint qtGraphDesc)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].rasterOp = Qt::CopyROP;
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetComposite(JNIEnv *env, jclass cls, 
                                       jint qtGraphDesc, jint blendMode, 
                                       jint extraAlpha)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].extraalpha = extraAlpha & 0xFF;
	QtGraphDescPool[qtGraphDesc].blendmode = blendMode;

	switch(blendMode){
    case java_awt_AlphaComposite_CLEAR:
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::ClearROP;
        break;
    case java_awt_AlphaComposite_SRC :
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::CopyROP;
        break;
    case java_awt_AlphaComposite_SRC_OVER:
        QtGraphDescPool[qtGraphDesc].rasterOp = 
            (QtGraphDescPool[qtGraphDesc].extraalpha&0x80)? 
            Qt::CopyROP : Qt::NopROP;
        break;
    default:
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::NopROP; 
        QtGraphDescPool[qtGraphDesc].blendmode = -1;
        break;
	}
	
/*	
	QtGraphDescPool[qtGraphDesc].rasterOp = (QtGraphDescPool[qtGraphDesc].extraalpha&0x80)?
		Qt::CopyROP :
		Qt::NopROP;
*/	
	//FIXME: what happens when in XOR mode we will lose that setting...
    AWT_QT_UNLOCK;
}
 

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pChangeClipRect(JNIEnv *env, jclass cls, 
                                         jint qtGraphDesc, jint cx, 
                                         jint cy, jint cw, jint ch)
{
#if 0 // the caller should ensure proper clip before calling this method
	QtImageDesc *qid = &QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid];
	if(cx<0) cx=0;
	if(cw>qid->width) cw = qid->width;
	if(cy<0) cy=0;
	if(ch>qid->height) ch = qid->height;
#endif

    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].clipped = TRUE;
	QtGraphDescPool[qtGraphDesc].clip[0] = cx;
	QtGraphDescPool[qtGraphDesc].clip[1] = cy;
	QtGraphDescPool[qtGraphDesc].clip[2] = cw;
	QtGraphDescPool[qtGraphDesc].clip[3] = ch;		
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pRemoveClip(JNIEnv *env,jclass cls,jint qtGraphDesc)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].clipped = FALSE;
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pSetColor(JNIEnv *env, jclass cls, 
                                   jint qtGraphDesc, jint argb)
{
    AWT_QT_LOCK;
	QtGraphDescPool[qtGraphDesc].currentalpha = (argb & 0xFF000000) >> 24;
	
	/* Have to premultiply alpha in some cases ... what are they? */
	
	QtGraphDescPool[qtGraphDesc].currentalpha = 
        (QtGraphDescPool[qtGraphDesc].currentalpha * 
         QtGraphDescPool[qtGraphDesc].extraalpha) >> 8;

    // 6213575
    QColor c(argb);
    QtGraphDescPool[qtGraphDesc].qp->setColor(c);
    QtGraphDescPool[qtGraphDesc].qb->setColor(c);
    QtGraphDescPool[qtGraphDesc].qp->setStyle(Qt::SolidLine);
    QtGraphDescPool[qtGraphDesc].qb->setStyle(Qt::SolidPattern);			

    switch(QtGraphDescPool[qtGraphDesc].blendmode){
    case java_awt_AlphaComposite_CLEAR: {
        QtGraphDescPool[qtGraphDesc].currentalpha = 0;
        // 6221732
#ifdef QWS
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::CopyROP;
        // Qt::RasterOp specification states that
        // "On Qt/Embedded, only CopyROP, XorROP, and NotROP are supported."
        // 
        // So we force the pen and brush color to black.
        QColor black(0,0,0);
        QtGraphDescPool[qtGraphDesc].qp->setColor(black);
        QtGraphDescPool[qtGraphDesc].qb->setColor(black);
#else
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::ClearROP;
#endif /* QWS */
        // 6221732
        }
        break;
    case java_awt_AlphaComposite_SRC :
        {
            QtGraphDescPool[qtGraphDesc].rasterOp = Qt::CopyROP;
        }
        break;
    case java_awt_AlphaComposite_SRC_OVER:
        if(QtGraphDescPool[qtGraphDesc].currentalpha & 0x80) {
            QtGraphDescPool[qtGraphDesc].rasterOp = Qt::CopyROP;
        }
        else {
            QtGraphDescPool[qtGraphDesc].rasterOp = Qt::NopROP;
        }
        break;
    default:
        QtGraphDescPool[qtGraphDesc].rasterOp = Qt::NopROP; break;
	}
    // 6213575

	/* FIXME: have to add a Bitmask for transparency need alpha test */
    AWT_QT_UNLOCK;
}

#ifdef QT_AWT_1BIT_ALPHA
// Note : should be called with lock held
static inline bool maskPainter(int qtGraphDesc, QPainter &p)
{
	bool r = false;

	QColor c;
	
	switch (QtGraphDescPool[qtGraphDesc].blendmode) {
    case java_awt_AlphaComposite_SRC_OVER:
        if((QtGraphDescPool[qtGraphDesc].currentalpha & 0x80) != 0) {
            c = Qt::color1;
            r = true;
        }
        break;
			
    case java_awt_AlphaComposite_SRC:
        c = (QtGraphDescPool[qtGraphDesc].currentalpha & 0x80) ? 
            Qt::color1 : Qt::color0;
        r = true;
        break;
			
    case java_awt_AlphaComposite_CLEAR:
        c = Qt::color0;
        r = true;
        break;
			
    default:
        r = false;
	}
	
	if(r)
	{
		QPen qp(p.pen());
		QBrush qb(p.brush());
		qp.setColor(c);
		qb.setColor(c);
		
		if(p.isActive()) p.end();
		p.begin((QPaintDevice *)(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].mask));			
		
		p.setPen(qp);
		p.setBrush(qb);
		
        setPainterClip(&p, qtGraphDesc) ;
	}
	
	return r;
}
#else
#define maskPainter(desc, painter) (FALSE)
#endif

// Note : All the methods that clones a QPainter should use the locking
//        as follows 
// AWT_QT_LOCK; 
// {
//   code that clones and uses QPainter
// }
// AWT_QT_UNLOCK;
//
// This will ensure that the deletion of QPainter local variable is
// done with the lock held.
JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pFillRect (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x, jint y, 
                                    jint w, jint h)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setPen(Qt::NoPen);	
	p.setBrush(*((QBrush *)(QtGraphDescPool[qtGraphDesc].qb)));
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
    setPainterClip(&p, qtGraphDesc);
	p.drawRect(x, y, w, h);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawRect(x, y, w, h);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pClearRect (JNIEnv * env, jclass cls, 
                                     jint qtGraphDesc, jint x, jint y, 
                                     jint width, jint height, jint rgb)
{
	QColor qc(rgb);
	
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
    setPainterClip(&p, qtGraphDesc);
	p.fillRect(x, y, width, height, qc);
	if(maskPainter(qtGraphDesc, p))
		p.fillRect(x, y, width, height, qc);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawRect (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x, jint y, 
                                    jint w, jint h)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
    setPainterClip(&p, qtGraphDesc);
	p.drawRect(x, y, w, h);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawRect(x, y, w, h);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pCopyArea (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x, jint y, 
                                    jint w, jint h,
                                    jint dx, jint dy)
{
    AWT_QT_LOCK;
	QPaintDevice *qpd = QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd;
	bitBlt(qpd, x + dx, y + dy, 
		   qpd, x, y, w, h, 
		   QtGraphDescPool[qtGraphDesc].rasterOp, FALSE);
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawLine (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x1, jint y1, 
                                    jint x2, jint y2)
{	
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
    setPainterClip(&p, qtGraphDesc);
	p.drawLine(x1, y1, x2, y2);	
	
	if(maskPainter(qtGraphDesc, p))
		p.drawLine(x1, y1, x2, y2);	
    }
    AWT_QT_UNLOCK;

}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawPolygon (JNIEnv * env, jclass cls, 
                                       jint qtGraphDesc, jint originX, 
                                       jint originY,
                                       jintArray xp, jintArray yp, 
                                       jint nPoints, jboolean close)
{
	jint *xpoints = env->GetIntArrayElements (xp, NULL);
	jint *ypoints = env->GetIntArrayElements (yp, NULL);
	
    AWT_QT_LOCK; {
	QPointArray qpa(nPoints+1);
	
	for(int i=0; i<nPoints; i++)
		qpa.setPoint(i, xpoints[i]+originX, ypoints[i]+originY);
	
	if(close)
		qpa.setPoint(nPoints, xpoints[0]+originX, ypoints[0]+originY);
	
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
    setPainterClip(&p, qtGraphDesc);
	
	if(close) {
		p.drawPolyline(qpa);
		if(maskPainter(qtGraphDesc, p))
			p.drawPolyline(qpa);
	}
	else
	{
		p.drawPolyline(qpa, 0, nPoints);
		if(maskPainter(qtGraphDesc, p))
			p.drawPolyline(qpa, 0, nPoints);
	}
    }
    AWT_QT_UNLOCK;
	
	env->ReleaseIntArrayElements (xp, xpoints, JNI_ABORT);
	env->ReleaseIntArrayElements (yp, ypoints, JNI_ABORT);
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pFillPolygon (JNIEnv * env, jclass cls, 
                                       jint qtGraphDesc, jint originX, 
                                       jint originY,
                                       jintArray xp, jintArray yp, 
                                       jint nPoints)
{
	jint *xpoints = env->GetIntArrayElements (xp, NULL);
	jint *ypoints = env->GetIntArrayElements (yp, NULL);
	

    AWT_QT_LOCK; {
	QPointArray qpa(nPoints);
	
	for(int i=0; i<nPoints; i++)
		qpa.setPoint(i, xpoints[i]+originX, ypoints[i]+originY);
		
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(Qt::NoPen);	
	p.setBrush(*((QBrush *)(QtGraphDescPool[qtGraphDesc].qb)));
    setPainterClip(&p, qtGraphDesc);
	p.drawPolygon(qpa);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawPolygon(qpa);
    }
    AWT_QT_UNLOCK;
	
	env->ReleaseIntArrayElements (xp, xpoints, JNI_ABORT);
	env->ReleaseIntArrayElements (yp, ypoints, JNI_ABORT);
}

JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawArc (JNIEnv * env, jclass cls, 
                                   jint qtGraphDesc, jint x, jint y, 
                                   jint width, jint height, 
                                   jint start, jint sweep)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
    setPainterClip(&p, qtGraphDesc);
	p.drawArc(x, y, width, height, start*16, sweep*16);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawArc(x, y, width, height, start*16, sweep*16);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pFillArc (JNIEnv * env, jclass cls, 
                                   jint qtGraphDesc, jint x, jint y, 
                                   jint width, jint height, jint start, 
                                   jint sweep)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(Qt::NoPen);	
	p.setBrush(*((QBrush *)(QtGraphDescPool[qtGraphDesc].qb)));
    setPainterClip(&p, qtGraphDesc);
	p.drawPie(x, y, width, height, start*16, sweep*16);
	
 	if(maskPainter(qtGraphDesc, p))
 		p.drawPie(x, y, width, height, start*16, sweep*16);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawOval (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x, jint y, 
                                    jint width,
                                    jint height)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
    setPainterClip(&p, qtGraphDesc);
	p.drawEllipse(x, y, width, height);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawEllipse(x, y, width, height);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pFillOval (JNIEnv * env, jclass cls, 
                                    jint qtGraphDesc, jint x, jint y, 
                                    jint width, jint height)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(Qt::NoPen);	
	p.setBrush(*((QBrush *)(QtGraphDescPool[qtGraphDesc].qb)));
    setPainterClip(&p, qtGraphDesc);
	p.drawPie(x, y, width, height, 0, 16*360);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawPie(x, y, width, height, 0, 16*360);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawRoundRect (JNIEnv * env, jclass cls, 
                                         jint qtGraphDesc, jint x, jint y, 
                                         jint w, jint h,
                                         jint arcW, jint arcH)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
    setPainterClip(&p, qtGraphDesc);
	p.drawRoundRect(x, y, w, h, arcW, arcH);
	
	if(maskPainter(qtGraphDesc, p))
		p.drawRoundRect(x, y, w, h, arcW, arcH);
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pFillRoundRect (JNIEnv * env, jclass cls, 
                                         jint qtGraphDesc, jint x, jint y, 
                                         jint w, jint h,
                                         jint arcW, jint arcH)
{
    AWT_QT_LOCK; {
	QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
	p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
	p.setPen(Qt::NoPen);	
	p.setBrush(*((QBrush *)(QtGraphDescPool[qtGraphDesc].qb)));
    setPainterClip(&p, qtGraphDesc);
	p.drawRoundRect(x, y, w, h, arcW, arcH);

	if(maskPainter(qtGraphDesc, p))
		p.drawRoundRect(x, y, w, h, arcW, arcH);	
    }
    AWT_QT_UNLOCK;
}


JNIEXPORT void JNICALL
Java_java_awt_QtGraphics_pDrawString (JNIEnv * env, jclass cls, 
                                      jint qtGraphDesc, jstring string, 
                                      jint x, jint y)
{
	const char *chars;
	jboolean isCopy;
	jsize length;

	chars = env->GetStringUTFChars (string, &isCopy);

	if (chars == NULL) return;

	length = env->GetStringUTFLength (string);

    AWT_QT_LOCK;
	if (length > 0)
	{
		QPainter p(QtImageDescPool[QtGraphDescPool[qtGraphDesc].qid].qpd);
		p.setRasterOp(QtGraphDescPool[qtGraphDesc].rasterOp);
		p.setPen(*((QPen *)(QtGraphDescPool[qtGraphDesc].qp)));
		p.setFont(*((QFont *)(QtGraphDescPool[qtGraphDesc].font)));
        setPainterClip(&p, qtGraphDesc);
		p.drawText(x, y, QString::fromUtf8(chars, length));
		
		if(maskPainter(qtGraphDesc, p)) {
			p.setFont(*((QFont *)(QtGraphDescPool[qtGraphDesc].font)));
			p.drawText(x, y, QString::fromUtf8(chars, length));
		}
	}
    AWT_QT_UNLOCK;

	env->ReleaseStringUTFChars (string, chars);
}


JNIEXPORT jobject JNICALL
Java_java_awt_QtGraphics_getBufferedImagePeer(JNIEnv *env, jclass cls, 
                                              jobject BufferedImage)
{
	return env->GetObjectField(BufferedImage, 
                QtCachedIDs.java_awt_image_BufferedImage_peerFID);
}
