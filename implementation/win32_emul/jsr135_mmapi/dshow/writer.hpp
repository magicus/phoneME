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

#include <strmif.h>
#include "types.hpp"

void print(char8 const *fmt, ...);
void print(char16 const *fmt, ...);
void error(HRESULT hr);
void error(char8 const *str, HRESULT hr);
void print(GUID guid);
void dump_media_type(AM_MEDIA_TYPE const *pamt);
bool dump_media_types(IPin *pp, nat32 indent);
bool dump_pin(IPin *pp, nat32 indent);
bool dump_pins(IBaseFilter *pbf, nat32 indent);
bool dump_filter(IBaseFilter *pbf, nat32 indent);
bool dump_filters(IFilterGraph *pfg, nat32 indent);
bool dump_filter_graph(IFilterGraph *pfg, nat32 indent);
