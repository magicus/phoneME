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
 * @brief ImageItem-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_imageitem.h>
#include <fcntl.h>

#include "midpMalloc.h"
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


static gboolean
lfpport_imageitem_expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    guchar *pixels;
    int rowstride;
    int width, height;
    GdkPixbuf *gdkPixBuf;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    if (!GTK_IS_DRAWING_AREA(widget)) {
        LIMO_TRACE("%s unexpectedly invoked on not drawing area\n", __FUNCTION__);
        return FALSE;
    }
    gdkPixBuf = gtk_object_get_user_data(widget);

    if (gdkPixBuf != NULL) {
        rowstride = gdk_pixbuf_get_rowstride (gdkPixBuf);
        pixels = gdk_pixbuf_get_pixels(gdkPixBuf);

        width = gdk_pixbuf_get_width(gdkPixBuf);
        height = gdk_pixbuf_get_height(gdkPixBuf);

        LIMO_TRACE("%s drawing: x=%d y=%d width=%d height=%d xdith=%d ydith=%d \n", __FUNCTION__,
                   event->area.x, event->area.y, width, height,
                   event->area.x, event->area.x);

        gdk_draw_rgb_image_dithalign(widget->window,    /* drawable */
                                    widget->style->black_gc, /*gc*/
                                    event->area.x, event->area.y,   /*top left x, y*/
                                    width, height, /*size of rectangle to be drawn */
                                    GDK_RGB_DITHER_NORMAL,
                                    pixels, rowstride,
                                    0, 0);/*offset for dither*/
        LIMO_TRACE("%s drew image\n", __FUNCTION__);
    }
    else {
        LIMO_TRACE("%s null data\n", __FUNCTION__);
    }

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return TRUE;
}

MidpError lfpport_image_item_show_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_show_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}
MidpError lfpport_image_item_hide_cb(MidpItem* itemPtr){
    GtkWidget *widget = (GtkWidget*)itemPtr->widgetPtr;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    gtk_widget_hide_all(widget);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_set_label_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}
MidpError lfpport_image_item_destroy_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_get_min_height_cb(int *height, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_MIN_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_get_min_width_cb(int *width, MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_MIN_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_get_pref_height_cb(int* height,
                                                 MidpItem* itemPtr,
                                                 int lockedWidth){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *height = STUB_PREF_HEIGHT;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_get_pref_width_cb(int* width,
                                                MidpItem* itemPtr,
                                                int lockedHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *width = STUB_PREF_WIDTH;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError lfpport_image_item_handle_event_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_image_item_relocate_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

MidpError lfpport_image_item_resize_cb(MidpItem* itemPtr){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

/**
 * Creates an image item's native peer, but does not display it.
 * When this function returns successfully, the fields in *itemPtr should
 * be set.
 *
 * @param itemPtr pointer to the image item's MidpItem structure.
 * @param ownerPtr pointer to the item's owner(form)'s MidpDisplayable
 *                 structure.
 * @param label the item label.
 * @param layout the item layout directive.
 * @param imgPtr pointer to the image used to create this item.
 * @param altText alternative text to be displayed to the user in case of
 *                an image not being displayed.
 * @param appearanceMode appearance mode for the image
 *                       item (eg. PLAIN/HYPERLINK).
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_imageitem_create(MidpItem* itemPtr,
				   MidpDisplayable* ownerPtr,
				   const pcsl_string* label, int layout,
				   unsigned char* imgPtr,
				   const pcsl_string* altText, int appearanceMode){
    GtkWidget *da;
    GtkWidget *gtk_pix_map;
    GtkWidget *image_item_label;
    GtkWidget *form;
    GtkWidget *vbox;
    GtkWidget *box;
    GtkFrame *frame;
    GdkPixbuf *gdkPixBuf;
    gchar label_buf[MAX_TEXT_LENGTH];
    int label_len;
    int tfd;
    gint width, height;
    GError *error = NULL;

    LIMO_TRACE(">>>%s imgPtr=%x\n", __FUNCTION__, imgPtr);
    if (imgPtr == NULL) {
        LIMO_TRACE(">>>%s received null gdkPixBuf\n", __FUNCTION__);
    }
    gdkPixBuf = (GdkPixbuf*)imgPtr;
    pcsl_string_convert_to_utf8(label, label_buf, MAX_TEXT_LENGTH, &label_len);
    frame = gtk_frame_new(NULL);
    if (label_len > 0) {
        gtk_frame_set_label(frame, label_buf);
    }
    da = gtk_drawing_area_new();

    width = gdk_pixbuf_get_width(gdkPixBuf);
    height = gdk_pixbuf_get_height(gdkPixBuf);
    /* set drawing area size requrest */
    gtk_widget_set_size_request(da, width, height);
    gtk_widget_show(da);

    /* TODO:  save appearanceMode */
    gtk_container_add(GTK_CONTAINER (frame), da);
    gtk_object_set_user_data(da, gdkPixBuf);
    g_object_set_qdata(gdkPixBuf, PIXBUF_QUARK, da);

    g_signal_connect(G_OBJECT (da), "expose_event",
                   G_CALLBACK (lfpport_imageitem_expose_event_callback), NULL);

    /* add image item to the form */
    form = (GtkWidget*)ownerPtr->frame.widgetPtr;
    vbox = gtk_object_get_user_data(form);
    gtk_box_pack_start(GTK_BOX(vbox),
                       frame,
                       FALSE, FALSE, 0);


    /* set font */
    itemPtr->widgetPtr = frame;
    itemPtr->ownerPtr = ownerPtr;
    itemPtr->layout = layout;

    itemPtr->show = lfpport_image_item_show_cb;
    itemPtr->hide = lfpport_image_item_hide_cb;
    itemPtr->setLabel = lfpport_image_item_set_label_cb;
    itemPtr->destroy = lfpport_image_item_destroy_cb;

    itemPtr->getMinimumHeight = lfpport_image_item_get_min_height_cb;
    itemPtr->getMinimumWidth = lfpport_image_item_get_min_width_cb;
    itemPtr->getPreferredHeight = lfpport_image_item_get_pref_height_cb;
    itemPtr->getPreferredWidth = lfpport_image_item_get_pref_width_cb;
    itemPtr->handleEvent = lfpport_image_item_handle_event_cb;
    itemPtr->relocate = lfpport_image_item_relocate_cb;
    itemPtr->resize = lfpport_image_item_resize_cb;

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Notifies the native peer of a content change in the corresponding
 * image item.
 *
 * @param itemPtr pointer to the image item's MidpItem structure.
 * @param imgPtr the new image
 * @param altText the new alternative text
 * @param appearanceMode the new appearance mode
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_imageitem_set_content(MidpItem* itemPtr,
					unsigned char* imgPtr,
					const pcsl_string* altText,
					int appearanceMode){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return -1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

