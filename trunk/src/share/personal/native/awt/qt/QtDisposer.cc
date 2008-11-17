/*
 * @(#)QtDisposer.cc	1.8 06/10/25
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

#include "QtDisposer.h"

#include "QtApplication.h"
#include "QtPeerBase.h"
#include "QtSync.h"
#include "awt.h"

#include <qvaluelist.h>


#include <stdio.h>
#include <assert.h>




namespace
{

typedef QValueList<QtPeerBase*> DisposeRequests;

QtReentrantMutex*   pmutex;
DisposeRequests     queue;
int                 lockCounter = 0;
bool                recursive   = false;

} // namespace




///////////////////////////////////////////////////
//
// QtDisposer implementation
//
void
QtDisposer::peerReferenced()
{
    assert(pmutex != NULL);

    pmutex->lock();

    ++lockCounter;

    pmutex->unlock();
}


void
QtDisposer::peerUnreferenced(JNIEnv* env)
{
    //
    // The function decrements lockCounter and calls QtPeerBase::dispose
    // for queued peers if lockCounter is 0 and if it is not a recursive call.
    // The function consists of two blocks:
    //  1. check if it is legal do destroy queued peers;
    //  2. destruction of queued peers if legal;
    //

    //
    // 1. Check if it legal to destroy queued peers.
    //
    pmutex->lock();

    if(--lockCounter || recursive || queue.isEmpty())
    {   // it is not legal to destroy queued peers
        env = 0;
    }
    else if(!env && (JVM->AttachCurrentThread((void **)&env, NULL) != 0))
    {   // it is legal to destroy queued peers, but "env" parameter was passed
        // uninitialized and its initialization failed, so do nothing
        // with queued peers
        env = 0;
    }

    pmutex->unlock();

    if(env)
    {
        QtPeerBase* cur = 0;

        //
        // 2. Destroy queued peers.
        //

        // lock Qt do avoid deadlocks, since QtPeerBase::dispose also locks Qt
        AWT_QT_LOCK;
        pmutex->lock();

        // double check if it is safe to destroy queued peers;
        // the check is needed since after previous pmutex->unlock()
        // get called in this QtDisposer::peerUnreferenced some another thread
        // can call QtDisposer::peerReferenced and increment lockCounter or
        // set recursive flag.
        if(!lockCounter && !recursive)
        {
            // set flag to prevent recursive queued peers destruction
            recursive = true;
            while(!queue.isEmpty())
            {
                cur = queue.first();
                queue.remove(queue.begin());

                //printf("dispose peer=%p\n", cur);
                cur->dispose(env);
                QpObject::deleteQObjectLater(cur);
            }
            // reset flag preventing recursive queued peers destruction
            recursive = false;
        }

        pmutex->unlock();
        AWT_QT_UNLOCK;
    }
}


void
QtDisposer::postDisposeRequest(QtPeerBase *peer)
{
    if(peer)
    {
        //printf("posting peer=%p\n", peer);
        pmutex->lock();

        queue.append(peer);

        pmutex->unlock();
    }
}


void
QtDisposer::init()
{
    pmutex = new QtReentrantMutex();
}
