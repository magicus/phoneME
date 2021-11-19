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
 * The base exception for unexpected processing errors.  This Exception
 * class is used to report well-formedness errors as well as unexpected
 * processing conditions.
 * @version 1.0
 * @author Copyright (c) 2003 by BEA Systems. All Rights Reserved.
 */
public class XMLStreamException extends Exception {

  protected Throwable nested;
  protected Location location;
  
  /**
   * Default constructor
   */
  public XMLStreamException(){ 
    super();
  }

  /**
   * Construct an exception with the associated message.
   *
   * @param msg the message to report
   */
  public XMLStreamException(String msg) {
    super(msg);
  }

  /**
   * Construct an exception with the associated exception
   *
   * @param th a nested exception
   */
  public XMLStreamException(Throwable th) {
    nested = th;
  }

  /**
   * Construct an exception with the associated message and exception
   *
   * @param th a nested exception
   * @param msg the message to report
   */
  public XMLStreamException(String msg, Throwable th) {
    super(msg);
    nested = th;
  }

  /**
   * Construct an exception with the associated message, exception and location.
   *
   * @param th a nested exception
   * @param msg the message to report
   * @param location the location of the error 
   */
  public XMLStreamException(String msg, Location location, Throwable th) {
    super("ParseError at [row,col]:["+location.getLineNumber()+","+
          location.getColumnNumber()+"]\n"+
          "Message: "+msg);
    nested = th;
    this.location = location;
  }

  /**
   * Construct an exception with the associated message and location.
   *
   * @param msg the message to report
   * @param location the location of the error 
   */
  public XMLStreamException(String msg, 
                            Location location) {
    super("ParseError at [row,col]:["+location.getLineNumber()+","+
          location.getColumnNumber()+"]\n"+
          "Message: "+msg);
    this.location = location;
  }
  

  /**
   * Gets the nested exception.
   *
   * @return Nested exception
   */
  public Throwable getNestedException() {
    return nested;
  }

  /**
   * Gets the location of the exception
   *
   * @return the location of the exception, may be null if none is available
   */
  public Location getLocation() {
    return location;
  }

}






