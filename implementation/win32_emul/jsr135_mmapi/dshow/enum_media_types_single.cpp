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

#include <strmif.h>
#include "types.hpp"

#define write_level 0

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class enum_media_types_single : public IEnumMediaTypes
{
    nat32 reference_count;
    AM_MEDIA_TYPE amt;
    nat32 index;

    enum_media_types_single();
    ~enum_media_types_single();
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IEnumMediaTypes
    virtual HRESULT __stdcall Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cMediaTypes);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumMediaTypes **ppEnum);

    friend bool create_enum_media_types_single(const AM_MEDIA_TYPE *pamt, IEnumMediaTypes **ppenum);
};


enum_media_types_single::enum_media_types_single()
{
#if write_level > 2
    print("enum_media_types_single::enum_media_types_single called...\n");
#endif
}

enum_media_types_single::~enum_media_types_single()
{
#if write_level > 2
    print("enum_media_types_single::~enum_media_types_single called...\n");
#endif
}

HRESULT __stdcall enum_media_types_single::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
    print("enum_media_types_single::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumMediaTypes)
    {
        *(IEnumMediaTypes **)ppvObject = this;
        ((IEnumMediaTypes *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall enum_media_types_single::AddRef()
{
#if write_level > 2
    print("enum_media_types_single::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall enum_media_types_single::Release()
{
#if write_level > 2
    print("enum_media_types_single::Release called...\n");
#endif
    if(reference_count == 1)
    {
        if(amt.pUnk) amt.pUnk->Release();
        if(amt.cbFormat) delete[] (bits8 *)amt.pbFormat;
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall enum_media_types_single::Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
#if write_level > 1
    print("enum_media_types_single::Next called...\n");
#endif
    if(!ppMediaTypes) return E_POINTER;
    if(cMediaTypes != 1 && !pcFetched) return E_INVALIDARG;
    nat32 i;
    for(i = 0; i < cMediaTypes; i++)
    {
        if(index >= 1)
        {
            if(pcFetched) *pcFetched = i;
            return S_FALSE;
        }
        AM_MEDIA_TYPE *pamt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if(!pamt)
        {
            if(pcFetched) *pcFetched = i;
            return E_OUTOFMEMORY;
        }
        if(amt.cbFormat)
        {
            pamt->pbFormat = (bits8 *)CoTaskMemAlloc(amt.cbFormat);
            if(!pamt->pbFormat)
            {
                CoTaskMemFree(pamt);
                if(pcFetched) *pcFetched = i;
                return E_OUTOFMEMORY;
            }
            memcpy(pamt->pbFormat, amt.pbFormat, amt.cbFormat);
        }
        else pamt->pbFormat = null;
        pamt->majortype = amt.majortype;
        pamt->subtype = amt.subtype;
        pamt->bFixedSizeSamples = amt.bFixedSizeSamples;
        pamt->bTemporalCompression = amt.bTemporalCompression;
        pamt->lSampleSize = amt.lSampleSize;
        pamt->formattype = amt.formattype;
        pamt->pUnk = amt.pUnk;
        if(pamt->pUnk) pamt->pUnk->AddRef();
        pamt->cbFormat = amt.cbFormat;
        ppMediaTypes[i] = pamt;
        index++;
    }
    if(pcFetched) *pcFetched = i;
    return S_OK;
}

HRESULT __stdcall enum_media_types_single::Skip(ULONG cMediaTypes)
{
#if write_level > 1
    print("enum_media_types_single::Skip called...\n");
#endif
    if(index + cMediaTypes > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cMediaTypes;
    return S_OK;
}

HRESULT __stdcall enum_media_types_single::Reset()
{
#if write_level > 1
    print("enum_media_types_single::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall enum_media_types_single::Clone(IEnumMediaTypes **ppEnum)
{
#if write_level > 1
    print("enum_media_types_single::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    enum_media_types_single *penum_media_types = new enum_media_types_single;
    if(!penum_media_types) return E_OUTOFMEMORY;
    if(amt.cbFormat)
    {
        penum_media_types->amt.pbFormat = new bits8[amt.cbFormat];
        if(!penum_media_types->amt.pbFormat)
        {
            delete penum_media_types;
            return E_OUTOFMEMORY;
        }
        memcpy(penum_media_types->amt.pbFormat, amt.pbFormat, amt.cbFormat);
    }
    else penum_media_types->amt.pbFormat = null;
    penum_media_types->reference_count = 1;
    penum_media_types->amt.majortype = amt.majortype;
    penum_media_types->amt.subtype = amt.subtype;
    penum_media_types->amt.bFixedSizeSamples = amt.bFixedSizeSamples;
    penum_media_types->amt.bTemporalCompression = amt.bTemporalCompression;
    penum_media_types->amt.lSampleSize = amt.lSampleSize;
    penum_media_types->amt.formattype = amt.formattype;
    penum_media_types->amt.pUnk = amt.pUnk;
    if(penum_media_types->amt.pUnk) penum_media_types->amt.pUnk->AddRef();
    penum_media_types->amt.cbFormat = amt.cbFormat;
    penum_media_types->index = index;
    *ppEnum = penum_media_types;
    return S_OK;
}

bool create_enum_media_types_single(const AM_MEDIA_TYPE *pamt, IEnumMediaTypes **ppenum)
{
#if write_level > 1
    print("create_enum_media_types_single called...\n");
#endif
    if(!pamt || !ppenum) return false;
    enum_media_types_single *penum_media_types = new enum_media_types_single;
    if(!penum_media_types) return false;
    if(pamt->cbFormat)
    {
        penum_media_types->amt.pbFormat = new bits8[pamt->cbFormat];
        if(!penum_media_types->amt.pbFormat)
        {
            delete penum_media_types;
            return false;
        }
        memcpy(penum_media_types->amt.pbFormat, pamt->pbFormat, pamt->cbFormat);
    }
    else penum_media_types->amt.pbFormat = null;
    penum_media_types->reference_count = 1;
    penum_media_types->amt.majortype = pamt->majortype;
    penum_media_types->amt.subtype = pamt->subtype;
    penum_media_types->amt.bFixedSizeSamples = pamt->bFixedSizeSamples;
    penum_media_types->amt.bTemporalCompression = pamt->bTemporalCompression;
    penum_media_types->amt.lSampleSize = pamt->lSampleSize;
    penum_media_types->amt.formattype = pamt->formattype;
    penum_media_types->amt.pUnk = pamt->pUnk;
    if(penum_media_types->amt.pUnk) penum_media_types->amt.pUnk->AddRef();
    penum_media_types->amt.cbFormat = pamt->cbFormat;
    penum_media_types->index = 0;
    *ppenum = penum_media_types;
    return true;
}
