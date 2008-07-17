/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _LFPPORT_COMPONENT_H_
#define _LFPPORT_COMPONENT_H_

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Global functions and data types for the LCDUI subsystem.
 * 
 * <p>The pointer types defined in this file are for cross-platform 
 * code. The cross-platform code doesn't create or use the pointers. 
 * Instead, it stores them for and passes them back to
 * platform-dependent functions. The pointers are stored as
 * void pointers, and the functions that use them cast them back to
 * the platform widget's pointer types. This design is essential for
 * keeping the cross platform code compilable in both C and
 * C++ environments.
 *
 * <p>For example, on a Qt Embedded platform, functions that will
 * create and use a Form's native widget pointer are ported as:
 * <pre>
 * // Create pointer
 * MidpError pdMidpFormCreate(MidpDisplayable* formPtr, ...) {
 *	QWidget* myQWidget = new QWidget();
 *	formPtr->widgetPtr = (PlatformScreenWidgetPtr) myQWidget;
 *	...
 * }
 * // Use pointer
 * MidpError pdMidpFormSetItems(MidpDisplayable* formPtr, ...) {
 *	QWidget* myQWidget = (QWidget *) formPtr->widgetPtr;
 *	...
 * }
 * </pre>
 */

#include <lfpport_error.h>
#include <midpString.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 32-bit identifier for (or pointer to) a displayable's native
 * resource.  The native resource could be a platform widget used by a
 * displayable's native peer. It could also be a data structure that
 * contains information about a native resource.
 *
 * <p>PlatformScreenWidgetPtr's value will be passed as an integer id
 * to a displayable's Java platform peer.
 */
typedef void* PlatformScreenWidgetPtr;

/**
 * 32-bit identifier for (or pointer to) an item's native resource.
 * The native resource could be a platform widget used by an item's
 * native peer. It could also be a data structure that contains
 * information about a native resource.
 *
 * <p>PlatformItemWidgetPtr's value will be passed as an integer to an
 * item's Java platform peer.
 */
typedef void* PlatformItemWidgetPtr;

/**
 * 32-bit identifier for (or pointer to) a platform event
 */
typedef void* PlatformEventPtr;

/**
 * Identifiers for the different types of displayables and items in
 * LCDUI.
 *
 *  <p>Note that a text box is implemented in terms of a text field, so
 *  a text box does not have its own enum type. Similarly, a list is
 *  implemented in terms of a choice group, so the various kinds of lists
 *  do not have their own enum types.</p>
 */
typedef enum {
    /** Item types */
    MIDP_EXCLUSIVE_CHOICE_GROUP_TYPE,	/**< Java platform CG Type: 1 */
    MIDP_MULTIPLE_CHOICE_GROUP_TYPE,	/**< Java platform CG Type: 2 */
    MIDP_IMPLICIT_CHOICE_GROUP_TYPE,	/**< Java platform CG Type: 3 */
    MIDP_POPUP_CHOICE_GROUP_TYPE,	/**< Java platform CG Type: 4 */

    MIDP_CUSTOM_ITEM_TYPE,

    MIDP_DATE_FIELD_TYPE,

    MIDP_NON_INTERACTIVE_GAUGE_TYPE,
    MIDP_INTERACTIVE_GAUGE_TYPE,

    MIDP_PLAIN_IMAGE_ITEM_TYPE,	    /**< Java platform AppearanceMode: 0 */
    MIDP_HYPERLINK_IMAGE_ITEM_TYPE, /**< Java platform AppearanceMode: 1 */
    MIDP_BUTTON_IMAGE_ITEM_TYPE,    /**< Java platform AppearanceMode: 2 */

    MIDP_SPACER_TYPE,

    MIDP_PLAIN_STRING_ITEM_TYPE,    /**< Java platform AppearanceMode: 0 */
    MIDP_HYPERLINK_STRING_ITEM_TYPE,/**< Java platform AppearanceMode: 1 */
    MIDP_BUTTON_STRING_ITEM_TYPE,   /**< Java platform AppearanceMode: 2 */

    MIDP_TEXT_FIELD_TYPE,

    /** Displayable types */
    MIDP_NULL_ALERT_TYPE,	  /**< Java platform AlertType ID: 0 */
    MIDP_INFO_ALERT_TYPE,	  /**< Java platform AlertType ID: 1 */
    MIDP_WARNING_ALERT_TYPE,	  /**< Java platform AlertType ID: 2 */
    MIDP_ERROR_ALERT_TYPE,	  /**< Java platform AlertType ID: 3 */
    MIDP_ALARM_ALERT_TYPE,	  /**< Java platform AlertType ID: 4 */
    MIDP_CONFIRMATION_ALERT_TYPE, /**< Java platform AlertType ID: 5 */
	
    MIDP_CANVAS_TYPE,

    MIDP_FORM_TYPE,

    /** System dialog types */
    MIDP_MENU_TYPE

} MidpComponentType;

