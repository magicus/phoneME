/*
 *
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

package com.sun.j2me.content;

import javax.microedition.content.ContentHandlerException;
import javax.microedition.content.Invocation;

import com.sun.midp.security.SecurityToken;

public abstract class AppProxyAgent implements AMSGate {

	static final public int LIT_APP_START_FAILED = 0;
	static final public int LIT_APP_STARTED = 1;
	static final public int LIT_NATIVE_STARTED = 2;
	static final public int LIT_INVOCATION_REMOVED = 3;
	
    /** This class has a different security domain than the MIDlet suite */
    protected static SecurityToken classSecurityToken;
    
    protected static final boolean isInSvmMode = isInSvmMode();

    /**
     * Sets the security token used for privileged operations.
     * The token may only be set once.
     * @param token a Security token
     */
    static void setSecurityToken(Object token) {
        token.getClass(); // null pointer check
        if (classSecurityToken != null) {
            throw new SecurityException();
        }
        classSecurityToken = (SecurityToken)token;
    }
    
	/*
	 * REMOTE_AMS_CFG: this method must be called on 'AMS side'
	 */
	public int launchInvocationTarget(InvocationImpl invoc){
		if( Logger.LOGGER != null ){
			Logger.LOGGER.println( "launchInvocationTarget: " + invoc ); 
		}
        // check if it is native handler
        /* IMPL_NOTE: null suite ID is an indication of platform request */
        if (invoc.destinationApp.isNative()){
        	// call native handler only for unprocessed invocations
        	// status is returned without launching of a handler
        	if( invoc.getStatus() == Invocation.WAITING ) {
	            try {
	                if( launchNativeHandler(invoc.getID()) )
	                	invoc.finish(Invocation.INITIATED);
	                return LIT_NATIVE_STARTED;
	            } catch (ContentHandlerException che) {
	                // Ignore => invocation will be deleted
	            }
        	} else {
            	// 'native to java' invocation is finished
                platformFinish(invoc.tid);
                
                // TODO: invocation should not be deleted there
                // interface to native handlers should be redesigned
                invoc.setStatus(InvocationImpl.DISPOSE);
                
                return LIT_NATIVE_STARTED;
            }
        } else {
            try {
                AppProxy appl = AppProxy.forApp(invoc.destinationApp);
            	// if MIDlet already started report STARTED
                int rc = CLDCAppProxyAgent.launch(appl, "Application")? LIT_APP_STARTED 
                						: LIT_APP_START_FAILED;
                return rc;
            } catch (ClassNotFoundException cnfe) {
                // Ignore => invocation will be deleted
            }
        } 
        
        // can't process this invocation - remove it
        invoc.setStatus(InvocationImpl.DISPOSE);
        return LIT_INVOCATION_REMOVED;
	}

    /**
     * Execute the User Environment Policy to select the next
     * application to run.
     * Check for and select the next MIDlet suite to run
     * based on the contents of the Invocation queue.
     * 
     * From the most recently queued Invocation that is an Invocation
     * in INIT.
     * If none, find the most recently queued Invocation that is
     * a response.
     * 
     * @return <code>true</code> if some midlets are started 
     */
    static public boolean invokeNext() {
    	if(Logger.LOGGER != null) {
    		Logger.LOGGER.println( "InvocationStoreProxy.invokeNext() called. Invocations count = " + InvocationImpl.store.size());
    		int tid = 0;
    		InvocationImpl invoc;
    		while( (invoc = InvocationImpl.store.getByTid(tid, true)) != null ){
	        	Logger.LOGGER.println( "invocation[" + tid + "]: " + invoc ); 
                tid = invoc.tid;
    		}
    	}

    	int launchedAppsCount = 0;
    	boolean done = false;
        InvocationImpl invoc = null;
        int tid;

        // Look for a recently queued Invocation to launch
        tid = 0;
        while (!done && (invoc = InvocationImpl.store.getByTid(tid, true)) != null) {
            switch (invoc.getStatus()){
	            case Invocation.WAITING: {
	                switch( AMSGate.inst.launchInvocationTarget(invoc) ){
		                case LIT_APP_START_FAILED: done = true; break;
		                case LIT_APP_STARTED: launchedAppsCount++; break;
		                case LIT_NATIVE_STARTED: case LIT_INVOCATION_REMOVED: break;
	                }
	            } 	break;
	            case Invocation.OK: 
	            case Invocation.CANCELLED: 
	            case Invocation.ERROR: 
	            case Invocation.INITIATED:
	            {
	            	if( invoc.getResponseRequired() ){
	            		switch( AMSGate.inst.launchInvocationTarget(invoc) ){
		                	case LIT_APP_START_FAILED: done = true; break;
	            			case LIT_APP_STARTED: launchedAppsCount++; break; 
			                case LIT_NATIVE_STARTED: case LIT_INVOCATION_REMOVED: break;
	            		}
	            	} else invoc.setStatus(InvocationImpl.DISPOSE);
	            }	break;
	            case Invocation.INIT:
	            	// wrong state
	            	if( Logger.LOGGER != null )
	            		Logger.LOGGER.println( "invocation has wrong state (INIT)" );
	            	break;
	            case Invocation.ACTIVE:
	            case Invocation.HOLD:
	            	break;
            }
            tid = invoc.tid;
            
            // AMS way to determine number of started midlets is wrong, 
            // so we will start only one new midlet
            done = done || (launchedAppsCount > 0);
        }
        if(Logger.LOGGER != null) 
        	Logger.LOGGER.println( "invokeNext() finished: started apps = " + launchedAppsCount);
        return launchedAppsCount > 0;
    }
    /**
     * Starts native content handler.
     * 
	 * REMOTE_AMS_CFG: this method must be called on 'AMS side'
	 * 
     * @param handler Content handler to be executed.
     * @return true if invoking app should exit.
     * @exception ContentHandlerException if no such handler ID in the Registry
     * or native handlers execution is not supported.
     */
    static boolean launchNativeHandler(String handlerID) 
    										throws ContentHandlerException {
        int result = launchNativeHandler0(handlerID);
        if (result < 0) {
            throw new ContentHandlerException(
                        "Unable to launch platform handler",
                        ContentHandlerException.NO_REGISTERED_HANDLER);
        }
        return (result > 0);
    }

    /**
     * Informs platform about finishing of processing platform's request
     * 
	 * REMOTE_AMS_CFG: this method must be called on 'AMS side'
	 * 
     * @param invoc finished invocation
     * @return should_exit flag for the invocation handler
     */
    static boolean platformFinish(int tid) {
        return platformFinish0(tid);
    }
    
    // native methods
    /**
     * Starts native content handler.
     * @param handlerId ID of the handler to be executed
     * @return result status:
     * <ul>
     * <li> 0 - LAUNCH_OK 
     * <li> > 0 - LAUNCH_OK_SHOULD_EXIT
     * <li> &lt; 0 - error
     * </ul>
     */
    private static native int launchNativeHandler0(String handlerId);

    private static native boolean platformFinish0(int tid);

    private static native boolean isInSvmMode();
}
