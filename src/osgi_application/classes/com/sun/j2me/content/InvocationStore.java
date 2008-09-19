/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

import java.util.Iterator;
import java.util.Vector;

import javax.microedition.content.Invocation;

/**
 * The store for pending Invocations.
 * New Invocations are queued with {@link #put} method and
 * retrieved with the {@link #get} method. The {@link #cancel}
 * method is used to unblock calls to blocking {@link #get} methods.
 * <p>
 * Synchronization is performed by the native methods; access
 * is serialized by the VM running in a single native thread and
 * by NOT preempting native method calls.
 * The native code uses the SNI ability to block a thread and
 * unblock it at a later time. The implementation does not poll for
 * requests but blocks, if requested, until it is unblocked.
 */
public class InvocationStore {
	
	private static int freeTid = 1;
	
	private static class InvocationData {
		InvocationImpl invoc;
		boolean cleanup = false, notified = false;
		
		InvocationData(InvocationImpl invoc) {
			this.invoc = invoc;
		}
	}
	private static final Vector/*<InvocationData>*/ invocations = new Vector();
	
    private static interface IDReceiver {
    	void push( InvocationData invocData );
    }
    
    private static abstract class InvocationsEnumerator {
    	protected abstract boolean modeCheck( InvocationData invocData, int mode );
    	
    	public void enumerate( int suiteId, String classname, int mode, IDReceiver out ){
    		Iterator it = invocations.iterator();
    		InvocationData invocData = null;
    		while( it.hasNext() ){
    			invocData = (InvocationData)it.next();
    			if( modeCheck(invocData, mode) && suiteId == invocData.invoc.suiteId && 
    					classname.equals(invocData.invoc.classname) ){
					out.push(invocData);
    			}
    		}
    	}
    }
    
	private static final int findIndexFor( int tid, boolean exact ){
		int head = 0, tail = invocations.size();
		while( head < tail ){
			int pos = (head + tail) / 2;
			if( ((InvocationData)invocations.get(pos)).invoc.tid < tid ) head = pos + 1;
			else tail = pos;
		}
		if( exact && head < invocations.size() &&
				((InvocationData)invocations.get(head)).invoc.tid != tid ){
			head = invocations.size();
		}
		return head;
	}

    /** The mode for get to retrieve a new request. */
    private static final int MODE_REQUEST = 0;

    /** The mode for get to retrieve a new response. */
    private static final int MODE_RESPONSE = 1;

    /** The mode for get to retrieve a new cleanup. */
    private static final int MODE_CLEANUP = 2;

    /** The mode for listen for new unmarked request. */
    private static final int MODE_LREQUEST = 3;

    /** The mode for listen for a new unmarked response. */
    private static final int MODE_LRESPONSE = 4;

    /* The mode for get to retrieve a new ACTIVE, HOLD, or WAITING request. */
    // private static final int MODE_PENDING = 5;

    /**
     * Private constructor to prevent instance creation.
     */
    private InvocationStore() {
    }

    /**
     * Put a new Invocation into the store.
     * It can be modified by {@link #setStatus}.
     * The TID (transaction ID) is updated with a newly assigned value.
     *
     * @param invoc an InvocationImpl instance with the members properly
     *  initialized.
     * @see #getRequest
     * @see #getResponse
     */
    static void put(InvocationImpl invoc) {
        invoc.classname.length(); // null pointer check
        /* put0 */
        invoc.tid = freeTid++;
        invocations.add(findIndexFor( invoc.tid, false ), new InvocationData(invoc));
        unblockWaitingThreads(Invocation.OK);
    }

	private static InvocationData find(int suiteId, String classname, final int mode) {
		
		class enumerator extends InvocationsEnumerator {
		    protected boolean modeCheck(InvocationData invocData, int mode) {
		        switch (mode) {
			        case MODE_REQUEST:
			            return (invocData.invoc.status == Invocation.WAITING);
			
			        case MODE_RESPONSE:
			            return (invocData.invoc.status >= Invocation.OK &&
			                    invocData.invoc.status <= Invocation.INITIATED);
			
			        case MODE_LREQUEST:
			            return (!invocData.notified && invocData.invoc.status == Invocation.WAITING);
			
			        case MODE_LRESPONSE:
			            return (!invocData.notified && 
			                    invocData.invoc.status >= Invocation.OK &&
			                    invocData.invoc.status <= Invocation.INITIATED);
			
			        case MODE_CLEANUP:
			            /*
			             * If the Invocation is an old one then it needs
			             * to be cleaned up if it is a response or is active.
			             * That's everything except HOLD.
			             */
			            return (invocData.invoc.status == Invocation.ACTIVE ||
			                   (invocData.cleanup && invocData.invoc.status != Invocation.HOLD));
			    }
		        return false;
		    }
		};
		
		class receiver implements IDReceiver {
		    class FoundException extends RuntimeException {
				public InvocationData invocData;
			    public FoundException(final InvocationData invocData) {
			    	this.invocData = invocData;
				}
		    }
			public void push(InvocationData invocData) {
				if( mode == MODE_CLEANUP ){
	                /* An active or waiting Invocation needs a response */
	                if ((invocData.invoc.status != Invocation.WAITING && 
	                		invocData.invoc.status != Invocation.ACTIVE) ||
	                        !invocData.invoc.responseRequired) {
	                    /* A regular response, discard and continue */
						dispose( invocData.invoc.tid );
						return; // continue searching
	                }
				}
				throw new FoundException(invocData);
			}
		}
		
		try {
			new enumerator().enumerate(suiteId, classname, mode, new receiver());
		} catch( receiver.FoundException x ){
			return x.invocData;
		}
        return null;
	}

