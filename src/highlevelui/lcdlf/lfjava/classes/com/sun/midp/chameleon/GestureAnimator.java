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

import com.sun.midp.chameleon.skins.TickerSkin;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Utility class used to animate gestures
 */
public class GestureAnimator {
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
     * Animation recalculation rate
     */
    protected static final int rate = 100;
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
        stepCnt = 3;
        currStep = 0;
        if (stY > 0) {
            yDistance = yLeft = stY;
            ySign = 1;
        } else {
            yDistance = yLeft = -stY;
            ySign = -1;
        }
        a = (yDistance - b)/(yDistance * yDistance * yDistance);

        timerTask = new dragTimerTask();
        getTimer().schedule(timerTask, 0, rate);
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
        stepCnt = 7;
        currStep = 0;

        yLeft = (1.5 * dy - b) * stepCnt * stepCnt * stepCnt /
                (stepCnt - 1) / (stepCnt - 1) / (stepCnt - 1) + b;

        if (yLeft > 0) {
            yDistance = yLeft;
            ySign = 1;
        } else {
            yDistance = yLeft = -yLeft;
            ySign = -1;
        }
        a = (yDistance - b)/(yDistance * yDistance * yDistance);

        timerTask = new dragTimerTask();
        getTimer().schedule(timerTask, 0, rate);
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
                    listener.dragContent(stableY);
                }
                stop();
            } else {
                currStep++;
                int y = (int)(yLeft - f((stepCnt - currStep)/stepCnt * yDistance));
                listener.dragContent(ySign * y);
                yLeft -= y;
            }
        }
    }

}
