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

#ifndef WMAINTERFACE_H
#define WMAINTERFACE_H

#include <jsr120_types.h>
#ifdef ENABLE_MIDP
#include <midpServices.h>
#else
typedef enum midp_SignalType {
    WMA_SMS_READ_SIGNAL,
    WMA_SMS_WRITE_SIGNAL,
    WMA_CBS_READ_SIGNAL,
    PUSH_SIGNAL,
    /* Note: There is no WRITE signal for CBS, a receive-only protocol */
    WMA_MMS_READ_SIGNAL,
    WMA_MMS_WRITE_SIGNAL
} midpSignalType;
#endif


/**
 * @file
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open datagram ports for receiving/sending wma messages
 *
 * @return  <code>WMA_NET_SUCCESS</code> for success;
 *          <code>WMA_NET_IOERROR</code> for failure.
 */
WMA_STATUS init_jsr120();

/**
 * Close datagram ports for a protocol.
 */
void finalize_jsr120();

/**
 * Checks if given signal is related to operation performed by WMA
 * and if so performs required processing.
 *
 * @param signalType the signal type occurred
 * @param fd the platform-specific file descriptor the event is signalled on
 *
 * @return KNI_TRUE if given fd is owned by WMA, KNI_FALSE otherwise
 */
jboolean jsr120_check_signal(midpSignalType signalType, int descriptor, int status);


#ifdef __cplusplus
}
#endif

#endif
