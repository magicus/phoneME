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

struct IBaseFilter;
struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;
struct IPin;
class filter_in;

class audioplayer
{
public:
    audioplayer();
    ~audioplayer();
    bool init1(nat32 len, char16 *format);
    bool init2();
    bool data(nat32 len, void const *pdata);
    bool play();
    bool stop();
    bool seek(double time);
    bool tell(double *time);
    bool shutdown();
protected:
    IGraphBuilder *pgb;
    IMediaControl *pmc;
    IMediaSeeking *pms;
    filter_in *pfi;
    IPin *pp;
    IBaseFilter *pbf_flv_split;
    IBaseFilter *pbf_flv_dec;
};
