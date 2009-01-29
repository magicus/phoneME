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
    sf=NULL;
}

audioplayer::~audioplayer()
{
    if(sf)shutdown();
}

bool audioplayer::init(unsigned int len,const wchar_t*format, ap_callback* cb)
{
    if(sf)return false;

    HRESULT hr=S_OK;

    hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);
    if( FAILED( hr ) )
    {
        return false;
    }

    sf=new sourcefilter(NULL, &hr, cb);
    if(!sf)
    {
        return false;
    }
    if( FAILED( hr ) )
    {
        delete sf;
        return false;
    }
    IBaseFilter*pbf;
    hr=sf->QueryInterface(IID_IBaseFilter,(void**)&pbf);
    if(hr != S_OK)
    {
        delete sf;
        return false;
    }
    IPin*pp;
    hr=pbf->FindPin(L"1",&pp);
    if(hr != S_OK)
    {
        pbf->Release();
        delete sf;
        return false;
    }
    hr=CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,IID_IGraphBuilder,(void**)&pgb);
    if(hr != S_OK)
    {
        pp->Release();
        pbf->Release();
        delete sf;
        return false;
    }
    hr=pgb->AddFilter(pbf,L"Source Filter");
    if(hr != S_OK)
    {
        pgb->Release();
        pp->Release();
        pbf->Release();
        delete sf;
        return false;
    }
    hr=pgb->Render(pp);
    if(hr != S_OK)
    {
        pgb->Release();
        pp->Release();
        pbf->Release();
        delete sf;
        return false;
    }
    hr=pgb->QueryInterface(IID_IMediaControl,(void**)&pmc);
    if(hr != S_OK)
    {
        pgb->Release();
        pp->Release();
        pbf->Release();
        delete sf;
        return false;
    }
    pgb->Release();
    pp->Release();
    pbf->Release();
    return true;
}

bool audioplayer::data(unsigned int len,const void*src)
{
    sf->data(len,(UINT8*)src);
    return true;
}

bool audioplayer::play()
{
    HRESULT hr=pmc->Run();
    if(hr != S_OK)return false;
    return true;
}

bool audioplayer::stop()
{
    HRESULT hr=pmc->Stop();
    if(hr != S_OK)return false;
    return true;
}

bool audioplayer::seek(double time)
{
    return false;
}

bool audioplayer::tell(double*time)
{
    return false;
}

bool audioplayer::shutdown()
{
    if(!sf)return false;
    pmc->Release();
    pgb->Release();
    //delete sf;
    sf=NULL;
    return true;
}
