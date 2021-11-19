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

/** The interface <code>javax.xml.rpc.Stub</code> is the common base interface
 *  for the stub classes. All generated stub classes are required to 
 *  implement the <code>javax.xml.rpc.Stub</code> interface. An instance
 *  of a stub class represents a client side proxy or stub instance for
 *  the target service endpoint. 
 *
 *  <p>The <code>javax.xml.rpc.Stub</code> interface provides an 
 *  extensible property mechanism for the dynamic configuration of 
 *  a stub instance. 
 *
 *  @version   0.1
**/

public interface Stub {

  /** Standard property: User name for authentication.
   *  <p>Type: java.lang.String
  **/
  public static final String USERNAME_PROPERTY = 
                      "javax.xml.rpc.security.auth.username";
  
  /** Standard property: Password for authentication.
   *  <p>Type: java.lang.String
  **/
  public static final String PASSWORD_PROPERTY = 
                      "javax.xml.rpc.security.auth.password";
 
  /** Standard property: Target service endpoint address. The 
   *  URI scheme for the endpoint address specification must 
   *  correspond to the protocol/transport binding for this 
   *  stub class.
   *  <p>Type: java.lang.String
  **/
  public static final String ENDPOINT_ADDRESS_PROPERTY =  
                      "javax.xml.rpc.service.endpoint.address";

  /** Standard property: This boolean property is used by a service 
   *  client to indicate whether or not it wants to participate in
   *  a session with a service endpoint. If this property is set to
   *  true, the service client indicates that it wants the session
   *  to be maintained. If set to false, the session is not maintained. 
   *  The default value for this property is false.
   *  <p>Type: java.lang.Boolean
  **/
  public static final String SESSION_MAINTAIN_PROPERTY =
                      "javax.xml.rpc.session.maintain";


  /** Sets the name and value of a configuration property
   *  for this Stub instance. If the Stub instances contains
   *  a value of the same property, the old value is replaced.
   *  <p>Note that the <code>_setProperty</code> method may not 
   *  perform validity check on a configured property value. An 
   *  example is the standard property for the target service 
   *  endpoint address that is not checked for validity in the 
   *  <code>_setProperty</code> method.
   *  In this case, stub configuration errors are detected at 
   *  the remote method invocation.
   *
   *  @param name  Name of the configuration property
   *  @param value Value of the property
   *  @throws javax.xml.rpc.JAXRPCException
   *          <UL>
   *          <LI>If an optional standard property name is 
   *              specified, however this Stub implementation
   *              class does not support the configuration of
   *              this property.
   *          <LI>If an invalid or unsupported property name is
   *              specified or if a value of mismatched property
   *              type is passed. 
   *          <LI>If there is any error in the configuration of
   *              a valid property.
   *          </UL>
  **/
  public void _setProperty(String name, Object value);

  /** Gets the value of a specific configuration property.
   *  @param  name Name of the property whose value is to be
   *               retrieved
   *  @return Value of the configuration property
   *  @throws javax.xml.rpc.JAXRPCException if an invalid or 
   *          unsupported property name is passed.
  **/
  public Object _getProperty(String name);

}
