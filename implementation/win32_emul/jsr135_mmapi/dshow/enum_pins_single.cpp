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


class enum_pins_single : public IEnumPins
{
    nat32 reference_count;
    IPin *ppin;
    nat32 index;

    enum_pins_single();
    ~enum_pins_single();
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IEnumPins
    virtual HRESULT __stdcall Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cPins);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumPins **ppEnum);

    friend bool create_enum_pins_single(IPin *ppin, IEnumPins **ppenum);
};


enum_pins_single::enum_pins_single()
{
#if write_level > 2
    print("enum_pins_single::enum_pins_single called...\n");
#endif
}

enum_pins_single::~enum_pins_single()
{
#if write_level > 2
    print("enum_pins_single::~enum_pins_single called...\n");
#endif
}

HRESULT __stdcall enum_pins_single::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
    print("enum_pins_single::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumPins)
    {
        *(IEnumPins **)ppvObject = this;
        ((IEnumPins *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall enum_pins_single::AddRef()
{
#if write_level > 2
    print("enum_pins_single::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall enum_pins_single::Release()
{
#if write_level > 2
    print("enum_pins_single::Release called...\n");
#endif
    if(reference_count == 1)
    {
        ppin->Release();
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall enum_pins_single::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
#if write_level > 1
    print("enum_pins_single::Next called...\n");
#endif
    if(!ppPins) return E_POINTER;
    if(cPins != 1 && !pcFetched) return E_INVALIDARG;
    nat32 i;
    for(i = 0; i < cPins; i++)
    {
        if(index >= 1)
        {
            if(pcFetched) *pcFetched = i;
            return S_FALSE;
        }
        ppPins[i] = ppin;
        ppPins[i]->AddRef();
        index++;
    }
    if(pcFetched) *pcFetched = i;
    return S_OK;
}

HRESULT __stdcall enum_pins_single::Skip(ULONG cPins)
{
#if write_level > 1
    print("enum_pins_single::Skip called...\n");
#endif
    if(index + cPins > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cPins;
    return S_OK;
}

HRESULT __stdcall enum_pins_single::Reset()
{
#if write_level > 1
    print("enum_pins_single::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall enum_pins_single::Clone(IEnumPins **ppEnum)
{
#if write_level > 1
    print("enum_pins_single::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    enum_pins_single *penum_pins = new enum_pins_single;
    if(!penum_pins) return E_OUTOFMEMORY;
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = index;
    *ppEnum = penum_pins;
    return S_OK;
}

bool create_enum_pins_single(IPin *ppin, IEnumPins **ppenum)
{
#if write_level > 1
    print("create_enum_pins_single called...\n");
#endif
    if(!ppin || !ppenum) return false;
    enum_pins_single *penum_pins = new enum_pins_single;
    if(!penum_pins) return false;
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = 0;
    *ppenum = penum_pins;
    return true;
}