	private static InvocationImpl get(int suiteId, String classname, int mode) {
		InvocationData invocData = find( suiteId, classname, mode );
		if( invocData != null ){
			switch( mode ){
				case MODE_REQUEST:
					if( invocData.invoc.status == Invocation.WAITING )
						invocData.invoc.status = Invocation.ACTIVE;
					break;
				case MODE_RESPONSE:
					if( invocData.invoc.status >= Invocation.OK && 
							invocData.invoc.status <= Invocation.INITIATED )
						invocations.remove(invocData); // remove from the store
					break;
			}
	        return invocData.invoc;
		}
		return null;
	}

	static void update(InvocationImpl invoc) {
		// do nothing
	}

	static void resetFlags(int tid) {
		int index = findIndexFor( tid, true );
		if( index < invocations.size() ){
			InvocationData data = (InvocationData)invocations.get( index );
	        data.cleanup = false;
	        data.notified = false;
	        /* Unblock any waiting threads so they can retrieve this. */
	        unblockWaitingThreads(Invocation.OK);
		}
	}

	static void dispose(int tid) {
		int index = findIndexFor( tid, true );
		if( index < invocations.size() )
			invocations.remove(index);
	}
	
    /**
     * Get an Invocation from the store based on its <code>tid</code>.
     * The normal state transitions and dispositions are NOT performed.
     * If TID == 0 then the first tid is used as the reference.
     * If TID == 0 and relative == 0 then null is returned.
     * This method never waits.
     *
     * @param tid the <code>tid</code> to fetch
     * @param next to get equal, or next
     * @return an InvocationImpl object if a matching tid was found;
     *  otherwise <code>null</code>
     */
    static InvocationImpl getByTid(int tid, boolean next) {
    	int index = 0;
    	if( tid != 0 ){
	    	index = findIndexFor(tid, true);
	    	if( next ) index++;
    	}
    	if( index < invocations.size() )
    		return ((InvocationData)invocations.get(index)).invoc;
    	return null;
    }

    /**
     * Get an InvocationImpl from the store using a MIDlet suiteId
     * and classname.
     * The mode controls whether getting an Invocation
     * from the store removes it from the store.
     *
     * @param invoc InvocationImpl to fill in with result
     * @param mode one of {@link #MODE_REQUEST}, {@link #MODE_RESPONSE},
     *    or {@link #MODE_CLEANUP}.
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname if one was requested;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    private static InvocationImpl get(int suiteId, String classname,
				      int mode, boolean shouldBlock, Counter cancelCounter) {
    	classname.length(); // null pointer check

		InvocationImpl invoc = get(suiteId, classname, mode);
		if( shouldBlock ){
	    	int cancels = cancelCounter.getCounter();
		    while (invoc == null && cancels == cancelCounter.getCounter()){
		    	try {
					blockThread();
				} catch (InterruptedException e) {
					break;
				}
		    	invoc = get(suiteId, classname, mode);
		    }
		}
		return invoc;
    }
    
    /**
     * Get a new InvocationImpl request from the store using a MIDlet
     * suiteId and classname.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname with
     *  its status is set to ACTIVE;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getRequest(int suiteId, String classname, 
    							boolean shouldBlock, Counter cancelCounter) {
        return get(suiteId, classname, MODE_REQUEST, shouldBlock, cancelCounter);
    }

    /**
     * Get a new InvocationImpl response from the store using a
     * MIDlet suiteId and classname.
     * The response is removed from the store.
     *
     * @param invoc an InvocationImpl to fill with the response
     * @param suiteId the MIDletSuite ID
     * @param classname the classname
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname if one was requested;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getResponse(int suiteId, String classname, 
    							boolean shouldBlock, Counter cancelCounter) {
		return get(suiteId, classname, MODE_RESPONSE, shouldBlock, cancelCounter);
    }

    /**
     * Performs cleanup for a ContentHandler
     * by suiteId and classname.
     * <p>
     * Any marked {@link #setCleanup} invocations still in the queue
     * are handled based on status:
     * <UL>
     * <li>ACTIVE Invocations are returned from this method
     *    so they can be have the ERROR status set and so the
     *    invoking application relaunched.</li>
     * <li>INIT Invocations are requeued to the invoking application
     *    with ERROR status. </li>
     * <li>OK, CANCELLED, ERROR, or INITIATED Invocations are
     *    discarded.</li>
     * <li>HOLD status Invocations are retained pending
     *    completion of previous Invocation.  TBD: Chained HOLDs...</li>
     * </ul>
     *
     * @param suiteId the MIDletSuite ID
     * @param classname the classname
     *
     * @return <code>InvocationImpl</code> if any was found with
     *  the same MIDlet suiteId and classname;
     *  <code>null</code> is returned if there is no matching Invocation
     */
    static InvocationImpl getCleanup(int suiteId, String classname) {
    	return get(suiteId, classname, MODE_CLEANUP, false, null);
    }

