/*
 *   
 *
 * Copyright 2006 Sun Microsystems, Inc. All rights reserved. 
 * Use is subject to license terms
 */

package javax.microedition.io;

import java.util.Stack;

/**
 * Implements default path normalization, used for http, 
 * https and file protocols.
 */
class DefaultPathNormalizer implements PathNormalizer {
  public String normalize(String path) {
    if ("".equals(path)) {
      return "/";
    }

    // Remove Dot Segments, as specified by RFC 3986, section 5.2.4
    {
      Stack segments = new Stack();
      int p = 0;
      do {
        int q = path.indexOf("/", p + 1);
        if (q == -1) {
          q = path.length();
        }

        String segment = path.substring(p, q);
        if (".".equals(segment) || "..".equals(segment)) {
          // Skip following slash
          q++;
          segment = null;
        } else if ("/.".equals(segment) || "/..".equals(segment)) {
          if (!segments.empty() && "/..".equals(segment)) {
            segments.pop();
          }

          if (q < path.length()) {
            segment = null;
          } else {
            segment = "/";
          } 
        }

        if (segment != null) {
          segments.push(segment);
        }
        p = q;
      } while (p < path.length());

      String normalizedPath = "";

      while (!segments.empty()) {
        String s = (String)segments.pop();
        normalizedPath = s + normalizedPath;
      }

      return normalizedPath;
    }
  }
}
