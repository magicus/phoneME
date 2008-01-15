/*
 *   
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved. 
 * Use is subject to license terms
 */

package javax.microedition.io;

/**
 * Implements port range normalization for schemes with default port defined.
 */
class DefaultPortRangeNormalizer implements PortRangeNormalizer {
  private final int defaultPort;

  public DefaultPortRangeNormalizer(int port) {
    defaultPort = port;
  }

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
   * @param portspec the host specification from an URI
   * @param portspec the port range specification from an URI
   * @param portRange array of length two to store the port range
   *
   * @return the normalized port range specification string or 
   * <code>null</code> if no scheme-specific normalization is applicable
   */
  public String normalize(String host, String portspec, int[] portRange) {
    if (portspec.length() == 0 || ":".equals(portspec)) {
      portRange[0] = defaultPort;
      portRange[1] = defaultPort;
      return "";
    }

    return null;
  }

  /**
   * Given the port range parsed from an URI, returns a string representation 
   * of the port range normalized for this protocol.
   *
   * @param portRange array of length two specifying port range
   *
   * @return the normalized port range specification string or 
   * <code>null</code> if no scheme-specific normalization is applicable
   */
  public String normalize(int[] portRange) {
    if (portRange[0] == defaultPort && portRange[1] == defaultPort) {
      return "";
    }

    return null;
  }
}
