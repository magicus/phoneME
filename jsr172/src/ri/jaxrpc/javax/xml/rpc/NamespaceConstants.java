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

/** Constants used in JAX-RPC for namespace prefixes and URIs
 *  @version 1.0
**/

public class NamespaceConstants {

  /** Namespace prefix for SOAP Envelope
  **/
  public static final String NSPREFIX_SOAP_ENVELOPE = "soapenv"; 

  /** Namespace prefix for XML schema XSD
  **/
  public static final String NSPREFIX_SCHEMA_XSD    = "xsd"; 

  /** Namespace prefix for XML Schema XSI
  **/
  public static final String NSPREFIX_SCHEMA_XSI    = "xsi"; 

  /** Nameapace URI for SOAP 1.1 Envelope
  **/
  public static final String NSURI_SOAP_ENVELOPE    = 
	    "http://schemas.xmlsoap.org/soap/envelope/";

  /** Nameapace URI for SOAP 1.1 Encoding
  **/  
  public static final String NSURI_SOAP_ENCODING    =
	    "http://schemas.xmlsoap.org/soap/encoding/";

  /** Nameapace URI for SOAP 1.1 next actor role
  **/
  public static final String NSURI_SOAP_NEXT_ACTOR  =
	    "http://schemas.xmlsoap.org/soap/actor/next";

  /** Namespace URI for XML Schema XSD
  **/
  public static final String NSURI_SCHEMA_XSD = 
            "http://www.w3.org/2001/XMLSchema";


  /** Namespace URI for XML Schema XSI
  **/
  public static final String NSURI_SCHEMA_XSI =
            "http://www.w3.org/2001/XMLSchema-instance";

}
