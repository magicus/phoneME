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

#pragma once

#include "types.hpp"

class player_callback
{
public:
    virtual ~player_callback() = 0 {}
    virtual void frame_ready(const bits16 *pframe) = 0;
    virtual void sample_ready(nat32 len, const void *pdata) = 0;
    virtual void size_changed(int16 w, int16 h) = 0;
    virtual void audio_format_changed(nat32 samples_per_second, nat32 channels, nat32 bits_per_sample) = 0;
    virtual void playback_finished() = 0;
};