    /**
     * Listen for a matching invocation.
     * When a matching invocation is present, true is returned.
     * Each Invocation instance is only returned once.
     * After it has been returned once; it is ignored subsequently.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param request true to listen for a request; else a response
     * @param shouldBlock true if the method should block
     *      waiting for an Invocation
     *
     * @return true if a matching invocation is present; false otherwise
     */
    static boolean listen(int suiteId, String classname,
               			boolean request, boolean shouldBlock, 
               			Counter cancelCounter) {
    	classname.length(); // null pointer check 
        int mode = (request ? MODE_LREQUEST : MODE_LRESPONSE);

        InvocationData invocData = find(suiteId, classname, mode);
		if( shouldBlock ){
	        int oldCancelCount = cancelCounter.getCounter();
		    while (invocData == null && oldCancelCount == cancelCounter.getCounter()){
		    	try {
					blockThread();
				} catch (InterruptedException e) {
					break;
				}
				invocData = find(suiteId, classname, mode);
		    }
		}
		if( invocData != null )
			invocData.notified = true;
        return invocData != null;
    }

    /**
     * Reset the flags for requests or responses that are pending.
     * Once reset, any pending requests or responses will be
     * returned when listen0 is called.
     *
     * @param suiteId the MIDlet suiteId to search for,
     *  MUST not be <code>null</code>
     * @param classname to match, must not be null
     * @param request true to reset request notification flags;
     *   else reset response notification flags
     */
    static void setListenNotify(int suiteId, String classname, boolean request) {
    	classname.length();
        int mode = (request ? MODE_LREQUEST : MODE_LRESPONSE);
        
		class enumerator extends InvocationsEnumerator {
		    protected boolean modeCheck(InvocationData invocData, int mode) {
		        switch (mode) {
			        case MODE_LREQUEST:
			            return invocData.invoc.status == Invocation.WAITING;
			
			        case MODE_LRESPONSE:
			            return invocData.invoc.status >= Invocation.OK &&
			                    invocData.invoc.status <= Invocation.INITIATED;
			    }
		        return false;
		    }
		};
		
		new enumerator().enumerate(suiteId, classname, mode, 
				new IDReceiver(){
					public void push(InvocationData invocData) {
						invocData.notified = false;
					}
				});
    }

    /**
     * Cancel a blocked {@link #get}  or {@link #listen}
     * method if it is blocked in the native code.
     */
    static void cancel() {
    	unblockWaitingThreads(Invocation.CANCELLED);
    }

    /**
     * Marks any existing invocations for the content handler.
     * Any marked invocation will be modified by {@link #getCleanup}.
     *
     * @param suiteId the suite to mark
     * @param classname the MIDlet within the suite
     * @param cleanup <code>true</code> to mark the Invocation for
     *   cleanup at exit
     */

    public static void setCleanup(int suiteId, String classname, final boolean cleanup) {
    	classname.length();
    	
		class enumerator extends InvocationsEnumerator {
		    protected boolean modeCheck(InvocationData invocData, int mode) {
		    	return true;
		    }
		};
		
		new enumerator().enumerate(suiteId, classname, MODE_CLEANUP, 
				new IDReceiver(){
					public void push(InvocationData invocData) {
						invocData.cleanup = cleanup;
						invocData.notified = false;
					}
				});
    }

    /**
     * Return the number of invocations in the native queue.
     * @return the number of invocations in the native queue
     */
    static int size() {
        return invocations.size();
    }
    
    private static final void blockThread() throws InterruptedException{
    	synchronized (invocations) {
			invocations.wait();
		}
    }

    private static final void unblockWaitingThreads( int status ){
    	invocations.notifyAll();
    }
}
