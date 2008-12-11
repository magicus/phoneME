/*
 * @(#)GComponentPeer.c	1.45 06/10/10
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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "jni.h"
#include "sun_awt_gtk_GComponentPeer.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_Cursor.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "GComponentPeer.h"
#include "KeyCodes.h"
#include "GdkGraphics.h"

extern void GToolkit_destroyStaticLocals(jobject ref);

struct gtkDataStruct {
	GtkStyle		*style;
	GdkWindow		*window;
	GtkStateType    state_type;
	GdkRectangle    *area;
	GtkWidget       *widget;
	gchar           *detail;		
};

static void multichar_draw_string(GtkStyle               *style,
	GdkWindow              *window,
	GtkStateType            state_type,
	GdkRectangle           *area,
	GtkWidget              *widget,
	gchar                  *detail,
	gint                    x,
	gint                    y,
	const gchar            *string)
{
	
	GComponentPeerData *peerData = (GComponentPeerData *)gtk_object_get_data (GTK_OBJECT(widget), "peerData");
	struct gtkDataStruct gds;
	JNIEnv *env;
	jobject this = peerData->peerGlobalRef;
		
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
	
	awt_gtk_callbackEnter();

	gds.window = window;
	gds.state_type = state_type;
	gds.area = area;
	gds.widget = widget;
	gds.detail = detail;
		
	(*env)->CallVoidMethod (env, this, GCachedIDs.GComponentPeer_drawMCStringMID,
							string,	(jint)x, (jint)y, (jint)&gds);
	
	awt_gtk_callbackLeave();
	
}

/* drawMCStringNative is only ever called from multichar_draw_string. This is because Java is used
 to break the UTF8 string up into "codepage"d strings or multichar strings */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_drawMCStringNative (JNIEnv *env, jobject this, jbyteArray text, jint textLength, jint gfont, jint x, jint y, jint gtkData)
{
	gchar *gtext;
	GdkFont *gdkfont = (GdkFont *)gfont;
	struct gtkDataStruct *gds = (struct gtkDataStruct *)gtkData;
	
	if (gdkfont == NULL || text == NULL || gds == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return;
	}
	
	gtext = (gchar *)(*env)->GetByteArrayElements (env, text, NULL);
	
	if (gtext == NULL)
		return;

	if (gds->area)
    {
		gdk_gc_set_clip_rectangle (gds->style->white_gc, gds->area);
		gdk_gc_set_clip_rectangle (gds->style->fg_gc[gds->state_type], gds->area);
	}

	if (gds->state_type == GTK_STATE_INSENSITIVE)
		gdk_draw_text (gds->window, gdkfont, gds->style->white_gc, x+1, y+1, gtext, textLength);

	gdk_draw_text (gds->window, gdkfont, gds->style->fg_gc[gds->state_type], x, y, gtext, textLength);

	if (gds->area)
	{
		gdk_gc_set_clip_rectangle (gds->style->white_gc, NULL);
		gdk_gc_set_clip_rectangle (gds->style->fg_gc[gds->state_type], NULL);
	}
	
	(*env)->ReleaseByteArrayElements (env, text, gtext, JNI_ABORT);
}

/* Utility function to post paint events to the AWT event dispatch thread. */

static void
postPaintEvent (GComponentPeerData *data, GdkRectangle *area)
{
	jobject this = data->peerGlobalRef;
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	awt_gtk_callbackEnter();
	
	(*env)->CallVoidMethod (env, this, GCachedIDs.GComponentPeer_postPaintEventMID,
							(jint)area->x,
							(jint)area->y,
						    (jint)area->width,
						    (jint)area->height);
						    
	awt_gtk_callbackLeave();
	
	if ((*env)->ExceptionCheck (env))
		(*env)->ExceptionDescribe (env);
}

static void
componentExposed (GtkWidget *widget, GdkEventExpose *event, GComponentPeerData *data)
{
	postPaintEvent (data, &event->area);
}

static void
componentDrawn (GtkWidget *widget, GdkRectangle *area, GComponentPeerData *data)
{
	postPaintEvent (data, area);
}

/* Sets the style for the widget and any children of the widget that belong to the Java component represented
   by the supplied data. If style is not NULL then the style will be set to the supplied style and the foreground
   and font parameters are ignored. Otherwise a style is created which is a clone of the widget's style and the foreground and
   font are set on the style. This is necessary to ensure that backgrounds can be pixmaps when possible. */

