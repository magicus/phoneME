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



MidpError lfpport_gauge_show_cb(MidpItem* gaugePtr){
    GtkWidget *widget = (GtkWidget*)gaugePtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_show_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_hide_cb(MidpItem* gaugePtr){
    GtkWidget *widget = (GtkWidget*)gaugePtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_hide_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_set_label_cb(MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}


MidpError lfpport_gauge_destroy_cb(MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}


MidpError lfpport_gauge_get_min_height_cb(int *height, MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_MIN_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


MidpError lfpport_gauge_get_min_width_cb(int *width, MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_MIN_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_get_pref_height_cb(int* height,
                                                 MidpItem* gaugePtr,
                                                 int lockedWidth){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_PREF_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_get_pref_width_cb(int* width,
                                                MidpItem* gaugePtr,
                                                int lockedHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_PREF_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_handle_event_cb(MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_gauge_relocate_cb(MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_gauge_resize_cb(MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


 /**
 * Creates a gauge's native peer, but does not display it.
 * When this function returns successfully, the fields *gaugePtr will be
 * set.
 *
 * @param gaugePtr pointer to the gauge's MidpItem structure.
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
MidpError lfpport_gauge_create(MidpItem* gaugePtr, MidpDisplayable* ownerPtr,
			       const pcsl_string* label, int layout,
			       jboolean interactive,
			       int maxValue, int initialValue){
    GtkWidget *pBar;
    gchar label_buf[MAX_TEXT_LENGTH];
    gdouble fraction;
    int label_len;

    pcsl_string_convert_to_utf8(label, label_buf, MAX_TEXT_LENGTH, &label_len);

    LIMO_TRACE(">>>%s interactive=%d maxValue=%d initialValue=%d\n", __FUNCTION__);

    pBar = gtk_progress_bar_new();
    gtk_progress_bar_set_text(pBar, label_buf);

    if (initialValue != 0) {
        fraction = (gdouble)initialValue/(gdouble)maxValue ;
        gtk_progress_bar_set_fraction(pBar, fraction);
    }
    gtk_widget_show(pBar);

    gaugePtr->widgetPtr = pBar;
    gaugePtr->ownerPtr = ownerPtr;
    gaugePtr->layout = layout;

    gaugePtr->show = lfpport_gauge_show_cb;
    gaugePtr->hide = lfpport_gauge_hide_cb;
    gaugePtr->setLabel = lfpport_gauge_set_label_cb;
    gaugePtr->destroy = lfpport_gauge_destroy_cb;

    //gaugePtr->component
    gaugePtr->getMinimumHeight = lfpport_gauge_get_min_height_cb;
    gaugePtr->getMinimumWidth = lfpport_gauge_get_min_width_cb;
    gaugePtr->getPreferredHeight = lfpport_gauge_get_pref_height_cb;
    gaugePtr->getPreferredWidth = lfpport_gauge_get_pref_width_cb;
    gaugePtr->handleEvent = lfpport_gauge_handle_event_cb;
    gaugePtr->relocate = lfpport_gauge_relocate_cb;
    gaugePtr->resize = lfpport_gauge_resize_cb;


    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Changes the gauge to have the given current value and maximum values.
 *
 * @param gaugePtr pointer to the gauge's MidpItem structure.
 * @param value the current value to be set on the gauge.
 * @param maxValue the maximum value to be set on the gauge.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_set_value(MidpItem* gaugePtr, int value, int maxValue){
    GtkWidget *pBar;
    gdouble fraction;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    pBar = gaugePtr->widgetPtr;
    fraction = (gdouble)value/(gdouble)maxValue ;
    gtk_progress_bar_set_fraction(pBar, fraction);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Gets the native peer's current value for the gauge.
 *
 * @param value pointer that will be set to the gauge's current value. This
 * function sets value's value.
 * @param gaugePtr pointer to the gauge's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_gauge_get_value(int* value, MidpItem* gaugePtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

