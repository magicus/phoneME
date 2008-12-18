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

/**
 * @file
 *
 *
 */
#include "kni.h"
#include "sni.h"
#include "midpError.h"
#include "midp_logging.h"
#include "midp_pks_security.h"

/**
 * Get the next public key data.
 *
 * @param PublicKeyInfo object.
 *
 * @return  0        - no more keys, 
 *         -1        - error, 
 *          other    - sucsess.
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_publickeystore_PublicKeyLoader_getPublicKeyInfo0(void) {

    int              handle = 0;
    unsigned short*  owner;                                        

    int    ownerSize;
    int    modulusSize;
    int    exponentSize;
    int    domainSize;

    jlong   validityStartMillissec;
    jlong   validityEndMillisec;

    unsigned char*   modulus;                                      
    unsigned char*   exponent;                                
    char*            domain;                                       

    int              result = ALL_OK;
    int              status = 1;

    jfieldID  owner_field_id;    
    jfieldID  notBefore_field_id;
    jfieldID  notAfter_field_id; 
    jfieldID  modulus_field_id;  
    jfieldID  exponent_field_id; 
    jfieldID  domain_field_id; 


    KNI_StartHandles(6);
    KNI_DeclareHandle(publicKeyInfoClazz);
    KNI_DeclareHandle(publicKeyInfoObject);
    KNI_DeclareHandle(byteArrayEx);
    KNI_DeclareHandle(byteArrayMd);
    KNI_DeclareHandle(ownerString);
    KNI_DeclareHandle(domainString);

    handle  =  KNI_GetParameterAsInt(2);
    do {
        KNI_GetParameterAsObject(1, publicKeyInfoObject);           
        if(KNI_IsNullHandle(publicKeyInfoObject)) {
            REPORT_ERROR(LC_SECURITY, 
                        "PublicKeyInfo_getPublicKeyInfo0 publicKeyInfoObject is NULL\n");
            KNI_ThrowNew(midpOutOfMemoryError, 
                         "Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0: can't get publicKeyInfoObject\n");
            status = -1;
            break;
        }

        KNI_GetObjectClass(publicKeyInfoObject, publicKeyInfoClazz);
        if(KNI_IsNullHandle(publicKeyInfoClazz)) {
            REPORT_ERROR(LC_SECURITY, 
                        "PublicKeyInfo_getPublicKeyInfo0 publicKeyInfoClazz is NULL\n");
            KNI_ThrowNew(midpOutOfMemoryError, 
                         "Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0: can't get publicKeyInfoClazz\n");
            status = -1;
            break;
        }

        owner_field_id     = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "owner", "Ljava/lang/String;");
        notBefore_field_id = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "notBefore", "J");
        notAfter_field_id  = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "notAfter", "J");
        modulus_field_id   = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "modulus", "[B");
        exponent_field_id  = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "exponent", "[B");
        domain_field_id    = KNI_GetFieldID(publicKeyInfoClazz, 
                                            "domain", "Ljava/lang/String;");


        if((owner_field_id == 0) || 
           (notBefore_field_id == 0) || 
           (notAfter_field_id == 0) || 
           (modulus_field_id == 0) || 
           (exponent_field_id == 0) || 
           (domain_field_id == 0)) {

            REPORT_ERROR(LC_SECURITY, 
                        "PublicKeyInfo_getPublicKeyInfo0 one of the fiels ids is NULL\n");
                      

            KNI_ThrowNew(midpRuntimeException, 
                         "Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0: can't create field ids\n");            
            status = -1;
            break;
        }

        result = midp_security_keystore_has_next_md(handle);
        if(result == 0) {
            /* no more keys available from any reason */
            status = 0;
            break;
        }

        result = 
            midp_security_keystore_get_next_md(handle,                       
                                               &owner, &ownerSize,
                                               &validityStartMillissec,               
                                               &validityEndMillisec,
                                               &modulus,
                                               &modulusSize,              
                                               &exponent, &exponentSize,
                                               &domain, &domainSize);            

        if(result == -1) {
            /* something wrong happened */
            status = -1;
            break;
        }

        /* set modulus field */
        SNI_NewArray(SNI_BYTE_ARRAY, modulusSize, byteArrayMd);
        if(KNI_IsNullHandle(byteArrayMd) == KNI_FALSE) {

            KNI_SetRawArrayRegion(byteArrayMd, 0, 
                                  modulusSize, 
                                  (jbyte*)modulus);

            KNI_SetObjectField(publicKeyInfoObject, 
                               modulus_field_id, 
                               byteArrayMd);

        } else {
            REPORT_ERROR(LC_SECURITY, 
                        "PublicKeyInfo_getPublicKeyInfo0 can't create byte array\n");

            KNI_ThrowNew(midpOutOfMemoryError, 
                         "Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0: can't create byte array\n");
            status = -1;
            break;
        }

        /* set exponent field */
        SNI_NewArray(SNI_BYTE_ARRAY, exponentSize, byteArrayEx);
        if(KNI_IsNullHandle(byteArrayEx) == KNI_FALSE) {

            KNI_SetRawArrayRegion(byteArrayEx, 0, 
                                  exponentSize, 
                                  (jbyte*)exponent);

            KNI_SetObjectField(publicKeyInfoObject, 
                               exponent_field_id, 
                               byteArrayEx);

        } else {
            REPORT_ERROR(LC_SECURITY, 
                        "PublicKeyInfo_getPublicKeyInfo0 can't create byte array\n");

            KNI_ThrowNew(midpOutOfMemoryError, 
                         "Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0: can't create byte array\n");
            status = -1;
            break;
        }


        /* set long field notBefore */
        KNI_SetLongField(publicKeyInfoObject, 
                         notBefore_field_id, 
                         validityStartMillissec);

        /* set long field notAfter */
        KNI_SetLongField(publicKeyInfoObject, 
                         notAfter_field_id, 
                         validityEndMillisec);   

        /* set strings - owner */
        KNI_NewStringUTF((const char*)owner, ownerString);
        KNI_SetObjectField(publicKeyInfoObject, 
                           owner_field_id, 
                           ownerString);

        /* set strings - domain */
        KNI_NewStringUTF((const char*)domain, domainString);
        KNI_SetObjectField(publicKeyInfoObject, 
                           domain_field_id, 
                           domainString);

    } while( 0 );

    KNI_EndHandles();

    KNI_ReturnInt(status);

} /* end of Java_com_sun_midp_publickeystore_PublicKeyInfo_getPublicKeyInfo0 */


KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_publickeystore_PublicKeyLoader_keyStoreInit(void) {

    int handle;
    int result;

    result = midp_security_keystore_start_md(&handle);
    if(result == -1) {
        REPORT_ERROR(LC_SECURITY, 
                    "PublicKeyStore_keyStoreInit: can't create handle\n");

        KNI_ThrowNew(midpIOException, 
                     "Java_com_sun_midp_publickeystore_PublicKeyStore_keyStoreInit: can't create handle\n");
    }

    KNI_ReturnInt((jint)handle);

} /* end of Java_com_sun_midp_publickeystore_PublicKeyStore_keyStoreInit */

KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_publickeystore_PublicKeyLoader_keyStoreFinalize(void) {

    int  handle;
    int  status;

    handle = KNI_GetParameterAsInt(1);
    status = midp_security_keystore_end_md(handle);
    KNI_ReturnInt(status);

} /* end of Java_com_sun_midp_publickeystore_PublicKeyStore_keyStoreFinalize */

