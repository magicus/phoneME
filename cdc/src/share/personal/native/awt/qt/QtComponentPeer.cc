/*
 * @(#)QtComponentPeer.cc	1.47 06/10/25
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
#include "jlong.h"
#include "sun_awt_qt_QtComponentPeer.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_Cursor.h"
#include "java_awt_event_FocusEvent.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "QtComponentPeer.h"
#include "QtApplication.h"
#include "KeyCodes.h"
#include "QtGraphics.h"
#include <qptrdict.h>
#include <qapplication.h>
#include <qwidgetlist.h>
#include <qscrollview.h>
#include "QtWindowPeer.h"
#include "java_awt_event_WindowEvent.h"
#include "QtToolkit.h"
#include "java_awt_KeyboardFocusManager.h"

#include "QtApplication.h"

#include "QtEvent.h"
#include "QtDisposer.h"

/*
 * Define a maximum number of paint events which we'll coalesce
 * together, while waiting for a timer event, before sending
 * them off to the Java event queue.
 */
#define COALESCE_MAX   15

// See QtToolkit.cc.
// 6176847
// Converted all dictionary to pointers instead of objects
extern QPtrDict<bool> *keyEventDict;
extern QPtrDict<bool> *crossEventDict;
extern QPtrDict<bool> *mouseEventDict;
extern QPtrDict<bool> *focusEventDict;
// 6176847
static bool BOOL_TRUE = true;   //6228134

QPtrDict<QtComponentPeer> *QtComponentPeer::widgetAssoc = 
new QPtrDict<QtComponentPeer>();

/* Initializes the ComponentData. This sets up the global reference to
   the peer and attaches the data to the peer.  It also connects
   signals to the widgets necessary so that the appropriate callbacks
   are called.  These signals handle events that are common to all
   objects that are instances of the java.awt.Component class.  widget
   defines the main qt widget for the component (this should be the
   outermost widget for the component).  drawWidget defines the widget
   used for expose events and also the widget that will be used for
   the graphics for the Java component.  styleWidget defines the
   widget that will be used to install font, background and foreground
   on the Java component if they haven't already been set. If
   styleWidget is NULL then this will not be done.  Returns JNI_TRUE
   if initialization was successful or JNI_FALSE if an exception has
   been thrown. */

QtComponentPeer::QtComponentPeer (JNIEnv *env,
				  /* The peer object being initialized */
				  jobject thisObj,           
				  /* The main widget */
				  QpWidget *thisWidget,           
				  /* The widget used to initialize the
				     foreground, background, font on
				     the Java component from its
				     style. */
				  QpWidget *refWidget): QtPeerBase(NULL, "QtComponentPeer")
{
    this->widget = thisWidget;
    this->peerGlobalRef = env->NewGlobalRef (thisObj);
    this->installedBackground = NULL;
    this->installedForeground = NULL;
    this->installedFont = NULL;
    this->cursor = NULL;
    this->coalescedPaints = QRect( 0, 0, -1, -1 );
    this->coalescedCount = 0;
    this->noCoalescing = FALSE;
    this->coalesceTimer = 0;
    
    if (this->peerGlobalRef == NULL)
	// an exception has been thrown
	return;
    
    /* If a widget has been specified to get the style from then we
       install background, foreground and font on the Java component
       if they haven't been set usinf the values in the
       refWidget. */
    
    if (refWidget != NULL) {
	jobject target, color, font;
	
	/* Set the background/foreground/font on the target component
           if these have not yet been set. */
		   
	target = env->GetObjectField (thisObj, QtCachedIDs.QtComponentPeer_targetFID);
	color = env->GetObjectField (target, QtCachedIDs.java_awt_Component_backgroundFID);
	
	if (color == NULL) {	/* Background not set so install a
                                   background on component. */
	    color = QtComponentPeer::createColor (env,
				 &(refWidget->backgroundColor()));
				 
	    if (color == NULL) {
		env->DeleteLocalRef(target);
		return;
	    }
				 
	    this->installedBackground = color;
	    env->SetObjectField (target, QtCachedIDs.java_awt_Component_backgroundFID, color);
	} else {
	    env->DeleteLocalRef(color);
	}
	
	color = env->GetObjectField (target, QtCachedIDs.java_awt_Component_foregroundFID);
	
	if (color == NULL) {	/* Foreground not set so install a
                                   foreground on component. */
	    color = QtComponentPeer::createColor (env, &(refWidget->foregroundColor()));
	    
	    if (color == NULL) {
		env->DeleteLocalRef(target);
		return;
	    }
	    
	    this->installedForeground = color;
	    env->SetObjectField (target, QtCachedIDs.java_awt_Component_foregroundFID, color);
	} else {
	    env->DeleteLocalRef(color);
	}
	
	/* Install font on component. We need to generate a Java font
           object for the QFont and set it on the component. */
	
	font = env->GetObjectField (target, QtCachedIDs.java_awt_Component_fontFID);

	if (font == NULL) {
	    qtFont = new QFont(refWidget->font());

	    font = env->CallStaticObjectMethod (QtCachedIDs.QtFontPeerClass,
						QtCachedIDs.QtFontPeer_getFontMID,
						(jint) qtFont);
	    
	    if (env->ExceptionCheck ()) {
            env->DeleteLocalRef(target);
            return;
	    }
	    
	    this->installedFont = env->NewGlobalRef (font);
	    env->SetObjectField (target,
				 QtCachedIDs.java_awt_Component_fontFID,
				 this->installedFont);
	}

	env->DeleteLocalRef(font);
	env->DeleteLocalRef(target);
    }
    
    this->updateWidgetStyle (env);
    
    /* Store the peer in widgetAssoc so that given a Qt widget it is
       possible to determine the Java component that it represents. */
    putPeerForWidget(thisWidget, this);
		
    /* Attach the data to the peer. */
    
    env->SetIntField (thisObj,
		      QtCachedIDs.QtComponentPeer_dataFID,
		      (jint)this);
	
    /* Enable events for this widget so that we can post equivalent
       Java events to the Java event queue when they occur. */
	
    thisWidget->installEventFilter(this);

#ifdef QWS
    /* This is to simulate right button mouse click using stylus
       hold. However, it is commented out for now because we are
       seeing problems in other areas. */
    /*
    QPEApplication::setStylusOperation(thisWidget,
				       QPEApplication::RightOnHold);
    */
#endif
}

QtComponentPeer::~QtComponentPeer()
{
    if(this->widget)
    {
        this->widget->deleteLater();
        this->widget = 0;
    }

  if ( coalesceTimer != 0 ) {
    killTimer( coalesceTimer );
  }

}

/* Gets the component data associated with the widget.
   Returns the component data or NULL if the widget is not represented by a Java component. */
   
QtComponentPeer *
QtComponentPeer::getPeerForWidget (QWidget *widget)
{
    QtComponentPeer *data = NULL;
    
    AWT_QT_LOCK;
        data = widgetAssoc->find((void *)widget);
    AWT_QT_UNLOCK;

    return data;
}

