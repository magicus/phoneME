/*
 * %W% %E%
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

package com.sun.jump.command;

/**
 * <code>JUMPResponse</code> encapsulates the response command
 */
public class JUMPResponse extends JUMPCommand {
   
    public static final String ID_SUCCESS      = "Success";
    public static final String ID_FAILURE      = "Failure";
    /**
     * Response that contains some data, which can be retrieved using
     * <code>JUMPCommand.getCommandData</code>
     */
    public static final String ID_DATA         = "Data";
    
    
    public static final JUMPResponse Success  = new JUMPResponse(ID_SUCCESS);
    public static final JUMPResponse Failure  = new JUMPResponse(ID_FAILURE);
    
    public static JUMPResponse newInstance(String id) {
        return new JUMPResponse(id, null);
    }
    
    public static JUMPResponse newInstance(String id, String[] args){
        return new JUMPResponse(id, args);
    }
    
    /**
     * Return an uninitialized <code>JUMPResponse</code>, which can later be
     * initialized by calling <code>JUMPCommand.deserialize()</code>
     */
    public static JUMPResponse newInstance() {
        return new JUMPResponse(null, null);
    }
    
    private JUMPResponse(String id){
        this(id, null);
    }
    
    private JUMPResponse(String id, String[] args){
        super(id, args);
    }
}
