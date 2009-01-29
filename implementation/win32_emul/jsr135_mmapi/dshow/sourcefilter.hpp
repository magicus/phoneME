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

#include <streams.h>

class  sourcefilter;
struct ap_callback;

class sourcefilterpin : public CSourceStream
{
    friend sourcefilter;

    FILE*         f;
    UINT32        len;
    UINT32        offs;
    UINT8*        data;
    bool          end;
    sourcefilter* psf;
    ap_callback*  pcb;

    int           rate;
    int           channels;

    sourcefilterpin(sourcefilter* pms, HRESULT* phr, ap_callback* cb);
    ~sourcefilterpin();

    virtual HRESULT DecideBufferSize(IMemAllocator* pAlloc, ALLOCATOR_PROPERTIES* ppropInputRequest);
    virtual HRESULT FillBuffer(IMediaSample* pSample);
    virtual HRESULT GetMediaType(CMediaType* pMediaType);
};

class sourcefilter : public CSource
{
    sourcefilterpin* psfp;

public:
    sourcefilter(LPUNKNOWN lpunk, HRESULT* phr, ap_callback* cb);
    bool data(UINT32 len, const UINT8* data);
    bool end();
};
