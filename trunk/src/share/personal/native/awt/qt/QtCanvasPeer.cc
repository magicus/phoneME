/*
 * @(#)QtCanvasPeer.cc	1.8 06/10/25
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
#include <qt.h>

#include "QtCanvasPeer.h"
#include "sun_awt_qt_QtCanvasPeer.h"
#include "QpWidget.h"

/**
 * Construtor for QtCanvasPeer
 */
QtCanvasPeer :: QtCanvasPeer(JNIEnv *env,
                             jobject thisObj,
                             QpWidget* canvasWidget)
    :QtComponentPeer(env, thisObj, canvasWidget, canvasWidget)
{
    if (canvasWidget) {
	canvasWidget->show();
    }
  
}

/**
 * Destructor for QtCanvasPeer
 */
QtCanvasPeer :: ~QtCanvasPeer(void)
{
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtCanvasPeer_create (JNIEnv *env, jobject thisObj, 
				     jobject parent)
{
  
    QtCanvasPeer *canvasPeer;

    // Create the label widget & then the peer object
    QpWidget *parentWidget = 0;
  
    if (parent) {
	QtComponentPeer *parentPeer = (QtComponentPeer *)
	    env->GetIntField (parent,
			      QtCachedIDs.QtComponentPeer_dataFID);
	parentWidget = parentPeer->getWidget();
    }
  
    QpWidget *mycanvas = new QpWidget(parentWidget, "QtCanvas");
  
    if (mycanvas == NULL) {    
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }

    canvasPeer = new QtCanvasPeer(env, thisObj, mycanvas);
  
    if (canvasPeer == NULL) {    
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
	return;
    }
}
