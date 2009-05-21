/* 
* Copyright (c) 2008 Sun Microsystems, Inc. All rights reserved.
* 
* Use is subject to license terms.
*/

package com.sun.orientation;

public interface OrientationListener {

    /**
     * Handset states.
     * 
     *          bow roll
     *    -----------------
     *    !      !        !
     *   L!      !a       !R
     *   e!      !x       !i
     *   f!      !i       !g
     *   t!      !s       !h
     *    !      !!       !t
     * -----------y-------------- axis_x
     *    !      !        !
     *   b!      !        !b
     *   a!      !        !a
     *   n!      !        !n
     *   k!      !        !k
     *    !      !        !
     *    -----------------
     *      fodder bank
     */

    public static final int FODDER_BANK = 0;
    
    public static final int LEFT_BANK = 1;
    
    public static final int RIGHT_BANK = 2;

    public static final int BOW_ROLL = 3;
    
    /**
     * Calls when the orienation has changed.
     *
     * @param orientation the value
     */
    public void orientationChanged(int orientation);
}