static void
setStyleRecursively (GtkWidget *widget, GComponentPeerData *data, GtkStyle *style, GdkColor *foreground, GdkFont *font)
{
	/* If component has no explicit background, foreground, font set then we don't change the style of the widget
	   and just use its native style. */
	
	if (style == NULL && foreground == NULL && font == NULL)
		return;

	if (GTK_IS_CONTAINER(widget))
	{
		GList *children = gtk_container_children(GTK_CONTAINER(widget));
		
		while (children != NULL)
		{
			GtkWidget *child = GTK_WIDGET(children->data);
			GComponentPeerData *childData = (GComponentPeerData *)gtk_object_get_data (GTK_OBJECT(child), "peerData");
		
			/* Only set the style for children that aren't java components or belong to this component. */
			
			if (childData == NULL || childData == data)
				setStyleRecursively (child, data, style, foreground, font);
				
			children = children->next;
		}
	}
	
	/* If no specific style is supplied then clone widget's style and set foreground and font accordingly. */
	
	if (style == NULL)
	{
		gtk_widget_ensure_style (widget);
		style = gtk_style_copy (widget->style);
		
		if (foreground != NULL)
		{
			style->fg[GTK_STATE_NORMAL] = *foreground;
			style->text[GTK_STATE_NORMAL] = *foreground;
		}
		
		if (font != NULL)
		{
			gdk_font_ref (font);
			gdk_font_unref (style->font);
			style->font = font;
		}
		
		gtk_widget_set_style (widget, style);
		gtk_style_unref (style);
	}
	
	else
		gtk_widget_set_style (widget, style);
}

/* Updates the style of the widget to reflect the cureent settings of the target component
   (e.g. font, background, foreground). This is called when the widget is first created or if one
   of the settings is changed on the component whilst it is realized. We need to create a new style to make sure that
	   this widget is not sharing the style with another widget (otherwise background colors etc
	   will be changed across all widgets that share it). If the background has been set on the target
	   component then we need to create a new style which does not use the theme engine to render
	   backgrounds. Otherwise we copy the current style so as to use the theme engine for backgrounds.
	   Once the new style is created we initialize it with the font and other properties such as foreground.*/

static void
updateWidgetStyle (JNIEnv *env, GComponentPeerData *data)
{
	jobject this = data->peerGlobalRef;
	jobject target;
	jobject foreground;
	jobject background;
	jobject font;
	GtkWidget *widget = data->widget;
	jboolean canHavePixmapBackground;
	GdkColor gdkforeground, gdkbackground;
	GdkColor *gdkforegroundptr = NULL;
	GdkFont *gdkfont = NULL;
	GtkStyle *style = NULL;

	/* Retrieve settings from target component */
	
	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
	background = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_backgroundFID);
	foreground = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_foregroundFID);
	font = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_fontFID);
	canHavePixmapBackground = (*env)->CallBooleanMethod (env, this, GCachedIDs.GComponentPeer_canHavePixmapBackgroundMID);
	
	awt_gtk_threadsEnter();
	
	/* If this component is allowed to have a oxmap background and the user has not explicitly set
	   the background colour then initialize the gdkbackgroundptr so as to use any possible pixmap
	   for the background. */
	
	if (!(canHavePixmapBackground &&
	    (background == NULL || (*env)->IsSameObject (env, background, data->installedBackground))))
	{
		awt_gtk_getColor (env, background, &gdkbackground);
		
		style = gtk_style_new();
		style->bg[GTK_STATE_NORMAL] = gdkbackground;
		style->base[GTK_STATE_NORMAL] = gdkbackground;
		
		/* Poke in our own string drawing */
		style->klass->draw_string = multichar_draw_string;
	}
	
	/* Get foreground color. */
	
	if (!(foreground == NULL || (*env)->IsSameObject (env, foreground, data->installedForeground)))
	{
		awt_gtk_getColor (env, foreground, &gdkforeground);
		
		if (style != NULL)
		{
			style->fg[GTK_STATE_NORMAL] = gdkforeground;
			style->text[GTK_STATE_NORMAL] = gdkforeground;
		}
		
		gdkforegroundptr = &gdkforeground;
	}
	
	/* Get font. */
