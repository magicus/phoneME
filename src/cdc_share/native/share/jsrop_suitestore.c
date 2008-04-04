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

#include <jsrop_suitestore.h>

/**
 * @param suite_id original suite id
 * @return size in characters of suite_id in string representation
 */
int jsrop_suiteid_string_size(SuiteIdType suite_id){
	return sizeof(suite_id) * 2; // maximum length of integer in hexadecimal format
}

/**
 * Convert suit_id to string representation
 * @param suite_id original suite id
 * @param suite_id_str_out buffer contatining space for atleast jsrop_suiteid_max_size() + 1 character 
 *                         for receiving suite_id in string representation
 * @return non zero in case of success zero in case of error
 */
int jsrop_suiteid_to_string(SuiteIdType suite_id, /* OUT */ jchar* suite_id_str_out){
    static jchar xdig[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t idx = jsrop_suiteid_string_size(suite_id);
    suite_id_str_out[ idx ] = '\0';
    while( idx-- ){ 
        suite_id_str_out[ idx ] = xdig[suite_id & 0xF];
        suite_id >>= 4;
    }
	return -1;
}

/**
 * Convert string representation of suit id to its original type
 * @param suite_id suite id in string representation
 * @param suite_id_out pointer on suit id of original type
 * @return non zero in case of success zero in case of error
 */
int jsrop_string_to_suiteid(const jchar* suite_id_str, /* OUT */ SuiteIdType* suite_id_out){
	int i=0,sign=1,ch;
	if (!suite_id_str) return 0;
	if (*suite_id_str=='-') {sign =-1;suite_id_str++;}
	while ((ch=*suite_id_str++)){
		i <<= 4;
		if (ch<='9' && ch>='0') i+=(ch-'0');else
		    if (ch<='F' && ch>='A') i+=(ch-'A'+0xa);else
			    if (ch<='f' && ch>='a') i+=(ch-'a'+0xa);else
					return 0;
	}
	*suite_id_out = (SuiteIdType)(sign*i);
	return -1;
}
