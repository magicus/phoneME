package com.sun.tck.wma;

/**
 * MMS address parsing and validation.
 * MMS address is validated according to Table D-2 in WMA2.0 specification.
 * In addition address is parsed into the fields of <code>MMSAddress </code>
 * object.
 */
public class MMSAddress {

    /**
     * Determines whether a character represents a hexadecimal number.
     * ('0' - '9' and 'A' - 'F')
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a hexadecimal number;
     * <code> false </code> otherwise
     */
    private static boolean hex(char c) {
	return (digit(c) || (c >= 'A' && c <= 'F'));
    }

    /**
     * Determines whether a character is a letter.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a letter; 
     * <code> false </code> otherwise
     */
    private static boolean alpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    /**
     * Determines whether a character represents a digit.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a digit; 
     * <code> false </code> otherwise
     */
    private static boolean digit(char c) {
	return (c >= '0' && c <= '9');
    }

    /**
     * Determines whether a string represents a valid phone number
     * starting from an index. If the address string is a valid
     * phone number representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (address and application id).
     * General phone number syntax is specified in WMA 2.0 spec as :
     * ("+" 1*digit | 1*digit) [applicationId]
     *
     * @param s The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address and
     * application id (which can be null), 
     * its type is set to GLOBAL_PHONE_NUMBER.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * phone number starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parsePhoneNumber(String s, int i,
				    MMSAddress mmsAddress) {

	int len = s.length();
	if (i < 0 || i >= len) {
	    return false;
	}

	int initI = i;
	char c = s.charAt(i);

	if (c == '+') {
	    i++;
	    if (len == i) {
		return false;
	    }
	}

	int j = 0;
	for (; i < len; j++, i++) {
	    c = s.charAt(i);
	    if (!digit(c)) {
		break;
	    }
	}
	
	if (j == 0) {
	    return false;
	}

	if (i == len) {
	    mmsAddress.set(s.substring(initI), null, GLOBAL_PHONE_NUMBER);
	    return true;
	}

	if (c == ':') {
	    mmsAddress.set(s.substring(initI, i), 
			   null, GLOBAL_PHONE_NUMBER);
	    return parseApplicationId(s, i, mmsAddress);
	}
	
	return false;
    }

    /**
     * Determines whether a string represents a valid ipv4 address
     * starting from an index. If the address string is a valid
     * ipv4 representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (address and application id).
     * ipv4 syntax is specified in WMA 2.0 spec as :
     * 1*3digit "." 1*3digit "." 1*3digit "." 1*3digit [applicationId]
     *
     * @param s The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address and
     * application id (which can be null), its type is set to IPV4.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * ipv4 address starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parseIpv4(String s, int i, MMSAddress mmsAddress) {
	int len = s.length();
	if (i <= 0 || i >= len) {
	    return false;
	}

	char c;
	int j;
	int initI = i;

	for (int num = 1; num < 5; num++) {

	    for (j = 0; j < 3 && i < len; i++, j++) {
		
		c = s.charAt(i);
		
		if (!digit(c)) {
		    break;
		}
	    }
	    
	    // there should be at least one digit and 
	    // the number between dots should be less then 256
	    if (j == 0 || 
		(j == 3 && 
		 (((s.charAt(i-3)-'0')*10 + (s.charAt(i-2)-'0'))*10 +
		  (s.charAt(i-1)-'0')) > 255)) {
		return false;
	    }
	    
	    // check if this is the end of the string
	    if (i == len)  {
		if (num == 4) {
		    mmsAddress.set(s.substring(initI), null, IPV4);
		    return true;
		}
		return false;
	    }

	    c = s.charAt(i);

	    if (c == ':') {
		mmsAddress.set(s.substring(initI, i), null, IPV4);
		return parseApplicationId(s, i, mmsAddress);
	    }

	    if (c != '.') {
		return false; // 4th character after beg (or dot)
		// should be dot;
		// only dots are allowed as non digit char
	    }

	    // allowed '.' => continue
	    i++;
	}
	
	return false;
    }

    /**
     * Determines whether a string represents a valid ipv6 address
     * starting from an index. If the address string is a valid
     * ipv6 representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (address and application id).
     * ipv6 syntax is specified in WMA 2.0 spec as :
     * ipv6-atom ":" ipv6-atom ":" ipv6-atom ":" ipv6-atom ":" 
     * ipv6-atom ":" ipv6-atom ":" ipv6-atom ":" ipv6-atom [appId]
     * where ipv6-atom is 1*4(digit | hex-alpha).
     *
     * @param s The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address and
     * application id (which can be null), its type is set to IPV6.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * ipv6 address starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parseIpv6(String s, int i, MMSAddress mmsAddress) {
	int len = s.length();
	if (i <= 0 || i >= len) {
	    return false;
	}

	char c;
	int j;
	int initI = i;

	for (int num = 1; num < 9; num++) {

	    for (j = 0; j < 4 && i < len; i++, j++) {
		
		c = s.charAt(i);
		
		if (!hex(c)) {
		    break;
		}
	    }
	    
	    // there should be at least one digit 
	    if (j == 0) {
		return false;
	    }

	    
	    // check if this is the end of the string
	    if (i == len)  {
		if (num == 8) {
		    mmsAddress.set(s.substring(initI), null, IPV6);
		    return true;
		}
		return false;
	    }

	    c = s.charAt(i);

	    if (c == ':') { 
		if (num == 8) {
		    mmsAddress.set(s.substring(initI, i), 
				   null, IPV6);
		    return parseApplicationId(s, i, mmsAddress);
		}
	    } else {
		return false; // 5th character after beg (or :)
		// should be :
		// only : are allowed as non digit char
	    }

	    // allowed ':' => continue
	    i++;
	}
	
	return false;
    }

    /**
     * Determines whether a string represents a valid shortcode
     * starting from an index. If the address string is a valid
     * shortcode representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (address and null for appId).
     * Shortcode syntax is specified in WMA 2.0 spec as :
     * *(digit | alpha)
     *
     * @param s The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address and application id (null in
     * this case), its type is set to SHORTCODE.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * shortcode starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parseShortCode(String s, int i, 
				  MMSAddress mmsAddress) {
	int len = s.length();
	if (i > len) {
	    return false;
	}

	if (i == len) {
            return false; // Even though spec allows "mms://" as a valid 
            // shortcode, we disallow it here
	}
        int initI = i;
	char c;
	
	for (; i < s.length(); i++) {
	    c = s.charAt(i);

	    if (!digit(c) && !alpha(c)) {
		return false;
	    }
	}
	mmsAddress.set(s.substring(initI), null, SHORTCODE);
	return true;
    }
    
    /**
     * Determines whether a string represents a valid application ID
     * starting from an index.If the address string is a valid
     * application id representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (application id and null for the 
     * address).
     * Application ID syntax is specified in WMA 2.0 spec as :
     * ":"[*(1*(alpha | digit | "." | "_") ".")]1*(alpha | digit | "." | "_").
     * The number of characters in application ID must not exceed 32.
     *
     * @param s The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address (null in this case) and
     * application id, its type is set to APP_ID.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * application ID starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parseApplicationId(String s, int i, 
				      MMSAddress mmsAddress) {

	int len = s.length();

        /*
	 * Empty string or string like ":" are not allowed.
	 * Only 32 characters are allowed. 
         * Note: When checking appID length, keep in mind that
         * i is pointing to ":"
         */
        if (i <= 0 || i >= len || (len-i-1) > 32 || (len-i-1) < 0 ||
            s.charAt(i) != ':') {
	    return false;
	}
	
