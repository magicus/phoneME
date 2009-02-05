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

#include <kni.h>

#include <midp_logging.h>
#include <jcapp_export.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <javacall_lcd.h>
#include <string.h>
#include <javacall_time.h>
#include <javacall_lifecycle.h>
#include <kni.h>

#include <javacall_logging.h>
#include <suitestore_listeners.h>

gxj_screen_buffer gxj_system_screen_buffer;

static jboolean isLcdDirty = KNI_FALSE;
static javacall_time_milliseconds lastFlushTimeTicks = 0;
static jboolean disableRefresh=KNI_FALSE;

/**
 * @file
 * Additional porting API for Java Widgets based port of abstract
 * command manager.
 */

int jcapp_get_screen_buffer(int hardwareId) {
     javacall_lcd_color_encoding_type color_encoding;
     gxj_system_screen_buffer.alphaData = NULL;
     gxj_system_screen_buffer.pixelData = 
         javacall_lcd_get_screen (hardwareId,
                                  &gxj_system_screen_buffer.width,
                                  &gxj_system_screen_buffer.height,
                                  &color_encoding);                                
     if (JAVACALL_LCD_COLOR_RGB565 != color_encoding) {        
	    return -2;
     };

     return 0;
}


/**
 * Reset screen buffer content.
 */
void static jcapp_reset_screen_buffer(int hardwareId) {
    memset (gxj_system_screen_buffer.pixelData, 0,
        gxj_system_screen_buffer.width * gxj_system_screen_buffer.height *
            sizeof (gxj_pixel_type));
}

/**
 * Decleration of the notifier used for midlet
 * installation/uninstalltion.
 * The implementation will call the appropriate javacall
 * notifiers after modifying  the parameters
 */
/**
 * Internal function to be registered with the MIDP notification
 * mechanism. Used for converting parameters between the MIDP
 * definition and the javacall listeners
 */

static void notify_javacall_installation(int listenerType, int when, MIDPError status, \
              const MidletSuiteData* pSuiteData) {

    struct javacall_lifecycle_additional_info additionalData;


    switch (listenerType) {
    case SUITESTORE_LISTENER_TYPE_INSTALL:
                additionalData.command = JAVACALL_LIFECYCLE_MIDLET_INSTALL_COMPLETED;
                additionalData.data.installation.midletName = pSuiteData->varSuiteData.suiteName.data;
                additionalData.data.installation.midletNameLen = pSuiteData->varSuiteData.suiteName.length;
                additionalData.data.installation.className = pSuiteData->varSuiteData.midletClassName.data;
                additionalData.data.installation.classNameLen = pSuiteData->varSuiteData.midletClassName.length;
                additionalData.data.installation.midletIcon = pSuiteData->varSuiteData.iconName.data;
                additionalData.data.installation.midletIconLen = pSuiteData->varSuiteData.iconName.length;
                additionalData.data.installation.suiteID = pSuiteData->suiteId;
                javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_INSTALL_COMPLETED,
                                                 status,
                                                 &additionalData);
             break;
            

    case SUITESTORE_LISTENER_TYPE_REMOVE:
                additionalData.command = JAVACALL_LIFECYCLE_MIDLET_UNINSTALL_COMPLETED;
                additionalData.data.uninstallation.suiteID = pSuiteData->suiteId;                 
                javacall_lifecycle_state_changed(JAVACALL_LIFECYCLE_MIDLET_UNINSTALL_COMPLETED,
                                                 status,
                                                 &additionalData);
                break;
    }

}


/**
 * Initializes the Javacall native resources.
 *
 * @return <tt>0</tt> upon successful initialization, or
 *         <tt>other value</tt> otherwise
 */
int jcapp_init() {
    javacall_lcd_color_encoding_type color_encoding;
    int hardwareId = jcapp_get_current_hardwareId();
 
    if (!JAVACALL_SUCCEEDED(javacall_lcd_init ()))
        return -1;        
 
    /**
     *   NOTE: Only JAVACALL_LCD_COLOR_RGB565 encoding is supported by phoneME 
     *     implementation. Other values are reserved for future  use. Returning
     *     the buffer in other encoding will result in application termination.
     */
    if (jcapp_get_screen_buffer(hardwareId) == -2) {
        REPORT_ERROR(LC_LOWUI, "Screen pixel format is the one different from RGB565!");
        return -2;
    }    

    jcapp_reset_screen_buffer(hardwareId);

    /* Initialize Install/Uninstall responders*/
    midp_suite_add_listener(notify_javacall_installation, SUITESTORE_LISTENER_TYPE_INSTALL,
                            SUITESTORE_OPERATION_END);

    midp_suite_add_listener(notify_javacall_installation, SUITESTORE_LISTENER_TYPE_REMOVE,
                            SUITESTORE_OPERATION_END);


    

    return 0;
}

/**
 * Finalize the Javacall native resources.
 */
void jcapp_finalize() {
    javacall_lcd_finalize ();
}

