/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef __MIDP_PKS_SECURITY_H
#define __MIDP_PKS_SECURITY_H


#ifdef __cplusplus
extern "C" {
#endif

#include "kni.h"

/**
 * create a new keystore iterator instance 
 * See example in the documentation for midp_security_keystore_get_next_md()
 *
 * @param handle address of pointer to file identifier
 *        on successful completion, file identifier is returned in this 
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.  
 * @return <tt>1</tt> on success, 
 *         <tt>-1</tt>
 */
int midp_security_keystore_start_md(/*OUT*/ int* handle);

/**
 * check if further keystore entries exist for the current iterator
 * See example in the documentation for midp_security_keystore_get_next_md()
 *
 * @param keyStoreHandle the handle of the iterator 
 *
 * @retval 1 if more entries exist
 * @retval 0 if no more entries exist
 */
int midp_security_keystore_has_next_md(int keyStoreHandle);

/**
 * get the next keystore entry record and advance the iterator
 * The implementation should supply pointers to platform-alloacated 
 * parameters and buffers.
 * The platform is responsible for any allocations of deallocations
 * of the returned pointers of all parameters:
 * Caller of this function will not free the passed pointers.
 * The out parameters are valid for usage by the caller of this function 
 * only until calling midp_security_keystore_end_md() or subsequent calls
 * to midp_security_keystore_get_next_md().
 *
 * Deallocation of necessary pointers can be performed by the platform in
 * the implementation of midp_security_keystore_end_md() and
 * midp_security_keystore_get_next_md().
 *
 * A sample usage of this function is the following:
 *
 *      void foo(void) {
 *         unsigned short*  owner;
 *         int              ownerSize,modulusSize,exponentSize,domainSize;
 *         jlong   validityStartMillissec, validityEndMillisec;
 *         unsigned char*   modulus;
 *         unsigned char*   exponentSize;,
 *         char*            domain,
 *         int h=midp_security_iterator_start_md();
 *         while(midp_security_keystore_has_next_md(h)==JAVACALL_OK) {
 *
 *           midp_security_iterator_get_next_md(h,
 *               &owner,&ownerSize,&validityStartMillissec,
 *               &validityEndMillisec,&modulus,&modulusSize,
 *               &exponent,&exponentSize,&domain,&domainSize);
 *           //...
 *         }
 *         midp_security_keystore_end_md(h);
 *
 *
 *
 *
 *
 * @param keyStoreHandle the handle of the iterator 
 * @param owner a poiner to the distinguished name of the owner
 * @param ownerSize length of the distinguished name of the owner
 * @param validityStartMillisec start time of the key's validity period 
 *                  in milliseconds since Jan 1, 1970
 * @param validityEndMillisec end time of the key's validity period 
 *                  in milliseconds since Jan 1, 1970
 * @param modulus RSA modulus for the public key
 * @param modulusSize length of RSA modulus 
 * @param exponent RSA exponent for the public key
 * @param exponentSize length of RSA exponent
 * @param domain name of the security domain
 * @param domainSize length of the security domain
 * @retval 1 if more entries exist
 * @retval 0 if no more entries exist
 */
int midp_security_keystore_get_next_md(int keyStoreHandle,
                                       unsigned short**   owner,
                                       int*               ownerSize,
                                       jlong*      validityStartMillissec,
                                       jlong*      validityEndMillisec,
                                       unsigned char**    modulus,
                                       int*               modulusSize,
                                       unsigned char      **exponent,
                                       int*               exponentSize,
                                       char**             domain,
                                       int*               domainSize);

/**
 * free a keystore iterator. 
 * After calling this function, the keyStoreHandle handle will not be used.
 * The implementation may perform any platfrom-sepcific deallocations.
 *
 * @param keyStoreHandle the handle of the iterator 
 * 
 * @retval 1 if successful
 * @retval -1 on error
 */
int midp_security_keystore_end_md(int keyStoreHandle);


#ifdef __cplusplus
}
#endif

#endif  /* __MIDP_PKS_SECURITY_H */


