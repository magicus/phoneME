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

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>

#include <midpMalloc.h>
#include <javautil_unicode.h>

#ifdef ENABLE_SPRINT_CIQ
#include <javacall_sprint_ciq.h>
#endif

/**
 * @file
 * 
 * The default implementation of the native functions that are needed
 * for supporting the "http:" Generic Connection protocols.
 */


int convert_string_object_to_ascii(jstring stringHandle, char *buffer, int buffer_size) {

    int stringLength;
    jchar *stringBuffer;
    int asciiStringLength;
    javacall_result status;
    int result = -1;

    if (NULL == buffer || buffer_size <= 0)
        return -1;
    
    stringLength = KNI_GetStringLength(stringHandle);        

    if (stringLength < 0)
        return -1;

    stringBuffer = midpMalloc(sizeof(jchar) * (stringLength + 1));
    if (NULL == stringBuffer)
        return -1;

    KNI_GetStringRegion(stringHandle, 0, stringLength, stringBuffer);

    status =  javautil_unicode_utf16_to_utf8(stringBuffer, stringLength, buffer, buffer_size-1, 
                                                                 &asciiStringLength);
    if (JAVACALL_OK == status) {
        buffer[asciiStringLength] = '\0';
        result = 0;
    }

    midpFree(stringBuffer);

    return result;    
}

#ifdef ENABLE_SPRINT_CIQ
#define MAX_CIQ_PARAMETER_SIZE                  128


/**
 * Report CIQ data for HTTP Header
 *
 *  void ciqReportHttpHeader(String header)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_http_Protocol_ciqReportHttpHeader(void){
    char asciiHeader[MAX_CIQ_PARAMETER_SIZE];
    int status;
        
    KNI_StartHandles(1);
    KNI_DeclareHandle(headerHandle);
    KNI_GetParameterAsObject(1, headerHandle);

    do 
    {
        status = convert_string_object_to_ascii(headerHandle, asciiHeader, 
                                                                    sizeof(asciiHeader));
        if (0 != status)
            break;

        javacall_sprint_ciq_report_http_header(asciiHeader);
    } while(0); 

    KNI_EndHandles(); 
    KNI_ReturnVoid();
}


/**
 * Report CIQ data for HTTP Request
 *
 *   ciqReportHttpRequest(String method, int length, String uri, String network_type); 
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_http_Protocol_ciqReportHttpRequest(void){
    int requestLength = (int)KNI_GetParameterAsInt(2);
    int status;
       
    char asciiMethod[MAX_CIQ_PARAMETER_SIZE];
    char asciiURI[MAX_CIQ_PARAMETER_SIZE];
    char asciiNetworkType[MAX_CIQ_PARAMETER_SIZE];

    KNI_StartHandles(3); 
    KNI_DeclareHandle(methodHandle);
    KNI_DeclareHandle(uriHandle);
    KNI_DeclareHandle(networkTypeHandle);

    KNI_GetParameterAsObject(1, methodHandle);
    KNI_GetParameterAsObject(3, uriHandle);
    KNI_GetParameterAsObject(4, networkTypeHandle);    

    do 
    {              
        status = convert_string_object_to_ascii(methodHandle, asciiMethod, sizeof(asciiMethod));
        if (0 != status)
            break;
        
        status = convert_string_object_to_ascii(uriHandle, asciiURI, sizeof(asciiURI));
        if (0 != status)
            break;

        status = convert_string_object_to_ascii(networkTypeHandle, asciiNetworkType, sizeof(asciiNetworkType));
        if (0 != status)
            break;


        javacall_sprint_ciq_report_http_request(asciiMethod,
                                                              requestLength,
                                                              asciiURI,
                                                              asciiNetworkType);
    } while (0);

    KNI_EndHandles(); 
    KNI_ReturnVoid();
}


/**
 * Report CIQ data for HTTP Response
 *
 *  void ciqReportHttpResponse(int responseCode, int responseMsgLength, 
 *                                        String uri, String network_type);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_http_Protocol_ciqReportHttpResponse(void){
    int responseCode = (int)KNI_GetParameterAsInt(1);
    int responseMsgLength = (int)KNI_GetParameterAsInt(2);
    int status;
       
    char asciiURI[MAX_CIQ_PARAMETER_SIZE];
    char asciiNetworkType[MAX_CIQ_PARAMETER_SIZE];

    KNI_StartHandles(2); 
    KNI_DeclareHandle(uriHandle);
    KNI_DeclareHandle(networkTypeHandle);

    KNI_GetParameterAsObject(3, uriHandle);
    KNI_GetParameterAsObject(4, networkTypeHandle);    

    do 
    {
        status = convert_string_object_to_ascii(uriHandle, asciiURI, sizeof(asciiURI));
        if (0 != status)
            break;

        status = convert_string_object_to_ascii(networkTypeHandle, asciiNetworkType, sizeof(asciiNetworkType));
        if (0 != status)
            break;


        javacall_sprint_ciq_report_http_response(responseCode,
                                                                responseMsgLength,
                                                                asciiURI,
                                                                asciiNetworkType);
    } while(0);


    KNI_EndHandles(); 
    KNI_ReturnVoid();
}

#endif