/*	
	if (!(font == NULL || (*env)->IsSameObject (env, font, data->installedFont)))
	{
		jobject fontPeer = (*env)->GetObjectField (env, font, GCachedIDs.java_awt_Font_peerFID);
		gdkfont = (GdkFont *)(*env)->GetIntField (env, fontPeer, GCachedIDs.GFontPeer_dataFID);
		
		if (style != NULL && gdkfont != NULL)
		{
			gdk_font_ref (gdkfont);
			gdk_font_unref (style->font);
			style->font = gdkfont;
		}
	}
*/	
	/* Now recursively set the style for the widget and any children it may have. */
	
	setStyleRecursively (widget, data, style, gdkforegroundptr, gdkfont);
	
	if (style != NULL)
		gtk_style_unref (style);
	
	awt_gtk_threadsLeave();
}

static void
updateWidgetCursor (JNIEnv *env, GComponentPeerData *data)
{
	jobject this = data->peerGlobalRef;
	jobject cursor = NULL;
	GdkCursorType gdktype = -1;
	jint type;
	int i;
	
	static jint cursorTypes [] =
	{
		java_awt_Cursor_DEFAULT_CURSOR, -1,
		java_awt_Cursor_CROSSHAIR_CURSOR, GDK_CROSSHAIR,
		java_awt_Cursor_TEXT_CURSOR, GDK_XTERM,
		java_awt_Cursor_WAIT_CURSOR, GDK_WATCH,
		java_awt_Cursor_SW_RESIZE_CURSOR, GDK_BOTTOM_LEFT_CORNER,
		java_awt_Cursor_SE_RESIZE_CURSOR, GDK_BOTTOM_RIGHT_CORNER,
		java_awt_Cursor_NW_RESIZE_CURSOR, GDK_TOP_LEFT_CORNER,
		java_awt_Cursor_NE_RESIZE_CURSOR, GDK_TOP_RIGHT_CORNER,
		java_awt_Cursor_N_RESIZE_CURSOR, GDK_TOP_SIDE,
		java_awt_Cursor_S_RESIZE_CURSOR, GDK_BOTTOM_SIDE,
		java_awt_Cursor_W_RESIZE_CURSOR, GDK_LEFT_SIDE,
		java_awt_Cursor_E_RESIZE_CURSOR, GDK_RIGHT_SIDE,
		java_awt_Cursor_HAND_CURSOR, GDK_HAND2,
		java_awt_Cursor_MOVE_CURSOR, GDK_FLEUR,
	};
	
	if (data == NULL || data->widget->window == NULL)
		return;
		
	cursor = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_cursorFID);
		
	if (cursor != NULL)
	{
		type = (*env)->GetIntField (env, cursor, GCachedIDs.java_awt_Cursor_typeFID);
		
		for (i = 0; i < sizeof(cursorTypes); i += 2)
		{
			if (cursorTypes[i] == type)
			{
				gdktype = cursorTypes[i + 1];
				break;
			}
		}
	}
	
	awt_gtk_threadsEnter();
	
	if (data->cursor != NULL)
		gdk_cursor_destroy (data->cursor);
		
	if (gdktype != -1)
		data->cursor = gdk_cursor_new (gdktype);
		
	else data->cursor = NULL;
	
	gdk_window_set_cursor (data->widget->window, data->cursor);
	
	awt_gtk_threadsLeave();
}

