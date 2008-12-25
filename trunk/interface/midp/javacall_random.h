/*
 *
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
#ifndef __JAVACALL_RANDOM_H_
#define __JAVACALL_RANDOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file javacall_random.h
 * @ingroup Security
 * @brief Javacall interfaces for prand, a pseudo random number generator used for cryptography
 * @{
 */

#include "javacall_defs.h"



/**
 * Get an unpredictable sequence of bytes, suitable as cryptographic seed.
 * (see IETF RFC 1750, Randomness Recommendations for Security,
 *  http://www.ietf.org/rfc/rfc1750.txt)
 *
 * @param outbuf buffer to receive random bytes, starting at index 0
 * @param bufsize  the size of outbuf
 * @retval JAVACALL_OK if success,
 * @retval JAVACALL_FAIL in case of an error
 * @retval JAVACALL_NOT_IMPLEMENTED if not implemented (this must never happen on end user's device)
 */
javacall_result javacall_random_get_seed(unsigned char* outbuf, int bufsize);


/** @} */

#ifdef __cplusplus
}
#endif

#endif


