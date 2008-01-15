/*
 *   
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved. 
 * Use is subject to license terms
 */

package javax.microedition.io;

/**
 * Implementors of this interface encapsulate scheme-specific normalization of
 * port range specification for an URI.
 */
interface PortRangeNormalizer {
  /**
   * Given the host and the original port range specification string from 
   * an URI, returns the port range and the string representing port range
   * normalized as defined in RFC 3986 and the defining specification for the
   * scheme.
   * <p>
   * If <code>host</code>, <code>portspec</code> or <code>portRange</code> is
   * <code>null</code>, the behavior is undefined. If <code>portRange</code>
   * is not an array of two elements, the behavior is undefined.
   *
   * @param host the host specification from an URI
   * @param portspec the port range specification from an URI
   * @param portRange array of two elements to store the port range
   *
   * @throws IllegalArgumentException if <code>portspec</code> is malformed.
   *
   * @return the normalized port range specification string or 
   * <code>null</code> if no scheme-specific normalization is applicable
   */
  String normalize(String host, String portspec, int[] portRange);

  /**
   * Given the port range parsed from an URI, returns a string representation 
   * of the port range normalized for this protocol.
   *
   * @param portRange array of length two specifying port range
   *
   * @return the normalized port range specification string or 
   * <code>null</code> if no scheme-specific normalization is applicable
   */
  String normalize(int[] portRange);
}
