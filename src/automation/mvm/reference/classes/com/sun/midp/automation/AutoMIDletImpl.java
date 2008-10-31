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

package com.sun.midp.automation;

/**
 * AutoMIDlet interface implementation.
 */
final class AutoMIDletImpl implements AutoMIDlet {
    /** MIDlet descriptor for this MIDlet */
    private AutoMIDletDescriptorImpl midletDescriptor;

    /** For tracking life cycle states */
    private AutoStateTracker lifeCycleStateTracker;

    /** For tracking background/foreground states */
    private AutoStateTracker foregroundStateTracker;
    
    /** For switching MIDlet state */
    private static AutoMIDletStateController 
        midletStateController = null;

    /** For switching MIDlet state */
    private static AutoForegroundStateController 
        foregroundStateController = null;
   

    /**
     * Constructor.
     *
     * @param midletDescriptor MIDlet descriptor for this MIDlet
     */
    AutoMIDletImpl(AutoMIDletDescriptorImpl midletDescriptor) {
        this.midletDescriptor = midletDescriptor;
        this.lifeCycleStateTracker = 
            new AutoStateTracker(AutoMIDletLifeCycleState.PAUSED);

        this.foregroundStateTracker = 
            new AutoStateTracker(AutoMIDletForegroundState.BACKGROUND);

        if (midletStateController == null) {
            midletStateController = 
                AutoMIDletStateController.getMIDletStateController();
        }

        if (foregroundStateController == null) {
            foregroundStateController = 
                AutoForegroundStateController.getForegroundStateController();
        }
    }


    /**
     * To be called when MIDlet's life cycle state has changed.
     *
     * @param newState new life cycle state
     */
    void stateChanged(AutoMIDletLifeCycleState newState) {
        boolean interruptWaiters = false;
        if (newState == AutoMIDletLifeCycleState.DESTROYED) {
            interruptWaiters = true;
        }

        lifeCycleStateTracker.setCurrentState(newState, interruptWaiters);
        
        if (newState == AutoMIDletLifeCycleState.DESTROYED) {
            foregroundStateTracker.interruptWait();
            midletDescriptor.midletDestroyed();
        }
    }
    

    /**
     * To be called when MIDlet's foreground state has changed.
     *
     * @param newState new forgeround state
     */
    void stateChanged(AutoMIDletForegroundState newState) {
        foregroundStateTracker.setCurrentState(newState, false);
    }
   

    /**
     * Guarantees that MIDlet is not destroyed: if MIDlet
     * is destroyed, exception is thrown.
     *
     * @param s error string
     */
    private void guaranteeMIDletNotDestroyed(String s) 
        throws IllegalStateException {
            
        if (lifeCycleStateTracker.getCurrentState() == 
                AutoMIDletLifeCycleState.DESTROYED) {
            throw new IllegalStateException(s);
        }
    }

    /**
     * Guarantees that state transition is valid (possible):
     * if state is unreachable, exception is thrown.
     *
     * @param state state to check for
     * @param s error string
     */
    private void guaranteeStateTransitionIsValid(
            AutoMIDletLifeCycleState state, String s)
        throws IllegalStateException {

        if (state != AutoMIDletLifeCycleState.DESTROYED) {
            guaranteeMIDletNotDestroyed(s);
        }
    }
    
    /**
     * Guarantees that waiting for state is valid: 
     * if state is unreachable, exception is thrown.
     *
     * @param state state to check for
     * @param s error string
     */
    private void guaranteeStateTransitionIsValid(
            AutoMIDletForegroundState state, String s)
        throws IllegalStateException {

        guaranteeMIDletNotDestroyed(s);
    }


    /**
     * AutoMIDlet implementation
     */
    
    /**
     * Initiate a switch (transition) from current to specified 
     * lifecycle state.
     *
     * @param state state to switch to
     * @param wait if true, wait (block) until transition to the 
     * specified state has been completed
     * @throws IllegalStateException thrown when switching to specified
     * state is invalid
     */
    public void switchTo(AutoMIDletLifeCycleState state, boolean wait) 
        throws IllegalStateException {
            
        synchronized (lifeCycleStateTracker) {
            guaranteeStateTransitionIsValid(state, "switchTo");

            midletStateController.switchTo(this, state);
            if (wait) {
                waitFor(state);
            }
        }        
    }
    
    /**
     * Initiate a switch (transition) from current to specified 
     * foreground state.
     *
     * @param state state to switch to
     * @param wait if true, wait (block) until transition to the 
     * specified state has been completed
     * @throws IllegalStateException thrown when switching to specified
     * state is invalid
     */
    public void switchTo(AutoMIDletForegroundState state, boolean wait) 
        throws IllegalStateException {

        synchronized (foregroundStateTracker) {
            guaranteeStateTransitionIsValid(state, "switchTo");

            foregroundStateController.switchTo(this, state);
            if (wait) {
                waitFor(state);
            }
        }
    }


    /**
     * Wait (block) until MIDlet reaches specified lifecycle state.
     *
     * @param state state to wait for
     * @throws IlegalStateException thrown if state to wait for is invalid,
     * or if during waiting MIDlet has reached the state where waiting for
     * specified state is no longer valid
     */
    public void waitFor(AutoMIDletLifeCycleState state)
        throws IllegalStateException {
        
        synchronized (lifeCycleStateTracker) {
            guaranteeStateTransitionIsValid(state, "waitFor");
            
            // wait
            lifeCycleStateTracker.waitFor(state);
            
            // wait might have been interrupted, so check again
            // if waiting for this state is still valid
            guaranteeStateTransitionIsValid(state, "waitFor");
        }
    }
        
    /**
     * Wait (block) until MIDlet reaches specified foreground state.
     *
     * @param state state to wait for
     * @throws IlegalStateException thrown if state to wait for is invalid,
     * or if during waiting MIDlet has reached the state where waiting for
     * specified state is no longer valid
     */
    public void waitFor(AutoMIDletForegroundState state)
        throws IllegalStateException {
            
        synchronized (foregroundStateTracker) {
            guaranteeStateTransitionIsValid(state, "waitFor");
            
            foregroundStateTracker.waitFor(state);
            
            // wait might have been interrupted, so check again
            // if waiting for this state is still valid
            guaranteeStateTransitionIsValid(state, "waitFor");
        }
    }

    /**
     * Get current lifecycle state.
     *
     * @return AutoLifeCycleState representing current
     * lifecycle state
     */
    public AutoMIDletLifeCycleState getLifeCycleState() {
        synchronized (lifeCycleStateTracker) {
            Object state = lifeCycleStateTracker.getCurrentState();
            return (AutoMIDletLifeCycleState)state; 
        }
    }

    
    /**
     * Get current foreground state.
     *
     * @return AutoForegroundState representing current
     * foreground state
     */
    public AutoMIDletForegroundState getForegroundState() {
        synchronized (foregroundStateTracker) {
            Object state = foregroundStateTracker.getCurrentState();
            return (AutoMIDletForegroundState)state; 
        }
    }
    
    /**
     * Get MIDlet's descriptor.
     *
     * @return AutoMIDletDescriptor representing descriptor.
     */
    public AutoMIDletDescriptor getMIDletDescriptor() {
        return midletDescriptor;
    }
}
