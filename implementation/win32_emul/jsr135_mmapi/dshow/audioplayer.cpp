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

#include "filter_in.hpp"
#include "audioplayer.hpp"

extern "C" { FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]}; }

namespace On2FlvSDK
{
    HRESULT FlvSplitCreateInstance(IUnknown *, const IID &, void **);
    HRESULT FlvDecVP6CreateInstance(IUnknown *, const IID &, void **);
}

audioplayer::audioplayer()
{
    pfi  = NULL;
    pgb  = NULL;
    pmc  = NULL;
}

audioplayer::~audioplayer()
{
    shutdown();
}

bool audioplayer::init(unsigned int len,const wchar_t*format)
{
    if(NULL != pfi) return false;

    HRESULT hr = S_OK;

    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if( FAILED( hr ) )
    {
        return false;
    }

    if(!filter_in::create(L"", &pfi))
    {
        pfi = NULL;
        return false;
    }

    IPin* pp;

    hr = pfi->FindPin(L"Output", &pp);
    if(hr != S_OK)
    {
        pfi->Release(); pfi = NULL;
        return false;
    }

    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder, (void**)&pgb);
    if(hr != S_OK)
    {
        pp->Release(); pfi->Release();
        pfi = NULL;
        return false;
    }

    hr = pgb->AddFilter(pfi, L"Source Filter");
    if(hr != S_OK)
    {
        pp->Release();
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    hr = pgb->Render(pp);
    pp->Release();

    if(hr != S_OK)
    {
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    hr = pgb->QueryInterface(IID_IMediaControl, (void**)&pmc);
    if(hr != S_OK)
    {
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    hr = pgb->QueryInterface(IID_IMediaSeeking, (void**)&pms);
    if( FAILED( hr ) )
    {
        pmc->Release(); pmc = NULL;
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    hr = pmc->Pause();
    if( FAILED( hr ) )
    {
        pms->Release(); pms = NULL;
        pmc->Release(); pmc = NULL;
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    hr = pms->SetTimeFormat( &TIME_FORMAT_MEDIA_TIME );
    if( FAILED( hr ) )
    {
        pms->Release(); pms = NULL;
        pmc->Release(); pmc = NULL;
        pfi->Release(); pfi = NULL;
        pgb->Release(); pgb = NULL;
        return false;
    }

    IBaseFilter *pbf_flv_split;
    IBaseFilter *pbf_flv_dec;
    hr = On2FlvSDK::FlvSplitCreateInstance(NULL, IID_IBaseFilter, (void **)&pbf_flv_split);
    hr = On2FlvSDK::FlvDecVP6CreateInstance(NULL, IID_IBaseFilter, (void **)&pbf_flv_dec);

    return true;
}

bool audioplayer::data(unsigned int len,const void*src)
{
    if(NULL == pfi) return false;
    return pfi->data(len, (UINT8*)src);
}

bool audioplayer::play()
{
    if(NULL == pmc) return false;
    HRESULT hr = pmc->Run();
    return SUCCEEDED(hr);
}

bool audioplayer::stop()
{
    if(NULL == pmc) return false;
    HRESULT hr = pmc->Pause();
    return SUCCEEDED(hr);
}

bool audioplayer::seek(double time)
{
    return false;
}

bool audioplayer::tell(double*time)
{
    if(NULL == pgb) return false;

    /*
    IMediaPosition* pmp = NULL;

    HRESULT hr = pgb->QueryInterface(IID_IMediaPosition, (void**)&pmp);
    if(NULL == pmp) return false;

    hr = pmp->get_CurrentPosition(time);
    pmp->Release();
    return SUCCEEDED(hr);
    */

    HRESULT hr = pgb->QueryInterface(IID_IMediaSeeking, (void**)&pms);
    if(NULL == pms) return false;

    LONGLONG cur;
    hr = pms->GetCurrentPosition( &cur );
    if( SUCCEEDED( hr ) )
    {
        *time = double(cur) / 1E7;
        return true;
    }
    else
    {
        *time = -1;
        return false;
    }
}

bool audioplayer::shutdown()
{
    if(NULL != pms)
    {
        pms->Release();
        pms = NULL;
    }

    if(NULL != pmc)
    {
        pmc->Release();
        pmc = NULL;
    }

    if(NULL != pgb)
    {
        pgb->Release();
        pgb = NULL;
    }

    if(NULL != pfi)
    {
        pfi->Release();
        pfi = NULL;
    }

    return true;
}
