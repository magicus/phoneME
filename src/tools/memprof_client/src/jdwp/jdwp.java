/*
 *   
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
 */


package com.sun.cldchi.tools.memoryprofiler.jdwp;

/**
 * This class contains JDWP constants and parameters.
 */

class jdwp {

	/** EventKind constant. */
    public static final int	evVM_INIT		= 0x5a;
	/** EventKind constant. */
    public static final int	evSINGLE_STEP	= 0x01;
	/** EventKind constant. */
    public static final int	evBREAKPOINT	= 0x02;
	/** EventKind constant. */
    public static final int	evEXCEPTION		= 0x04;
	/** EventKind constant. */
    public static final int	evCLASS_PREPARE	= 0x08;
	/** EventKind constant. */
    public static final int	evVM_DEATH		= 0x63;
	/** EventKind constant. */
    public static final int	evVM_METHOD_ENTRY	= 0x28;

    /** TypeTag constant. */
    public static final int	ttCLASS			= 0x01;
    /** TypeTag constant. */
    public static final int	ttINTERFACE		= 0x02;
    /** TypeTag constant. */
    public static final int	ttARRAY			= 0x03;

    /** Tag constant. */
    public static final int	tagARRAY		= 91;
    /** Tag constant. */
    public static final int	tagBYTE			= 66;
    /** Tag constant. */
    public static final int	tagOBJECT		= 76;
    /** Tag constant. */
    public static final int	tagINT			= 73;
    /** Tag constant. */
    public static final int	tagSHORT		= 83;
    /** Tag constant. */
    public static final int	tagVOID			= 86;
    /** Tag constant. */
    public static final int	tagBOOLEAN		= 90;
    /** Tag constant. */
    public static final int	tagLONG			= 74;
    /** Tag constant. */
    public static final int	tagSTRING	        = 115;
    /** Tag constant. */
    public static final int	tagCHAR 	        = 67;
    
    // VM-dependent sizes

	/** FieldID size in bytes. */
    public static int fieldIDSize			= 4;
	/** MethodID size in bytes. */
    public static int methodIDSize			= 4;
	/** ObjectID size in bytes. */
    public static int objectIDSize			= 4;
	/** ReferenceTypeID size in bytes. */
    public static int referenceTypeIDSize	        = 4;
	/** FrameID size in bytes. */
    public static int frameIDSize			= 4;
}
