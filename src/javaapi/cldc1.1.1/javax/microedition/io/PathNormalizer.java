/*
 *   
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved. 
 * Use is subject to license terms
 */

package javax.microedition.io;

/**
 * Implementors of this interface encapsulate scheme-specific normalization of
 * path component for an URI.
 */
interface PathNormalizer {
  /**
   * Given the original path component from an URI, returns the path
   * normalized as defined in RFC 3986 and the defining specification for the
   * scheme.
   *
   * @param path the path component from an URI
   *
   * @throws IllegalArgumentException if <code>path</code> is malformed.
   *
   * @return the normalized path component
   */
  String normalize(String path);
}
