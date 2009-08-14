/*
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

#ifndef __PCM_OUT_H
#define __PCM_OUT_H

#ifdef __cplusplus
extern "C" {
#endif

#define     PCM_MAX_CHANNELS    128
//#define     PCM_VIA_DSOUND // uncomment to use DirectSound-based
                             // implementation instead of winapi-based

typedef struct tag_pcm_channel * pcm_handle_t; /* channel handle */

/* Callback function will be called by pcm mixing thread
   to get channel data. Return number of bytes
   actually written to buf (<= size).
*/
typedef size_t (*get_ch_data)( void* buf, size_t size, void* param );

pcm_handle_t pcm_out_open_channel( int          bits,
                                   int          nch,
                                   long         rate,
                                   long         blk_size,
                                   get_ch_data  gd_callback,
                                   void*        cb_param );


void         pcm_out_close_channel( pcm_handle_t hch );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __PCM_OUT_H */
