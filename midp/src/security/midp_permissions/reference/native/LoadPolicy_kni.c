
#include <kni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <sni.h>

#include <stdio.h>
#include <string.h>
#include <midpError.h>
#include "policy_load.h"


static void jchar_to_char(jchar *src, char *dst, int len) {
    while(len > 0) {
        *dst++ = (char)*src++;
        len--;
    }
    *dst = 0; //null
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadDomainList)
{
    int lines, i1;
    void* array;

    KNI_StartHandles(2);
    KNI_DeclareHandle(domains);
    KNI_DeclareHandle(tmpString);

    lines = permissions_load_domain_list(&array);
    if (lines > 0) {
        char** list = (char**)array;
        SNI_NewArray(SNI_STRING_ARRAY,  lines, domains);
        if (KNI_IsNullHandle(domains))
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        else
            for (i1 = 0; i1 < lines; i1++) {
                KNI_NewStringUTF(list[i1], tmpString);
                KNI_SetObjectArrayElement(domains, (jint)i1, tmpString);
            }
        permissions_dealloc(array);
    } else
        KNI_ReleaseHandle(domains);  /* set object to NULL */

    KNI_EndHandlesAndReturnObject(domains);
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadGroupList)
{
    int lines, i1;
    void* array;

    KNI_StartHandles(2);
    KNI_DeclareHandle(groups);
    KNI_DeclareHandle(tmpString);
    
    lines = permissions_load_group_list(&array);
    if (lines > 0) {
        char** list = (char**)array;
        SNI_NewArray(SNI_STRING_ARRAY,  lines, groups);
        if (KNI_IsNullHandle(groups))
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        else
            for (i1 = 0; i1 < lines; i1++) {
                KNI_NewStringUTF(list[i1], tmpString);
                KNI_SetObjectArrayElement(groups, (jint)i1, tmpString);
            }
        permissions_dealloc(array);
    } else
        KNI_ReleaseHandle(groups); /* set object to NULL */

    KNI_EndHandlesAndReturnObject(groups);
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadGroupPermissions)
{
    int lines, i1;
	unsigned int str_len;
    void *array;
    jchar jbuff[64];
    char  group_name[64];

    KNI_StartHandles(3);
    KNI_DeclareHandle(members);
    KNI_DeclareHandle(tmpString);
    KNI_DeclareHandle(group);

    KNI_GetParameterAsObject(1, group);
    if (!KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(group);
        if (str_len <= sizeof(group_name)-1) {
            KNI_GetStringRegion(group, 0, str_len, jbuff);
            jchar_to_char(jbuff, group_name, str_len);
            lines = permissions_load_group_permissions(&array, group_name);
            if (lines > 0) {
                char **list = (char**)array;
                SNI_NewArray(SNI_STRING_ARRAY,  lines, members);
                if (KNI_IsNullHandle(members))
                    KNI_ThrowNew(midpOutOfMemoryError, NULL);
                else
                    for (i1 = 0; i1 < lines; i1++) {
                        KNI_NewStringUTF(list[i1], tmpString);
                        KNI_SetObjectArrayElement(members, (jint)i1,
                                                            tmpString);
                    }
                permissions_dealloc(array);
            } else
                KNI_ReleaseHandle(members);  /* set object to NULL */
        }
    } else
        KNI_ThrowNew(midpNullPointerException, "null group parameter");


    KNI_EndHandlesAndReturnObject(members);
}


KNI_RETURNTYPE_BYTE
KNIDECL(com_sun_midp_security_Permissions_getDefaultValue) {
    int str_len;
    jbyte value;
    jchar jbuff[64];
    char  domain_name[64], group_name[64];

    KNI_StartHandles(2);
    KNI_DeclareHandle(domain);
    KNI_DeclareHandle(group);

    value = 0;
    KNI_GetParameterAsObject(1, domain);
    KNI_GetParameterAsObject(2, group);
    if (!KNI_IsNullHandle(domain) && !KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(domain);
        KNI_GetStringRegion(domain, 0, str_len, jbuff);
        jchar_to_char(jbuff, domain_name, str_len);
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
        jchar_to_char(jbuff, group_name, str_len);
        value = (jbyte)permissions_get_default_value(domain_name, group_name);
    }

    KNI_EndHandles();
    KNI_ReturnByte(value);
}

KNI_RETURNTYPE_BYTE
KNIDECL(com_sun_midp_security_Permissions_getMaxValue) {
    
    int str_len;
    jbyte value;
    jchar jbuff[64];
    char  domain_name[64], group_name[64];

    KNI_StartHandles(2);
    KNI_DeclareHandle(domain);
    KNI_DeclareHandle(group);

    value = 0;
    KNI_GetParameterAsObject(1, domain);
    KNI_GetParameterAsObject(2, group);
    if (!KNI_IsNullHandle(domain) && !KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(domain);
        KNI_GetStringRegion(domain, 0, str_len, jbuff);
        jchar_to_char(jbuff, domain_name, str_len);
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
        jchar_to_char(jbuff, group_name, str_len);
        value = (jbyte)permissions_get_max_value(domain_name, group_name);
    }

    KNI_EndHandles();
    KNI_ReturnByte(value);
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_getGroupMessages) {
    int lines, i1, str_len;
    void *array;
    jchar jbuff[64];
    char  group_name[64];

    KNI_StartHandles(3);
    KNI_DeclareHandle(group);
    KNI_DeclareHandle(messages);
    KNI_DeclareHandle(tmpString);

    KNI_GetParameterAsObject(1, group);
    if (!KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
        jchar_to_char(jbuff, group_name, str_len);
        lines = permissions_load_group_messages(&array, group_name);
        if (lines > 0) {
            char **list = (char**)array;
            SNI_NewArray(SNI_STRING_ARRAY,  lines, messages);
            if (KNI_IsNullHandle(messages))
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            else
                for (i1 = 0; i1 < lines; i1++) {
                    KNI_NewStringUTF(list[i1], tmpString);
                    KNI_SetObjectArrayElement(messages, (jint)i1,
                                                        tmpString);
                }
            permissions_dealloc(array);
        } else
            KNI_ReleaseHandle(messages);  /* set object to NULL */
    } else
        KNI_ThrowNew(midpNullPointerException, "null group parameter");


    KNI_EndHandlesAndReturnObject(messages);
}

KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_security_Permissions_loadingFinished)
{
    permissions_loading_finished();
    KNI_ReturnVoid();
}