typedef struct _MidpComponent MidpComponent; /**< Type alias */
typedef struct _MidpFrame MidpFrame; /**< Type alias */

/**
 * Common data structure for a MIDP UI widget, like Displayable, Item, 
 * system menu or dialog.
 *
 * This data structure should only be updated by platform independent
 * layer. Platform specific porting functions should only read its values
 * without any changes.
 */
struct _MidpComponent {
    /**
     * Identifier for the MIDP UI component's type.
     */
    MidpComponentType type;

    /**
     * Version number of the data copied from this component's Java
     * platform peer.
     *
     * The Java platform peer passes a copy of its contents data
     * model to native peer when this component is shown. This
     * data copy is used to populate a platform widget and notifies
     * Java platform peer of user's new selection and changes
     * through some Java platform events. It is possible that
     * the Java platform peer data model has been changed by
     * MIDlet before such asynchronously Java platform events are
     * delivered to the Java platform peer. By versioning these
     * events with data model number, the Java platform peer can
     * decide whether the user action should be accepted or ignored.
     *
     * This model version is synchronized with the Java platform
     * peer every time the Java platform peer sets data on native peer.
     */
    int modelVersion;

    /**
     * Pointer to the next Displayable or Item component.
     * A linked list is used to keep track of all the native resources
     * created through the platform independent layer. When the 
     * Java runtime environment exits, both normally and abnormally,
     * this list will be checked to clean up any remaining native
     * resources.
     */
    MidpComponent *next;

    /**
     * Pointer to first child component structure this component
     * contains. 
     * For Displayable, child points to its first Item.
     * For Item, child should always be null.
     */
    MidpComponent *child;
};

/**
 * Common interface of a native window with a frame. Native windows
 * with frames include:
 *
 * <ul>
 *   <li>Displayable's native peer</li>
 *   <li>Native menu</li>
 *   <li>Native system dialog</li>
 * </ul>
 */
struct _MidpFrame {
    /**
     * Common data structure used by platform independent layer.
     * Platform specific porting functions should only read 
     * its values without any changes.
     */
    MidpComponent component;

    /**
     * Native resource identifier/pointer to the current Displayable.
     */
    PlatformScreenWidgetPtr widgetPtr;

    /**
     * Function pointer for showing the given screen.
     */
    MidpError (*show)(MidpFrame* screenPtr);

    /**
     * Hides and deletes the given screen.  This function also notifies
     * its items to hide.  It deletes all native resources if the given
     * boolean indicates the VM is exiting.
     *
     * @param screenPtr pointer to the screen's MidpFrame structure.
     * @param onExit true whether the VM is exiting: true if it is,
     *        and false otherwise.
     * 
     * @return an indication of success or the reason for failure
     */
    MidpError (*hideAndDelete)(MidpFrame* screenPtr, jboolean onExit);

    /**
     * Handles screen events. This function handles ticker and scroll-bar
     * events locally. It has item events (identified by the item's
     * pointer/id) forwarded to the item's handleEvent function.
     *
     * @param screenPtr pointer to the screen's MidpFrame structure.
     * @param eventPtr pointer to the screen event to be handled.
     *
     * @return true if the event has been handled and should not be further
     * dispatched, false otherwise.
     */
    jboolean (*handleEvent)(MidpFrame* screenPtr, PlatformEventPtr eventPtr);

};


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFPPORT_COMMAND_UTIL_H_ */
