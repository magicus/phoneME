
#include <kni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <sni.h>

#include <stdio.h>
#include <string.h>
#include <midpError.h>

#include "javacall_security.h" 
#include "javacall_memory.h" 


KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadPermissionList)
{
	//int lines, i1;
	//void *array;

    KNI_StartHandles(2);
    KNI_DeclareHandle(permmissionlist);
    KNI_DeclareHandle(tmpString);

/*	lines = javacall_load_permissions((void**)&array);
	if (lines > 0) {
		char **list = (char**)array;
        SNI_NewArray(SNI_STRING_ARRAY,  lines, permmissionlist);
		if (KNI_IsNullHandle(permmissionlist))
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        else
			for (i1 = 0; i1 < lines; i1++) {
				KNI_NewStringUTF(list[i1], tmpString);
				KNI_SetObjectArrayElement(permmissionlist, (jint)i1, tmpString);
			}
		javacall_free(array);
	} else
*/
	    KNI_ReleaseHandle(permmissionlist);  //set object to NULL

    KNI_EndHandlesAndReturnObject(permmissionlist);
}


KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadDomainList)
{
	int lines, i1;
	void *array;

    KNI_StartHandles(2);
    KNI_DeclareHandle(domains);
    KNI_DeclareHandle(tmpString);

	lines = javacall_load_domain_list(&array);
	if (lines > 0) {
		char **list = (char**)array;
        SNI_NewArray(SNI_STRING_ARRAY,  lines, domains);
		if (KNI_IsNullHandle(domains))
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        else
			for (i1 = 0; i1 < lines; i1++) {
				KNI_NewStringUTF(list[i1], tmpString);
				KNI_SetObjectArrayElement(domains, (jint)i1, tmpString);
			}
		javacall_free(array);
	} else
	    KNI_ReleaseHandle(domains);  //set object to NULL

    KNI_EndHandlesAndReturnObject(domains);
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadGroupList)
{
	int lines, i1;
	void *array;

    KNI_StartHandles(2);
    KNI_DeclareHandle(groups);
    KNI_DeclareHandle(tmpString);
	
	lines = javacall_load_group_list(&array);
	if (lines > 0) {
		char **list = (char**)array;
        SNI_NewArray(SNI_STRING_ARRAY,  lines, groups);
		if (KNI_IsNullHandle(groups))
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        else
			for (i1 = 0; i1 < lines; i1++) {
				KNI_NewStringUTF(list[i1], tmpString);
				KNI_SetObjectArrayElement(groups, (jint)i1, tmpString);
			}
		javacall_free(array);
	} else
	    KNI_ReleaseHandle(groups);  //set object to NULL

    KNI_EndHandlesAndReturnObject(groups);
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_loadGroupPermissions)
{
	int lines, i1, str_len;
	void *array;
	jchar jbuff[32];
	char  group_name[32];

    KNI_StartHandles(3);
    KNI_DeclareHandle(members);
    KNI_DeclareHandle(tmpString);
    KNI_DeclareHandle(group);

    KNI_GetParameterAsObject(1, group);
	if (!KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
		if (javautil_unicode_utf16_to_utf8(jbuff, str_len, group_name, sizeof(group_name), &i1) ==
			JAVACALL_OK) {
			if (i1 > 0) {
				group_name[i1] = 0;
				lines = javacall_load_group_permissions(&array, group_name);
				if (lines > 0) {
					char **list = (char**)array;
					SNI_NewArray(SNI_STRING_ARRAY,  lines, members);
					if (KNI_IsNullHandle(members))
						KNI_ThrowNew(midpOutOfMemoryError, NULL);
					else
						for (i1 = 0; i1 < lines; i1++) {
							KNI_NewStringUTF(list[i1], tmpString);
							KNI_SetObjectArrayElement(members, (jint)i1, tmpString);
						}
					javacall_free(array);
				} else
					KNI_ReleaseHandle(members);  //set object to NULL
			}
		}
	} else
		KNI_ThrowNew(midpNullPointerException, "null group parameter");


    KNI_EndHandlesAndReturnObject(members);
}


