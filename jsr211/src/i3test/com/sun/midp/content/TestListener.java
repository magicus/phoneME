/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.content;

import com.sun.midp.i3test.TestCase;

import javax.microedition.content.Invocation;
import javax.microedition.content.RequestListener;
import javax.microedition.content.ResponseListener;
import javax.microedition.content.Registry;
import javax.microedition.content.ContentHandlerServer;

import java.util.Vector;

/**
 * Test that the ContentListenerImpl class correctly notified
 * when Invocations are present.
 */
public class TestListener extends ExtendedTestCase
    implements ResponseListener, RequestListener {

    /** Incremented when the invocationRequestNotify method is called. */
    private int requestNotified;

    /** Incremented when the invocationResponseNotify method is called. */
    private int responseNotified;

    /** Registry in use for the tests. */
    RegistryImpl registry;

    /**
     * Run the tests of the Listener.
     */
    public void runTests() {
	test001();
    }

    /**
     * Test that the listener is notified when a new Invocation
     * is queued.
     */
    private void test001() {
	declare("Listener called when invocation available");

	// Registry that will be doing the listening
	registry = getRegistry();

	// ContentHandlerImpl doing the listening
	ContentHandlerImpl handler = new ReqListener(this);
	handler.storageId = registry.application.getStorageId();
	handler.classname = registry.application.getClassname();

	InvocationImpl put = new InvocationImpl();
	put.suiteId = registry.application.getStorageId();
	put.classname = registry.application.getClassname();
	put.invokingSuiteId = put.suiteId;
	put.invokingClassname = put.classname;
	put.status = Invocation.INIT;
	put.responseRequired = true;

	requestNotified = 0;
	responseNotified = 0;
	InvocationStore.put(put);

	// Verify that there is no notification if no listener
	sleep(200);		// wait a bit
	assertEquals("listener should not be notified", 0, requestNotified);
	assertEquals("listener should not be notified", 0, responseNotified);

	// Now create the monitor threads so the notification will occur
	RequestListenerImpl requestImpl =
	    new RequestListenerImpl(handler, this);
	ResponseListenerImpl responseImpl =
	    new ResponseListenerImpl(registry, this);

	sleep(200L);		// wait a bit
	assertEquals("request listener should be notified",
		     1, requestNotified);
	assertEquals("response listener should not be notified",
		     0, responseNotified);

	// Remove the pending request
	InvocationImpl get =
	    InvocationStore.getRequest(registry.application.getStorageId(),
				       registry.application.getClassname(),
				       true);
	assertEquals("verify request invocation", put, get);

	// Requeue as a response
	requestNotified = 0;
	responseNotified = 0;
        get.status = Invocation.OK;
	InvocationStore.setStatus(get);

	sleep(200L);		// wait a bit
	assertEquals("request listener should not be notified",
		     0, requestNotified);
	assertEquals("response listener should be notified",
		     1, responseNotified);

	// Remove the pending notification
	get = InvocationStore.getResponse(new InvocationImpl(),
					  registry.application.getStorageId(),
					  registry.application.getClassname(),
					  true);
	assertEquals("verify invocation", put, get);

	// Now stop the listener thread; same as setListener(null).
	requestImpl.setListener(null);
	responseImpl.setListener(null);


	/*
	 * With listeners reset put another request and verify no
	 * notifications.
	 */
	requestNotified = 0;
	responseNotified = 0;
	InvocationStore.put(put);
	sleep(200L);
	assertEquals("request listener should not be notified",
		     0, requestNotified);
	assertEquals("response listener should not be notified",
		     0, responseNotified);

	// Remove the pending request
	get = InvocationStore.getRequest(registry.application.getStorageId(),
					 registry.application.getClassname(),
					 true);
	assertEquals("verify request invocation", put, get);

	// Requeue as a response
	get.status = Invocation.OK;
	InvocationStore.setStatus(get);

	sleep(200L);		// wait a bit
	assertEquals("request listener should not be notified",
		     0, requestNotified);
	assertEquals("response listener should not be notified",
		     0, responseNotified);

	// Remove the pending notification
	get = InvocationStore.getResponse(new InvocationImpl(),
					  registry.application.getStorageId(),
					  registry.application.getClassname(),
					  true);
	assertEquals("verify invocation", put, get);

	/*
	 * Verify that setting a new listener restarts the monitors.
	 * Put another Invocation and verify the notification does occur
	 */
	requestNotified = 0;
	responseNotified = 0;

	requestImpl.setListener(this);
	responseImpl.setListener(this);

	InvocationStore.put(put);
	sleep(200L);
	assertEquals("listener should be notified", 1, requestNotified);
	assertEquals("listener should not be notified", 0, responseNotified);

	// Remove the pending request
	get = InvocationStore.getRequest(registry.application.getStorageId(),
					 registry.application.getClassname(),
					 true);
	assertEquals("verify request invocation", put, get);

	// Requeue as a response
	requestNotified = 0;
	responseNotified = 0;
	get.status = Invocation.OK;
	InvocationStore.setStatus(get);

	sleep(200L);
	assertEquals("request listener should not be notified",
		     0, requestNotified);
	assertEquals("response listener should be notified",
		     1, responseNotified);

	// Remove the pending notification
	get = InvocationStore.getResponse(new InvocationImpl(),
					  registry.application.getStorageId(),
					  registry.application.getClassname(),
					  true);
	assertEquals("verify invocation", put, get);

	// Now stop the listener thread; same as setListener(null).
	requestImpl.setListener(null);
	responseImpl.setListener(null);
    }


    /**
     * Notified when an invocation request is present.
     * @param handler the ContentHandlerServer with the request
     */
    public void invocationRequestNotify(ContentHandlerServer handler) {
	requestNotified++;
    }

    /**
     * Notified when an invocation response is present.
     * @param handler the ContentHandlerServer with the response
     */
    public void invocationResponseNotify(Registry handler) {
	responseNotified++;
    }


    /**
     * Check that there are no Invocations pending;
     * none should be.
     */
    private void assertEmpty() {
	InvocationImpl get;
	do {
	    get = InvocationStore.
		getRequest(registry.application.getStorageId(),
			   registry.application.getClassname(),
			   false);
	    assertNull("Verify no request pending", get);
	} while (get != null);
	do {
	    get = InvocationStore.getResponse(new InvocationImpl(),
					      registry.application.getStorageId(),
					      registry.application.getClassname(),
					      false);
	    assertNull("Verify no response pending", get);
	} while (get != null);
	assertEquals("verify invocation queue is empty",
		     0, InvocationStore.size());
    }

    /**
     * Shell class to be notified of available requests.
     */
    static class ReqListener
        extends ContentHandlerImpl
        implements ContentHandlerServer
    {
        /** Listener to be notified on request available. */
        RequestListener listener;
        /**
         * Constructor for test handler.
         * @param l the listener to be notified
         */
        ReqListener(RequestListener l) {
            listener = l;
        }
        /**
         * Overridden method to be notified.
         */
        protected void requestNotify() {
            listener.invocationRequestNotify(this);
        }

        public boolean finish(Invocation invocation, int status) {
            return false;
        }

        public Invocation getRequest(boolean wait) {
            return null;
        }

    }
}
