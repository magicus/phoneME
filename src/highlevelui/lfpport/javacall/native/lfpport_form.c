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
 * @brief Form-specific porting functions and data structures.
 */

#include <lfpport_displayable.h>
#include <lfpport_item.h>
#include <lfpport_form.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "lfpport_gtk.h"

#ifdef __cplusplus
extern "C" {
#endif


GtkWidget *main_window = NULL;
GtkVBox *main_container = NULL;
GdkScreen *gdk_screen = NULL;
GtkLabel  *ticker = NULL;
GtkLabel  *statusbar = NULL;
GtkWidget *main_canvas = NULL;
GdkPixmap *pixmap = NULL;   /* Backing pixmap for drawing area */


gint screen_width;
gint screen_height;
gint display_width;
gint display_height;

#define MAX_TICKER_LENGTH   128


MidpError form_show(MidpFrame* framePtr) {
    GtkWidget *form, *tmp;
    gint retVal;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    form = (GtkWidget *)framePtr->widgetPtr;
    pthread_mutex_lock(&mutex);
    gtk_widget_show_all(form);
    tmp = gtk_main_window_get_current_form(main_window);
    retVal = gtk_main_window_set_current_form(main_window, GTK_FORM(form));
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("%<<<s old %x, new: %x, retval:%d\n", __FUNCTION__, tmp, form, retVal);
    return KNI_OK;
}


MidpError form_hide_and_delete(MidpFrame* framePtr, jboolean onExit) {
    GtkForm *currentForm, *form;
    gint ret;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    form = framePtr->widgetPtr;
    pthread_mutex_lock(&mutex);
    form = framePtr->widgetPtr;

    gtk_widget_hide_all(form);
    ret = gtk_main_window_remove_form(main_window, form);
    LIMO_TRACE("%s removed form=%x ret=%d\n",
               __FUNCTION__, form, ret);

    gtk_widget_destroy(form);
    pthread_mutex_unlock(&mutex);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError displayable_set_title(MidpDisplayable* screenPtr,
                                const pcsl_string* title) {
    gchar buf[MAX_TITLE_LENGTH];
    int len;

    GtkWidget *form;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pcsl_string_convert_to_utf8(title, buf, MAX_TITLE_LENGTH, &len);
    buf[len] = 0;
    form = screenPtr->frame.widgetPtr;
    LIMO_TRACE("%s form=%x, title=%s\n", __FUNCTION__, form, buf);
    pthread_mutex_lock(&mutex);
    gtk_form_set_title(GTK_FORM(form), buf);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

MidpError displayable_set_ticker(MidpDisplayable* dispPtr, const pcsl_string* text) {
    gchar buf[MAX_TICKER_LENGTH];
    int len;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    //pcsl_string_convert_to_utf8(text, buf, MAX_TITLE_LENGTH, &len);
    //gtk_label_set_text(ticker, "tmp ticker");

//     gtk_table_attach(GTK_TABLE(main_container),
//             ticker, /* ticker */
//             /* X direction */       /* Y direction */
//             0, 1,                   1, 2,
//             GTK_EXPAND | GTK_FILL,  GTK_EXPAND | GTK_FILL,
//             0,                      0);

    //gtk_widget_show(ticker);

    //show label
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Creates the form's native peer (the container window for a form), but
 * does not display it.
 *
 * The MIDP implementation should call this function in the background and
 * display the form after it is given all of its content. When this
 * function returns successfully, the fields in *formPtr will be set.
 *
 * @param formPtr pointer to the form's MidpDisplayable structure.
 * @param title title of the form.
 * @param tickerText text to be displayed in the ticker.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_create(MidpDisplayable* dispPtr,
			      const pcsl_string* title, const pcsl_string* tickerText){
    GtkVBox  *gtkVBox;  /* form contents */
    GtkWidget *form;
    GtkWidget *vbox;
    GtkWidget *sw;

    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    pthread_mutex_lock(&mutex);
    form = gtk_form_new(TRUE);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw),
                                      GTK_POLICY_NEVER,
                                      GTK_POLICY_AUTOMATIC);
    gtkVBox = gtk_vbox_new(FALSE, /* not equal space allotments */
                           2);    /* spacing between children */

    gtk_scrolled_window_add_with_viewport(GTK_CONTAINER(sw), gtkVBox);
    gtk_container_add(GTK_CONTAINER(form), sw);

    gtk_object_set_user_data(form, gtkVBox);

    gtk_widget_show_all(sw);
    // Fill in MidpDisplayable structure
    dispPtr->frame.widgetPtr = form;
    LIMO_TRACE("%s form=%x\n", __FUNCTION__, form);
    dispPtr->frame.show = form_show;
    dispPtr->frame.hideAndDelete = form_hide_and_delete;
    dispPtr->frame.handleEvent	 = NULL;
    dispPtr->setTitle = displayable_set_title;
    dispPtr->setTicker = displayable_set_ticker;


    gtk_main_window_add_form(main_window, GTK_FORM(form));
    pthread_mutex_unlock(&mutex);

    dispPtr->setTitle(dispPtr, title);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Sets the virtual size of the form. The size assumes an infinitely
 * large screen, without scrolling.
 *
 * @param formPtr pointer to the form's MidpDisplayable structure.
 * @param w width of the form.
 * @param h height of the form.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_set_content_size(MidpDisplayable* formPtr,
					int w, int h){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    pthread_mutex_lock(&mutex);
    gtk_widget_set_size_request(formPtr->frame.widgetPtr,
                                w,
                                h);
    pthread_mutex_unlock(&mutex);
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Sets the form's current item (the item that has focus) to the
 * given item and makes the form visible.
 *
 * @param itemPtr pointer to the item to be made current.
 * @param yOffset offset relative to the item's y co-ordinate.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_set_current_item(MidpDisplayable* formPtr,
                                        MidpItem* itemPtr, int yOffset){
    GtkWidget *form;
    GtkWidget *box;
    (void)itemPtr;
    (void)yOffset;


    LIMO_TRACE(">>>%s formPtr=%x\n", __FUNCTION__, formPtr);

    pthread_mutex_lock(&mutex);
    gtk_widget_grab_focus((GtkWidget*)itemPtr->widgetPtr);
    pthread_mutex_unlock(&mutex);

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Gets the native peer's current "y" scroll position.
 *
 * @param pos pointer that will be to the user's current "y" position on
 *        the form. This function sets pos's value.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_get_scroll_position(int *pos){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *pos = 0;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Sets the native peer's current "y" scroll position.
 *
 * @param pos new current "y" position on the form.
 *
 */
MidpError lfpport_form_set_scroll_position(int pos) {
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    (void)pos;
    LIMO_TRACE("<<<%s\n", __FUNCTION__);
    return KNI_OK;
}

/**
 * Gets the native peer's current viewport height.
 *
 * @param height pointer that will be pointing to the current viewport height
 *        in the form. This function sets height's value.
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_form_get_viewport_height(int *pHeight){
    LIMO_TRACE(">>>%s\n", __FUNCTION__);
    *pHeight = display_height;
    LIMO_TRACE("<<<%s returning %d\n", __FUNCTION__, display_height);
    return KNI_OK;
}


#ifdef __cplusplus
} /* extern "C" */
#endif

