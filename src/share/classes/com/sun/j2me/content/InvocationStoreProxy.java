package com.sun.j2me.content;

import javax.microedition.content.ContentHandlerException;
import javax.microedition.content.Invocation;

public final class InvocationStoreProxy {

	static final public int LIT_MIDLET_START_FAILED = 0;
	static final public int LIT_MIDLET_STARTED = 1;
	static final public int LIT_NATIVE_STARTED = 2;
	static final public int LIT_INVOCATION_REMOVED = 3;
	
	static public int launchInvocationTarget(InvocationImpl invoc){
		if( AppProxy.LOGGER != null ){
			AppProxy.LOGGER.println( "launchInvocationTarget: " + invoc ); 
		}
        // check if it is native handler
        /* IMPL_NOTE: null suite ID is an indication of platform request */
        if (invoc.suiteId == AppProxy.INVALID_SUITE_ID){
        	// call native handler only for unprocessed invocations
        	// status is returned without launching of a handler
        	if( invoc.getStatus() == Invocation.WAITING ) {
	            try {
	                if( AppProxy.launchNativeHandler(invoc.getID()) )
	                	invoc.finish(Invocation.INITIATED);
	                return LIT_NATIVE_STARTED;
	            } catch (ContentHandlerException che) {
	                // Ignore => invocation will be deleted
	            }
        	}
        } else if (invoc.classname != null) {
            try {
            	AppProxy current = AppProxy.getCurrent();
                AppProxy appl = current.forApp(invoc.suiteId, invoc.classname);
            	// if MIDlet already started report STARTED
                int rc = appl.launch("Application")? LIT_MIDLET_STARTED 
                						: LIT_MIDLET_START_FAILED;
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
    	if(AppProxy.LOGGER!=null) {
    		AppProxy.LOGGER.println( "InvocationStoreProxy.invokeNext() called. Invocations count = " + InvocationStore.size());
    		int tid = 0;
    		InvocationImpl invoc;
    		while( (invoc = InvocationStore.getByTid(tid, 1)) != null ){
	        	AppProxy.LOGGER.println( "invocation[" + tid + "]: " + invoc ); 
                tid = invoc.tid;
    		}
    	}

    	int launchedMidletsCount = 0;
    	boolean done = false;
        InvocationImpl invoc = null;
        int tid;

        // Look for a recently queued Invocation to launch
        tid = 0;
        while (!done && (invoc = InvocationStore.getByTid(tid, 1)) != null) {
            switch (invoc.getStatus()){
	            case Invocation.WAITING: {
	                switch( launchInvocationTarget(invoc) ){
		                case LIT_MIDLET_START_FAILED: done = true; break;
		                case LIT_MIDLET_STARTED: launchedMidletsCount++; break;
		                case LIT_NATIVE_STARTED: case LIT_INVOCATION_REMOVED: break;
	                }
	            } 	break;
	            case Invocation.OK: 
	            case Invocation.CANCELLED: 
	            case Invocation.ERROR: 
	            case Invocation.INITIATED:
	            {
	            	if( invoc.getResponseRequired() ){
	            		switch( launchInvocationTarget(invoc) ){
		                	case LIT_MIDLET_START_FAILED: done = true; break;
	            			case LIT_MIDLET_STARTED: launchedMidletsCount++; break; 
			                case LIT_NATIVE_STARTED: case LIT_INVOCATION_REMOVED: break;
	            		}
	            	} else invoc.setStatus(InvocationImpl.DISPOSE);
	            }	break;
	            case Invocation.INIT:
	            	// wrong state
	            	break;
	            case Invocation.ACTIVE:
	            case Invocation.HOLD:
	            	break;
            }
            tid = invoc.tid;
            
            // AMS way to determine number of started midlets is wrong, 
            // so we will start only one new midlet
            done = done || (launchedMidletsCount > 0);
        }
        if(AppProxy.LOGGER!=null) AppProxy.LOGGER.println( InvocationStore.class.getName() + ".invokeNext() finished: started midlets = " + launchedMidletsCount);
        return launchedMidletsCount > 0;
    }
}