void
QtComponentPeer::putPeerForWidget (QWidget *widget, QtComponentPeer *peer)
{
    if (widget != NULL && peer != NULL) {
	// 6227406: PP-TCK DlgEventTests signal 11.
	// The widgetAssoc data structure needs to be protected with
	// AWT_QT_LOCK/UNLOCK while it is being updated.
	AWT_QT_LOCK;
        widgetAssoc->insert ((void *)widget, peer);
	AWT_QT_UNLOCK;
    }
}

void
QtComponentPeer::removePeerForWidget (QWidget *widget)
{
    if (widget != NULL) {
	// 6227406: PP-TCK DlgEventTests signal 11.
	// The widgetAssoc data structure needs to be protected with
	// AWT_QT_LOCK/UNLOCK while it is being updated.
	AWT_QT_LOCK;
        widgetAssoc->remove ((void *)widget);
	AWT_QT_UNLOCK;
    }
}

/*
 * The following methods are automatically called during creation and 
 * destruction of the peer. The methods that take "QWidget" are available
 * to subclasses if they wish to install additional widgets for the 
 * peer.
 */
void
QtComponentPeer::putPeerForWidget (QpWidget *widget, QtComponentPeer *peer)
{
    // In Qt callbacks we get instance of QObject and not the proxy object,
    // so we maintain the hash map between the QObject and the peer. This 
    // also solves the backpointer from impl (QObject) to proxy (QtObject)
    // using the following
    // map(QObject) = QtComponentPeer
    // QtComponentPeer->getWidget() = QtObject 
    putPeerForWidget(widget->getQWidget(), peer);
}

void
QtComponentPeer::removePeerForWidget (QpWidget *widget)
{
    removePeerForWidget(widget->getQWidget());
}

/* Gets the component data associated with the peer.
   Returns the component data or NULL if an exception occurred.  */
/*
 * It is the responsibility of the caller to grab the qt lock
 * (by calling awt_qt_threadsEnter( env )) before calling this
 * method.  Failure in doing so is a multithreading bug where
 * the peer can be in the process of being disposed when this
 * method call is made, resulting in a crash.
 */
QtComponentPeer *
QtComponentPeer::getPeerFromJNI (JNIEnv *env, jobject peer)
{
    QtComponentPeer *peerObj = (QtComponentPeer *)
	env->GetIntField (peer, QtCachedIDs.QtComponentPeer_dataFID);
/*	
    if (peerObj == 0)
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, 
		       "peer object is null");
*/    
    return peerObj;
}

QpWidget *
QtComponentPeer::getWidget(void) const
{
    return this->widget;
}

jobject 
QtComponentPeer::getPeerGlobalRef(void) const
{
    return this->peerGlobalRef;
}

void 
QtComponentPeer::resetClickCount(void)
{
    this->clickCount = 1;
}

void 
QtComponentPeer::incrementClickCount(void)
{
    this->clickCount++;
}

int 
QtComponentPeer::getClickCount(void) const
{
    return this->clickCount;
}

//return FALSE or TRUE??
//right now, for handled events, return TRUE
bool
QtComponentPeer::eventFilter(QObject *orig, QEvent *event) 
{
  JNIEnv *env = 0;
  bool returnVal = FALSE;
  bool done = FALSE;

  if( JVM->AttachCurrentThread( (void **) &env, NULL ) != 0 ) {
      return FALSE;
  }
  
  if (widget->isSameObject(orig)) {
	switch(event->type()) {
	case QEvent::Paint :
	    {
		// We coalesce paint events together and use
		// a timer to send them to the Java side as a
		// single event.
                QtGraphics::EndPaintingWidget((QWidget*)orig);
		QPaintEvent *paintEvent = (QPaintEvent *) event;
		if ( noCoalescing == TRUE ) {
		    this->postPaintEvent( &( paintEvent->rect() ) );
		} else {
		    if ( coalesceTimer == 0 ) {
			coalesceTimer = startTimer( 100 );
		    }
		    if ( coalesceTimer != 0 ) {
			// Add this paint event to our record
			coalescePaintEvents( &( paintEvent->rect() ) );
		    } else {
			// There is no timer, so we're not coalescing.
			this->postPaintEvent( &( paintEvent->rect() ) );
			noCoalescing = TRUE;
		    }
		}
		returnVal = QObject::eventFilter( orig, event );
		done = TRUE;
		break;
	    }
	case QEvent::Show :
	    if (widgetShown((QShowEvent *) event)) {
              returnVal = done = TRUE;
	    }
	    break;
	case QEvent::FocusIn :
	case QEvent::FocusOut :
                widgetFocusChanged(env,
				   event->type(),
                                   ((QFocusEvent*)event)->reason());
          break;
	default :
	    break;
	}	
  }
  // If we haven't yet got a final return value, pass the event along.
  // This is still all performed inside a thread guard to prevent us
  // entering the Qt libraries while the event is being processed.
  if ( ! done ) {
    returnVal = QObject::eventFilter( orig, event );
  }

  return returnVal;
}

/* Creates a Java color object from the supplied QColor. This is used when installing
   backgrounds and foregrounds on Java components so that they match the native ones. */

jobject 
QtComponentPeer::createColor (JNIEnv *env, const QColor *color)
{
    jint redValue, greenValue, blueValue, alphaValue;

    QRgb rgba = color->rgb();
    redValue = (jint) qRed(rgba);
    greenValue = (jint) qGreen(rgba);
    blueValue = (jint) qBlue(rgba);
    alphaValue = (jint) qAlpha(rgba);

    jobject localRefColor = env->NewObject(QtCachedIDs.java_awt_ColorClass,
					   QtCachedIDs.java_awt_Color_constructorMID,
					   redValue,
					   greenValue,
					   blueValue,
					   alphaValue);

    if (localRefColor == NULL) {
	return NULL;		// Exception thrown.
    }

    jobject globalRefColor = NULL;

    if (localRefColor) {
	globalRefColor = env->NewGlobalRef(localRefColor);
	env->DeleteLocalRef(localRefColor);
    }

    return globalRefColor;
}

/* Utility function to post paint events to the AWT event dispatch thread. */

void
QtComponentPeer::postPaintEvent (const QRect *area)
{
    QtDisposer::Trigger guard;

    jobject thisObj = this->getPeerGlobalRef();
    JNIEnv *env;
	
    if ((JVM->AttachCurrentThread ((void **) &env, NULL) != 0) ||
        thisObj == NULL)
	    return;
    guard.setEnv(env);

    int x = area->x() ;
    int y = area->y() ;

// Fixed 6178052: the DemoFrame program not rendered correctly problem when
// Alt-Tabbing to switch between windows on SuSE.
/*
 * Removed the #ifdef QWS, since the same logic also applies for SuSE/Qt-3.3.1.
 */
//#ifdef QWS
    if ( env->IsInstanceOf(thisObj,QtCachedIDs.QtWindowPeerClass) ) {
        /*
         * Fix for bug #4807236
         * The paint event location for any QFrame is the inner frame 
         * (that is not including the frame decoration if any).
         * The Graphics orgin for all Window subclasses is 
         * (-leftBorder,-rightBorder). So we need to translate the paint
         * location coordinates with respect to the outer frame.
         */
        x += env->GetIntField (thisObj,QtCachedIDs.QtWindowPeer_leftBorderFID);
        y += env->GetIntField (thisObj,QtCachedIDs.QtWindowPeer_topBorderFID);
    }
/*
 * Removed the #ifdef QWS, since the same logic also applies for SuSE/Qt-3.3.1.
 */
//#endif /* QWS */
    env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtComponentPeer_postPaintEventMID,
			 (jint)x,
			 (jint)y,
			 (jint)area->width(),
			 (jint)area->height());
						    
    if (env->ExceptionCheck())
	env->ExceptionDescribe();
}