static void
componentRealized (GtkWidget *widget, GComponentPeerData *data)
{
	JNIEnv *env;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
		
	updateWidgetCursor (env, data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_FIELD_ID (GComponentPeer_dataFID, "data", "I");
	GET_FIELD_ID (GComponentPeer_targetFID, "target", "Ljava/awt/Component;");
	GET_FIELD_ID (GComponentPeer_cursorFID, "cursor", "Ljava/awt/Cursor;");

	GET_METHOD_ID (GComponentPeer_canHavePixmapBackgroundMID, "canHavePixmapBackground", "()Z");
	GET_METHOD_ID (GComponentPeer_postMouseEventMID, "postMouseEvent", "(IJIIIIZI)V");
	GET_METHOD_ID (GComponentPeer_postPaintEventMID, "postPaintEvent", "(IIII)V");
	GET_METHOD_ID (GComponentPeer_postKeyEventMID, "postKeyEvent", "(IJIICI)V");
	GET_METHOD_ID (GComponentPeer_getUTF8BytesMID, "getUTF8Bytes", "(C)[B");
	GET_METHOD_ID (GComponentPeer_drawMCStringMID, "drawMCString", "([BIII)V");
	
	cls = (*env)->FindClass (env, "java/awt/Cursor");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_Cursor_typeFID, "type", "I");
	
	cls = (*env)->FindClass (env, "java/awt/AWTEvent");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_AWTEvent_dataFID, "data", "I");
	
	cls = (*env)->FindClass (env, "java/awt/event/MouseEvent");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_event_MouseEvent_clickCountFID, "clickCount", "I");
	
	cls = (*env)->FindClass (env, "java/awt/event/KeyEvent");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_event_KeyEvent_modifiedFieldsFID, "modifiedFields", "I");
	GET_FIELD_ID (java_awt_event_KeyEvent_keyCodeFID, "keyCode", "I");
	GET_FIELD_ID (java_awt_event_KeyEvent_keyCharFID, "keyChar", "C");
	
	cls = (*env)->FindClass (env, "java/awt/event/InputEvent");
	
	if (cls == NULL)
		return;
	
	GET_FIELD_ID (java_awt_event_InputEvent_modifiersFID, "modifiers", "I");
}

/* Gets the component data associated with the peer.
   Returns the component data or NULL if an exception occurred.  */

GComponentPeerData *
awt_gtk_getData (JNIEnv *env, jobject peer)
{
	GComponentPeerData *data = (GComponentPeerData *)(*env)->GetIntField (env, peer, GCachedIDs.GComponentPeer_dataFID);
	
	if (data == NULL)
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, "peer data is null");
	
	return data;
}

/* Gets the component data associated with the widget.
   Returns the component data or NULL if the widget is not represented by a Java component. */
   
GComponentPeerData *
awt_gtk_getDataFromWidget (GtkWidget *widget)
{
	GComponentPeerData *data = NULL;

	awt_gtk_threadsEnter();
	
	while (widget != NULL)
	{
		if (GTK_IS_MENU_ITEM (widget))
			break;
	
		data = (GComponentPeerData *)gtk_object_get_data (GTK_OBJECT(widget), "peerData");
		
		if (data != NULL)
			break;
			
		widget = widget->parent;
	}
	
	awt_gtk_threadsLeave();
	
	return data;
}

/* Creates a Java color object from the supplied GdkColor. This is used when installing
    backgrounds and foregrounds on Java components so that they match the native ones. */

static jobject
createColor (JNIEnv *env, GdkColor *color)
{
	jint red, green, blue;
	
	red = (jint)(color->red / (65535 / 255));
	green = (jint)(color->green / (65535 / 255));
	blue = (jint)(color->blue / (65535 / 255));
	
	return (*env)->NewGlobalRef (env,
								(*env)->NewObject (env,
								                   GCachedIDs.java_awt_ColorClass,
								                   GCachedIDs.java_awt_Color_constructorMID,
								                   red,
								                   green,
								                   blue));
}

/* Initializes the ComponentData. This sets up the global reference to the peer and attaches the data to the peer.
   It also connects signals to the widgets necessary so that the appropriate callbacks are called.
   These signals handle events that are common to all objects that are instances of the java.awt.Component class.
   widget defines the main gtk widget for the component (this should be the outermost widget for the component).
   drawWidget defines the widget used for expose events and also the widget that will be used for the graphics
   for the Java component.
   styleWidget defines the widget that will be used to install font, background and foreground on the Java component
   if they haven't already been set. If styleWidget is NULL then this will not be done.
   Returns JNI_TRUE if initialization was successful or JNI_FALSE if an exception has been thrown. */

