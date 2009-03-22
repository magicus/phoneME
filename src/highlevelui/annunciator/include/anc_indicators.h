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

#ifndef _ANC_INDICATORS_H_
#define _ANC_INDICATORS_H_

#include <kni.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup highui High Level UI
 * @ingroup subsystems
 */

/**
 * @defgroup highui_anc Annunciator Porting interface
 * @ingroup highui
 */

/**
 * @file
 * @ingroup highui_anc
 *
 * @brief Interface for drawing indicators, such as trusted, networking,
 * backlight, and so on
 */


/**
 * Platform should draw a trusted indicator (e.g. icon)
 * @param trusted whether to draw a trusted or non-trusted
 * indicator
 */ 
void anc_show_trusted_indicator(jboolean trusted);


/**
 * Operations with backlight.
 * The function anc_show_backlight() requires one of these values.
 */
typedef enum {
    ANC_BACKLIGHT_OFF           = 0, /**< Turns off backlight          */
    ANC_BACKLIGHT_ON            = 1, /**< Turns on backlight           */
    ANC_BACKLIGHT_TOGGLE        = 2, /**< Toggles the current state    */
    ANC_BACKLIGHT_SUPPORTED     = 3  /**< Tests if backlight supported */
} AncBacklightState;

/**
 * Control the device's backlight.  Turn it on or off, 
 * toggle it, or check to see if control of the backlight
 * is supported by the system without changing the light's 
 * state.
 *
 * @param mode One of the constants from AncBacklightState enumeration. 
 *             ANC_BACKLIGHT_ON to turn on the backlight, 
 *             ANC_BACKLIGHT_OFF to turn off the backlight,
 *             ANC_BACKLIGHT_TOGGLE to toggle the backlight, and
 *             ANC_BACKLIGHT_SUPPORTED to see if the system
 *             supports this function without changing the
 *             state of the backlight.  <code>mode</code>
 *             values not listed above are ignored.
 * @return KNI_TRUE if the device supports backlight
 *         control or KNI_FALSE otherwise
 */
jboolean anc_show_backlight(AncBacklightState mode);

/**
 * @name Network Indicator Support
 * The network indicators are meant to show networking activity on the
 * mobile device. Please refer to the function read0 (fully qualified
 * name Java_com_sun_midp_io_j2me_socket_Protocol_read0()) in socketProtocol.c
 * for usage of these macros.
 *
 * When an application performs read operation over the socket, it results
 * in calling read0() two times if the function is about to block for read.
 * In the first invocation, START_NETWORK_INDICATOR is called and
 * INC_NETWORK_INDICATOR is called just before read_start().  If the
 * read_start() returns PCSL_NET_WOULDBLOCK as status code,
 * the function read0() returns after blocking the calling Java thread.
 * It calls STOP_NETWORK_INDICATOR just before returning from read0().
 *
 * In the second invocation when the data becomes available, read0() calls
 * START_NETWORK_INDICATOR again and does read_finish(). Note that network
 * indicator count is already incremented in first invocation, so no need
 * to do it again. Now, the read_finish() normally returns with number
 * of bytes read and read operation is successful. It calls
 * DEC_NETWORK_INDICATOR which decrements the network indicator count.
 * In the end, before returning from the function read0(), it calls
 * STOP_NETWORK_INDICATOR again.
 *
 * So, the network indicators are used to show networking activities
 * to the user. It's really not related number of pending operations
 * or number of sockets used or even number of bytes read. When the
 * network indicator count is greater that 0 and network indicator
 * is enabled (by calling START_NETWORK_INDICATOR), the user should
 * see a network indicator icon on the screen.
 *
 * The network indicator support is enabled by compiling with
 * ENABLE_NETWORK_INDICATOR set to true.
 * @{
*/
#if ENABLE_NETWORK_INDICATOR
/**
 * Porting layer of setting the networking indicator status.
 *
 * @param status What the status of the indicator should be.
 */
void anc_set_network_indicator(int counter);

extern int gAncNetworkIndicatorCount;

#define ANC_INC_NETWORK_INDICATOR     { gAncNetworkIndicatorCount++; };
#define ANC_DEC_NETWORK_INDICATOR     { if (gAncNetworkIndicatorCount > 0) \
                                            gAncNetworkIndicatorCount--; \
                                        else  { REPORT_WARN(LC_PROTOCOL, \
                                   "Network connection is closed without opening"); ((void)0); } };

/** deploy the network indicator count */
#define ANC_IND_NETWORK_INDICATOR    {  anc_set_network_indicator( \
                                          gAncNetworkIndicatorCount); };

#else

/** increment the network indicator count */
#define ANC_INC_NETWORK_INDICATOR
/** decrement the network indicator count */
#define ANC_DEC_NETWORK_INDICATOR
/** deploy the network indicator count */
#define ANC_IND_NETWORK_INDICATOR
#endif /* ENABLE_NETWORK_INDICATOR */
/** @} */

/**
 * Switch home icon on or off.
 * Home icon is to prompt user to switch back to AMS home screen to see some
 * information, like other MIDlets that request foreground.
 *
 * @param isHomeOn true if home icon should be turned on
 */
void anc_toggle_home_icon(jboolean isHomeOn);

#ifdef __cplusplus
}
#endif

#endif /* _ANC_INDICATORS_H_ */