/* Sets the style for the widget and any children of the widget that
   belong to the Java component represented by the supplied data. If
   style is not NULL then the style will be set to the supplied style
   and the foreground and font parameters are ignored. Otherwise a
   style is created which is a clone of the widget's style and the
   foreground and font are set on the style. This is necessary to
   ensure that backgrounds can be pixmaps when possible. */

void
QtComponentPeer::propagateSettings( const QColor& background,
                                    const QColor& foreground, 
                                    const QFont& font)
{
    /* If component has no explicit background, foreground, font set
       then we don't change the style of the widget and just use its
       native style. */
	
    if (&background == NULL && &foreground == NULL && &font == NULL)
	return;

    /*
    //thread synchronization
    QWidgetList *childWidgets = getChildWidgetsForWidget(widget);
    QWidgetListIt it(*childWidgets);
    QWidget *childWidget;
    
    while ( (childWidget = it.current()) != 0) {
	++it;
	QtComponentPeer *childPeer = widgetAssoc->find((void
							*)childWidget);
	if (childPeer == 0) {
	    childWidget->setBackgroundColor(background);
	    //childWidget->setForegroundColor(foreground);
	    childWidget->setFont(font);
	}
    }
    */
   
    QpWidget *thisWidget = getWidget() ;

    if (&foreground || &background) {

        QPalette thisPalette = thisWidget->palette();

        AWT_QT_LOCK;

        QPalette p(thisPalette);

        if (&foreground) {
            p.setColor(QColorGroup::Foreground, foreground);
            p.setColor(QColorGroup::Text, foreground);
            p.setColor(QColorGroup::ButtonText, foreground);  
        } 

        if (&background) {
            // Bug Fix : 4716802
            // create a palette based on the background color specified.
            QPalette bgPalette(background,    // button color
                               background) ;  // background color
            // get the active color group from the palette
            QColorGroup cg(bgPalette.active()) ;

            // set all the color roles from the active color group that
            // background color can affect.
            p.setColor(QColorGroup::Button,     background);
            p.setColor(QColorGroup::Background, background);
            p.setColor(QColorGroup::Light,      cg.light());
            p.setColor(QColorGroup::Midlight,   cg.midlight());
            p.setColor(QColorGroup::Dark,       cg.dark());
            p.setColor(QColorGroup::Mid,        cg.mid());
            p.setColor(QColorGroup::Base,       cg.base());
            p.setColor(QColorGroup::Shadow,     cg.shadow());
            // Bug Fix : 4716802
        } 

        AWT_QT_UNLOCK;

        thisWidget->setPalette(p);
    }
    
    if (&font) {
        thisWidget->setFont(font);
    }
}

bool
QtComponentPeer::widgetShown(QShowEvent *evt) 
{
    if (evt->spontaneous()) {
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread ((void **) &env,
				      NULL) == 0) {
	    this->updateWidgetCursor(env);
	}
	return TRUE;
    }
    return FALSE;
}

bool
QtComponentPeer::widgetFocusChanged(JNIEnv *env,
				    QEvent::Type type,
                                    QFocusEvent::Reason reason) 
{
    // This is a workaround for the problem where Qt/Embedded does not
    // properly generate QEvent::WindowActivate event. The logic in
    // QtWindowPeer.java should prevent multiple window-activated
    // events to actually be posted in the AWT event queue.
    // If the bug in Qt/Embedded is fixed, we can remove this code.
/*
#ifdef QWS
    if (evt->type() == QEvent::FocusIn) {
	QWidget *windowWidget = this->getWidget()->topLevelWidget();
	QtWindowPeer *windowPeer = (QtWindowPeer *)
	    QtComponentPeer::getPeerForWidget( windowWidget );
	windowPeer->postWindowEvent(java_awt_event_WindowEvent_WINDOW_ACTIVATED);
    }
#endif
*/
    /*
     * Focus management is dealt with in the Java layer.  In particular,
     * KeyboardFocusManager and DefaultKeyboardFocusManager are responsible for
     * generating the AWT FocusEvents.
     */

    /* Note: QtEmbedded does not set the Reason to ActiveWindow on a focus in
     * event.  So, for FocusIn, we don't do the check - just post the event. 
     * DefaultKeyboardFocusManager will handle it if an unnecessary
     * WINDOW_GAINED_FOCUS_EVENT gets posted
     */
    if (type == QEvent::FocusIn) {
        QtDisposer::Trigger guard(env);
        
        QWidget *windowWidget = this->getWidget()->topLevelWidget();
        QtWindowPeer *windowPeer = (QtWindowPeer *)
            QtComponentPeer::getPeerForWidget( windowWidget );
        if (windowPeer != NULL)
            windowPeer->handleFocusIn(env);
        
    }
    if (reason == QFocusEvent::ActiveWindow) {
        if (type == QEvent::FocusOut) {
            QtDisposer::Trigger guard(env);
            
            QWidget *windowWidget = this->getWidget()->topLevelWidget();
            QtWindowPeer *windowPeer = (QtWindowPeer *)
                QtComponentPeer::getPeerForWidget( windowWidget );
            if (windowPeer != NULL)
                windowPeer->handleFocusOut(env);
        }
    }
    // we'll handle the events in java - then, if we need qt to see the
    // focus event, we'll post a new one back to qt
    return TRUE;

}    

/* Updates the style of the widget to reflect the current settings of
   the target component (e.g. font, background, foreground). This is
   called when the widget is first created or if one of the settings
   is changed on the component whilst it is realized. We need to
   create a new style to make sure that this widget is not sharing the
   style with another widget (otherwise background colors etc will be
   changed across all widgets that share it). If the background has
   been set on the target component then we need to create a new style
   which does not use the theme engine to render
   backgrounds. Otherwise we copy the current style so as to use the
   theme engine for backgrounds.  Once the new style is created we
   initialize it with the font and other properties such as
   foreground.*/