	i++; // to skip ':'

	char c;
	int initI = i;

	for (int j = 0; i < len; i++) {
	    
	    c = s.charAt(i);
	    
	    for (j = 0; i < len; i++, j++) {
		c = s.charAt(i);

	        if ((c != '_') && /* dot is also allowed in the spec ??? */
		    !digit(c) && !alpha(c)) {
		    break;
		}
	    }

	    if (i == len) {
		if (mmsAddress.type == INVALID_ADDRESS) {
		    mmsAddress.set(null, s.substring(initI), APP_ID);
		} else {
		    mmsAddress.setAppid(s.substring(initI));
		}
		return true;
	    }

	    // there has to be at least one digit or letter and
	    // nondigit and nonalpha char can be only dot (one dot only)
	    if (j == 0 || c != '.') {
		return false;
	    }
	}
	
	return false;
    }
    
    // ------------------------------- E-mail validation ----------------
    /**
     * Determines whether a character represents a special e-mail character.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a special character.
     * <code> false </code> otherwise
     */
    private static boolean specials(char c) {
	if (c == '(' || c == ')' || c == '<' || c == '>' ||
	    c == '@' || c == ',' || c == ';' || c == ':' ||
	    c == '\\' || c == '"' || c == '.' || c == '[' || c == ']') {
	    return true;
	}
	return false;
    }

    /**
     * Determines whether a character represents a character.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a character.
     * <code> false </code> otherwise
     */
    private static boolean isChar(char c) {
	return ((int)c) >= 0 && ((int)c) <= 127;
    }

    /**
     * Determines whether a character represents an ascii control
     * character.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a character.
     * <code> false </code> otherwise
     */
    private static boolean asciiControl(char c) {
	return ((int)c >= 0 && (int)c <= 31);
    }

    /**
     * Determines whether a character represents an atom character.
     * as specified in WMA 2.0 specification.
     * @param c The character to check.
     * @return <code>true </code> if <code>c</code> is a special character.
     * <code> false </code> otherwise
     */
    private static boolean atomChar(char c) {
        return ((int)c <= 127 && (int)c > 32 && !specials(c));
    }
    

    /**
     * Determines first not linear white space character 
     * in <code>s</code> after character with index <code>i</code> but
     * before character with index <code>n</code>.
     * 
     * @param s The string to check.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code>
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     * @return The first non linear white character after position 
     *          <code>i</code>, it could the the same as passed in
     *          <code>i</code> value if there are no linear white
     *          space characters.
     */
    private static int linearWhiteSpaceSeq(String s, int i, int n) {
	char c;
	for (; i < n; i++) {
	    c = s.charAt(i);
            if (c == ' ' || c == '\t') {
                continue;
            }
	    if (c == '\r') {
		// only CR LF one after another are allowed
		// CR by itself is not allowed
	        if (i == n-1 || s.charAt(i+1) != '\n') {
		    return i;
		}
		i++;
	    } else {
		return i;
	    }
	}

	return i;
    }

    /**
     * Determines whether a string represents a valid e-mail address
     * starting from an index. If the address string is a valid
     * e-mail address representation it is parsed into the fields of 
     * <code>MMSAddress </code>object (address and null for appId).
     * E-mail syntax is specified in WMA 2.0 spec.
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code>
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields are filled with parsed address and
     * application id (null in this case),  its type is set to EMAIL.
     *
     * @return <code>true </code> if <code>s</code> represents a valid 
     * e-mail address starting from index <code>i</code>;
     * <code> false </code> otherwise and if index <code>i</code> is
     * invalide value
     */
    static boolean parseEmail(String s, int i, MMSAddress mmsAddress) {

	int len = s.length();
	if (i <= 0 || i >= len) {
	    return false;
	}

	int initI = i;

	// email can be on of the following:
	// emial ::== mailbox | phrase : [#mailbox] ;

	// if there is a ';' at the end then there should be a phrase
	// at the beginning
	if (s.charAt(len-1) == ';') {

	    if ((i = isPhrase(s, i, len-1)) <= 0) {
		return false;
	    }

	    // there was no ":"
	    if (i >= len-1 || s.charAt(i) != ':') {
		return false;
	    }

	    i++; // move after ':'

	    if (i == len-1) {
                mmsAddress.set(s.substring(initI), null, EMAIL);
		return true; // phrase : ; is allowed string
	    }

	    if ((i = isMailbox(s, i, len-1)) <= 0) {
		return false;
	    }

	    while (i < len-1) {

		// there has to be a ',' sep
		if (i >= len-2 && s.charAt(i) != ',') {
		    return false;
		}

		if ((i = isMailbox(s, i+1, len-1)) <= 0) {
		    return false;
		}
	    }
	    if (i == len-1) {
		mmsAddress.set(s.substring(initI), null, EMAIL);
		return true;
	    }
	    return false;
	}

	if (isMailbox(s, i, len) == len) {
	    mmsAddress.set(s.substring(initI), null, EMAIL);
	    return true;
	}
	return false;
    }

    /**
     * Determines whether a string represents a valid mailbox address
     * starting from an index.
     * Mailbox syntax is specified in WMA 2.0 spec as
     * local-part"@"domain  or as
     * [phrase] "<"[("@"domain) [*("," ("@"domain))]] local-part"@"domain">"
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     *
     * @return index till which 
     * <code>s</code> contains valid mailbox specification;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * do not comply with mailbox specification
     */
    private static int isMailbox(String s, int i, int n) {
        if (n > s.length()) {
            n = s.length();
        }
	if (i >= n) {
	    return -1;
	}

	int initI = i;

	if ((i = isLocalAtDomain(s, initI, n)) > 0) {
	    return i;
	}

	i = initI;

	// check for  [phrase] <[1#(@ domain):] local @ domain>
	// note phrase is optional
	if ((s.charAt(i) != '<') &&
	    (i = isPhrase(s, initI, n)) < 0) {
	    return -1;
	}

	if (i == n || s.charAt(i) != '<') {
	    return -1; 
	}

	i++;
	    
	if (i == n) {
	    return -1;
	}

	// there can be a list of domains: 
	// @ domain, @domain, @domain :
	boolean atLeastOneDomain = false;
	while (s.charAt(i) == '@') {
	    atLeastOneDomain = true;

	    if ((i = isDomain(s, i+1, n)) < 0) {
		return -1;
	    }

	    if (i == n) {
		return -1;
	    }
	    
	    if (s.charAt(i) == ':') {
		break;
	    }

	    // @domain,,@domain is allowed
	    while (s.charAt(i) == ',') {
		i++;
		if (i == n || s.charAt(i) == ':') {
		    return -1;
		}
	    }
	}

	if (atLeastOneDomain) {
	    if (i == n || s.charAt(i) != ':') {
		return -1;
	    }
	    i++;
	}

	// local @ domain
	if ((i = isLocalAtDomain(s, i, n)) <= 0) {
	    return -1;
	}

	if (i < n && s.charAt(i) == '>') {
	    return i + 1;
	}
	return -1;
    }

    /**
     * Determines whether a string represents a valid "local-part"
     * starting from an index.
     * Local-part syntax is specified in WMA 2.0 spec as
     * (atom | "\"alpha) *("." (atom | "\"alpha)) where 
     * atom is represented by at least one char which is 
     * not a space or a special or control character.
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     *
     * @return index till which 
     * <code>s</code> contains valid local-part specification;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * do not comply with local-part specification
     */
    private static int isLocalPart(String s, int i, int n) {
	return isSequenceOfAtomAndText(s, i, n, '"', '"', true);
    }


    /**
     * Determines whether a string represents a valid "phrase"
     * starting from an index.
     * Phrase syntax is specified in WMA 2.0 spec as
     * 1*(atom | """ *(text w/o ",\,\r but with linear space | "\"alpha) """) 
     * where atom is represented by at least one char or more which is 
     * not a space or a special or control character.
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     *
     * @return index till which 
     * <code>s</code> contains valid phrase specification;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * do not comply with phrase specification
     */
    private static int isPhrase(String s, int i, int n) {
	return isSequenceOfAtomAndText(s, i, n, '"', '"', false);
    }


    /**
     * Determines whether a string represents a valid "domain"
     * starting from an index.
     * Domain syntax is specified in WMA 2.0 spec as
     * 1*(atom | "[" *(text w/o ",[,],\r but with linear space | "\"alpha) "]")
     * where atom is represented by at least one char or more which is 
     * not a space or a special or control character.
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     *
     * @return index till which 
     * <code>s</code> contains valid domain specification;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * do not comply with domain specification
     */
    private static int isDomain(String s, int i, int n) {
	return isSequenceOfAtomAndText(s, i, n, '[', ']', true);
    }


    /**
     * Determines whether a string starting from an index
     * represents a valid sequence that satisfies the following
     * syntax:
     * text ::== <begSep>
     *           *(1*(<any char w/o begSep and eSep incl LWSP>|\ALPHA))
     *           <endSep>
     * sequence ::== 1*(atom | text) | (atom | text) *("." (atom | text))
     * where atom is represented by at least one or more characters that  
     * are not space or special or control characters
     * and begSep/endSep are separators that are placed around text.
     * LWSP is a linear white space (space and tabs and \r\n but not \r).
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     * @param begSep The character marking beginning of quoted text
     * @param endSep The character marking the end of the text
     * @param dotSeparated If true atom and quoted text should be 
     *        separated by a "." character; ".." is not allowed
     *
     * @return index till which 
     * <code>s</code> contains valid sequence;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * do not comply with the sequence specification.
     */
    private static int isSequenceOfAtomAndText(String s, int i, int n, 
					       char begSep, char endSep, 
					       boolean dotSeparated) { 
	if (i >= s.length()) {
	    return -1;
	}
        if (n > s.length()) {
            n = s.length();
        }

	// see if there is at least one word
	boolean withinQuotation = false;
	boolean atLeastOneWord  = false;

	char c;
	int initI = i;

	for (; i < n; i++) {
	    c = s.charAt(i);
  	    if (c == begSep) {
		if (begSep == endSep) {
		    withinQuotation = !withinQuotation;
		} else if (withinQuotation) {
		    return -1;
		} else {
		    withinQuotation = true;
		}
		atLeastOneWord = true;
	    } else if (c == endSep) {
		if (!withinQuotation) {
		    return -1;
		}
		withinQuotation = false;
	    } else if (c == '\\') {
		// chars can be quoated only within separators
		// and there has to be another char after '\'
		if (!withinQuotation || i == n-1) {
		    return -1;
		}
		i++;
		c = s.charAt(i);
		// only ALPHA character can be quoted
		if (!alpha(c)) {
		    return -1;
		}

	    } else if (dotSeparated && c == '.') {
		if (i == initI ||
		    ((i - initI) > 0 && s.charAt(i-1) == '.')) {
		    // empty word
		    // string starts with .
		    return -1;
		}
	    } else if (withinQuotation) {
		// within quotation \r is not allowed but
		// \r is allowed as part of linear-white-space
		if (c == ' ' || c == '\t' || c == '\r') {
		    if ((i = linearWhiteSpaceSeq(s, i, n)) < 0) {
			return -1;
		    }
		    i--;
		}
	    } else if (atomChar(c)) {
		atLeastOneWord = true;
	    } else {
		// this is supposed to be an atom 
		break;
	    }
	}

	// open quotation or no words or
	// dot separator is the last char
	if (withinQuotation || !atLeastOneWord ||
	    (dotSeparated && i > initI && s.charAt(i-1) == '.')) {
	    return -1;
	}

	return i;
    }

    /**
     * Determines whether a string starting from an index satisfies the
     * following syntax: local-part"@"domain
     *
     * @param s The string to check.
     * @param i The index of the string at which to start the check. 
     * <code> 0 <= i <= s.length() </code> 
     * @param n The index of the last character to be checked in 
     *          <code>s</code>.
     *
     * @return index till which 
     * <code>s</code> contains valid mailbox specification;
     * <code> -1 </code> if index <code>i</code> is invalid or
     * if characters in <code>s</code> starting from <code>i</code>
     * is not in the form of local-part"@"domain
     */
    private static int isLocalAtDomain(String s, int i, int n) {
        if (n > s.length()) {
            n = s.length();
        }
	if (i >= n) {
	    return -1;
	}

	// local
	if ((i = isLocalPart(s, i, n)) <= 0) {
	    return -1;
	}

	if (i == n || s.charAt(i) != '@') {
	    return -1;
	}

        i++;

	// domain
	return isDomain(s, i, n);
    }

    /**
     * Determines whether a string represents a valid mms address
     * as specified in WMA 2.0 specification and parses the
     * incoming string into address and application id strings.
     *
     * @param addressStr The string to check.
     *
     * @return newly created <code>MMSAddress </code> object 
     * if <code>addressStr</code> is a valid mms address; 
     * <code> null </code> otherwise
     */
    public static MMSAddress getParsedMMSAddress(String addressStr) {
	return getParsedMMSAddress(addressStr, null);
    }

    /**
     * Determines whether a string represents a valid mms address
     * as specified in WMA 2.0 specification and parses the
     * incoming string into address and application id strings.
     * 
     * @param addressStr The string to check.
     * @param mmsAddress The return <code>MMSAddress</code> object which
     * fields will be filled with parsed address and
     * application id, and correspoinding type (EMAIL, GLOBAL_PHONE_NUMBER,
     * IPV4, IPV6, SHORTCODE, APP_ID).
     *
     * @return <code>MMSAddress </code> object if <code>addressStr</code> 
     * is a valid mms address; <code> null </code> otherwise
     */
    public static MMSAddress getParsedMMSAddress(String addressStr, 
						 MMSAddress mmsAddress) {

	if (addressStr == null || !addressStr.startsWith("mms://")) {
	    return null;
	}

        if (mmsAddress == null) {
            mmsAddress = new MMSAddress();
        }

	if (parsePhoneNumber(addressStr, 6, mmsAddress) || 
	    parseIpv4(addressStr, 6, mmsAddress) || 
	    parseIpv6(addressStr, 6, mmsAddress) || 
	    parseEmail(addressStr, 6, mmsAddress) ||
	    parseShortCode(addressStr, 6, mmsAddress) || 
	    parseApplicationId(addressStr, 6, mmsAddress)) {
	    return mmsAddress;
	}

	return null;
    }


    /** Type corresponding to invalid address. */
    public static final int INVALID_ADDRESS     = -1;

    /** Type corresponding to the e-mail address. */
    public static final int EMAIL               = 0;

    /** Type corresponding to global phone number address. */
    public static final int GLOBAL_PHONE_NUMBER = 1;

    /** Type corresponding to the ipv4 address. */
    public static final int IPV4                = 2;

    /** Type corresponding to the ipv6 address. */
    public static final int IPV6                = 3;

    /** Type corresponding to the shortcode address. */
    public static final int SHORTCODE           = 4;

    /** Type corresponding to the application id address. */
    public static final int APP_ID              = 5;

    /** 
     * Field that holds address part of the mms address 
     * (without "mms://" and ":" separator before app id).
     */
    public String address;

    /** 
     * Field that holds application id part of the mms address
     * which appears after "mms://:" or 
     * after phone number, ipv4, or ipv6 followed by ":" .
     */
    public String appId;

    /** 
     * Type of this MMSAddress instance, 
     * <code>INVALID_ADDRESS</code> when uninitialized .
     */
    public int type;

    /**
     *  MMSAddress constructor to create uninitialized instance.
     */
    MMSAddress() {
	clear();
    }

    /**
     * MMSAddress constructor to create initialized instance.
     * @param address The address part of the mms address
     * @param appId The application id part of the mms address
     * @param type The type of this mms addreess
     * (EMAIL, GLOBAL_PHONE_NUMBER, IPV4, IPV6, SHORTCODE, APP_ID) 
     */
    MMSAddress(String address, String appId, int type) {
	set(address, appId, type);
    }

    /**
     * Clears <code>MMSAddress</code> fields.
     * Type is set to INVALID_ADDRESS while
     * address and appId are set to null.
     */
    void clear() {
	address = appId = null;
	type = INVALID_ADDRESS;
    }

    /**
     * Sets <code>MMSAddress</code> fields to the passed in values.
     * @param address The address part of the mms address
     * @param appId The application id part of the mms address
     * @param type The type of this mms addreess
     * (EMAIL, GLOBAL_PHONE_NUMBER, IPV4, IPV6, SHORTCODE, APP_ID) 
     */
    void set(String address, String appId, int type) {
	this.address = address;
	this.appId   = appId;
	this.type    = type;
    }

    /**
     * Sets <code>MMSAddress</code> application id field to 
     * the passed in value.
     * @param appId The application id part of the mms address
     */
    void setAppid(String appId) {
	this.appId = appId; 
    }

    /**
     * Creates a valid mms address corresponding to the values
     * in this <code>MMSAddress</code> object.
     * If the object is not intialized null is returned.
     * @return mms address corresponding to this MMSAddress object.
     */
    String getMMSAddressString() {

	if (type == INVALID_ADDRESS || 
	    (address == null && appId == null)) {
	    return null;
	}

	StringBuffer mmsAddr = new StringBuffer("mms://");
	if (address != null) mmsAddr.append(address);
	if (appId != null) mmsAddr.append(":").append(appId);
	return mmsAddr.toString();
    }
}

