package com.sun.j2me.content;

import javax.microedition.content.ContentHandlerException;
import javax.microedition.content.Invocation;

public final class InvocationStoreProxy {
	protected static final java.io.PrintStream DEBUG_OUT = null; //System.out;

	static final public int LIT_MIDLET_START_FAILED = 0;
	static final public int LIT_MIDLET_STARTED = 1;
	static final public int LIT_NATIVE_STARTED = 2;
	static final public int LIT_INVOCATION_REMOVED = 3;
	
	static public int launchInvocationTarget(InvocationImpl invoc){
		if( DEBUG_OUT != null ) DEBUG_OUT.println( "launchInvocationTarget:" ); 
		if( DEBUG_OUT != null && invoc != null ){
			invoc.debugTo(DEBUG_OUT);
			DEBUG_OUT.println();
		}
        // check if it is native handler
        /* IMPL_NOTE: null suite ID is an indication of platform request */
        if (invoc.suiteId == AppProxy.INVALID_STORAGE_ID){
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
        	}
        } else if (invoc.classname != null) {
            try {
                AppProxy appl = AppProxy.getCurrent().forApp(invoc.suiteId, invoc.classname);
            	// if MIDlet already started report STARTED
                return appl.launch("Application")? LIT_MIDLET_STARTED 
                						: LIT_MIDLET_START_FAILED;
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
    	if(DEBUG_OUT!=null) {
    		DEBUG_OUT.println( InvocationStoreProxy.class.getName() + ".invokeNext() called. Invocations count = " + InvocationStore.size());
    		int tid = 0;
    		InvocationImpl invoc;
    		while( (invoc = InvocationStore.getByTid(tid, 1)) != null ){
	        	DEBUG_OUT.print( "invocation[" + tid + "]: " ); 
	        	invoc.debugTo(DEBUG_OUT);
	        	DEBUG_OUT.println();
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
        if(DEBUG_OUT!=null) DEBUG_OUT.println( InvocationStore.class.getName() + ".invokeNext() finished: started midlets = " + launchedMidletsCount);
        return launchedMidletsCount > 0;
    }

    /**
     * Starts native content handler.
     * @param handler Content handler to be executed.
     * @return true if invoking app should exit.
     * @exception ContentHandlerException if no such handler ID in the Registry
     * or native handlers execution is not supported.
     */
    static private boolean launchNativeHandler(String handlerID) 
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
     * @param invoc finished invocation
     * @return should_exit flag for the invocation handler
     */
    static boolean platformFinish(int tid) {
        return platformFinish0(tid);
    }
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
}