jboolean
awt_gtk_GComponentPeerData_init (JNIEnv *env,
								 jobject this, 						/* The peer object being initialized */
								 GComponentPeerData *data,			/* The data to be initialized */
								 GtkWidget *widget,					/* The main widget */
								 GtkWidget *drawWidget,				/* The widget used for drawing and expose signals */
								 GtkWidget *styleWidget,			/* The widget used to initialise the foreground,backgorund,
								 									   font on the Java component from its style. */
								 jboolean usesBaseForBackground)	/* Some widgets draw their background using the base field of the style
								 									   and some use the bg field. This decides which will be used to
								 									   set the background on the Java component. */
{
	data->widget = widget;
	data->drawWidget = drawWidget;
	data->peerGlobalRef = (*env)->NewGlobalRef (env, this);
	data->cursor = NULL;
	data->installedBackground = NULL;
	data->installedForeground = NULL;
	data->installedFont = NULL;
	
	/* gtk_widget_ref(widget); */
	
	if (data->peerGlobalRef == NULL)
		return JNI_FALSE;
		
	/* If a widget has been specified to get the style from then we install background, foreground
	   and font on the Java component if they haven't been set usinf the values in the styleWidget. */
	
	if (styleWidget != NULL)
	{
		jobject target, color, font;
		gtk_widget_ensure_style(styleWidget);
	
		/* Set the background/foreground/font on the target component if these have not yet
		   been set. */
		   
		target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
		color = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_backgroundFID);
		
		if (color == NULL)	/* Background not set so install a background on component. */
		{
			color = createColor (env, usesBaseForBackground ? &styleWidget->style->base[GTK_STATE_NORMAL] : &styleWidget->style->bg[GTK_STATE_NORMAL]);
			
			if (color == NULL)
				return JNI_FALSE;
				
			data->installedBackground = color;
			(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_backgroundFID, color);
		}
		
		color = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_foregroundFID);
		
		if (color == NULL)	/* Background not set so install a background on component. */
		{
			color = createColor (env, &styleWidget->style->fg[GTK_STATE_NORMAL]);
			
			if (color == NULL)
				return JNI_FALSE;
				
			data->installedForeground = color;
			(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_foregroundFID, color);
		}
		
		/* Install font on component. We need to generate a Java font object for the GdkFont and set it on the component. */
		
		font = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_fontFID);
		
		if (font == NULL)
		{
			font = (*env)->CallStaticObjectMethod (env,
												   GCachedIDs.GFontPeerClass,
												   GCachedIDs.GFontPeer_getFontMID,
												   (jint)styleWidget->style->font);
			
			if ((*env)->ExceptionCheck (env))
				return JNI_FALSE;
				
			data->installedFont = (*env)->NewGlobalRef (env, font);
			(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_fontFID, font);
		}
	}
	
	updateWidgetStyle (env, data);
		
	/* Store the peer data in the widget so that given a Gtk widget it is possible
	   to detrmine the java component that it represents. */
		
	gtk_object_set_data (GTK_OBJECT(widget), "peerData", data);
	
	/* Attatch the data to the peer. */
	
	(*env)->SetIntField (env, this, GCachedIDs.GComponentPeer_dataFID, (jint)data);
	
	/* Enable events for this widget so that we can post equivalent Java events to the Java event queue
	   when they occur. */
	
	gtk_widget_add_events (widget, GDK_POINTER_MOTION_MASK |
								   GDK_BUTTON_MOTION_MASK |
								   GDK_BUTTON_PRESS_MASK |
								   GDK_BUTTON_RELEASE_MASK |
								   GDK_ENTER_NOTIFY_MASK |
								   GDK_LEAVE_NOTIFY_MASK |
								   GDK_KEY_PRESS_MASK |
								   GDK_KEY_RELEASE_MASK);
								   
	gtk_widget_add_events (drawWidget, GDK_EXPOSURE_MASK);
	
	gtk_signal_connect (GTK_OBJECT(drawWidget), "expose-event", GTK_SIGNAL_FUNC (componentExposed), data);
	gtk_signal_connect (GTK_OBJECT(drawWidget), "draw", GTK_SIGNAL_FUNC (componentDrawn), data);
	gtk_signal_connect (GTK_OBJECT(widget), "realize", GTK_SIGNAL_FUNC (componentRealized), data);
	
	return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_disposeNative (JNIEnv *env, jobject this)
{
	GComponentPeerData *data;
	jobject target, color;

	awt_gtk_threadsEnter();
	
	data = awt_gtk_getData (env, this);
	
	if (data == NULL)
	{
		awt_gtk_threadsLeave();
		return;
	}

	/*
	if (data->drawWidget != NULL) {
		gtk_widget_destroy (data->drawWidget);
	}
	*/
	
	if (data->widget != NULL) {
		gtk_widget_unref (data->widget);
	}
	
	if (data->peerGlobalRef != NULL)
	{
		target = (*env)->GetObjectField (env, data->peerGlobalRef, GCachedIDs.GComponentPeer_targetFID);
		
		/* Uninstall any foreground/background/font that may have been installed during peer creation. */
		
		if (data->installedBackground != NULL)
		{
			color = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_backgroundFID);
			
			if ((*env)->IsSameObject (env, color, data->installedBackground))
				(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_backgroundFID, NULL);
				
			(*env)->DeleteGlobalRef (env, data->installedBackground);
		}
		
		if (data->installedForeground != NULL)
		{
			color = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_foregroundFID);
			
			if ((*env)->IsSameObject (env, color, data->installedForeground))
				(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_foregroundFID, NULL);
				
			(*env)->DeleteGlobalRef (env, data->installedForeground);
		}
		
		if (data->installedFont != NULL)
		{
			color = (*env)->GetObjectField (env, target, GCachedIDs.java_awt_Component_fontFID);
			
			if ((*env)->IsSameObject (env, color, data->installedFont))
				(*env)->SetObjectField (env, target, GCachedIDs.java_awt_Component_fontFID, NULL);
				
			(*env)->DeleteGlobalRef (env, data->installedFont);
		}

                GToolkit_destroyStaticLocals(data->peerGlobalRef);
	
		(*env)->DeleteGlobalRef (env, data->peerGlobalRef);
	}
		
	if (data->cursor != NULL)
		gdk_cursor_destroy (data->cursor);
			
	(*env)->SetIntField (env, this, GCachedIDs.GComponentPeer_dataFID, (jint)NULL);
	free (data);
	awt_gtk_threadsLeave();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GComponentPeer_getLocationOnScreen (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	int x, y;
	unsigned int width, height, borderWidth, depth;
	Display *display;
	GdkWindow *window;
	Window xwindow, xroot, xparent, child;
	Window *children;
	unsigned int nchildren;
	
	if (data == NULL)
		return NULL;
	
	if (data->widget == NULL || (window = data->widget->window) == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return NULL;
	}
	
	awt_gtk_threadsEnter();
	gdk_error_trap_push();
	
	xwindow = GDK_WINDOW_XWINDOW(window);
	display = GDK_WINDOW_XDISPLAY(window);
		
	XGetGeometry (display, xwindow, &xroot, &x, &y, &width, &height, &borderWidth, &depth);
	XQueryTree (display, xwindow, &xroot, &xparent, &children, &nchildren);
	
	if (children != NULL)
		XFree (children);
	
	XTranslateCoordinates (display, xparent, xroot, x, y, &x, &y, &child);
	
	gdk_error_trap_pop();
	awt_gtk_threadsLeave();
	
	return (*env)->NewObject (env, GCachedIDs.java_awt_PointClass, GCachedIDs.java_awt_Point_constructorMID, (jint)x, (jint)y);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GComponentPeer_getPreferredSize (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	jobject size;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return NULL;
	}
	
	awt_gtk_threadsEnter();
	
	/* We have to pass NULL here to ensure that the size returned is the one calculated and does not
	   get clobberred with the preferred user size as set with gtk_widget_set_usize (e.g. in setBounds
	   and may be other places in Gtk code.). */
	
	gtk_widget_size_request (data->widget, NULL);
	size = (*env)->NewObject (env,
							  GCachedIDs.java_awt_DimensionClass,
							  GCachedIDs.java_awt_Dimension_constructorMID,
							  data->widget->requisition.width,
							  data->widget->requisition.height);
	awt_gtk_threadsLeave();

	return size;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_setBoundsNative (JNIEnv *env, jobject this, jint x, jint y, jint width, jint height)
{
	GComponentPeerData *data = (GComponentPeerData *)(*env)->GetIntField (env, this, GCachedIDs.GComponentPeer_dataFID);
	GtkAllocation allocation;
	
	if (data == NULL)
		return;
		
	allocation.x = x;
	allocation.y = y;
	allocation.width = (width < 0) ? 0 : width;
	allocation.height = (height < 0) ? 0 : height;
	
	awt_gtk_threadsEnter();

	/* Although we don't use the bounds here for anything as our custom panel uses the allocation
	   as set above, the GtkScrolledWindow does use this for the size of the component to be scrolled. */

	gtk_widget_size_allocate (data->widget, &allocation);

	gtk_widget_set_usize (data->widget, allocation.width, allocation.height);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_show (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_widget_show (data->widget);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_hide (JNIEnv *env, jobject this)
{
	GComponentPeerData *data;
	
	awt_gtk_threadsEnter();
	
	data = awt_gtk_getData (env, this);
	
	if (data != NULL)
		gtk_widget_hide (data->widget);
		
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_setEnabled (JNIEnv *env, jobject this, jboolean enabled)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	awt_gtk_threadsEnter();
	gtk_widget_set_sensitive (data->widget, (gboolean)enabled);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_updateWidgetStyle (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	updateWidgetStyle (env, data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_setCursorNative (JNIEnv *env, jobject this, jobject cursor)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	updateWidgetCursor (env, data);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_requestFocus (JNIEnv *env, jobject this)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
		
	awt_gtk_threadsEnter();
        
        /* if its a container type widget, like checkBox, call */
        /* gtk_container_focus                                 */
        if (GTK_IS_CONTAINER (data->widget))
        {
            gtk_container_focus (GTK_CONTAINER (data->widget), GTK_DIR_TAB_FORWARD);
        }
        else
	    gtk_widget_grab_focus (data->widget);
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_setNativeEvent (JNIEnv *env, jobject this, jobject event, jint nativeEvent)
{
	(*env)->SetIntField (env, event, GCachedIDs.java_awt_AWTEvent_dataFID, (jint)nativeEvent);
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_postMouseEventToGtk (JNIEnv *env, jobject this, jobject event)
{
	GdkEvent *gdkevent = (GdkEvent *)(*env)->GetIntField (env, event, GCachedIDs.java_awt_AWTEvent_dataFID);
	
	if (gdkevent != NULL)
	{
		awt_gtk_threadsEnter();
		
		/* Mark event so that we know this event has already been processed by Java and we shouldn't
		   post the event back to Java. We do this by setting the high bit of the send_event field
		   as this is only a boolean and the upper bits are not used by Gtk. This bit will be cleared
		   by our GDK event processing function before it is passed on to Gtk to ensure it is not
		   modified. See event handling code in GToolkit.c (handleEvent). */
		
		gdkevent->any.send_event |= (1 << 7);
		gdk_event_put (gdkevent);
		
		/* If this was a double or triple click then post another event to Gtk to indicate this.
		   This is necessary because we handle our own click counting system because Java requires
		   more than triple clicks (which is the maximum Gtk handles). Gtk has special event types
		   for these kinds of events and requires extra events to be posted. We emulate this here to
		   ensure than native widgets etc still handle double and triple clicks. */
		
		if (gdkevent->any.type == GDK_BUTTON_PRESS)
		{
			jint clickCount = (*env)->GetIntField (env, event, GCachedIDs.java_awt_event_MouseEvent_clickCountFID);
		
			if (clickCount == 2)
			{
				gdkevent->any.type = GDK_2BUTTON_PRESS;
				gdk_event_put (gdkevent);
			}
			
			else if (clickCount == 3)
			{
				gdkevent->any.type = GDK_3BUTTON_PRESS;
				gdk_event_put (gdkevent);
			}
		}
		
		/* gdk_event_put copies the event so we need to free this one. */
		
		gdk_event_free (gdkevent);
		(*env)->SetIntField (env, event, GCachedIDs.java_awt_AWTEvent_dataFID, (jint)NULL);
		
		awt_gtk_threadsLeave();
	}
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GComponentPeer_postKeyEventToGtk (JNIEnv *env, jobject this, jobject event)
{
	GdkEvent *gdkevent = (GdkEvent *)(*env)->GetIntField (env, event, GCachedIDs.java_awt_AWTEvent_dataFID);
	
	if (gdkevent != NULL)
	{
		jint modifiedFields;
	
		awt_gtk_threadsEnter();
		
		/* Mark event so that we know this event has already been processed by Java and we shouldn't
		   post the event back to Java. We do this by setting the high bit of the send_event field
		   as this is only a boolean and the upper bits are not used by Gtk. This bit will be cleared
		   by our GDK event processing function before it is passed on to Gtk to ensure it is not
		   modified. See event handling code in GToolkit.c (handleEvent). */
		
		gdkevent->any.send_event |= (1 << 7);
		
		/* Check if the event has been modified in Java. If so then we need to modify the Gdk event
		   as well. Most key events won't be modified so checking the modified field imroves performance
		   and also reduces problems that could arrise when converting from Java key events to native ones. */
		   
		modifiedFields = (*env)->GetIntField (env, event, GCachedIDs.java_awt_event_KeyEvent_modifiedFieldsFID);
				
		/* Modify GDK key code if the Java key code has been changed. */
		
		if (modifiedFields & java_awt_event_KeyEvent_KEY_CODE_MODIFIED_MASK)
		{
			jint keyCode = (*env)->GetIntField (env, event, GCachedIDs.java_awt_event_KeyEvent_keyCodeFID);
			gdkevent->key.keyval = awt_gtk_getGdkKeyCode (keyCode);
		}
			
		/* Modify GDK string to represent the keyChar of the event if it has been modified. */
		
		if (modifiedFields & java_awt_event_KeyEvent_KEY_CHAR_MODIFIED_MASK)
		{
			jchar keyChar = (*env)->GetCharField (env, event, GCachedIDs.java_awt_event_KeyEvent_keyCharFID);
			
			if (keyChar == java_awt_event_KeyEvent_CHAR_UNDEFINED)
			{
				if (gdkevent->key.string != NULL)
					g_free (gdkevent->key.string);
					
				gdkevent->key.length = 0;
				gdkevent->key.string = NULL;
			}
			
			else
			{
				/* Get keyChar as a sequence of bytes in UTF-8 encoding. */
				
				if (keyChar < 127) /* Unicode and UTF-8 are the same in this range. */
				{
					if (gdkevent->key.string != NULL)
						g_free (gdkevent->key.string);
						
					gdkevent->key.length = 1;
					gdkevent->key.string = g_strndup ((const gchar *)&keyChar, 1);
				}
				
				else	/* Call into Java to do conversion to UTF-8 */
				{
					jbyteArray byteArray = (*env)->CallObjectMethod (env, this, GCachedIDs.GComponentPeer_getUTF8BytesMID, keyChar);
					
					if ((*env)->ExceptionCheck (env))
						(*env)->ExceptionDescribe (env);
						
					else
					{
						jsize length = (*env)->GetArrayLength (env, byteArray);
						jbyte *bytes = (*env)->GetByteArrayElements (env, byteArray, NULL);
						
						if (bytes == NULL)
							(*env)->ExceptionDescribe (env);
							
						else
						{
							if (gdkevent->key.string != NULL)
								g_free (gdkevent->key.string);
								
							gdkevent->key.length = length;
							gdkevent->key.string = g_strndup((const gchar *)bytes, length);
							
							(*env)->ReleaseByteArrayElements (env, byteArray, bytes, JNI_ABORT);
						}
					}
				}
			}
		}
		
		if (modifiedFields & java_awt_event_KeyEvent_MODIFIERS_MODIFIED_MASK)
		{
			jint modifiers = (*env)->GetIntField (env, event, GCachedIDs.java_awt_event_InputEvent_modifiersFID);
			
			gdkevent->key.state = 0;
			
			if (modifiers & java_awt_event_InputEvent_SHIFT_MASK)
				gdkevent->key.state |= GDK_SHIFT_MASK;
			
			if (modifiers & java_awt_event_InputEvent_CTRL_MASK)
				gdkevent->key.state |= GDK_CONTROL_MASK;
				
			if (modifiers & java_awt_event_InputEvent_ALT_MASK)
				gdkevent->key.state |= GDK_MOD1_MASK;
				
			if (modifiers & java_awt_event_InputEvent_META_MASK)
				gdkevent->key.state |= GDK_MOD2_MASK;
				
			if (modifiers & java_awt_event_InputEvent_BUTTON1_MASK)
				gdkevent->key.state |= GDK_BUTTON1_MASK;
				
			if (modifiers & java_awt_event_InputEvent_BUTTON2_MASK)
				gdkevent->key.state |= GDK_BUTTON2_MASK;
				
			if (modifiers & java_awt_event_InputEvent_BUTTON3_MASK)
				gdkevent->key.state |= GDK_BUTTON3_MASK;
		}
		
		gdk_event_put (gdkevent);
		
		/* gdk_event_put copies the event so we need to free this one. */
		
		gdk_event_free (gdkevent);
		(*env)->SetIntField (env, event, GCachedIDs.java_awt_AWTEvent_dataFID, (jint)NULL);
		
		awt_gtk_threadsLeave();
	}
}