void
QtComponentPeer::updateWidgetStyle (JNIEnv *env)
{
    QtDisposer::Trigger guard(env);

    jobject thisObj = this->getPeerGlobalRef();
    if(!thisObj)
    {
        return;
    }

    jobject target;
    jobject foreground;
    jobject background;
    jobject font;
    jboolean canHavePixmapBackground;
    QColor *qtforeground = NULL;
    QColor *qtbackground = NULL;
    QFont *qtfont = NULL;
    
    /* Retrieve settings from target component */
    
    target = env->GetObjectField (thisObj, 
                                  QtCachedIDs.QtComponentPeer_targetFID);
    background = env->GetObjectField (target, 
                               QtCachedIDs.java_awt_Component_backgroundFID);
    foreground = env->GetObjectField (target, 
                               QtCachedIDs.java_awt_Component_foregroundFID);
    font = env->GetObjectField (target, 
                                QtCachedIDs.java_awt_Component_fontFID);
    canHavePixmapBackground = env->CallBooleanMethod (thisObj,
		  QtCachedIDs.QtComponentPeer_canHavePixmapBackgroundMID);
    
    /* If this component is allowed to have a oxmap background and the
       user has not explicitly set the background colour then
       initialize the qtbackgroundptr so as to use any possible pixmap
       for the background. */
	
    if (!(canHavePixmapBackground &&
          (background == NULL || 
           env->IsSameObject (background,
                              this->installedBackground))))	{
        qtbackground = QtGraphics::getColor (env, background);
        //should we check whether qtbackground is NULL?
    }
    
    /* Get foreground color. */
    
    if (!(foreground == NULL || 
          env->IsSameObject (foreground,
                             this->installedForeground))) {
        qtforeground= QtGraphics::getColor (env, foreground);
        //should we check whether qtforeground is NULL?
    }
    
    /* Get font. */
    
    if (!(font == NULL || env->IsSameObject (font, this->installedFont))) {
        jobject fontPeer = env->GetObjectField (font,
                                QtCachedIDs.java_awt_Font_peerFID);
        qtfont = (QFont *) env->GetIntField (fontPeer,
                                             QtCachedIDs.QtFontPeer_dataFID);
        env->DeleteLocalRef(fontPeer);
    }
    
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(foreground);
    env->DeleteLocalRef(background);
    env->DeleteLocalRef(font);

    /* Now recursively set the style for the widget and any children it may have. */
    
    this->propagateSettings (*qtbackground, *qtforeground, *qtfont);

    delete qtbackground;   //6228132
    delete qtforeground;   //6228132
}

// 6201639
void
QtComponentPeer::setCursor(int javaCursorType)
{
    int qttype = -1;

    // [i]   = java Cursor Type
    // [i+1] = Qt Cursor Type
    static jint cursorTypes [] =
    {
	java_awt_Cursor_DEFAULT_CURSOR, ArrowCursor,
	java_awt_Cursor_CROSSHAIR_CURSOR, CrossCursor,
	java_awt_Cursor_TEXT_CURSOR, IbeamCursor,
	java_awt_Cursor_WAIT_CURSOR, WaitCursor,
	java_awt_Cursor_SW_RESIZE_CURSOR, SizeBDiagCursor,
	java_awt_Cursor_SE_RESIZE_CURSOR, SizeFDiagCursor,
	java_awt_Cursor_NW_RESIZE_CURSOR, SizeFDiagCursor,
	java_awt_Cursor_NE_RESIZE_CURSOR, SizeBDiagCursor,
	java_awt_Cursor_N_RESIZE_CURSOR, SizeVerCursor,
	java_awt_Cursor_S_RESIZE_CURSOR, SizeVerCursor,
	java_awt_Cursor_W_RESIZE_CURSOR, SizeHorCursor,
	java_awt_Cursor_E_RESIZE_CURSOR, SizeHorCursor,
	java_awt_Cursor_HAND_CURSOR, PointingHandCursor,
	java_awt_Cursor_MOVE_CURSOR, CrossCursor,
	-1
    };

    if (this->widget == NULL)
        return;
    
	for (int i = 0; ; i += 2) {
	    jint cursorType = cursorTypes[i];
	    
	    if (cursorType == -1)
            break;
	    
	    if (cursorType == javaCursorType) {
            qttype = cursorTypes[i + 1];
            break;
	    }
	}
    
    if (qttype >= 0 ) {
        if (this->cursor == NULL || this->cursor->shape() != qttype) {
            QCursor *newCursor = new QCursor(qttype);
            this->widget->setCursor(*newCursor);
            if (this->cursor != NULL) {
                delete (this->cursor);
            }
            this->cursor = newCursor;
        }
    }
}
// 6201639

// 6201639
// Only used from widgetShown()
void
QtComponentPeer::updateWidgetCursor (JNIEnv *env)
{
    jobject thisObj = this->peerGlobalRef;
    jobject target;
    jobject javaCursor = NULL;
    
    if ( thisObj == NULL )
        return;

    target = env->GetObjectField (thisObj,
                      QtCachedIDs.QtComponentPeer_targetFID);
    javaCursor = env->GetObjectField (target,
                      QtCachedIDs.java_awt_Component_cursorFID);
    
    if (javaCursor != NULL) {
        jint type = env->GetIntField (javaCursor,
                                 QtCachedIDs.java_awt_Cursor_typeFID);
        this->setCursor(type);
    }
    
    env->DeleteLocalRef(target);
    env->DeleteLocalRef(javaCursor);
}
// 6201639

