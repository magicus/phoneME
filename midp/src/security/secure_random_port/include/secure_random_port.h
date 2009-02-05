/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _SECURE_RANDOM_PORT_H_
#define _SECURE_RANDOM_PORT_H_


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform a platform-defined procedure for obtaining random bytes
 * and store the obtained bytes into buffer, total (size) bytes.
 * (see IETF RFC 1750, Randomness Recommendations for Security,
 *  http://www.ietf.org/rfc/rfc1750.txt)
 * @param buffer array that receives random bytes
 * @param size the number of random bytes to receive, must not be less than size of buffer
 * @return true if success, false on failure
 */

jboolean get_random_bytes_port(unsigned char*buffer, jint size);



#ifdef __cplusplus
}
#endif

#endif /* _SECURE_RANDOM_PORT_H_ */
