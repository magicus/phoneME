/*
 *   
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved. 
 * Use is subject to license terms
 */

package javax.microedition.io;

/**
 * Implements port range normalization for datagram and socket protocols.
 */
class ServerModePortRangeNormalizer implements PortRangeNormalizer {
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
      if (host.length() == 0) {
        portRange[0] = 1024;
        portRange[1] = 65535;
        return ":1024-65535";
      } else {
        throw new IllegalArgumentException(
          "Port range spec must be specified for outbound connections"); 
      }
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
    return null;
  }
}
