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

package com.sun.cldc.i18n.j2me;

/**
 * Writer for UTF-16 encoded output streams (default byte order).
 * We assume that character strings
 * are correctly converted to UFT-16, so no additional checking is performed.
 */
 /* IMPL_NOTE: we need this class name because it is a part of
 * an internal interface: encoding name is converted into a class name,
 * and then Class.forName() is performed.
 */
public class UTF_16_Writer extends UTF_16BE_Writer {
    // Just extending the right class for the default byte order
    // To change the default encoding you need to change the base class
}



