/*
 * @(#)apdu_asynchro.h	1.3 06/04/26 @(#)
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _APDU_ACYNCHRO_H_
#define _APDU_ACYNCHRO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <carddevice.h>

#define SIGNAL_RESET    0x7781
#define SIGNAL_XFER     0x7782
#define SIGNAL_LOCK     0x7783

#ifdef APDU_ASYNCHRO

JSR177_STATUSCODE jsr177_xfer_data_start(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void **context);
JSR177_STATUSCODE jsr177_xfer_data_finish(jbyte *tx_buffer, 
                                            jsize tx_length,
                                            jbyte *rx_buffer, jsize *rx_length,
                                            void *context);
JSR177_STATUSCODE jsr177_reset_start(jbyte *rx_buffer, jsize *rx_length,
                                            void **context);
JSR177_STATUSCODE jsr177_reset_finish(jbyte *rx_buffer, jsize *rx_length,
                                            void *context);

#else

#define jsr177_xfer_data_start(tx_b, tx_l, rx_b, rx_l, context) \
    jsr177_xfer_data((tx_b), (tx_l), (rx_b), (rx_l))

#define jsr177_xfer_data_finish(tx_b, tx_l, rx_b, rx_l, context) \
    (jsr177_set_error("apdu_asynchro: not implemented"), \
        JSR177_STATUSCODE_NOT_IMPLEMENTED)

#define jsr177_reset_start(rx_b, rx_l, context) \
    jsr177_reset((rx_b), (rx_l))

#define jsr177_reset_finish(rx_b, rx_l, context)    \
    (jsr177_set_error("apdu_asynchro: not implemented"), \
        JSR177_STATUSCODE_NOT_IMPLEMENTED)

#endif

#ifdef __cplusplus
}
#endif

#endif /* ifndef _APDU_ACYNCHRO_H_ */
