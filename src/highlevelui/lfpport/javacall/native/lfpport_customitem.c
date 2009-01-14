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
 * @brief CustomItem-specific porting functions and data structures.
 */

/**
 * Porting interface of CustomItem.
 */
#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_customitem.h>
#include "lfpport_gtk.h"
#include "midpMalloc.h"

#ifdef __cplusplus
extern "C" {
#endif


extern GtkWidget *main_window;

static gboolean
lfpport_customitem_expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    GdkPixmap *gdk_pix_map;
    int width, height;
    LIMO_TRACE(">>>%s widget=%x area.x=%d area.y=%d area.width=%d area.height=%d\n",
               __FUNCTION__, widget, event->area.x, event->area.y, event->area.width, event->area.height);

    if (!GTK_IS_DRAWING_AREA(widget)) {
        LIMO_TRACE("%s unexpectedly invoked on not drawing area\n", __FUNCTION__);
        return FALSE;
    }
    gdk_pix_map = gtk_object_get_user_data(widget);
    if (!GDK_IS_PIXMAP(gdk_pix_map)) {
        LIMO_TRACE("%s gdk_pix_map expected\n", __FUNCTION__);
        return KNI_ERR;
    }

    gdk_drawable_get_size(gdk_pix_map, &width, &height);
    LIMO_TRACE("%s gdk_pix_map=%x width=%x height=%x\n",
               __FUNCTION__, gdk_pix_map, width, height);

    gdk_draw_drawable(widget->window,
                     widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                     gdk_pix_map,
                     event->area.x,
                     event->area.y,
                     event->area.x,
                     event->area.y,
                     width,    /* copy the entire image */
                     height);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return TRUE;
}