KNI_RETURNTYPE_BYTE
KNIDECL(com_sun_midp_security_Permissions_getDefaultValue) {
	int str_len, i1;
	jbyte value;
	jchar jbuff[32];
	char  domain_name[32], group_name[32];

    KNI_StartHandles(3);
    KNI_DeclareHandle(tmpString);
    KNI_DeclareHandle(domain);
    KNI_DeclareHandle(group);

	value = JAVACALL_NEVER;
    KNI_GetParameterAsObject(1, domain);
    KNI_GetParameterAsObject(2, group);
	if (!KNI_IsNullHandle(domain) && !KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(domain);
        KNI_GetStringRegion(domain, 0, str_len, jbuff);
		javautil_unicode_utf16_to_utf8(jbuff, str_len, domain_name, sizeof(domain_name), &i1);
		domain_name[i1] = 0;
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
		javautil_unicode_utf16_to_utf8(jbuff, str_len, group_name, sizeof(group_name), &i1);
		group_name[i1] = 0;
		value = (jbyte)javacall_get_default_value(domain_name, group_name);
	}

	KNI_EndHandles();
    return value;
}

KNI_RETURNTYPE_BYTE
KNIDECL(com_sun_midp_security_Permissions_getMaxValue) {
    
	int str_len, i1;
	jbyte value;
	jchar jbuff[32];
	char  domain_name[32], group_name[32];

    KNI_StartHandles(3);
    KNI_DeclareHandle(tmpString);
    KNI_DeclareHandle(domain);
    KNI_DeclareHandle(group);

	value = JAVACALL_NEVER;
    KNI_GetParameterAsObject(1, domain);
    KNI_GetParameterAsObject(2, group);
	if (!KNI_IsNullHandle(domain) && !KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(domain);
        KNI_GetStringRegion(domain, 0, str_len, jbuff);
		javautil_unicode_utf16_to_utf8(jbuff, str_len, domain_name, sizeof(domain_name), &i1);
		domain_name[i1] = 0;
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
		javautil_unicode_utf16_to_utf8(jbuff, str_len, group_name, sizeof(group_name), &i1);
		group_name[i1] = 0;
		value = (jbyte)javacall_get_max_value(domain_name, group_name);
	}

	KNI_EndHandles();
    return value;
}

KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_security_Permissions_getGroupMessages) {
	int lines, i1, str_len;
	void *array;
	jchar jbuff[32];
	char  group_name[32];

    KNI_StartHandles(3);
    KNI_DeclareHandle(group);
    KNI_DeclareHandle(messages);
    KNI_DeclareHandle(tmpString);

	//_asm int 3
    KNI_GetParameterAsObject(1, group);
	if (!KNI_IsNullHandle(group)) {
        str_len = KNI_GetStringLength(group);
        KNI_GetStringRegion(group, 0, str_len, jbuff);
		if (javautil_unicode_utf16_to_utf8(jbuff, str_len, group_name, sizeof(group_name), &i1) ==
			JAVACALL_OK) {
			if (i1 > 0) {
				group_name[i1] = 0;
				lines = javacall_load_group_messages(&array, group_name);
				if (lines > 0) {
					char **list = (char**)array;
					SNI_NewArray(SNI_STRING_ARRAY,  lines, messages);
					if (KNI_IsNullHandle(messages))
						KNI_ThrowNew(midpOutOfMemoryError, NULL);
					else
						for (i1 = 0; i1 < lines; i1++) {
							KNI_NewStringUTF(list[i1], tmpString);
							KNI_SetObjectArrayElement(messages, (jint)i1, tmpString);
						}
					javacall_free(array);
				} else
					KNI_ReleaseHandle(messages);  //set object to NULL
			}
		}
	} else
		KNI_ThrowNew(midpNullPointerException, "null group parameter");


    KNI_EndHandlesAndReturnObject(messages);
}

