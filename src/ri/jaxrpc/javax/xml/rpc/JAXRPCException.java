/*
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

package javax.xml.rpc;

/** The <code>javax.xml.rpc.JAXRPCException</code> is thrown from 
 *  the core JAX-RPC APIs to indicate an exception related to the 
 *  JAX-RPC runtime mechanisms.
 *
 *  @version 1.0
**/

public class JAXRPCException extends java.lang.RuntimeException {

  /** The Throwable associated with this rpc exception */
  private Throwable cause;

  /** Constructs a new exception with <code>null</code> as its 
   *  detail message. The cause is not initialized.
  **/
  public JAXRPCException() { 
    super();
  }

  /** Constructs a new exception with the specified detail 
   *  message.  The cause is not initialized.
   *  @param message The detail message which is later 
   *                 retrieved using the getMessage method
  **/
  public JAXRPCException(String message) {
    super(message);
  }


  /** Constructs a new exception with the specified detail 
   *  message and cause.
   *
   *  @param message The detail message which is later retrieved
   *                 using the getMessage method
   *  @param cause   The cause which is saved for the later
   *                 retrieval throw by the getLinkedCause method 
  **/ 
  public JAXRPCException(String message, Throwable cause) {
    super(message);
    this.cause = cause;
  }

  /** Constructs a new JAXRPCException with the specified cause
   *  and a detail message of <tt>cause.toString()</tt> (if cause
   *  is non-null) which typically contains the 
   *  class and detail message of <tt>cause</tt>.
   *
   *  @param cause   The cause which is saved for the later
   *                 retrieval throw by the getLinkedCause method.
   *                 (A <tt>null</tt> value is permitted, and
   *                 indicates that the cause is nonexistent or
     *               unknown.)
  **/ 
  public JAXRPCException(Throwable cause) {
    super(cause==null ? null : cause.toString());
    this.cause = cause;
  }

  /** Gets the Linked cause
   * 
   *  @return The cause of this Exception or <code>null</code>
   *          if the cause is noexistent or unknown
  **/
  public Throwable getLinkedCause() {
    return this.cause;
  }

}
