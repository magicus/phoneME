/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

package javax.xml.stream;

/**
 * @version 1.0
 * @author Copyright (c) 2003 by BEA Systems. All Rights Reserved.
 */

/**
 * This interface declares the constants used in this API.
 * Numbers in the range 0 to 256 are reserved for the specification,
 * user defined events must use event codes outside that range.
 */

public interface XMLStreamConstants {
  /**
   * Indicates an event is a start element
   */
  public static final int START_ELEMENT=1;
  /**
   * Indicates an event is an end element
   */
  public static final int END_ELEMENT=2;
  /**
   * Indicates an event is a processing instruction
   */
  public static final int PROCESSING_INSTRUCTION=3;

  /**
   * Indicates an event is characters
   */
  public static final int CHARACTERS=4;

  /**
   * Indicates an event is a comment
   */
  public static final int COMMENT=5;

  /**
   * The characters are white space
   * (see [XML], 2.10 "White Space Handling").
   * Events are only reported as SPACE if they are ignorable white
   * space.  Otherwise they are reported as CHARACTERS.
   */
  public static final int SPACE=6;

  /**
   * Indicates an event is a start document
   */
  public static final int START_DOCUMENT=7;

  /**
   * Indicates an event is an end document
   */
  public static final int END_DOCUMENT=8;

  /**
   * Indicates an event is an entity reference
   */
  public static final int ENTITY_REFERENCE=9;

  /**
   * Indicates an event is an attribute
   */
  public static final int ATTRIBUTE=10;

  /**
   * Indicates an event is a DTD
   */
  public static final int DTD=11;

  /**
   * Indicates an event is a CDATA section
   */
  public static final int CDATA=12;

  /**
   * Indicates the event is a namespace declaration
   */
  public static final int NAMESPACE=13;

  /**
   * Indicates a Notation 
   */
  public static final int NOTATION_DECLARATION=14;

  /**
   * Indicates a Entity Declaration
   */
  public static final int ENTITY_DECLARATION=15;
}

