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
package com.sun.midp.chameleon;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Utility class used to animate gestures
 */
public class GestureAnimator {
    
    /**
     * Animation recalculation parameters
     */
    protected static final int scrollAnimationRate = 100;
    protected static double flickSpeed = 2;
    protected static int ScrollStableSteps = 4;
    protected static int ScrollFlickSteps = 7;

    /**
     * Animation types
     */
    protected static int AT_STABLE = 1;
    protected static int AT_FLICK  = 2;

    /**
     * Current animation type
     */
    protected static int type;


    /**
     * Timer used to perform animation
     */
    protected static Timer timer;
    /**
     * Timer task for scrolling animation
     */
    protected static dragTimerTask timerTask;
    /**
     * Listener for which animation was requested.
     */
    protected static GestureAnimatorListener listener;

    /**
     * Scrolling animation parameters.
     */
    /**
     * Height we need to scroll.
     */
    protected static double yDistance;
    /**
     * Height left to scroll.
     */
    protected static double yLeft;
    /**
     * Current scrolling direction - up (1) or down (-1).
     */
    protected static int ySign;
    /**
     * Animation step count.
     */
    protected static double stepCnt;
    /**
     * Current animation step number.
     */
    protected static int currStep;
    /**
     * Parameters of function used to calculate scrolling speed.
     */
    protected static double a;
    protected static double b = -7;

    /**
     * Returns static timer instance.
     * @return timer
     */
    protected static Timer getTimer() {
        if (timer == null) {
            timer = new Timer();
        }
        return timer;
    }

    /**
     * Called when Listener content is draged out of bounds.
     * Performs smooth way to return content back to stable position
     * by means of listener callbacks.
     * @param l listener to notify
     * @param stY distance we need to scroll to return to stable position
     */
    public static void dragToStablePosition(GestureAnimatorListener l, int stY) {
        stop();
        
        listener = l;

        initAnimation(AT_STABLE, stY);
        timerTask = new dragTimerTask();
        getTimer().schedule(timerTask, 0, scrollAnimationRate);
    }

    /**
     * Called when Listener content is flicked to scroll smoothly
     * in a flick direction. Passed in parameter dy characterizes
     * speed of the flick and is used to calculate distance we need
     * to scroll. Usually dy is last drag speed.
     * @param l listener to notify
     * @param dy distance that characterizes speed of the flick 
     */
    public static void flick(GestureAnimatorListener l, int dy) {
        stop();
        
        listener = l;

        initAnimation(AT_FLICK, dy);
        timerTask = new dragTimerTask();
        getTimer().schedule(timerTask, 0, scrollAnimationRate);
    }

    /**
     * Set up animation parameters
     * @param t animation type
     * @param param animation parameter
     */
    protected static void initAnimation(int t, int param) {
        type = t;
        double dist = 0;

        if (type == AT_STABLE) {
            // param - distance we need to scroll to return to stable position
            stepCnt = ScrollStableSteps;
            dist = param;
        } else if (type == AT_FLICK) {
            // param - initial speed of the flick            
            stepCnt = ScrollFlickSteps;

            dist = (flickSpeed * param - b) * stepCnt * stepCnt * stepCnt /
                    (stepCnt - 1) / (stepCnt - 1) / (stepCnt - 1) + b;

        } else {
            //unsupported
            return;
        }

        currStep = 0;
        if (dist > 0) {
            yDistance = yLeft = dist;
            ySign = 1;
        } else {
            yDistance = yLeft = -dist;
            ySign = -1;
        }
        a = (yDistance - b)/(yDistance * yDistance * yDistance);
    }

    /**
     * Stop animation.
     */
    public static void stop() {
        if (timerTask == null) {
            return;
        }
        timerTask.cancel();
        timerTask = null;
    }

    /**
     * Law of distance variation used to animate scrolling.
     * @param x argument
     * @return value
     */
    private static double f(double x) {
        return a * x * x * x  + b;
    }

    /**
     * Scrolling animation task.
     */
    private static class dragTimerTask extends TimerTask {
        public final void run() {
            if (yLeft <= 0) {
                int stableY = listener.dragContent((int)(ySign * yLeft));
                if (stableY != 0) {
                    if (type == AT_STABLE) {
                        listener.dragContent(stableY);
                        stop();
                    } else {
                        // we flicked out of stable position
                        initAnimation(AT_STABLE, stableY);
                    }
                } else {
                    stop();
                }
            } else {
                currStep++;
                int y = (int)(yLeft - f((stepCnt - currStep)/stepCnt * yDistance));
                listener.dragContent(ySign * y);
                yLeft -= y;
            }
        }
    }

}
