/*
 * @(#)RLockingSocket.hpp	1.4 06/10/10
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
 */

class RLockingSocket : public RSocket {
private:
    TInt init_locks() {
	TInt err = rd.CreateLocal();
	if (err != KErrNone) {
	    return err;
	}
	err = wr.CreateLocal();
	if (err != KErrNone) {
	    wr.Close();
	    return err;
	}
	err = io.CreateLocal();
	if (err != KErrNone) {
	    wr.Close();
	    rd.Close();
	    return err;
	}
	select_reqs[0] = 0;
	select_reqs[1] = 0;
	select_reqs[2] = 0;
	in_wait = 0;
	in_select = 0;
	return KErrNone;
    }

    void destroy_locks() {
	io.Close();
	wr.Close();
	rd.Close();
    }

public:

    RCriticalSection &rd_lock() { return rd; }
    RCriticalSection &wr_lock() { return wr; }

    TInt Open(RSocketServ& aServer)
    {
	TInt err = init_locks();
	if (err != KErrNone) {
	    return err;
	}
	err = RSocket::Open(aServer);
	if (err != KErrNone) {
	    destroy_locks();
	}
	return err;
    }

    TInt Open(RSocketServ& aServer, TUint addrFamily, TUint sockType,
	TUint protocol)
    {
	TInt err = init_locks();
	if (err != KErrNone) {
	    return err;
	}
	err = RSocket::Open(aServer, addrFamily, sockType, protocol);
	if (err != KErrNone) {
	    destroy_locks();
	}
	return err;
    }

    TInt Open(RSocketServ& aServer,const TDesC& aName) {
	TInt err = init_locks();
	if (err != KErrNone) {
	    return err;
	}
	err = RSocket::Open(aServer, aName);
	if (err != KErrNone) {
	    destroy_locks();
	}
	return err;
    }

    TUint waitForWithSelect(TUint st) {
	TUint r = st;
	select(r);
	return r;
    }

    TUint waitFor(TUint st) {
	int done = 0;
	TUint r;
	do {
	    r = st;
	    TInt err = poll(r);
	    if (err != KErrNone) {
	    } else {
		if ((r & st) != 0) {
		    done = 1;
		}
	    }
	    if (!done) {
		sleep(500);
	    }
	} while (!done);
	return r;
    }

    void sleep(int ms) {
	TTimeIntervalMicroSeconds32 delta(1000 * ms);
	User::After(delta);
    }

    TUint waitForAny() {
	return waitFor(KSockSelectRead|KSockSelectExcept|KSockSelectWrite);
    }

    TUint waitForRead() {
	return waitFor(KSockSelectRead);
    }

    TUint waitForWrite() {
	return waitFor(KSockSelectWrite);
    }

    void select(TUint &bits0) {
	TRequestStatus rs;
	io.Wait();
	int newbits = 0;
	TUint bits = bits0;
	if ((bits & KSockSelectRead) != 0) {
	    ++select_reqs[0];
	    if (select_reqs[0] == 1) {
		newbits = 1;
	    }
	}
	if ((bits & KSockSelectWrite) != 0) {
	    ++select_reqs[1];
	    if (select_reqs[1] == 1) {
 		newbits = 1;
	    }
	}
	if ((bits & KSockSelectExcept) != 0) {
	    ++select_reqs[2];
	    if (select_reqs[2] == 1) {
		newbits = 1;
	    }
	}
	if (in_wait && newbits) {
	    CancelIoctl();
	}
	io.Signal();
	do {
	    int done = 0;
	    TUint nbits = 0;
	    io.Wait();
	    if (select_reqs[0]) {
		nbits  |= KSockSelectRead;
	    }
	    if (select_reqs[1]) {
		nbits  |= KSockSelectWrite;
	    }
	    if (select_reqs[2]) {
		nbits  |= KSockSelectExcept;
	    }

	    TPckg<TUint> bits1(nbits);
	    Ioctl(KIOctlSelect, rs, &bits1, KSOLSocket);
//	    Ioctl(KIOctlSelect, rs, (TDes8 *)&x, KSOLSocket);
	    in_wait = 1;
	    io.Signal();

	    User::WaitForRequest(rs);
	    if (rs == KErrNone) {
		assert(nbits == bits1());
		if ((nbits & bits) != 0) {
		    bits0 = nbits;
		    done = 1;
		}
	    }
	    io.Wait();
	    in_wait = 0;
	    io.Signal();
	} while (rs == KErrCancel);

	io.Wait();
	newbits = 0;
	if ((bits & KSockSelectRead) != 0) {
	    --select_reqs[0];
	    if (select_reqs[0] == 0) {
		newbits = 1;
	    }
	}
	if ((bits & KSockSelectWrite) != 0) {
	    --select_reqs[1];
	    if (select_reqs[1] == 0) {
		newbits = 1;
	    }
	}
	if ((bits & KSockSelectExcept) != 0) {
	    --select_reqs[2];
	    if (select_reqs[2] == 0) {
		newbits = 1;
	    }
	}
	if (in_wait && newbits) {
	    CancelIoctl();
	}
	io.Signal();
    }

    // Synchronous
    TInt poll(TUint &bitsRef) {
	TInt bits;
	TInt err = GetOpt(KSOSelectPoll, KSOLSocket, bits);
	if (err == KErrNone) {
	    bitsRef = (TUint)bits;
	    return err;
	}
	TInt poll_err;
	TInt err2 = GetOpt(KSOSelectLastError, KSOLSocket, poll_err);
	if (err2 == KErrNone) {
	    return poll_err;
	}
	return err;
    }

    void PreRead0() {
#if 0
	TUint bits;
	TInt err = poll(bits);
	if (err != KErrNone) {
	    return;
	}
	if ((bits & KSockSelectRead) != 0) {
	    fprintf(stderr, "read with Read status\n");
	} else {
	    fprintf(stderr, "read with status %x\n", bits);
	}
#endif
	waitForRead();
    }

    void PreWrite0() {
	while (1) {
	    TUint bits;
	    TInt err = poll(bits);
	    if (err != KErrNone) {
		return;
	    }
	    if ((bits & KSockSelectWrite) != 0) {
		break;
	    } else {
#if 0
		fprintf(stderr, "write with status %x\n", bits);
#endif
	    }
	    waitForWrite();
	}
	//CancelAll();
    }

    void PreRead() {
	TPckgBuf<TSoInetLastErr> des;
	TInt err = GetOpt(KSoInetLastError, KSolInetIp, des);
	TSoInetLastErr le = des();
	TInt status = le.iStatus;
    }

    void ignoreICMP() {
	TInt err = SetOpt(KSoUdpReceiveICMPError, KSolInetUdp, 0);
    }

    void allowReuse(TInt yes) {
	TInt err = SetOpt(KSoReuseAddr, KSolInetIp, yes);
    }

    void PreWrite() {}

    void Close() {
	rd.Close();
	wr.Close();
	RSocket::Close();
    }

private:

    RCriticalSection io;
    RCriticalSection rd;
    RCriticalSection wr;
    TUint select_reqs[3];
    int in_wait;
    int in_select;
};
