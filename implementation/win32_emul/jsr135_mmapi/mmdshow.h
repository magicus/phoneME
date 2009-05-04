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

#ifndef __MMDSHOW_H
#define __MMDSHOW_H

#include "multimedia.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

javacall_bool   is_dshow_player( javacall_handle handle );
javacall_result dshow_add_player_to_ss3d( javacall_handle handle, ISoundSource3D* ss3d );
javacall_result dshow_remove_player_from_ss3d( javacall_handle handle, ISoundSource3D* ss3d );
void            dshow_notify_ss3d_going_down( ISoundSource3D* ss3d );

javacall_result dshow_set_pan( javacall_handle handle, int* pPan );

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus

#endif // __MMDSHOW_H
