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

class player
{
public:
    enum result
    {
        result_success          = 0,
        result_illegal_argument = 1,
        result_illegal_state    = 2,
        result_media            = 3,
        result_security         = 4
    };

    enum state
    {
        stopped = 0,
        paused  = 1,
        running = 2,
    };

    static const int64 time_unknown = -1;

    virtual result get_state(state *p_state) = 0;
    virtual result stop() = 0;
    virtual result pause() = 0;
    virtual result run() = 0;
    virtual result destroy() = 0;
    virtual result get_media_time(int64 *p_time) = 0;
    virtual result set_media_time(int64 time_requested, int64 *p_time_actual) = 0;
    virtual result get_duration(int64 *p_duration) = 0;
    // virtual result set_loop_count(int32 count) = 0;

    // virtual result set_stream_length(int64 length) = 0;
};
