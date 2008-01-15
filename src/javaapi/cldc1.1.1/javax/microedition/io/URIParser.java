/*
 *   
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.io;

class URIParser {
  // Character-class masks, in reverse order from RFC2396 because
  // initializers for static fields cannot make forward references.
  
  // digit    = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
  //            "8" | "9"
  private static final long L_DIGIT = lowMask('0', '9');
  private static final long H_DIGIT = 0L;

  // upalpha  = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
  //            "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
  //            "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
  private static final long L_UPALPHA = 0L;
  private static final long H_UPALPHA = highMask('A', 'Z');

  // lowalpha = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
  //            "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
  //            "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
  private static final long L_LOWALPHA = 0L;
  private static final long H_LOWALPHA = highMask('a', 'z');

  // alpha         = lowalpha | upalpha
  private static final long L_ALPHA = L_LOWALPHA | L_UPALPHA;
  private static final long H_ALPHA = H_LOWALPHA | H_UPALPHA;

  // alphanum      = alpha | digit
  private static final long L_ALPHANUM = L_DIGIT | L_ALPHA;
  private static final long H_ALPHANUM = H_DIGIT | H_ALPHA;

  // hex           = digit | "A" | "B" | "C" | "D" | "E" | "F" |
  //                         "a" | "b" | "c" | "d" | "e" | "f"
  private static final long L_HEX = L_DIGIT;
  private static final long H_HEX = highMask('A', 'F') | highMask('a', 'f');

  // mark          = "-" | "_" | "." | "!" | "~" | "*" | "'" |
  //                 "(" | ")"
  private static final long L_MARK = lowMask("-_.!~*'()");
  private static final long H_MARK = highMask("-_.!~*'()");

  // unreserved    = alphanum | mark
  private static final long L_UNRESERVED = L_ALPHANUM | L_MARK;
  private static final long H_UNRESERVED = H_ALPHANUM | H_MARK;

  // reserved      = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
  //                 "$" | "," | "[" | "]"
  // Added per RFC2732: "[", "]"
  private static final long L_RESERVED = lowMask(";/?:@&=+$,[]");
  private static final long H_RESERVED = highMask(";/?:@&=+$,[]");

  // The zero'th bit is used to indicate that escape pairs and non-US-ASCII
  // characters are allowed; this is handled by the scanEscape method below.
  private static final long L_ESCAPED = 1L;
  private static final long H_ESCAPED = 0L;

  // uric          = reserved | unreserved | escaped
  private static final long L_URIC = L_RESERVED | L_UNRESERVED | L_ESCAPED;
  private static final long H_URIC = H_RESERVED | H_UNRESERVED | H_ESCAPED;
  
  // pchar         = unreserved | escaped |
  //                 ":" | "@" | "&" | "=" | "+" | "$" | ","
  private static final long L_PCHAR
    = L_UNRESERVED | L_ESCAPED | lowMask(":@&=+$,");
  private static final long H_PCHAR
    = H_UNRESERVED | H_ESCAPED | highMask(":@&=+$,");

  // All valid path characters
  private static final long L_PATH = L_PCHAR | lowMask(";/");
  private static final long H_PATH = H_PCHAR | highMask(";/");

  // Dash, for use in domainlabel and toplabel
  private static final long L_DASH = lowMask("-");
  private static final long H_DASH = highMask("-");

  // Dot, for use in hostnames
  private static final long L_DOT = lowMask(".");
  private static final long H_DOT = highMask(".");

  // Asterisk, for use in wildcard matches
  private static final long L_ASTERISK = lowMask("*");
  private static final long H_ASTERISK = highMask("*");

  // userinfo      = *( unreserved | escaped |
  //                    ";" | ":" | "&" | "=" | "+" | "$" | "," )
  private static final long L_USERINFO
    = L_UNRESERVED | L_ESCAPED | lowMask(";:&=+$,");
  private static final long H_USERINFO
    = H_UNRESERVED | H_ESCAPED | highMask(";:&=+$,");

  // reg_name      = 1*( unreserved | escaped | "$" | "," |
  //                     ";" | ":" | "@" | "&" | "=" | "+" )
  private static final long L_REG_NAME
    = L_UNRESERVED | L_ESCAPED | lowMask("$,;:@&=+");
  private static final long H_REG_NAME
    = H_UNRESERVED | H_ESCAPED | highMask("$,;:@&=+");

  // All valid characters for server-based authorities
  private static final long L_SERVER
    = L_USERINFO | L_ALPHANUM | L_DASH | lowMask(".:@[]");
  private static final long H_SERVER
    = H_USERINFO | H_ALPHANUM | H_DASH | highMask(".:@[]");

  // scheme        = alpha *( alpha | digit | "+" | "-" | "." )
  private static final long L_SCHEME = L_ALPHA | L_DIGIT | lowMask("+-.");
  private static final long H_SCHEME = H_ALPHA | H_DIGIT | highMask("+-.");

  // uric_no_slash = unreserved | escaped | ";" | "?" | ":" | "@" |
  //                 "&" | "=" | "+" | "$" | ","
  private static final long L_URIC_NO_SLASH
    = L_UNRESERVED | L_ESCAPED | lowMask(";?:@&=+$,");
  private static final long H_URIC_NO_SLASH
    = H_UNRESERVED | H_ESCAPED | highMask(";?:@&=+$,");

  private static final int PORT_MIN = 0;
  private static final int PORT_MAX = 65535;
  private static final int[] ALL_PORTS = new int[] { PORT_MIN, PORT_MAX };

  // Components of all URIs: [<scheme>:]<scheme-specific-part>[#<fragment>]
  private String scheme;		// null ==> relative URI
  private String fragment;

  // Hierarchical URI components: [//<authority>]<path>[?<query>]
  private String authority;		// Registry or server

  private String userInfo;
  private String host;		// null ==> registry-based
  private String canonicalHost;
  private int[] portrange = ALL_PORTS;
  private String portrangeString;

  // Remaining components of hierarchical URIs
  private String path;		// null ==> opaque
  private String query;

  private String schemeSpecificPart;

  private String input;		// URI input string
  private boolean requireServerAuthority = false;

  URIParser(String s, boolean rsa) {
    input = s;
    parse(rsa);
  }

  // -- Field accessor methods --

  public String getScheme() {
    return scheme;
  }
  public String getFragment() {
    return fragment;
  }

  // Hierarchical URI components: [//<authority>]<path>[?<query>]

  // Registry or server
  public String getAuthority() {
    return authority;
  }

  public String getUserInfo() {
    return userInfo;
  }

  // null ==> registry-based
  public String getHost() {
    return host;
  }

  public String getCanonicalHostName() {
    return canonicalHost;
  }

  public int[] getPortRange() {
    return portrange;
  }

  // Remaining components of hierarchical URIs
  // null ==> opaque
  public String getPath() {
    return path;		
  }
  public String getQuery() {
    return query;
  }

  public String getSchemeSpecificPart() {
    return schemeSpecificPart;
  }

  // -- Methods for throwing IllegalArgumentException in various ways --

  private void fail(String reason) throws IllegalArgumentException {
    throw new IllegalArgumentException(reason + " : " + input);
  }

  private void fail(String reason, int p) throws IllegalArgumentException {
    throw new IllegalArgumentException(reason + " : " + input);
  }

  private void failExpecting(String expected, int p) 
   throws IllegalArgumentException {
    fail("Expected " + expected, p);
  }
  
  private void failExpecting(String expected, String prior, int p)
   throws IllegalArgumentException {
    fail("Expected " + expected + " following " + prior, p);
  }
  

  // -- Simple access to the input string --

  // Return a substring of the input string
  //
  private String substring(int start, int end) {
    return input.substring(start, end);
  }
  
  // Return the char at position p,
  // assuming that p < input.length()
  //
  private char charAt(int p) {
    return input.charAt(p);
  }
  
  // Tells whether start < end and, if so, whether charAt(start) == c
  //
  private boolean at(int start, int end, char c) {
    return (start < end) && (charAt(start) == c);
  }
  
  // Tells whether start + s.length() < end and, if so,
  // whether the chars at the start position match s exactly
  //
  private boolean at(int start, int end, String s) {
    int p = start;
    int sn = s.length();
    if (sn > end - p)
      return false;
    int i = 0;
    while (i < sn) {
      if (charAt(p++) != s.charAt(i)) {
        break;
      }
      i++;
    }
    return (i == sn);
  }
  
  // -- Scanning --

  // The various scan and parse methods that follow use a uniform
  // convention of taking the current start position and end index as
  // their first two arguments.  The start is inclusive while the end is
  // exclusive, just as in the String class, i.e., a start/end pair
  // denotes the left-open interval [start, end) of the input string.
  //
  // These methods never proceed past the end position.  They may return
  // -1 to indicate outright failure, but more often they simply return
  // the position of the first char after the last char scanned.  Thus
  // a typical idiom is
  //
  //     int p = start;
  //     int q = scan(p, end, ...);
  //     if (q > p)
  //         // We scanned something
  //         ...;
  //     else if (q == p)
  //         // We scanned nothing
  //         ...;
  //     else if (q == -1)
  //         // Something went wrong
  //         ...;


  // Scan a specific char: If the char at the given start position is
  // equal to c, return the index of the next char; otherwise, return the
  // start position.
  //
  private int scan(int start, int end, char c) {
    if ((start < end) && (charAt(start) == c))
      return start + 1;
    return start;
  }

  // Scan forward from the given start position.  Stop at the first char
  // in the err string (in which case -1 is returned), or the first char
  // in the stop string (in which case the index of the preceding char is
  // returned), or the end of the input string (in which case the length
  // of the input string is returned).  May return the start position if
  // nothing matches.
  //
  private int scan(int start, int end, String err, String stop) {
    int p = start;
    while (p < end) {
      char c = charAt(p);
      if (err.indexOf(c) >= 0)
        return -1;
      if (stop.indexOf(c) >= 0)
        break;
      p++;
    }
    return p;
  }
  
  // Scan a potential escape sequence, starting at the given position,
  // with the given first char (i.e., charAt(start) == c).
  //
  // This method assumes that if escapes are allowed then visible
  // non-US-ASCII chars are also allowed.
  //
  private int scanEscape(int start, int n, char first)
   throws IllegalArgumentException {
    int p = start;
    char c = first;
    if (c == '%') {
      // Process escape pair
      if ((p + 3 <= n)
          && match(charAt(p + 1), L_HEX, H_HEX)
          && match(charAt(p + 2), L_HEX, H_HEX)) {
        return p + 3;
      }
      fail("Malformed escape pair", p);
/*
 *   } else if ((c > 128)
 *              && !Character.isSpaceChar(c)
 *              && !Character.isISOControl(c)) {
 *     // Allow unescaped but visible non-US-ASCII chars
 *     return p + 1;
 */
    }
    return p;
  }
  
  // Scan chars that match the given mask pair
  //
  private int scan(int start, int n, long lowMask, long highMask)
   throws IllegalArgumentException {
    int p = start;
    while (p < n) {
      char c = charAt(p);
      if (match(c, lowMask, highMask)) {
        p++;
        continue;
      }
      if ((lowMask & L_ESCAPED) != 0) {
        int q = scanEscape(p, n, c);
        if (q > p) {
          p = q;
          continue;
        }
      }
      break;
    }
    return p;
  }
  
  // Check that each of the chars in [start, end) matches the given mask
  //
  private void checkChars(int start, int end, long lowMask, long highMask,
                          String what)
   throws IllegalArgumentException {
    int p = scan(start, end, lowMask, highMask);
    if (p < end)
      fail("Illegal character in " + what, p);
  }

  // Check that the char at position p matches the given mask
  //
  private void checkChar(int p, long lowMask, long highMask, String what)
   throws IllegalArgumentException {
    checkChars(p, p + 1, lowMask, highMask, what);
  }

  // -- Parsing --

  // [<scheme>:]<scheme-specific-part>[#<fragment>]
  //
  private void parse(boolean rsa) throws IllegalArgumentException {
    requireServerAuthority = rsa;
    int ssp;			// Start of scheme-specific part
    int n = input.length();
    int p = scan(0, n, "/?#", ":");
    if ((p >= 0) && at(p, n, ':')) {
      if (p == 0)
        failExpecting("scheme name", 0);
      checkChar(0, L_ALPHA, H_ALPHA, "scheme name");
      checkChars(1, p, L_SCHEME, H_SCHEME, "scheme name");
      scheme = substring(0, p);
      p++;			// Skip ':'
      ssp = p;
      if (at(p, n, '/')) {
        p = parseHierarchical(p, n);
      } else {
        int q = scan(p, n, "", "#");
        if (q <= p)
          failExpecting("scheme-specific part", p);
        checkChars(p, q, L_URIC, H_URIC, "opaque part");
        p = q;
      }
    } else {
      ssp = 0;
      p = parseHierarchical(0, n);
    }
    schemeSpecificPart = substring(ssp, p);
    if (at(p, n, '#')) {
      checkChars(p + 1, n, L_URIC, H_URIC, "fragment");
      fragment = substring(p + 1, n);
      p = n;
    }
    if (p < n)
      fail("end of URI", p);
  }

  // [//authority]<path>[?<query>]
  //
  // DEVIATION from RFC2396: We allow an empty authority component as
  // long as it's followed by a non-empty path, query component, or
  // fragment component.  This is so that URIs such as "file:///foo/bar"
  // will parse.  This seems to be the intent of RFC2396, though the
  // grammar does not permit it.  If the authority is empty then the
  // userInfo, host, and port components are undefined.
  //
  // DEVIATION from RFC2396: We allow empty relative paths.  This seems
  // to be the intent of RFC2396, but the grammar does not permit it.
  // The primary consequence of this deviation is that "#f" parses as a
  // relative URI with an empty path.
  //
  private int parseHierarchical(int start, int n)
   throws IllegalArgumentException {
    int p = start;
    if (at(p, n, '/') && at(p + 1, n, '/')) {
      p += 2;
      int q = scan(p, n, "", "/?#");
      p = parseAuthority(p, q);
    }
    int q = scan(p, n, "", "?#"); // DEVIATION: May be empty
    checkChars(p, q, L_PATH, H_PATH, "path");
    path = substring(p, q);
    p = q;
    if (at(p, n, '?')) {
      p++;
      q = scan(p, n, "", "#");
      checkChars(p, q, L_URIC, H_URIC, "query");
      query = substring(p, q);
      p = q;
    }
    return p;
  }

  // authority     = server | reg_name
  //
  // Ambiguity: An authority that is a registry name rather than a server
  // might have a prefix that parses as a server.  We use the fact that
  // the authority component is always followed by '/' or the end of the
  // input string to resolve this: If the complete authority did not
  // parse as a server then we try to parse it as a registry name.
  //
  private int parseAuthority(int start, int n)
   throws IllegalArgumentException {
    int p = start;
    int q = p;
    IllegalArgumentException ex = null;
    
    boolean serverChars = (scan(p, n, L_SERVER, H_SERVER) == n);
    boolean regChars = (scan(p, n, L_REG_NAME, H_REG_NAME) == n);
    
    if (regChars && !serverChars) {
      // Must be a registry-based authority
      authority = substring(p, n);
      return n;
    }
    
    if (serverChars) {
      // Might be (probably is) a server-based authority, so attempt
      // to parse it as such.  If the attempt fails, try to treat it
      // as a registry-based authority.
      try {
        q = parseServer(p, n);
        if (q < n)
          failExpecting("end of authority", q);
        authority = substring(p, n);
      } catch (IllegalArgumentException x) {
        // Undo results of failed parse
        userInfo = null;
        host = null;
        canonicalHost = null;
        portrange = ALL_PORTS;
        portrangeString = null;
        if (requireServerAuthority) {
          // If we're insisting upon a server-based authority,
          // then just re-throw the exception
          throw x;
        } else {
          // Save the exception in case it doesn't parse as a
          // registry either
          ex = x;
          q = p;
        }
      }
    }
    
    if (q < n) {
      if (regChars) {
        // Registry-based authority
        authority = substring(p, n);
      } else if (ex != null) {
        // Re-throw exception; it was probably due to
        // a malformed IPv6 address
        throw ex;
      } else {
        fail("Illegal character in authority", q);
      }
    }
    
    return n;
  }
  
  
  // [<userinfo>@]<host>[:<portrange>]
  //
  private int parseServer(int start, int n) throws IllegalArgumentException {
    int p = start;
    int q;
      
    // userinfo
    q = scan(p, n, "/?#", "@");
    if ((q >= p) && at(q, n, '@')) {
      checkChars(p, q, L_USERINFO, H_USERINFO, "user info");
      userInfo = substring(p, q);
      p = q + 1;		// Skip '@'
    }
    
    // hostname, IPv4 address, or IPv6 address
    if (at(p, n, '[')) {
      // DEVIATION from RFC2396: Support IPv6 addresses, per RFC2732
      p++;
      q = scan(p, n, "/?#", "]");
      if ((q > p) && at(q, n, ']')) {
        parseIPv6Reference(p, q);
        p = q + 1;
      } else {
        failExpecting("closing bracket for IPv6 address", q);
      }
    } else {
      q = parseIPv4Address(p, n);
      if (q <= p)
        q = parseHostname(p, n);
      p = q;
    }

    // port
    if (at(p, n, ':')) {
      p++;
      q = scan(p, n, "", "/");
      if (q > p) {
        p = parsePortRange(p, q);
      } else {
        failExpecting("port number", p);
      }
    }
    if (p < n)
      failExpecting("port number", p);
    
    return p;
  }

  // Scan a string of decimal digits whose value fits in a byte
  //
  private int scanByte(int start, int n) throws IllegalArgumentException {
    int p = start;
    int q = scan(p, n, L_DIGIT, H_DIGIT);
    if (q <= p) return q;
    if (Integer.parseInt(substring(p, q)) > 255) return p;
    return q;
  }

  // Scan an IPv4 address.
  //
  // If the strict argument is true then we require that the given
  // interval contain nothing besides an IPv4 address; if it is false
  // then we only require that it start with an IPv4 address.
  //
  // If the interval does not contain or start with (depending upon the
  // strict argument) a legal IPv4 address characters then we return -1
  // immediately; otherwise we insist that these characters parse as a
  // legal IPv4 address and throw an exception on failure.
  //
  // We assume that any string of decimal digits and dots must be an IPv4
  // address.  It won't parse as a hostname anyway, so making that
  // assumption here allows more meaningful exceptions to be thrown.
  //
  private int scanIPv4Address(int start, int n, boolean strict)
   throws IllegalArgumentException {
    int p = start;
    int q;
    int m = scan(p, n, L_DIGIT | L_DOT, H_DIGIT | H_DOT);
    if ((m <= p) || (strict && (m != n)))
      return -1;
    for (;;) {
      // Per RFC2732: At most three digits per byte
      // Further constraint: Each element fits in a byte
      if ((q = scanByte(p, m)) <= p) break;   p = q;
      if ((q = scan(p, m, '.')) <= p) break;  p = q;
      if ((q = scanByte(p, m)) <= p) break;   p = q;
      if ((q = scan(p, m, '.')) <= p) break;  p = q;
      if ((q = scanByte(p, m)) <= p) break;   p = q;
      if ((q = scan(p, m, '.')) <= p) break;  p = q;
      if ((q = scanByte(p, m)) <= p) break;   p = q;
      if (q < m) break;
      return q;
    }
    fail("Malformed IPv4 address", q);
    return -1;
  }
  
  // Take an IPv4 address: Throw an exception if the given interval
  // contains anything except an IPv4 address
  //
  private int takeIPv4Address(int start, int n, String expected)
   throws IllegalArgumentException {
    int p = scanIPv4Address(start, n, true);
    if (p <= start)
      failExpecting(expected, start);
    return p;
  }
  
  private String canonicalIPv4Name(String name) {
    try {
      byte[] address = textToNumericFormatIPv4(name);
      return numericToTextFormatIPv4(address);
    } catch (RuntimeException e) {
      throw new IllegalArgumentException("Malformed IPv4 address");
    }
  }

  // Attempt to parse an IPv4 address, returning -1 on failure but
  // allowing the given interval to contain [:<characters>] after
  // the IPv4 address.
  //
  private int parseIPv4Address(int start, int n) {
    int p;
    
    try {
      p = scanIPv4Address(start, n, false);
    } catch (IllegalArgumentException x) {
      return -1;
    }
    
    if (p > start && p < n) {
      // IPv4 address is followed by something - check that
      // it's a ":" as this is the only valid character to
      // follow an address.
      if (charAt(p) != ':') {
        p = -1;
      }
    }
    
    if (p > start) {
      host = substring(start, p);
      canonicalHost = canonicalIPv4Name(host);
    }
    
    return p;
  }

  private String canonicalHostname(String name) {
    String canonical = name.toLowerCase();

    if (canonical.startsWith("*")) {
      canonical = canonical.substring(1);
    }

    return canonical;
  }


  // The wildcard "*" may be included once in a DNS name host specification.
  // If it is included, it must be in the left most position.
  // The host name can be empty that indicates server connection.
  //
  // whostname   = 
  //      "" | hostname | "*" [ *( "." domainlabel ) "." toplabel ] [ "." ]
  // 
  // hostname    = domainlabel [ "." ] | 1*( domainlabel "." ) toplabel [ "." ]
  // domainlabel = alphanum | alphanum *( alphanum | "-" ) alphanum
  // toplabel    = alpha | alpha *( alphanum | "-" ) alphanum
  private int parseHostname(int start, int n)
   throws IllegalArgumentException {
    int p = start;
    int q;
    int l = -1;			// Start of last parsed label
    
    // The leftmost label can be an asterisk
    if (at(p, n, '*')) {
      l = p;
      p++;
      q = scan(p, n, L_ALPHANUM | L_DASH, H_ALPHANUM | H_DASH);
      if (q > p) {
        fail("Illegal character in hostname", p);
      }
      p = scan(p, n, '.');
    }

    do {
      // domainlabel = alphanum [ *( alphanum | "-" ) alphanum ]
      q = scan(p, n, L_ALPHANUM, H_ALPHANUM);
      if (q <= p)
        break;
      l = p;
      if (q > p) {
        p = q;
        q = scan(p, n, L_ALPHANUM | L_DASH, H_ALPHANUM | H_DASH);
        if (q > p) {
          if (charAt(q - 1) == '-')
            fail("Illegal character in hostname", q - 1);
          p = q;
        }
      }
      q = scan(p, n, '.');
      if (q <= p)
        break;
      p = q;
    } while (p < n);
    
    if ((p < n) && !at(p, n, ':'))
      fail("Illegal character in hostname", p);
    
    // for a fully qualified hostname check that the rightmost
    // label starts with an alpha character.
    if (l > start && !match(charAt(l), L_ALPHA, H_ALPHA)) {
      fail("Illegal character in hostname", l);
    }
    
    host = substring(start, p);
    canonicalHost = canonicalHostname(host);
    return p;
  }
  

  private String canonicalIPv6Name(String name) {
    try {
      byte[] address = textToNumericFormatIPv6(name);
      return numericToTextFormatIPv6(address);
    } catch (RuntimeException e) {
      throw new IllegalArgumentException("Malformed IPv6 address");
    }
  }
  
  // IPv6 address parsing, from RFC2373: IPv6 Addressing Architecture
  //
  // Bug: The grammar in RFC2373 Appendix B does not allow addresses of
  // the form ::12.34.56.78, which are clearly shown in the examples
  // earlier in the document.  Here is the original grammar:
  //
  //   IPv6address = hexpart [ ":" IPv4address ]
  //   hexpart     = hexseq | hexseq "::" [ hexseq ] | "::" [ hexseq ]
  //   hexseq      = hex4 *( ":" hex4)
  //   hex4        = 1*4HEXDIG
  //
  // We therefore use the following revised grammar:
  //
  //   IPv6address = hexseq [ ":" IPv4address ]
  //                 | hexseq [ "::" [ hexpost ] ]
  //                 | "::" [ hexpost ]
  //   hexpost     = hexseq | hexseq ":" IPv4address | IPv4address
  //   hexseq      = hex4 *( ":" hex4)
  //   hex4        = 1*4HEXDIG
  //
  // This covers all and only the following cases:
  //
  //   hexseq
  //   hexseq : IPv4address
  //   hexseq ::
  //   hexseq :: hexseq
  //   hexseq :: hexseq : IPv4address
  //   hexseq :: IPv4address
  //   :: hexseq
  //   :: hexseq : IPv4address
  //   :: IPv4address
  //   ::
  //
  // Additionally we constrain the IPv6 address as follows :-
  //
  //  i.  IPv6 addresses without compressed zeros should contain
  //      exactly 16 bytes.
  //
  //  ii. IPv6 addresses with compressed zeros should contain
  //      less than 16 bytes.
  
  private int ipv6byteCount = 0;
  
  private int parseIPv6Reference(int start, int n)
   throws IllegalArgumentException {
    int p = start;
    int q;
    boolean compressedZeros = false;
    
    q = scanHexSeq(p, n);
    
    if (q > p) {
      p = q;
      if (at(p, n, "::")) {
        compressedZeros = true;
        p = scanHexPost(p + 2, n);
      } else if (at(p, n, ':')) {
        p = takeIPv4Address(p + 1,  n, "IPv4 address");
        ipv6byteCount += 4;
      }
    } else if (at(p, n, "::")) {
      compressedZeros = true;
      p = scanHexPost(p + 2, n);
    }
    if (p < n)
      fail("Malformed IPv6 address", start);
    if (ipv6byteCount > 16)
      fail("IPv6 address too long", start);
    if (!compressedZeros && ipv6byteCount < 16) 
      fail("IPv6 address too short", start);
    if (compressedZeros && ipv6byteCount == 16)
      fail("Malformed IPv6 address", start);
    
    host = substring(start-1, p+1);
    canonicalHost = canonicalIPv6Name(substring(start, p));
    return p;
  }
  
  private int scanHexPost(int start, int n)
   throws IllegalArgumentException {
    int p = start;
    int q;

    if (p == n)
      return p;

    q = scanHexSeq(p, n);
    if (q > p) {
      p = q;
      if (at(p, n, ':')) {
        p++;
        p = takeIPv4Address(p, n, "hex digits or IPv4 address");
        ipv6byteCount += 4;
      }
    } else {
      p = takeIPv4Address(p, n, "hex digits or IPv4 address");
      ipv6byteCount += 4;
    }
    return p;
  }

  // Scan a hex sequence; return -1 if one could not be scanned
  //
  private int scanHexSeq(int start, int n) throws IllegalArgumentException {
    int p = start;
    int q;

    q = scan(p, n, L_HEX, H_HEX);
    if (q <= p)
      return -1;
    if (at(q, n, '.'))		// Beginning of IPv4 address
      return -1;
    if (q > p + 4)
      fail("IPv6 hexadecimal digit sequence too long", p);
    ipv6byteCount += 2;
    p = q;
    while (p < n) {
      if (!at(p, n, ':'))
        break;
      if (at(p + 1, n, ':'))
        break;		// "::"
      p++;
      q = scan(p, n, L_HEX, H_HEX);
      if (q <= p)
        failExpecting("digits for an IPv6 address", p);
      if (at(q, n, '.')) {	// Beginning of IPv4 address
        p--;
        break;
      }
      if (q > p + 4)
        fail("IPv6 hexadecimal digit sequence too long", p);
      ipv6byteCount += 2;
      p = q;
    }
    
    return p;
  }

  // Parse port range:
  // portrange = portnumber | -portnumber | portnumber-[portnumber]
  private int parsePortRange(int start, int n) {
    int p = start;
    int q = scan(p, n, "", "-");
    int low = PORT_MIN;
    int high = PORT_MAX;

    try {
      if (q > p) {
        low = Integer.parseInt(substring(p, q));
        if (q >= n) {
          high = low;
        }
      } 
      
      if (q + 1 < n) {
        high = Integer.parseInt(substring(q + 1, n));
      }
    } catch (NumberFormatException x) {
      fail("Malformed port range", p);
    }

    if (low < 0 || high < 0 || high < low) { 
      fail("Invalid port range", p);
    }

    portrange = new int[] {low, high};
    portrangeString = substring(start, n);

    return n;
  }

  void checkNoFragment() {
    if (fragment != null && !"".equals(fragment)) {
      fail("Fragment component not allowed");
    }
  }

  void checkNoUserInfo() {
    if (userInfo != null && !"".equals(userInfo)) {
      fail("Userinfo component not allowed");
    }
  }

  void checkNoPath() {
    if (path != null && !"".equals(path)) {
      fail("Path component not allowed");
    }
  }

  void checkNoQuery() {
    if (query != null && !"".equals(query)) {
      fail("Query component not allowed");
    }
  }

  void checkNoPortRange() {
    if (portrangeString != null && !"".equals(portrangeString)) {
      fail("Port range component not allowed");
    }
  }
  
  void checkNoHost() {
    if (host != null && !"".equals(host)) {
      fail("Host component not allowed");
    }
  }
  
  // Compute the low-order mask for the characters in the given string
  private static long lowMask(String chars) {
    int n = chars.length();
    long m = 0;
    for (int i = 0; i < n; i++) {
      char c = chars.charAt(i);
      if (c < 64)
        m |= (1L << c);
    }
    return m;
  }

  // Compute the high-order mask for the characters in the given string
  private static long highMask(String chars) {
    int n = chars.length();
    long m = 0;
    for (int i = 0; i < n; i++) {
      char c = chars.charAt(i);
      if ((c >= 64) && (c < 128))
        m |= (1L << (c - 64));
    }
    return m;
  }

  // Compute a low-order mask for the characters
  // between first and last, inclusive
  private static long lowMask(char first, char last) {
    long m = 0;
    int f = Math.max(Math.min(first, 63), 0);
    int l = Math.max(Math.min(last, 63), 0);
    for (int i = f; i <= l; i++)
      m |= 1L << i;
    return m;
  }

  // Compute a high-order mask for the characters
  // between first and last, inclusive
  private static long highMask(char first, char last) {
    long m = 0;
    int f = Math.max(Math.min(first, 127), 64) - 64;
    int l = Math.max(Math.min(last, 127), 64) - 64;
    for (int i = f; i <= l; i++)
      m |= 1L << i;
    return m;
  }

  // Tell whether the given character is permitted by the given mask pair
  private static boolean match(char c, long lowMask, long highMask) {
    if (c < 64)
      return ((1L << c) & lowMask) != 0;
    if (c < 128)
      return ((1L << (c - 64)) & highMask) != 0;
    return false;
  }

  private final static int IN4ADDRSZ = 4;

  /* 
   * Converts IPv4 binary address into a string suitable for presentation.
   *
   * @param src a byte array representing an IPv4 numeric address
   * @return a String representing the IPv4 address in 
   *         textual representation format
   */
  private static String numericToTextFormatIPv4(byte[] src) {
    return (src[0] & 0xff) + "." + (src[1] & 0xff) + "." + 
      (src[2] & 0xff) + "." + (src[3] & 0xff);
  }

  /*
   * Converts IPv4 address in its textual presentation form 
   * into its numeric binary form.
   * 
   * @param src a String representing an IPv4 address in standard format
   * @return a byte array representing the IPv4 numeric address
   */
  private static byte[] textToNumericFormatIPv4(String src) {
    if (src.length() == 0) {
      return null;
    }
    
    int octets;
    char ch;
    byte[] dst = new byte[IN4ADDRSZ];
    char[] srcb = src.toCharArray();
    boolean saw_digit = false;
    
    octets = 0;
    int i = 0;
    int cur = 0;
    while (i < srcb.length) {
      ch = srcb[i++];
      if (Character.isDigit(ch)) {
        // note that Java byte is signed, so need to convert to int
        int sum = (dst[cur] & 0xff)*10
          + (Character.digit(ch, 10) & 0xff);
        
        if (sum > 255)
          return null;
        
        dst[cur] = (byte)(sum & 0xff);
        if (! saw_digit) {
          if (++octets > IN4ADDRSZ)
            return null;
          saw_digit = true;
        }
      } else if (ch == '.' && saw_digit) {
        if (octets == IN4ADDRSZ)
          return null;
        cur++;
        dst[cur] = 0;
        saw_digit = false;
      } else
        return null;
    }
    if (octets < IN4ADDRSZ)
      return null;
    return dst;
  }

  private final static int IN6ADDRSZ = 16;

  private final static int INT16SZ = 2;
  /*
   * Convert IPv6 binary address into presentation (printable) format.
   *
   * @param src a byte array representing the IPv6 numeric address
   * @return a String representing an IPv6 address in 
   *         textual representation format
   */
  private static String numericToTextFormatIPv6(byte[] src) {
    StringBuffer sb = new StringBuffer(39);
    for (int i = 0; i < (IN6ADDRSZ / INT16SZ); i++) {
      sb.append(Integer.toHexString(((src[i<<1]<<8) & 0xff00)
                                    | (src[(i<<1)+1] & 0xff)));
      if (i < (IN6ADDRSZ / INT16SZ) -1 ) {
        sb.append(":");
      }
    }
    return sb.toString();
  }

  /* 
   * Convert IPv6 presentation level address to network order binary form.
   * credit:
   *  Converted from C code from Solaris 8 (inet_pton)
   *
   * @param src a String representing an IPv6 address in textual format
   * @return a byte array representing the IPv6 numeric address
   */
  private static byte[] textToNumericFormatIPv6(String src) {
    if (src.length() == 0) {
      return null;
    }

    int colonp;
    char ch;
    boolean saw_xdigit;
    int val;
    char[] srcb = src.toCharArray();
    byte[] dst = new byte[IN6ADDRSZ];

    colonp = -1;
    int i = 0, j = 0;
    /* Leading :: requires some special handling. */
    if (srcb[i] == ':')
      if (srcb[++i] != ':')
        return null;
    int curtok = i;
    saw_xdigit = false;
    val = 0;
    while (i < srcb.length) {
      ch = srcb[i++];
      int chval = Character.digit(ch, 16);
      if (chval != -1) {
        val <<= 4;
        val |= chval;
        if (val > 0xffff)
          return null;
        saw_xdigit = true;
        continue;
      }
      if (ch == ':') {
        curtok = i;
        if (!saw_xdigit) {
          if (colonp != -1)
            return null;
          colonp = j;
          continue;
        } else if (i == srcb.length) {
          return null;
        }
        if (j + INT16SZ > IN6ADDRSZ)
          return null;
        dst[j++] = (byte) ((val >> 8) & 0xff);
        dst[j++] = (byte) (val & 0xff);
        saw_xdigit = false;
        val = 0;
        continue;
      }
      if (ch == '.' && ((j + IN4ADDRSZ) <= IN6ADDRSZ)) {
        byte[] v4addr = textToNumericFormatIPv4(src.substring(curtok));
        if (v4addr == null) {
          return null;
        }
        for (int k = 0; k < IN4ADDRSZ; k++) {
          dst[j++] = v4addr[k];
        }
        saw_xdigit = false;
        break;	/* '\0' was seen by inet_pton4(). */
      }
      return null;
    }
    if (saw_xdigit) {
      if (j + INT16SZ > IN6ADDRSZ)
        return null;
      dst[j++] = (byte) ((val >> 8) & 0xff);
      dst[j++] = (byte) (val & 0xff);
    }
    
    if (colonp != -1) {
      int n = j - colonp;
      
      if (j == IN6ADDRSZ)
        return null;
      for (i = 1; i <= n; i++) {
        dst[IN6ADDRSZ - i] = dst[colonp + n - i];
        dst[colonp + n - i] = 0;
      }
      j = IN6ADDRSZ;
    }
    if (j != IN6ADDRSZ)
      return null;
    byte[] newdst = convertFromIPv4MappedAddress(dst);
    if (newdst != null) {
      return newdst;
    } else {
      return dst;
    }
  }

  /**
   * Utility routine to check if the InetAddress is an
   * IPv4 mapped IPv6 address. 
   *
   * @return a <code>boolean</code> indicating if the InetAddress is 
   * an IPv4 mapped IPv6 address; or false if address is IPv4 address.
   */
  private static boolean isIPv4MappedAddress(byte[] addr) {
    if (addr.length < IN6ADDRSZ) {
      return false;
    }
    if ((addr[0] == 0x00) && (addr[1] == 0x00) && 
        (addr[2] == 0x00) && (addr[3] == 0x00) && 
        (addr[4] == 0x00) && (addr[5] == 0x00) && 
        (addr[6] == 0x00) && (addr[7] == 0x00) && 
        (addr[8] == 0x00) && (addr[9] == 0x00) && 
        (addr[10] == (byte)0xff) && 
        (addr[11] == (byte)0xff))  {   
      return true;
    }
    return false;
  }

  private static byte[] convertFromIPv4MappedAddress(byte[] addr) {
    if (isIPv4MappedAddress(addr)) {
      byte[] newAddr = new byte[IN4ADDRSZ];
      System.arraycopy(addr, 12, newAddr, 0, IN4ADDRSZ);
      return newAddr;
    }
    return null;
  }
}


