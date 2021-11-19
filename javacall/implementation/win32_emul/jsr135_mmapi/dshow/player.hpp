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
        result_security         = 4,
    };

    static const int32 unrealized = 100;
    static const int32 realized   = 200;
    static const int32 prefetched = 300;
    static const int32 started    = 400;
    static const int32 closed     =   0;
    static const int64 time_unknown = -1;

    virtual ~player() = 0 {}
    virtual result realize() = 0;
    virtual result prefetch() = 0;
    virtual result start() = 0;
    virtual result stop() = 0;
    virtual result deallocate() = 0;
    virtual void close() = 0;
    //virtual result set_time_base(time_base *master) = 0;
    //virtual time_base *get_time_base(result *presult = 0) = 0;
    virtual int64 set_media_time(int64 now, result *presult = 0) = 0;
    virtual int64 get_media_time(result *presult = 0) = 0;
    virtual int32 get_state() = 0;
    virtual int64 get_duration(result *presult = 0) = 0;
    //virtual string16c get_content_type(result *presult = 0) = 0;
    virtual result set_loop_count(int32 count) = 0;
    //virtual result add_player_listener(player_listener *pplayer_listener) = 0;
    //virtual result remove_player_listener(player_listener *pplayer_listener) = 0;

    virtual bool data(nat32 len, const void *pdata) = 0;
};
