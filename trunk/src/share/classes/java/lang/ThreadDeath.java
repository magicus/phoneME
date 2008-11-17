/*
 * @(#)ThreadDeath.java	1.20 06/10/10
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
 *
 */

package java.lang;

/**
 * An instance of <code>ThreadDeath</code> is thrown in the victim 
 * thread when the <code>stop</code> method with zero arguments in 
 * class <code>Thread</code> is called. 
 * <p>
 * An application should catch instances of this class only if it 
 * must clean up after being terminated asynchronously. If 
 * <code>ThreadDeath</code> is caught by a method, it is important 
 * that it be rethrown so that the thread actually dies. 
 * <p>
 * The top-level error handler does not print out a message if 
 * <code>ThreadDeath</code> is never caught. 
 * <p>
 * The class <code>ThreadDeath</code> is specifically a subclass of 
 * <code>Error</code> rather than <code>Exception</code>, even though 
 * it is a "normal occurrence", because many applications 
 * catch all occurrences of <code>Exception</code> and then discard 
 * the exception. 
 *
 * @author unascribed
 * @version 1.13, 05/03/00
 * @since   JDK1.0
 */

public class ThreadDeath extends Error {}