/**
 * Bridge function to request a repaint
 * of the area specified.
 *
 * @param hardwareId unique id of hardware display
 * @param x1 top-left x coordinate of the area to refresh
 * @param y1 top-left y coordinate of the area to refresh
 * @param x2 bottom-right x coordinate of the area to refresh
 * @param y2 bottom-right y coordinate of the area to refresh
 */
#define TRACE_LCD_REFRESH
void jcapp_refresh(int hardwareId, int x1, int y1, int x2, int y2)
{
    /*block any refresh calls in case of native master volume*/
    if(disableRefresh==KNI_TRUE){
        return;
    }

    javacall_lcd_flush_partial (hardwareId, y1, y2);
}

/**
 * Turn on or off the full screen mode
 *
 * @param hardwareId unique id of hardware display
 * @param mode true for full screen mode
 *             false for normal
 */
void jcapp_set_fullscreen_mode(int hardwareId, jboolean mode) {    

    javacall_lcd_set_full_screen_mode(hardwareId, mode);
    jcapp_get_screen_buffer(hardwareId);
    jcapp_reset_screen_buffer(hardwareId);
}

/**
 * Change screen orientation flag
 * @param hardwareId unique id of hardware display
 */
jboolean jcapp_reverse_orientation(int hardwareId) {
    jboolean res = javacall_lcd_reverse_orientation(hardwareId); 
    jcapp_get_screen_buffer(hardwareId);

    /** Whether current Displayable won't repaint the entire screen
     *  on resize event, the artefacts from the old screen content
     * can appear. That's why the buffer content is not preserved. 
     */ 

	jcapp_reset_screen_buffer(hardwareId);
	return res;
}

/**
 * Handle clamshell event
 */
void jcapp_handle_clamshell_event() {
    int hardwareId;
    
    javacall_lcd_handle_clamshell(); 
    
    hardwareId = jcapp_get_current_hardwareId();
    jcapp_get_screen_buffer(hardwareId);
    jcapp_reset_screen_buffer(hardwareId);
}
	 

/**
 * Get screen orientation flag
 * @param hardwareId unique id of hardware display
 */
jboolean jcapp_get_reverse_orientation(int hardwareId) {
    return javacall_lcd_get_reverse_orientation(hardwareId);
}

/**
 * Return screen width
 * @param hardwareId unique id of hardware display
 */
int jcapp_get_screen_width(int hardwareId) {
    return javacall_lcd_get_screen_width(hardwareId);   
}

/**
 *  Return screen height 
 * @param hardwareId unique id of hardware display
*/
int jcapp_get_screen_height(int hardwareId) {
    return javacall_lcd_get_screen_height(hardwareId);
}

/*
 * will be called from event handling loop periodically
 */
jboolean jcapp_is_native_softbutton_layer_supported() {
    return javacall_lcd_is_native_softbutton_layer_supported();
}

/**
 * Paints the Soft Buttons when using a native layer
 * acts as intermidiate layer between kni and javacall 
 * 
 * @param label Label to draw (UTF16)
 * @param len Length of the lable (0 will cause removal of current label)
 * @param index Index of the soft button in the soft button bar.
 */
 void jcapp_set_softbutton_label_on_native_layer(unsigned short *label, int len,int index) {
	javacall_lcd_set_native_softbutton_label(label, len, index);
}

/*Disables the refresh of the screen*/
void LCDUI_disable_refresh(void){
 disableRefresh=KNI_TRUE;
}

 /*Enables the refresh of the screen*/
void LCDUI_enable_refresh(void){
 disableRefresh=KNI_FALSE;
}

/**
 * get currently enabled hardware display id
 */
int jcapp_get_current_hardwareId() {
    return  javacall_lcd_get_current_hardwareId();  
}
/** 
 * Get display device name by id
 */
char* jcapp_get_display_name(int hardwareId) {
    return javacall_lcd_get_display_name(hardwareId);
}


/**
 * Check if the display device is primary
 */
jboolean jcapp_is_display_primary(int hardwareId) {
    return javacall_lcd_is_display_primary(hardwareId);
}

/**
 * Check if the display device is build-in
 */
jboolean jcapp_is_display_buildin(int hardwareId) {
    return javacall_lcd_is_display_buildin(hardwareId);
}

/**
 * Check if the display device supports pointer events
 */
jboolean jcapp_is_display_pen_supported(int hardwareId) {
    return javacall_lcd_is_display_pen_supported(hardwareId);
}

/**
 * Check if the display device supports pointer motion  events
 */
jboolean jcapp_is_display_pen_motion_supported(int hardwareId){
    return javacall_lcd_is_display_pen_motion_supported(hardwareId);
}

/**
 * Get display device capabilities
 */
int jcapp_get_display_capabilities(int hardwareId) {
  return javacall_lcd_get_display_capabilities(hardwareId);
}

jint* jcapp_get_display_device_ids(jint* n) {
    return javacall_lcd_get_display_device_ids(n);
}

void jcapp_display_device_state_changed(int hardwareId, int state) {
    (void)hardwareId;
    (void)state;
}