MidpError lfpport_custom_item_show_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_show_all(widget);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_hide_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_hide_all(widget);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_set_label_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}
MidpError lfpport_custom_item_destroy_cb(MidpItem* itemPtr){
    GtkFrame *frame = (GtkWidget*)itemPtr->widgetPtr;
    GtkWidget *widget;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    widget = gtk_bin_get_child(frame);
    if (NULL == widget) {
        LIMO_TRACE(">>>%s non-null widget expected as frame child\n", __FUNCTION__);
        return KNI_OK;  /* don't do anything */
    }

    pthread_mutex_lock(&mutex);
    gtk_widget_destroy(widget);
    gtk_widget_destroy(frame);

    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_get_min_height_cb(int *height, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_MIN_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_get_min_width_cb(int *width, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_MIN_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_get_pref_height_cb(int* height,
                                                 MidpItem* itemPtr,
                                                 int lockedWidth){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_PREF_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_get_pref_width_cb(int* width,
                                                MidpItem* itemPtr,
                                                int lockedHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_PREF_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_handle_event_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_custom_item_relocate_cb(MidpItem* itemPtr, int x, int y){
    LIMO_TRACE(">>>%s x=%d y=%d\n", __FUNCTION__, x, y);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_custom_item_resize_cb(MidpItem* itemPtr, int width, int height){
    LIMO_TRACE(">>>%s width=%d height=%d\n", __FUNCTION__, width, height);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}


/**
 * Creates a custom item's native peer, but does not display it.
 * When this function returns successfully, *itemPtr will be filled.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_create(MidpItem* itemPtr,
				    MidpDisplayable* ownerPtr,
				    const pcsl_string* label, int layout) {
    GtkWidget *da;
    GtkWidget *custom_item_label;
    GtkWidget *form;
    GtkWidget *vbox;
    GtkWidget *box;
    GtkFrame *frame;
    gchar label_buf[MAX_TEXT_LENGTH];
    int label_len;
    int tfd;
    GError *error = NULL;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    pcsl_string_convert_to_utf8(label, label_buf, MAX_TEXT_LENGTH, &label_len);

    pthread_mutex_lock(&mutex);
    frame = gtk_frame_new(NULL);
    if (label_len > 0) {
        gtk_frame_set_label(frame, label_buf);
    }
    da = gtk_drawing_area_new();
    gtk_widget_show(da);

    LIMO_TRACE("%s frame=%x da=%x\n", __FUNCTION__, frame, da);
    gtk_container_add(GTK_CONTAINER (frame), da);

    /* IMPL_NOTE:  for some reason lfpport_custom_item_show_cb is not called
      Until the reason is investigated, showing frame always*/
    g_signal_connect(G_OBJECT (da), "expose_event",
                   G_CALLBACK (lfpport_customitem_expose_event_callback), NULL);

    /* add image item to the form */
    form = (GtkWidget*)ownerPtr->frame.widgetPtr;
    vbox = gtk_object_get_user_data(form);
    gtk_box_pack_start(GTK_BOX(vbox),
                       frame,
                       FALSE, FALSE, 0);
    pthread_mutex_unlock(&mutex);

    /* set font */
    itemPtr->widgetPtr = frame;
    itemPtr->ownerPtr = ownerPtr;
    itemPtr->layout = layout;

    itemPtr->show = lfpport_custom_item_show_cb;
    itemPtr->hide = lfpport_custom_item_hide_cb;
    itemPtr->setLabel = lfpport_custom_item_set_label_cb;
    itemPtr->destroy = lfpport_custom_item_destroy_cb;

    itemPtr->getMinimumHeight = lfpport_custom_item_get_min_height_cb;
    itemPtr->getMinimumWidth = lfpport_custom_item_get_min_width_cb;
    itemPtr->getPreferredHeight = lfpport_custom_item_get_pref_height_cb;
    itemPtr->getPreferredWidth = lfpport_custom_item_get_pref_width_cb;
    itemPtr->handleEvent = lfpport_custom_item_handle_event_cb;
    itemPtr->relocate = lfpport_custom_item_relocate_cb;
    itemPtr->resize = lfpport_custom_item_resize_cb;

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Causes an immediate bitblt on the specified rectangle.
 * The given x and y  values are relative to the custom item's
 * content's co-ordinate system.
 *
 * @param itemPtr pointer to the custom item's MidpItem structure.
 * @param x x-coordinate of the refresh area.
 * @param y y-coordinate of the refresh area.
 * @param width width of area to be refreshed.
 * @param height height of area to be refreshed.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_refresh(MidpItem* itemPtr,
				     int x, int y,
				     int width, int height){
    GtkWidget *frame;
    GtkWidget *da;
    GdkRectangle rect;

    LIMO_TRACE(">>>%s x=%d y=%d width=%d height=%d\n",
               __FUNCTION__, x, y, width, height);

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    frame = (GtkWidget *)itemPtr->widgetPtr;
    pthread_mutex_lock(&mutex);
    da = gtk_bin_get_child(frame);
    if (!GTK_IS_DRAWING_AREA(da)) {
        LIMO_TRACE("%s unexpectedly invoked on not drawing area\n", __FUNCTION__);
        return KNI_ERR;
    }

    gdk_window_invalidate_rect(da->window, &rect, FALSE);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Gets the width of the custom item's native label.
 *
 * @param widthRet the calculated label width, based on tentative
 *                 width passed in.
 * @param width the tentative width for the label.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
 MidpError lfpport_customitem_get_label_width(int *widthRet,
					     int width,
					     MidpItem* ciPtr){
    GtkRequisition r;
    GtkWidget *widget = (GtkWidget*)ciPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_size_request(widget, &r);
    pthread_mutex_unlock(&mutex);
    *widthRet = r.width;
    LIMO_TRACE("<<<%s width=%d\n", __FUNCTION__, r.width);
    return KNI_OK;
}

/**
 * Gets the height of the custom item's native label.
 *
 * @param width the tentative width for the label.
 * @param heightRet the calculated label height, based on tentative
 *                  width passed in.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_get_label_height(int width,
					      int *heightRet,
					      MidpItem* ciPtr){
    GtkRequisition r;
    GtkWidget *widget = (GtkWidget*)ciPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_size_request(widget, &r);
    pthread_mutex_unlock(&mutex);
    *heightRet = r.height;
    LIMO_TRACE("<<<%s width=%d\n", __FUNCTION__, r.width);
    return KNI_OK;
}

/**
 * Gets the padding for the custom item.
 *
 * @param pad the padding for the custom item.
 * @param ciPtr pointer to the custom item's MidpItem structure.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_customitem_get_item_pad(int *pad, MidpItem* ciPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *pad = ITEM_BOUND_PAD;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Sets the content buffer. All paints are done to that buffer.
 * When paint is processed snapshot of the buffer is flushed to
 * the native resource content area.
 *
 * @param ciPtr pointer to the custom item's MidpItem structure.
 * @param imgPtr pointer to the native resource corresponding
 *        to the Java platform offscreen buffer for CustomItem content
 */
MidpError lfpport_customitem_set_content_buffer(MidpItem* ciPtr,
						unsigned char* imgPtr){
    GtkWidget *frame;
    GtkWidget *da;
    GdkPixmap* pixmap;
    int width, height;

    LIMO_TRACE(">>>%s imgPtr=%x\n", __FUNCTION__, imgPtr);

    if (imgPtr == NULL) {
        LIMO_TRACE("%s null imgPtr\n", __FUNCTION__);
    }
    else if (!GDK_IS_PIXMAP(imgPtr)) {
        LIMO_TRACE("%s GdkPixmap expected!\n", __FUNCTION__);
        return KNI_ERR;
    }
    
    frame = (GtkWidget *)ciPtr->widgetPtr;
    pthread_mutex_lock(&mutex);
    da = gtk_bin_get_child(frame);
    if (!GTK_IS_DRAWING_AREA(da)) {
        LIMO_TRACE("%s unexpectedly invoked on not drawing area\n", __FUNCTION__);
        pthread_mutex_unlock(&mutex);
        return KNI_ERR;
    }
    gtk_object_set_user_data(da, imgPtr);
    LIMO_TRACE("%s da=%x imgPtr=%x\n", __FUNCTION__, da, imgPtr);

    if (imgPtr) {
        gdk_drawable_get_size((GdkPixmap*)imgPtr, &width, &height);
        LIMO_TRACE("%s width=%d height=%d da=%x\n", __FUNCTION__, width, height, da);
        gtk_widget_set_size_request(da, width, height);
    }
    else {
        gtk_widget_set_size_request(da, 0, 0);
    }
    pthread_mutex_unlock(&mutex);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

