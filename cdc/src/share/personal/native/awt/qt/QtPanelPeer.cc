/*
 * @(#)QtPanelPeer.cc	1.14 06/10/25
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

#include "jni.h"
#include "sun_awt_qt_QtPanelPeer.h"
#include "QtPanelPeer.h"
#include <qobjectlist.h>
#ifdef QWS
#include <qwsdisplay_qws.h>
#include "QtApplication.h"
#endif /* QWS */
#include <qframe.h>
#include "QpFrame.h"
#include "QtDisposer.h"

/* Initializes the PanelPeerData. This creates the fixed widget and
 * initializes the ComponentPeerData. If widget, drawWidget or
 * styleWidget are NULL then then fixed widget will be used for them.
 * If canAddToPanel is false then a fixed widget will not be created
 * and components cannot be added to the panel. This is useful for
 * subclasses of panel for which it is not possible to add components
 * to (such as FileDialog).
 */

QtPanelPeer::QtPanelPeer (JNIEnv *env,
			              jobject thisObj,
			              QpWidget *panelFrame)
    : QtComponentPeer(env, thisObj, panelFrame, panelFrame)
{
    /*
    if (panelFrame) {
	panelFrame->show();
    }
    */
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtPanelPeer_create (JNIEnv *env, jobject thisObj, 
				    jobject parent)
{
    QtPanelPeer *panel;
    QpWidget *parentWidget = 0;
    
    if (parent) {
        QtComponentPeer *parentPeer = (QtComponentPeer *)
            env->GetIntField (parent,
                              QtCachedIDs.QtComponentPeer_dataFID);
        parentWidget = parentPeer->getWidget();
    }
    QpFrame *panelFrame = new QpFrame(parentWidget, "QtPanel") ;
    panelFrame->setFrameStyle(QFrame::Panel | QFrame::Plain);
    panelFrame->setLineWidth(0);
    //QWidget *panelFrame = new QWidget( parentWidget );
    
    panel = new QtPanelPeer(env, thisObj, panelFrame);
    
    if (!panel) {
	env->ThrowNew (QtCachedIDs.OutOfMemoryErrorClass, NULL);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtPanelPeer_insert (JNIEnv *env, 
				    jobject thisObj, 
				    jobject cmpPeer,
				    jint index)
{
    QtPanelPeer *panel;
    QtComponentPeer *component;
    
    QtDisposer::Trigger guard(env);
    
    panel = (QtPanelPeer *)QtComponentPeer::getPeerFromJNI(env,
							   thisObj);
    
    if (panel == NULL)
	return;
    
    component = QtComponentPeer::getPeerFromJNI (env, cmpPeer);
    
    if (component == NULL)
	return;
    
	QpWidget *panelWidget = panel->getWidget();
    if (panelWidget != NULL) {
	    QpWidget *compWidget = component->getWidget();
        panelWidget->insertWidgetAt(compWidget, index);
	}
}

/*
 * Fix for Bug ID 4778521, frame borders not repainting. In
 * an applet, the interior Panel which actually contains the
 * applet itself is validated last, after the enclosing Frame,
 * which causes a border repaint problem. All that is needed
 * is to trigger a repaint on the enclosing Frame, but this is
 * difficult. It seems that the  most effective method would be
 * to set the Qt widget's state to hidden, via a
 * widget->setWState( ~WState_Visible ) call, and then invoke
 * the widget's show() method. However, setWState is a protected
 * method and requires subclassing off of QWidget, which would
 * change our class hierarchy and is too heavyweight a change
 * for this late in the release cycle. This change should be
 * considered going forward. In the meantime, we implement
 * the endValidate() method of the PanelPeer instead. Here,
 * if the panel's toplevel widget is not hidden, we refresh
 * the frame by resetting its label to the current value.
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtPanelPeer_endValidate( JNIEnv *env, 
                                         jobject thisObj )
{
#ifdef QWS
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *panel = QtComponentPeer::getPeerFromJNI( env,
                                                              thisObj );

    AWT_QT_LOCK; // 6332813, setCaption() needs a lock.

    QpWidget *widget;
    QWidget  *topWidget;

    if ( panel == 0 ||
         ( widget = panel->getWidget() ) == 0 || 
         ( topWidget = widget->topLevelWidget() ) == 0 || 
         topWidget->isHidden() ) {
        AWT_QT_UNLOCK;
        return;
    }

    QWSDisplay *display = QPaintDevice::qwsDisplay();
    if ( display != 0 ) {
        display->setCaption( topWidget, topWidget->caption() );
        AWT_QT_UNLOCK;
    }
#endif // QWS
    return;
}

