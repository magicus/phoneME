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

#include<dshow.h>
#include<wmsdkidl.h>

#include "sourcefilter.hpp"
#include "audioplayer.hpp"

audioplayer::audioplayer()
{
    sf   = NULL;
    pgb  = NULL;
    pmc  = NULL;
}

audioplayer::~audioplayer()
{
    shutdown();
}

bool audioplayer::init(unsigned int len,const wchar_t*format, ap_callback* cb)
{
    if(sf)return false;

    HRESULT hr=S_OK;

    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if( FAILED( hr ) )
    {
        return false;
    }

    sf = new sourcefilter(NULL, &hr, cb);
    if(NULL == sf) return false;
    if( FAILED( hr ) )
    {
        delete sf;
        sf = NULL;
        return false;
    }

    return true;
}

bool audioplayer::data(unsigned int len,const void*src)
{
    if(NULL == sf) return false;
    sf->data(len, (UINT8*)src);
    return true;
}

bool audioplayer::play()
{
    if(NULL == pgb)
    {
        HRESULT      hr;
        IBaseFilter* pbf;
        IPin*        pp;

        hr = sf->QueryInterface(IID_IBaseFilter, (void**)&pbf );
        if(hr != S_OK) return false;

        hr = pbf->FindPin(L"1", &pp);
        if(hr != S_OK)
        {
            pbf->Release();
            return false;
        }

        hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                              IID_IGraphBuilder, (void**)&pgb);
        if(hr != S_OK)
        {
            pp->Release();
            pbf->Release();
            return false;
        }

        hr = pgb->AddFilter(pbf, L"Source Filter");
        if(hr != S_OK)
        {
            pgb->Release();
            pgb = NULL;
            pp->Release();
            pbf->Release();
            return false;
        }

        hr = pgb->Render(pp);
        if(hr != S_OK)
        {
            pgb->Release();
            pgb = NULL;
            return false;
        }

        pp->Release();
        pbf->Release();

        hr = pgb->QueryInterface(IID_IMediaControl, (void**)&pmc);
        if(hr != S_OK)
        {
            pgb->Release();
            pgb = NULL;
            return false;
        }
    }

    HRESULT hr = pmc->Run();
    return (hr == S_OK);
}

bool audioplayer::stop()
{
    if( NULL != pmc )
    {
        HRESULT hr = pmc->Stop();
        return (S_OK == hr);
    }
    else
    {
        return false;
    }
}

bool audioplayer::seek(double time)
{
    return false;
}

bool audioplayer::tell(double*time)
{
    if( NULL != pgb )
    {
        IMediaPosition* pmp = NULL;

        HRESULT hr = pgb->QueryInterface( IID_IMediaPosition, (void**)&pmp );
        if( NULL != pmp )
        {
            hr = pmp->get_CurrentPosition( time );
            pmp->Release();
            if( SUCCEEDED( hr ) ) return true;
        }
    }
    return false;
}

bool audioplayer::shutdown()
{
    if( NULL != pmc )
    {
        pmc->Release();
        pmc = NULL;
    }

    if( NULL != pgb )
    {
        pgb->Release();
        pgb = NULL;
    }

    if( NULL != sf )
    {
        delete sf;
        sf = NULL;
    }

    return true;
}
