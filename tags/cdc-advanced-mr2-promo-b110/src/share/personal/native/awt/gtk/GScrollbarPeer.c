/*
 * @(#)GScrollbarPeer.c	1.11 06/10/10
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

#include "jni.h"
#include "java_awt_Scrollbar.h"
#include "java_awt_event_AdjustmentEvent.h"
#include "sun_awt_gtk_GScrollbarPeer.h"
#include "GScrollbarPeer.h"

static void
valueChanged (GtkAdjustment *adjustment, GScrollbarPeerData *data)
{
	jobject this = ((GComponentPeerData *)data)->peerGlobalRef;
	JNIEnv *env;
	jint oldValue;
        jint value = (jint)adjustment->value;
	jobject target;
	
	if ((*JVM)->AttachCurrentThread (JVM, (void **) &env, NULL) != 0)
		return;
	
	target = (*env)->GetObjectField (env, this, GCachedIDs.GComponentPeer_targetFID);
	
	/* Because Gtk uses float values and Java uses integer values we need to check if the
	   integer value has actually changed before we post an event to the event queue. */
	   
	oldValue = (*env)->GetIntField (env, target, GCachedIDs.java_awt_Scrollbar_valueFID);
	
	if (oldValue != value)
	{
                jint moveType;
		jint moveValue = value - oldValue;
                jboolean increment = JNI_TRUE;

                if(moveValue < 0) {
                    moveValue *= -1;
                    increment = JNI_FALSE;
                }

                if(moveValue == (jint)adjustment->page_increment) // modified for bug #4724630 
                    moveType = increment == JNI_TRUE ? 
                                  java_awt_event_AdjustmentEvent_BLOCK_INCREMENT : 
                                  java_awt_event_AdjustmentEvent_BLOCK_DECREMENT;
                else if(moveValue == (jint)adjustment->step_increment) // modified for bug #4724630 
                    moveType = increment == JNI_TRUE ? 
                                  java_awt_event_AdjustmentEvent_UNIT_INCREMENT : 
                                  java_awt_event_AdjustmentEvent_UNIT_DECREMENT;
                else
                    moveType = java_awt_event_AdjustmentEvent_TRACK;

		(*env)->SetIntField (env, target, GCachedIDs.java_awt_Scrollbar_valueFID, value);
		
		awt_gtk_callbackEnter();

		(*env)->CallVoidMethod (env, this, GCachedIDs.GScrollbarPeer_postAdjustmentEvent, moveType, value);
		
		if ((*env)->ExceptionCheck (env))
			(*env)->ExceptionDescribe (env);
		
		awt_gtk_callbackLeave();
	}
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollbarPeer_initIDs (JNIEnv *env, jclass cls)
{
	GET_METHOD_ID (GScrollbarPeer_postAdjustmentEvent, "postAdjustmentEvent", "(II)V");
	
	cls = (*env)->FindClass (env, "java/awt/Scrollbar");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_Scrollbar_valueFID, "value", "I");
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollbarPeer_createScrollbar (JNIEnv *env, jobject this, jint orientation)
{
	GScrollbarPeerData *data = (GScrollbarPeerData *)calloc (1, sizeof (GScrollbarPeerData));
	GtkWidget *scrollbar;
	GtkAdjustment *adjustment;
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	
	if (orientation == java_awt_Scrollbar_HORIZONTAL)
		scrollbar = gtk_hscrollbar_new (NULL);
		
	else
		scrollbar = gtk_vscrollbar_new (NULL);
		
	if (scrollbar == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		awt_gtk_threadsLeave();
		return;
	}
	
	adjustment = gtk_range_get_adjustment (GTK_RANGE(scrollbar));
	
	/* Attatch a signal handler to this adjustment. */
	
	gtk_signal_connect (GTK_OBJECT(adjustment), "value-changed", GTK_SIGNAL_FUNC (valueChanged), data);
	
	gtk_widget_show (scrollbar);
		
	awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, scrollbar, scrollbar, scrollbar, JNI_FALSE);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollbarPeer_setValues (JNIEnv *env, jobject this,
						jint value,
						jint visible,
						jint minimum,
						jint maximum)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GtkAdjustment *adjustment;
	
	if (data == NULL)
		return;

	awt_gtk_threadsEnter();
	
	adjustment = gtk_range_get_adjustment (GTK_RANGE(data->widget));
	adjustment->lower = minimum;
	adjustment->upper = maximum;
	adjustment->value = value;
	adjustment->page_size = visible;
	gtk_adjustment_changed (adjustment);
	gtk_adjustment_value_changed (adjustment);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollbarPeer_setLineIncrement (JNIEnv *env, jobject this, jint increment)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GtkAdjustment *adjustment;
	
	if (data == NULL)
		return;

	awt_gtk_threadsEnter();
	
	adjustment = gtk_range_get_adjustment (GTK_RANGE(data->widget));
	adjustment->step_increment = increment;
	gtk_adjustment_changed (adjustment);
	
	awt_gtk_threadsLeave();
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GScrollbarPeer_setPageIncrement (JNIEnv *env, jobject this, jint increment)
{
	GComponentPeerData *data = awt_gtk_getData (env, this);
	GtkAdjustment *adjustment;
	
	if (data == NULL)
		return;

	awt_gtk_threadsEnter();
	
	adjustment = gtk_range_get_adjustment (GTK_RANGE(data->widget));
	adjustment->page_increment = increment;
	gtk_adjustment_changed (adjustment);
	
	awt_gtk_threadsLeave();
}
