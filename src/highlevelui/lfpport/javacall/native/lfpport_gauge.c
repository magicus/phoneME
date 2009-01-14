/*
 *
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

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Gauge-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_gauge.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif

extern MidpError gchar_to_pcsl_string(gchar *src, pcsl_string *dst);



MidpError lfpport_gauge_show_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_show_all(widget);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_hide_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_hide_all(widget);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_set_label_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}


MidpError lfpport_gauge_destroy_cb(MidpItem* itemPtr){
    GtkWidget *pBar;
    GtkWidget *string_item;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    pBar = (GtkWidget*)itemPtr->widgetPtr;

    //remove item from all its containers and destroy the widget
    pthread_mutex_lock(&mutex);
    gtk_widget_destroy(pBar);
    pthread_mutex_unlock(&mutex);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


MidpError lfpport_gauge_get_min_height_cb(int *height, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_MIN_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


MidpError lfpport_gauge_get_min_width_cb(int *width, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_MIN_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_get_pref_height_cb(int* height,
                                                 MidpItem* itemPtr,
                                                 int lockedWidth){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_PREF_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_get_pref_width_cb(int* width,
                                                MidpItem* itemPtr,
                                                int lockedHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_PREF_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_handle_event_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_gauge_relocate_cb(MidpItem* itemPtr, int x, int y){
    LIMO_TRACE(">>>%s x=%d y=%d\n", __FUNCTION__, x, y);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_resize_cb(MidpItem* itemPtr, int width, int height){
    LIMO_TRACE(">>>%s width=%d height=%d\n", __FUNCTION__, width, height);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


 /**
 * Creates a gauge's native peer, but does not display it.
 * When this function returns successfully, the fields *itemPtr will be
 * set.
 *
 * @param itemPtr pointer to the gauge's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param interactive true if the gauge should be created as an
 *                    interactive type or false otherwise.
 * @param maxValue the initial maximum value of the gauge.
 * @param initialValue the initial value of the gauge.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_create(MidpItem* itemPtr, MidpDisplayable* ownerPtr,
			       const pcsl_string* label, int layout,
			       jboolean interactive,
			       int maxValue, int initialValue){
    GtkWidget *pBar;
    GtkWidget *form;
    GtkWidget *vbox;
    gchar label_buf[MAX_TEXT_LENGTH];
    gdouble fraction;
    int label_len;

    pcsl_string_convert_to_utf8(label, label_buf, MAX_TEXT_LENGTH, &label_len);

    LIMO_TRACE(">>>%s interactive=%d maxValue=%d initialValue=%d\n", __FUNCTION__);

    pthread_mutex_lock(&mutex);
    pBar = gtk_progress_bar_new();
    gtk_progress_bar_set_text(pBar, label_buf);

    if (initialValue != 0) {
        fraction = (gdouble)initialValue/(gdouble)maxValue ;
        gtk_progress_bar_set_fraction(pBar, fraction);
    }

    form = ownerPtr->frame.widgetPtr;
    vbox = gtk_object_get_user_data(form);
    gtk_box_pack_start(GTK_BOX(vbox), pBar, FALSE, FALSE, 0);

    pthread_mutex_unlock(&mutex);
    itemPtr->widgetPtr = pBar;
    itemPtr->ownerPtr = ownerPtr;
    itemPtr->layout = layout;

    itemPtr->show = lfpport_gauge_show_cb;
    itemPtr->hide = lfpport_gauge_hide_cb;
    itemPtr->setLabel = lfpport_gauge_set_label_cb;
    itemPtr->destroy = lfpport_gauge_destroy_cb;

    //itemPtr->component
    itemPtr->getMinimumHeight = lfpport_gauge_get_min_height_cb;
    itemPtr->getMinimumWidth = lfpport_gauge_get_min_width_cb;
    itemPtr->getPreferredHeight = lfpport_gauge_get_pref_height_cb;
    itemPtr->getPreferredWidth = lfpport_gauge_get_pref_width_cb;
    itemPtr->handleEvent = lfpport_gauge_handle_event_cb;
    itemPtr->relocate = lfpport_gauge_relocate_cb;
    itemPtr->resize = lfpport_gauge_resize_cb;


    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Changes the gauge to have the given current value and maximum values.
 *
 * @param itemPtr pointer to the gauge's MidpItem structure.
 * @param value the current value to be set on the gauge.
 * @param maxValue the maximum value to be set on the gauge.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_set_value(MidpItem* itemPtr, int value, int maxValue){
    GtkWidget *pBar;
    gdouble fraction;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    pBar = itemPtr->widgetPtr;
    fraction = (gdouble)value/(gdouble)maxValue ;
    pthread_mutex_lock(&mutex);
    gtk_progress_bar_set_fraction(pBar, fraction);
    pthread_mutex_unlock(&mutex);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Gets the native peer's current value for the gauge.
 *
 * @param value pointer that will be set to the gauge's current value. This
 * function sets value's value.
 * @param itemPtr pointer to the gauge's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_get_value(int* value, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

