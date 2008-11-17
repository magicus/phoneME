/*
 * @(#)net_md.cpp	1.10 06/10/10
 *
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
 *
 */

extern "C" {
#include "javavm/include/porting/net.h"
#include "javavm/include/porting/ansi/stdlib.h"
}
#include <e32std.h>
#include <f32file.h>
#include <c32comm.h> // for StartC32
#include <in_sock.h> // replaces tcpip.h
#include <es_sock.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <utf.h>
#include "memory_impl.h"

static int init_done = 0;
static RCriticalSection cs;
RSocketServ sserv;
void SYMBIANdumpProtocols(RSocketServ &sserv);
TInt SYMBIANresolveInit();

/*
 **********************************************************************
 * Ensure that the Comms environment has been set up - this is normally
 * the responsibility of the system Shell, but this kind of test program
 * doesn't have such a thing.
 ***********************************************************************
 */
#if 0
static TInt CommInit()
{
  // 1. Make sure the FileServer is alive


  RFs fs;
  TInt err=fs.Connect();
  fs.Close();

    // 2. Load the appropriate serial device drivers

#if defined (__WINS__)
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM") // alternatively "FCOMM"
#endif

  err=User::LoadPhysicalDevice(PDD_NAME);
  if (err!=KErrNone && err!=KErrAlreadyExists) {
    return(err);
  }
  err=User::LoadLogicalDevice(LDD_NAME);
  if (err!=KErrNone && err!=KErrAlreadyExists) {
    return(err);
  }

  // 3. Start the Comms Server

  err = StartC32();
  if (err!=KErrNone && err!=KErrAlreadyExists) {
    return(err);
  }

  return (KErrNone);
}
#endif

int
SYMBIANsocketServerInit()
{
    TInt err;
    if (!init_done) {
	cs.Wait();
	if (!init_done) {
#if 0
	    StartC32();
#else
#if 0
            err = CommInit();
            if (err != KErrNone) {
                return 0;
            }
#endif
	    err = sserv.Connect();
	    if (err != KErrNone) {
		fprintf(stderr, "RServerSock.Connect failed with %d\n",
		    err);
		return 0;
	    }
#ifdef EKA2
	    if (sserv.ShareAuto() != KErrNone) {
#else
	    if (sserv.Share(RSessionBase::EAutoAttach) != KErrNone) {
#endif
		return 0;
	    }
#endif

#ifdef CVM_DEBUG
SYMBIANdumpProtocols(sserv);
#endif
	    err = SYMBIANresolveInit();
	    if (err != KErrNone) {
		fprintf(stderr, "SYMBIANresolveInit() failed with %d\n",
		    err);
		return 0;
	    }
	    init_done = 1;
	}
	cs.Signal();
    }
    return 1;
}

int
SYMBIANnetInit()
{
    if (cs.CreateLocal() != KErrNone) {
	return 0;
    }
    return 1;
}

void
SYMBIANnetDestroy()
{
    cs.Close();
}

void
SYMBIANdumpProtocols(RSocketServ &sserv)
{
    TUint nproto;
    TInt err = sserv.NumProtocols(nproto);
    if (err != KErrNone) {
	return;
    }
    TInt i;
    for (i = 1; i <= nproto; ++i) {
	TProtocolDesc desc;
	err = sserv.GetProtocolInfo(i, desc);
	if (err == KErrNone) {
#ifdef _UNICODE
	    TUint8 n[0x20];
	    TPtr8 name8(n, sizeof n);
	    TInt err = CnvUtfConverter::ConvertFromUnicodeToUtf8(name8,
		desc.iName);
	    fprintf(stderr, "Protocol %d (%s) fam %x proto %x stype %x"
		" ns %x\n", i,
		name8.PtrZ(),
		desc.iAddrFamily,
		desc.iProtocol,
		desc.iSockType,
		desc.iNamingServices);
#else
	    fprintf(stderr, "Protocol %d (%s) fam %x proto %x stype %x"
		" ns %x\n", i,
		desc.iName.PtrZ(),
		desc.iAddrFamily,
		desc.iProtocol,
		desc.iSockType,
		desc.iNamingServices);
#endif
	}
    }
}