jobject
QtComponentPeer::wrapInSequenced(jobject awtevent) {
    static jclass classSequencedEvent = NULL;
    static jmethodID mid = NULL;
    jobject wrapperEventLocal = NULL;
    jobject wrapperEvent = NULL;
                                                                                
    JNIEnv *env;
    if (JVM->AttachCurrentThread ((void **) &env, NULL) != 0)
	    return NULL;
                                                                                
    if (env->PushLocalFrame( 5) < 0)
        return NULL;
                                                                                
    if (classSequencedEvent == NULL) {
        jobject sysClass = env->FindClass("java/awt/SequencedEvent");
        if (sysClass != NULL) {
            /* Make this class 'sticky', we don't want it GC'd */
            classSequencedEvent = (jclass)env->NewGlobalRef( sysClass);
            if (mid == NULL) {
              mid = env->GetMethodID(classSequencedEvent
                                        ,"<init>"
                                        ,"(Ljava/awt/AWTEvent;)V");
            }
        }
        if ( classSequencedEvent == NULL || mid == NULL) {
	    jclass excls = env->FindClass("java/lang/ClassNotFoundException");
	    if (excls != NULL)
		env->ThrowNew(excls, "java/awt/SequencedEvent");
            env->PopLocalFrame( 0);
            return NULL;
        }
    }
    wrapperEventLocal = env->NewObject(classSequencedEvent, mid, awtevent);
                                                                                
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
    if (wrapperEventLocal == NULL) {
        env->ThrowNew(QtCachedIDs.NullPointerExceptionClass, 
		      "constructor failed.");
        env->PopLocalFrame(0);
        return NULL;
    }
    wrapperEvent = env->NewGlobalRef(wrapperEventLocal);
    if (env->ExceptionOccurred()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame (0);
        return NULL;
    }
    if (wrapperEvent == NULL) {
        env->ThrowNew(QtCachedIDs.NullPointerExceptionClass,
		      "NewGlobalRef failed.");
        env->PopLocalFrame( 0);
        return NULL;
    }
                                                                                
    env->PopLocalFrame( 0);
    return wrapperEvent;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_FIELD_ID (QtComponentPeer_dataFID, "data", "I");
    GET_FIELD_ID (QtComponentPeer_targetFID, "target", "Ljava/awt/Component;");
    GET_FIELD_ID (QtComponentPeer_cursorFID, "cursor", "Ljava/awt/Cursor;");

    GET_METHOD_ID (QtComponentPeer_canHavePixmapBackgroundMID,
		   "canHavePixmapBackground", "()Z");
    GET_METHOD_ID (QtComponentPeer_postFocusEventMID,
		   "postFocusEvent", "(IZI)V");
    GET_METHOD_ID (QtComponentPeer_postMouseEventMID, "postMouseEvent", "(IJIIIIZI)V");
    GET_METHOD_ID (QtComponentPeer_postPaintEventMID, "postPaintEvent", "(IIII)V");
    GET_METHOD_ID (QtComponentPeer_postKeyEventMID, "postKeyEvent",
		   "(IJIICI)V");
    GET_METHOD_ID (QtComponentPeer_getUTF8BytesMID, "getUTF8Bytes",
		   "(C)[B");
    GET_METHOD_ID (QtComponentPeer_postEventMID, "postEvent",
                   "(Ljava/awt/AWTEvent;)V");

    cls = env->FindClass("java/awt/KeyboardFocusManager");
                                                                                
    if (cls == NULL)
        return;
                                                                                
    QtCachedIDs.java_awt_KeyboardFocusManagerClass = (jclass) env->NewGlobalRef
        (cls);

    GET_STATIC_METHOD_ID(java_awt_KeyboardFocusManager_shouldNativelyFocusHeavyweightMID, 
                  "shouldNativelyFocusHeavyweight",
                  "(Ljava/awt/Component;Ljava/awt/Component;ZZJ)I");

    cls = env->FindClass ("java/awt/Cursor");
    
    if (cls == NULL)
	return;
    
    GET_FIELD_ID (java_awt_Cursor_typeFID, "type", "I");
    
    cls = env->FindClass ("java/awt/AWTEvent");
	
    if (cls == NULL)
	return;
		
    GET_FIELD_ID (java_awt_AWTEvent_dataFID, "data", "I");
	
    cls = env->FindClass ("java/awt/event/MouseEvent");
	
    if (cls == NULL)
	return;
		
    GET_FIELD_ID (java_awt_event_MouseEvent_clickCountFID, "clickCount", "I");
	
    cls = env->FindClass ("java/awt/event/KeyEvent");
	
    if (cls == NULL)
	return;
		
    GET_FIELD_ID (java_awt_event_KeyEvent_modifiedFieldsFID, "modifiedFields", "I");
    GET_FIELD_ID (java_awt_event_KeyEvent_keyCodeFID, "keyCode", "I");
    GET_FIELD_ID (java_awt_event_KeyEvent_keyCharFID, "keyChar", "C");
	
    cls = env->FindClass ("java/awt/event/InputEvent");
	
    if (cls == NULL)
	return;
	
    GET_FIELD_ID (java_awt_event_InputEvent_modifiersFID, "modifiers", "I");

    cls = env->FindClass("java/awt/event/FocusEvent");
    if (cls == NULL)
        return;
    
    QtCachedIDs.FocusEventClass = (jclass) env->NewGlobalRef
        (cls);
    GET_METHOD_ID(java_awt_event_FocusEvent_constructorMID, "<init>",
                  "(Ljava/awt/Component;IZLjava/awt/Component;)V");
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtComponentPeer_isPacked (JNIEnv *env, jclass cls, jobject target)
{
    jboolean b = env->GetBooleanField(target, 
				      QtCachedIDs.java_awt_Component_isPackedFID);
    return b;
}

void 
QtComponentPeer::dispose(JNIEnv *env)
{
    jobject target, color, font;

    if(this->widget)
    {
        removePeerForWidget(this->widget);
    }

    if (this->peerGlobalRef != NULL) {
	target = env->GetObjectField (this->peerGlobalRef, 
				      QtCachedIDs.QtComponentPeer_targetFID);
		
	/* Uninstall any foreground/background/font that may have been
	   installed during peer creation. */ 
		
	if (this->installedBackground != NULL) {
	    color = env->GetObjectField (target, QtCachedIDs.java_awt_Component_backgroundFID);
	    
	    if (env->IsSameObject (color, this->installedBackground))
		env->SetObjectField (target, QtCachedIDs.java_awt_Component_backgroundFID, NULL);
				
	    env->DeleteGlobalRef (this->installedBackground);
	    env->DeleteLocalRef(color);
	}
	
	if (this->installedForeground != NULL) {
	    color = env->GetObjectField (target, 
					 QtCachedIDs.java_awt_Component_foregroundFID);
	    
	    if (env->IsSameObject (color, this->installedForeground))
		env->SetObjectField (target, 
				     QtCachedIDs.java_awt_Component_foregroundFID, 
				     NULL);
		
	    env->DeleteGlobalRef (this->installedForeground);
	    env->DeleteLocalRef(color);
	}
	
	if (this->installedFont != NULL) {
	    font = env->GetObjectField (target, 
					QtCachedIDs.java_awt_Component_fontFID);
	    
	    if (env->IsSameObject (font, this->installedFont))
		env->SetObjectField (target, QtCachedIDs.java_awt_Component_fontFID, NULL);
		
	    env->DeleteGlobalRef (this->installedFont);
	    env->DeleteLocalRef(font);
	}

	env->DeleteGlobalRef (this->peerGlobalRef);
    this->peerGlobalRef = NULL;
	env->DeleteLocalRef(target);
    }
    
    if (this->cursor != NULL) {
	delete (this->cursor);
    }
    
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_disposeNative (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
    if (peer != 0) {
	    env->SetIntField (thisObj, 
                          QtCachedIDs.QtComponentPeer_dataFID, (jint)0);

        if(peer->getWidget())
        {
            peer->getWidget()->removeEventFilter(peer);
        }
        QtDisposer::postDisposeRequest(peer);
    }
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtComponentPeer_getLocationOnScreen (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
    jint x = 0, y = 0;
    
    if (peer == NULL) {
	return NULL;
    }

    if (peer->getWidget() == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return NULL;
    }
	
    QPoint widgetPos;
    /* Fix 5089664.  For a toplevel widget, mapToGlobal() returns a position
       of top-left drawable region instead of the top-left of the window 
       decoration.  Thus rely on QWidget.pos() instead for a toplevel.
       Toplevel distinction is a java level instance comparison here, but 
       we may want to switch to QWidget.isTopLevel() call if the performance
       becomes an issue here in the future. */
    if (env->IsInstanceOf(thisObj, QtCachedIDs.QtWindowPeerClass)) {
       widgetPos = peer->getWidget()->pos();
    } else {
       widgetPos = peer->getWidget()->mapToGlobal(QPoint(0,0));
    }
    //QPoint widgetPos = peer->getWidget()->mapToGlobal(QPoint(0, 0));

    x = widgetPos.x();
    y = widgetPos.y();

    if (env->IsInstanceOf(thisObj, QtCachedIDs.QtWindowPeerClass)) {
	jint topBorder = env->GetIntField (thisObj,
		 			  QtCachedIDs.QtWindowPeer_topBorderFID);
        if (topBorder != 0) { /* this is a decorated window */
	   jint qwsX = env->GetStaticIntField (QtCachedIDs.QtWindowPeerClass,
					    QtCachedIDs.QtWindowPeer_qwsXFID);
	   jint qwsY = env->GetStaticIntField (QtCachedIDs.QtWindowPeerClass,
					    QtCachedIDs.QtWindowPeer_qwsYFID);
	
	   x = x - qwsX;
	   y = y - qwsY;
        }
    }

    return env->NewObject (QtCachedIDs.java_awt_PointClass, 
			   QtCachedIDs.java_awt_Point_constructorMID, 
			   x, 
			   y);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtComponentPeer_getPreferredSize (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
    jint prefWidth, prefHeight;
    
    if (peer == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return NULL;
    }
    
    jobject target = env->GetObjectField (thisObj,
					  QtCachedIDs.QtComponentPeer_targetFID);
    jint setWidth = env->GetIntField (target,
				      QtCachedIDs.java_awt_Component_widthFID);
    jint setHeight = env->GetIntField (target,
				       QtCachedIDs.java_awt_Component_heightFID);

    QpWidget *widget = peer->getWidget();
    QSize preferredSize = widget->sizeHint();
    if (preferredSize.isValid()) {
	prefWidth = (jint) preferredSize.width();
	prefHeight = (jint) preferredSize.height();
    } else {
	prefWidth = ( (setWidth > 0) ? setWidth : 0 );
	prefHeight = ( (setHeight > 0) ? setHeight : 0);
    }

    jobject size = env->NewObject (QtCachedIDs.java_awt_DimensionClass,
				   QtCachedIDs.java_awt_Dimension_constructorMID,
				   prefWidth,
				   prefHeight);

    return size;
}

/*
JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtComponentPeer_getMinimumSize (JNIEnv *env, jobject thisObj)
{
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
    jint minWidth, minHeight;
    
    if (peer == NULL) {
	env->ThrowNew (QtCachedIDs.NullPointerExceptionClass, NULL);
	return NULL;
    }
    
    jobject target = env->GetObjectField (thisObj,
					  QtCachedIDs.QtComponentPeer_targetFID);
    jint setWidth = env->GetIntField (target,
				      QtCachedIDs.java_awt_Component_widthFID);
    jint setHeight = env->GetIntField (target,
				       QtCachedIDs.java_awt_Component_heightFID);

    QpWidget *widget = peer->getWidget();
    QSize minimumSize = widget->minimumSizeHint();
    if (minimumSize.isValid()) {
	minWidth = (jint) minimumSize.width();
	minHeight = (jint) minimumSize.height();
    } else {
	minWidth = ( (setWidth > 0) ? setWidth : 0 );
	minHeight = ( (setHeight > 0) ? setHeight : 0);
    }

    jobject size = env->NewObject (QtCachedIDs.java_awt_DimensionClass,
				   QtCachedIDs.java_awt_Dimension_constructorMID,
				   minWidth,
				   minHeight);

    return size;
}
*/

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_setBoundsNative (JNIEnv *env, jobject thisObj, jint x, jint y, jint width, jint height)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
    
    if (peer == NULL) {
	return;
    }

    QpWidget *widget = peer->getWidget();
    widget->setGeometry((int) x, (int) y, (int) width, (int) height);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_show (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (peer == NULL) {
	return;
    }

    QpWidget *widget = peer->getWidget();
    widget->show();
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_hide (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (peer == NULL) {
	return;
    }

    QpWidget *widget = peer->getWidget();
    widget->hide();
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_setEnabled (JNIEnv *env, jobject thisObj, jboolean enabled)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (peer == NULL) {
	return;
    }
	
    bool enabledCPP = ((enabled == JNI_TRUE) ? TRUE : FALSE);
    QpWidget *widget = peer->getWidget();
    widget->setEnabled(enabledCPP);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_updateWidgetStyle (JNIEnv *env, jobject thisObj)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (peer == NULL) {
	return;
    }

    peer->updateWidgetStyle (env);
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_setCursorNative (JNIEnv *env, 
                                                 jobject thisObj, 
                                                 jint cursorType)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI (env, thisObj);
	
    if (peer == NULL) {
	return;
    }

    // 6201639
    // Prior to calling setCursor(), we were calling updateWidgetCursor(env)
    // ignoring the Cursor java object passed. updateWidgetCursor() sets
    // the cursor to the Qt widget only if the "peer.target" cursor was
    // set. While this works fine for heavyweights, it does not work for
    // lightweights, since the nativeContainer associated with the 
    // lightweight does not have any Cursor set and the java code was passing
    // the Cursor of the lightweight to this method and it was ignored.
    // This was the reason why setCursor() on lightweights dont work as the
    // bug states.
    //
    // The fix involves refactoring the updateWidgetCursor() to another
    // method called setCursor(), which blindly sets the on the Qt widget.
    // This fix works fine great for lightweights, but exposed a problem 
    // in the java side (See LightweightDispatcher class) when there are
    // Heavyweight components within a Heavyweight container and it was 
    // incorrectly setting the cursor, when it should not have to. 
    //
    // The bottom line is, this method does not have any notion of 
    // lightweights and will simply set the cursor to the Qt widget 
    // associated with the peer. Java code should ensure that this 
    // method is called correctly.
    peer->setCursor (cursorType);
    // 6201639
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_setFocusable(JNIEnv *env, jobject thisObj,
					  jboolean focusable)
{
    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI(env,
							    thisObj);
    
    if (peer == NULL) {
	return;
    }

    QpWidget *widget = peer->getWidget();
    if (widget != NULL) {
	widget->setFocusable(focusable);
    }
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtComponentPeer_nativeRequestFocus (JNIEnv *env, 
                                         jobject thisObj,
                                         jobject lightweightChild, 
                                         jboolean temporary,
                                         jboolean focusedWindowChangeAllowed, 
                                         jlong time)
{
    jint retval=0;
    jobject target=NULL;
    jobject wrappedEvent=NULL;

    target = env->GetObjectField(thisObj, 
                                 QtCachedIDs.QtComponentPeer_targetFID);
    retval = env->CallStaticIntMethod(
             QtCachedIDs.java_awt_KeyboardFocusManagerClass,
             QtCachedIDs.java_awt_KeyboardFocusManager_shouldNativelyFocusHeavyweightMID,
             target, lightweightChild, temporary, JNI_FALSE, time);

    if (retval == java_awt_KeyboardFocusManager_SNFH_SUCCESS_HANDLED) {
        env->DeleteLocalRef(target);
        // in this case, focus events are generated in java
        return JNI_TRUE;
    }
    if (retval == java_awt_KeyboardFocusManager_SNFH_FAILURE) {
        env->DeleteLocalRef(target);
        return JNI_FALSE;
    }

    QtDisposer::Trigger guard(env);

    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer == NULL)
        return JNI_FALSE;
    QpWidget *widget = peer->getWidget();
    if (widget != NULL)
        widget->setFocus();

    // post the focus lost event for the current focus owner
    QWidget *focusWidget = qtApp->focusWidget();
    QtComponentPeer *focusPeer = 
        QtComponentPeer::getPeerForWidget(focusWidget);
    if (focusPeer != NULL && focusPeer != peer) {
        jobject focusObj = focusPeer->getPeerGlobalRef();
        env->MonitorEnter(focusObj);
        jobject focusTarget = env->GetObjectField(focusObj,
                                 QtCachedIDs.QtComponentPeer_targetFID);
        // create the new focus event
        jobject lostEvent = env->NewObject(QtCachedIDs.FocusEventClass,
                           QtCachedIDs.java_awt_event_FocusEvent_constructorMID,
                           focusTarget, java_awt_event_FocusEvent_FOCUS_LOST,
                           temporary, (jint)0);
        // wrap in sequenced event
        wrappedEvent = QtComponentPeer::wrapInSequenced(lostEvent);
        // now post the wrapped event to java
	env->CallVoidMethod(focusObj,
			    QtCachedIDs.QtComponentPeer_postEventMID,
                            wrappedEvent);
        env->MonitorExit(focusObj);
        env->DeleteGlobalRef(wrappedEvent);
    }

    // create the new focus gained event
    jobject gainedEvent = env->NewObject(QtCachedIDs.FocusEventClass, 
                           QtCachedIDs.java_awt_event_FocusEvent_constructorMID,
                           target, java_awt_event_FocusEvent_FOCUS_GAINED,
                           temporary, (jint)0);
    // wrap in sequenced event 
    wrappedEvent = QtComponentPeer::wrapInSequenced(gainedEvent);
    // now post the wrapped event to java
    env->CallVoidMethod(thisObj,
			QtCachedIDs.QtComponentPeer_postEventMID, wrappedEvent);
    env->DeleteGlobalRef(wrappedEvent);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_setNativeEvent (JNIEnv *env,
						jobject thisObj,
						jobject event,
						jint nativeEvent) 
{
    env->SetIntField (event, QtCachedIDs.java_awt_AWTEvent_dataFID,
		      nativeEvent);
}

/*
 * Note : Java calls post*EventToQt() or eventConsumed(). 
 * The eventConsumed() method cleans up the QEvent object and the 
 * wrapper QtEventObject, whereas the post*EventToQt() leaves the
 * deletion of QEvent object to Qt and just deletes the wrapper object
 * (Note :- postKeyEventToQt() does the deletion of QEvent and QtEventObject
 *  since it performs another clone of QKeyEvent, which qt will delete)
 *
 * The "data" field can be NULL, if the event was created by the java side.
 * Since the java code does not check if the event was generated by the
 * native code, we do it in post*EventToQt() and eventConsumed() method.
 */

#define QT_RETURN_IF_JAVA_EVENT(_eventObj) if (_eventObj == NULL) return

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_postMouseEventToQt (JNIEnv *env, 
						    jobject thisObj, 
						    jobject event)
{
    QtEventObject *eventObj = (QtEventObject *)env->GetIntField (event, 
                                   QtCachedIDs.java_awt_AWTEvent_dataFID);
    QT_RETURN_IF_JAVA_EVENT(eventObj);
    QEvent *ce = eventObj->event;

    QtDisposer::Trigger guard(env);
    
    QpWidget *sourceWidget = NULL;
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer != NULL)
        sourceWidget = peer->getWidget();

	/* This mouse event was created in our native peer implementation
	   and by nature it is not a spontaneous event, meaning not generated
	   from the windowing system.  A non-spontaneous event will not
	   be sent back to the Java layer. */
		
	// How should we handle multiple clicks?

    QObject *source = (sourceWidget == NULL) ? qApp : eventObj->source;

    // We need to protect the dictionary data structure from multithreaded accesses.
    // For qt-2.3.2-emb, QApplication::postEvent() also needs to be protected.
    AWT_QT_LOCK;
    int type = ce->type();
    switch (type) {

    case QEvent::Enter:
    case QEvent::Leave:
        crossEventDict->insert(ce, &BOOL_TRUE);   //6228134
        break;

    default:
        mouseEventDict->insert(ce, &BOOL_TRUE);   //6228134
        break;
    }
#ifndef QT_THREAD_SUPPORT
    qApp->postEvent(source, ce);
#endif
    AWT_QT_UNLOCK;

    // For QT_THREAD_SUPPORT, we post the event outside the AWT_QT_LOCK/UNLOCK
    // context.
#ifdef QT_THREAD_SUPPORT
    qApp->postEvent(source, ce);
#endif

    // Qt will take ownership of eventObject->event and will delete it, so
    // just delete the QtEventObject
    env->SetIntField (event, QtCachedIDs.java_awt_AWTEvent_dataFID, (jint)0);
    delete eventObj;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtComponentPeer_eventConsumed(JNIEnv *env, 
                                              jobject thisObj, 
                                              jobject event)
{
    QtEventObject *eventObj = (QtEventObject *)env->GetIntField (event, 
                                   QtCachedIDs.java_awt_AWTEvent_dataFID);
    QT_RETURN_IF_JAVA_EVENT(eventObj);
	env->SetIntField (event, QtCachedIDs.java_awt_AWTEvent_dataFID, (jint)0);
    delete eventObj->event;
    delete eventObj;
}

JNIEXPORT void JNICALL
    Java_sun_awt_qt_QtComponentPeer_postKeyEventToQt (JNIEnv *env, 
						       jobject thisObj, 
						       jobject event)
{
    QpWidget *sourceWidget = NULL;
    QtEventObject *eventObj = (QtEventObject *)env->GetIntField (event,
                              QtCachedIDs.java_awt_AWTEvent_dataFID); 
    QT_RETURN_IF_JAVA_EVENT(eventObj);
    QKeyEvent *qteventOrig = (QKeyEvent *)eventObj->event;

    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer != NULL)
        sourceWidget = peer->getWidget();
    
    jint modifiedFields;
        
    int key = qteventOrig->key();
    int state = qteventOrig->state();
    QString *text = new QString(qteventOrig->text());
    QString temp;
	
    // Check if the event has been modified in Java. If so then we
    // need to modify the Qt event as well. Most key events won't
    // be modified so checking the modified field improves
    // performance and also reduces problems that could arise when
    // converting from Java key events to native ones. 
		    
    modifiedFields = env->GetIntField (event,
                     QtCachedIDs.java_awt_event_KeyEvent_modifiedFieldsFID);
                                       
    if (modifiedFields & java_awt_event_KeyEvent_KEY_CODE_MODIFIED_MASK) {
        jint keyCode = env->GetIntField 
            (event, QtCachedIDs.java_awt_event_KeyEvent_keyCodeFID);
        key = awt_qt_getQtKeyCode (keyCode);
    }
        
    // Modify Qt string to represent the keyChar of the event if it has
    // been modified. 
	
    if (modifiedFields & java_awt_event_KeyEvent_KEY_CHAR_MODIFIED_MASK) {
        jchar keyChar = env->GetCharField (event, 
                             QtCachedIDs.java_awt_event_KeyEvent_keyCharFID);
			
        if (keyChar == java_awt_event_KeyEvent_CHAR_UNDEFINED) {
            if (text) {
                delete text;
                text = new QString(QString::null);
            } else { 
                // Get keyChar as a sequence of bytes in UTF-8 encoding.
                jbyteArray byteArray = (jbyteArray) env->CallObjectMethod
                    (thisObj,
                     QtCachedIDs.QtComponentPeer_getUTF8BytesMID,
                     keyChar); 
                    
                if (env->ExceptionCheck ())
                    env->ExceptionDescribe ();
                else {
                    jsize length = env->GetArrayLength (byteArray);
                    jbyte *bytes = env->GetByteArrayElements(byteArray,NULL);
                        
                    if (bytes == NULL)
                        env->ExceptionDescribe ();
                    else {
                        if (text)
                            delete text;
                        temp = QString::fromUtf8((const char *) bytes, 
                                                 length);
                        text = &temp;
                    } 
                    env->ReleaseByteArrayElements(byteArray, bytes, 
                                                  JNI_ABORT);
                }
            }
        }
            
        if (modifiedFields&java_awt_event_KeyEvent_MODIFIERS_MODIFIED_MASK){
            jint modifiers = env->GetIntField (event,
                 QtCachedIDs.java_awt_event_InputEvent_modifiersFID);
                
            state = 0;
                
            if (modifiers & java_awt_event_InputEvent_SHIFT_MASK)
                state |= Qt::ShiftButton;
            if (modifiers & java_awt_event_InputEvent_CTRL_MASK)
                state |= Qt::ControlButton;
            if (modifiers & java_awt_event_InputEvent_ALT_MASK)
                state |= Qt::AltButton;
            if (modifiers & java_awt_event_InputEvent_BUTTON1_MASK)
                state |= Qt::LeftButton;
            if (modifiers & java_awt_event_InputEvent_BUTTON2_MASK)
                state |= Qt::MidButton;
            if (modifiers & java_awt_event_InputEvent_BUTTON3_MASK)
                state |= Qt::RightButton;
        }
    }
            
    // Mark event so that we know this event has already been
    // processed by Java and we shouldn't post the event back to
    // Java. We do this by setting an unused bit in the state
    // argument for the QKeyEvent. Our eventFilter should reset
    // this bit. 
		
    QKeyEvent *qteventNew = new QKeyEvent(qteventOrig->type(),
                                          key,
                                          qteventOrig->ascii(),
                                          //changed the state here from
                                          // 0x8000 to 0x0800 - docs are
                                          // wrong; 0x8000 is used by Qt
                                          // to indicate soft keyboard
                                          state,// | 0x0800,
                                          *text,
                                          qteventOrig->isAutoRepeat(),
                                          qteventOrig->count());

    QObject *source = (sourceWidget == NULL) ? qApp : eventObj->source;

    // We need to protect the dictionary data structure from multithreaded accesses.
    // For qt-2.3.2-emb, QApplication::postEvent() also needs to be protected.
    AWT_QT_LOCK;
    keyEventDict->insert(qteventNew, &BOOL_TRUE);   //6228134
#ifndef QT_THREAD_SUPPORT
    qApp->postEvent(source, qteventNew);
#endif
    AWT_QT_UNLOCK;
        
    // For QT_THREAD_SUPPORT, we post the event outside the AWT_QT_LOCK/UNLOCK
    // context.
#ifdef QT_THREAD_SUPPORT
    qApp->postEvent(source, qteventNew);
#endif

    env->SetIntField (event, QtCachedIDs.java_awt_AWTEvent_dataFID, 
                      (jint)0);

    // Qt will take ownership of "qteventNew" and will delete it, so
    // we delete the the first cloned QKeyEvent (qteventOrig) here.
    // (see also eventConsumed())
    delete qteventOrig;
    delete eventObj;
    delete text;
}

JNIEXPORT void JNICALL
    Java_sun_awt_qt_QtComponentPeer_postFocusEventToQt (JNIEnv *env, 
						       jobject thisObj, 
						       jobject event)
{
    QpWidget *sourceWidget = NULL;
    QtEventObject *eventObj = (QtEventObject *)env->GetIntField (event,
                              QtCachedIDs.java_awt_AWTEvent_dataFID); 
    //QT_RETURN_IF_JAVA_EVENT(eventObj);
    if (eventObj == NULL) {
	return;
    }
    QFocusEvent *qteventOrig = (QFocusEvent *)eventObj->event;

    QtDisposer::Trigger guard(env);
    
    QtComponentPeer *peer = QtComponentPeer::getPeerFromJNI(env, thisObj);
    if (peer != NULL)
        sourceWidget = peer->getWidget();

    QObject *source = (sourceWidget == NULL) ? qApp : eventObj->source;

    // We need to protect the dictionary data structure from multithreaded accesses.
    // For qt-2.3.2-emb, QApplication::postEvent() also needs to be protected.
    AWT_QT_LOCK;
    focusEventDict->insert(qteventOrig, &BOOL_TRUE);   //6228134
#ifndef QT_THREAD_SUPPORT
    qApp->postEvent(source, qteventOrig);
#endif
    AWT_QT_UNLOCK;
        
    // For QT_THREAD_SUPPORT, we post the event outside the AWT_QT_LOCK/UNLOCK
    // context.
#ifdef QT_THREAD_SUPPORT
    qApp->postEvent(source, qteventOrig);
#endif

    env->SetIntField (event, QtCachedIDs.java_awt_AWTEvent_dataFID, 
                      (jint)0);

    delete eventObj;
}


JNIEXPORT jlong JNICALL
Java_sun_awt_qt_QtComponentPeer_getQWidget(JNIEnv *env,
                                           jclass thisClass,
                                           int data) {
    return ptr_to_jlong(((QtComponentPeer *)data)->getWidget());
}

/*
 * We keep a running record of paint events we've received so
 * that we can batch them up in a single call to the Java side.
 * Incorporate this rect into our record of changes.
 */
void
QtComponentPeer::coalescePaintEvents( const QRect *r )
{
    // Input is valid?
    if ( r == 0 ||
         ! r->isValid() ) {
      return;
    }
  
    coalescedCount++;

    // Does one completely contain the other?
    if ( r->contains( coalescedPaints, FALSE ) ) {
      coalescedPaints = *r;
    } else if ( coalescedPaints.contains( *r, FALSE ) ) {
      ; // Do nothing, but don't do a union.
    } else {
      // A union
      coalescedPaints |= *r;
    }

    // If our count has reached some "reasonable" number without
    // a timer event coming through, it's likely because of events
    // clogging the queue; send the coalesced data anyway.
    if ( coalescedCount == COALESCE_MAX ) {
      this->postPaintEvent( &coalescedPaints );
      coalescedPaints = QRect( 0, 0, -1, -1 );
      coalescedCount = 0;
    } 

    return;
}

/*
 * Implement QObject's virtual function which will deliver time
 * events to us from the main event loop. When we receive a timer
 * event, deliver valid, coalesced paint events to the Java event
 * queue. Then reset ourselves. By using timerEvent() rather than
 * a slot, we avoid locking issues around the coalesced rect data.
 */
void
QtComponentPeer::timerEvent( QTimerEvent * te )
{
  if ( coalesceTimer != 0 ) {
    killTimer( coalesceTimer );
    coalesceTimer = 0;
  }
  if ( coalescedPaints.isValid() ) {
    this->postPaintEvent( &coalescedPaints );
    coalescedPaints = QRect( 0, 0, -1, -1 );
    coalescedCount = 0;
  }
  return;
}


