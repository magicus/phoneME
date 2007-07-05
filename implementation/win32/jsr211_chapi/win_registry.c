/*
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

/**
 * @file javacall_registry.c
 * @ingroup CHAPI
 * @brief javacall win32 registry access implementation
 */

#include "javacall_chapi_registry.h"

#define WIN32_LEAN_AND_MEAN
#define UNICODE

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

static int getInvokerPath(WCHAR* buffer){
	wcscpy(buffer,L"C:\\TI\\WORK\\jsr211\\Invoker\\Debug\\Invoker.exe");
	return wcslen(buffer);
}

static int getDefaultIconPath(WCHAR* buffer){
	wcscpy(buffer,L"C:\\TI\\WORK\\jsr211\\dukedoc.ico,0");
	return wcslen(buffer);
}

static int wblen(const WCHAR* wstr){
	return sizeof(unsigned short)*(wcslen(wstr)+1);
}


/************************************************************************************************/
/*										PUBLIC PART												*/
/************************************************************************************************/

static const WCHAR* SUITE_ID = L"SuiteId";
static const WCHAR* CLASS_NAME = L"ClassName";
static const WCHAR* SHELL = L"shell";
static const WCHAR* COMMAND = L"command";
static const WCHAR* OPEN = L"open";
static const WCHAR* DEFAULT_ICON = L"DefaultIcon";
static const WCHAR* ARGS = L" \"%1\" %2 %3 %4";
static const WCHAR* OPENWITH = L"OpenWithProgIDs";
//static const WCHAR* AUTOPLAY=L"AutoplayContentTypeHandler";
static const WCHAR* CHAPIHANDLER=L"CHAPIContentTypeHandler";
static const WCHAR* MIME_CONTENTTYPE_BASE=L"MIME\\DataBase\\Content Type";
static const WCHAR* ACCESSES=L"TrustedCallers";
static const WCHAR* SUFFIXES=L"ContentSuffixes";
static const WCHAR* TYPES=L"ContentTypes";
static const WCHAR* FLAG=L"Flag";
static const WCHAR* EXTENSION=L"Extension";


#define MAX_BUFFER 512
#define KEY_VOLATILENESS REG_OPTION_VOLATILE

/**
 * Perform initialization of registry API
 *
 * @return JAVACALL_OK if initialization was successful, error code otherwise
 */
javacall_result javacall_chapi_init_registry(void){

}

/**
 * Finalize API, clean all used resources.
 *
 * @return nothing
 */
void javacall_chapi_finalize_registry(void){

}


//register java handler
javacall_result javacall_chapi_register_handler(
			javacall_const_utf16_string content_handler_id,
			javacall_const_utf16_string content_handler_friendly_appname,
			int suite_id,
			javacall_const_utf16_string class_name,
			javacall_chapi_handler_registration_type flag,
			javacall_const_utf16_string* content_types,     int nTypes,
			javacall_const_utf16_string* suffixes,  int nSuffixes,
			javacall_const_utf16_string* actions,   int nActions,  
			javacall_const_utf16_string* locales,   int nLocales,
			javacall_const_utf16_string* action_names, int nActionNames,
			javacall_const_utf16_string* access_allowed_ids,  int nAccesses)
{

HKEY key=0,subkey=0, shellkey=0, actionkey=0, commandkey=0, mimekey=0, chapikey=0;
LONG result;
DWORD isnew;
int i,j;
WCHAR buffer[MAX_BUFFER];

if (!content_handler_id || !*content_handler_id) return -1;
result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_QUERY_VALUE,&key);

if (result == ERROR_SUCCESS){
		RegCloseKey(key);
		RegDeleteKey(HKEY_CLASSES_ROOT,content_handler_id);
		//return ERROR_HANDLER_ALREADY_EXISTS;
}

result = RegCreateKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,NULL, KEY_VOLATILENESS,KEY_WRITE,NULL,&key, NULL);
if (result != ERROR_SUCCESS) return result;

result = RegSetValueEx(key,NULL,0,REG_SZ, (CONST BYTE*)content_handler_friendly_name, wblen(content_handler_friendly_name));

if (result == ERROR_SUCCESS)
	result = RegSetValueEx(key,SUITE_ID,0,REG_SZ, (CONST BYTE*)suite_id, wblen(suite_id));

if (result == ERROR_SUCCESS)
	result = RegSetValueEx(key,CLASS_NAME,0,REG_SZ, (CONST BYTE*)class_name, wblen(class_name));

if (result == ERROR_SUCCESS)
	result = RegSetValueEx(key,FLAG,0,REG_DWORD, (CONST BYTE*)&flag, sizeof(DWORD));


if (result == ERROR_SUCCESS)
for (i=0;i<nKeys;++i){
	RegSetValueEx(key,additional_keys[i].key,0,REG_SZ, (CONST BYTE*)additional_keys[i].value, wblen(additional_keys[i].value));
}

if (result == ERROR_SUCCESS){
	int len=0;
	for (i=0;i<nAccesses;++i){
		wcscpy(&buffer[len],accesses[i]);
		len+=wcslen(accesses[i]);
		buffer[len++]='\0';
	}
	buffer[len++]='\0';
	RegSetValueEx(key,ACCESSES,0,REG_MULTI_SZ, (CONST BYTE*)buffer, len*2);
}

if (result == ERROR_SUCCESS)
	result = RegCreateKeyEx(key,DEFAULT_ICON,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&subkey, NULL);


if (result == ERROR_SUCCESS){
	if (default_icon_path){
		result = RegSetValueEx(subkey,CLASS_NAME,0,REG_SZ, (CONST BYTE*)default_icon_path, wblen(default_icon_path));
	} else {
		int len = getDefaultIconPath(buffer);
		result = RegSetValueEx(subkey,NULL,0,REG_SZ, (CONST BYTE*)buffer, (len+1)*2);
	}
}

if (result == ERROR_SUCCESS){
	result = RegCreateKeyEx(key,SHELL,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&shellkey, NULL);

	if (result == ERROR_SUCCESS){
		//get execution module path
		int pathlen=getInvokerPath(buffer);	
		if (!nActionNames) {
			result = RegCreateKeyEx(shellkey,OPEN,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&actionkey,NULL);
			if (result == ERROR_SUCCESS){
				//no actions defined: use default open action
				//append args
				buffer[pathlen++]=' ';
				wcscpy(&buffer[pathlen],ARGS);
				pathlen+=wcslen(ARGS);

				result = RegCreateKeyEx(actionkey,COMMAND,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&commandkey,NULL);
				if (result == ERROR_SUCCESS){
					result = RegSetValueEx(commandkey, NULL,0, REG_EXPAND_SZ, (CONST BYTE*)buffer, (pathlen+1)*2);
					RegCloseKey(commandkey);
				}
				RegCloseKey(actionkey);
			}
		} else {
			for (i=0; i < nActions; ++i){
				int commandlen=pathlen;
				result = RegCreateKeyEx(shellkey,actions[i],0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&actionkey,NULL);
				if (result == ERROR_SUCCESS){
					//add locale names
					for (j=0;j<nLocales;++j){
						const WCHAR* local_name = action_names[j*nActions+i];
						result = RegSetValueEx(actionkey, locales[j],0,REG_SZ, (CONST BYTE*)local_name, wblen(local_name));
					}

					result = RegCreateKeyEx(actionkey,COMMAND,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&commandkey,NULL);
					if (result == ERROR_SUCCESS){
						//append comand
						buffer[commandlen++]=' ';
						wcscpy(&buffer[commandlen],actions[i]);
						commandlen+=wcslen(actions[i]);

						//append args
						buffer[commandlen++]=' ';
						wcscpy(&buffer[commandlen],ARGS);
						commandlen+=wcslen(ARGS);

						result = RegSetValueEx(commandkey, NULL,0,REG_EXPAND_SZ, (CONST BYTE*)buffer, (commandlen+1)*2);
						RegCloseKey(commandkey);
					}
					RegCloseKey(actionkey);
				}
		
			}
		}
		RegCloseKey(shellkey);
	}
}
if (key) {RegCloseKey(key);key=0;}
if (result != ERROR_SUCCESS) return result;


// assign handler to file extensions
if (suffixes){
	for (i=0; i < nSuffixes; ++i){
		result = RegCreateKeyEx(HKEY_CLASSES_ROOT,suffixes[i],0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&key,&isnew);
		if (result == ERROR_SUCCESS) {
			if (isnew==REG_OPENED_EXISTING_KEY){
				result = RegCreateKeyEx(key,OPENWITH,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&subkey,NULL);
				if (result == ERROR_SUCCESS) {
					result = RegSetValueEx(subkey,content_handler_id,0,REG_SZ, (CONST BYTE*)L"", 1);
					RegCloseKey(subkey);
				}
			} else {
				result = RegSetValueEx(key,NULL,0,REG_SZ, (CONST BYTE*)content_handler_id, wblen(content_handler_id));
			}
			RegCloseKey(key);
		}
	}
}

// assign handler to mime types
if (types){
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,MIME_CONTENTTYPE_BASE,0,KEY_WRITE,&mimekey);
	if (result == ERROR_SUCCESS) {
		for (i=0; i < nTypes; ++i){
			result = RegCreateKeyEx(mimekey,types[i],0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&key,NULL);
			if (result == ERROR_SUCCESS) {
				result = RegCreateKeyEx(key,CHAPIHANDLER,0,NULL,KEY_VOLATILENESS,KEY_WRITE,NULL,&chapikey,NULL);
				if (result == ERROR_SUCCESS) {
					result = RegSetValueEx(chapikey,content_handler_id, 0, REG_SZ, (CONST BYTE*)L"", 1);
					RegCloseKey(chapikey);
				}
			}
			RegCloseKey(key);
		}
		RegCloseKey(mimekey);
	}
}

return result;
}

static int read_array(const unsigned short* key_name, const unsigned short* value_name, short* buffer, int* length){
	HKEY key=0; 
	LONG result;
	int l;
	DWORD type;

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,key_name,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	l = *length;
	result = RegQueryValueEx(key,value_name,0,&type,(LPBYTE)buffer,&l);

	if (result == ERROR_SUCCESS){
		if (type == REG_MULTI_SZ) {
			*length = l;
		} else
	    if (type == REG_SZ) {
			if (*length)  {
				if (*length<l+1) 
					result = -1; 
				else {
					buffer[l]=0;
				}
			}
			*length = l+1;
		} else {
			if (result != ERROR_MORE_DATA) *length = 0;
			result = -1;
		}
	}

	RegCloseKey(key);
	return result;
}

static int read_string(const unsigned short* key_name, const unsigned short* value_name, short* buffer, int* length){
	HKEY key; 
	LONG result;
	DWORD type;

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,key_name,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	result = RegQueryValueEx(key,value_name,0,&type,(LPBYTE)buffer,length);

	RegCloseKey(key);

	return result;
}

static int read_int(const unsigned short* key_name, const unsigned short* value_name, DWORD* res){
	HKEY key; 
	LONG result;
	DWORD type;
	int len;

	if (!res) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,key_name,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	len = sizeof(DWORD);
	result = RegQueryValueEx(key,value_name,0,&type,(LPBYTE)res,&len);

	RegCloseKey(key);

	return result;
}

static int enum_keys(const unsigned short* searched_value_name, const unsigned short* searched_value_value, int* pos_id, short*  buffer, int* length){
	HKEY key; 
	LONG result=ERROR_SUCCESS;
	DWORD index=(DWORD)pos_id;
	WCHAR valbuf[MAX_BUFFER];
	DWORD type;
	int len;
	int maxlen = *length;

	if (!length || (*length && !buffer)) return -1;
	
	while  ( result == ERROR_SUCCESS){
		*length = maxlen;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, index++, buffer, length, 0, NULL, NULL, NULL);

		if (result == ERROR_SUCCESS) {
			result = RegOpenKeyEx(HKEY_CLASSES_ROOT,buffer,0,KEY_QUERY_VALUE,&key);
			if (result == ERROR_SUCCESS) {
				len = MAX_BUFFER;
				result = RegQueryValueEx(key,searched_value_name,0,&type,(LPBYTE)valbuf,&len);
				RegCloseKey(key);
				if (!wcscmp(searched_value_value,valbuf)) break;
			}
		}
	}

	*pos_id=index;

	if (result != ERROR_SUCCESS){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	return result;
}


static int suffix_belongs_to_handler(const unsigned short* content_handler_id,const unsigned short* suffix){
	HKEY key; 
	LONG result=ERROR_SUCCESS;
	WCHAR valbuf[MAX_BUFFER];
	DWORD type;
	int len=MAX_BUFFER;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,suffix,0,KEY_QUERY_VALUE,&key);
	if (result == ERROR_SUCCESS) {
		result = RegQueryValueEx(key,NULL,0,&type,(LPBYTE)valbuf,&len);
		RegCloseKey(key);
		if (!wcscmp(content_handler_id,valbuf)) return 1;
	}
	return 0;
}

javacall_result javacall_chapi_enum_suffixes(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  suffix_out, int* length){
	LONG result=ERROR_SUCCESS;
	DWORD index=(DWORD)*pos_id;
	int found = 0;
	int maxlen = *length;

	if (!length || (*length && !buffer)) return -1;

	while  (!found && result != ERROR_NO_MORE_ITEMS){
		*length = maxlen;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, index++, buffer, length, 0, NULL, NULL, NULL);

		if (result == ERROR_SUCCESS) {
			if (buffer[0]=='.'){
				if (!content_handler_id || suffix_belongs_to_handler(content_handler_id,buffer)){
					found =1;
					break;
				}
			}
		}
	}

	if (!found){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	*pos_id=index;

	return result;
}



javacall_result javacall_chapi_enum_types(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  type_out, int* length){
	HKEY mimekey,typekey,chapikey; 
	LONG result=ERROR_SUCCESS;
	DWORD index=(DWORD)*pos_id;
	WCHAR valbuf[MAX_BUFFER];
	DWORD type;
	int len, found = 0;
	int maxlen = *length;

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,MIME_CONTENTTYPE_BASE,0,KEY_READ,&mimekey);
	if (result == ERROR_SUCCESS) {
		while  (!found && result != ERROR_NO_MORE_ITEMS){
			*length = maxlen;
			result = RegEnumKeyEx(mimekey, index++, buffer, length, 0, NULL, NULL, NULL);

			if (result == ERROR_SUCCESS) {
				if (!content_handler_id) {
					found =1;
				} else {
					result = RegOpenKeyEx(mimekey,buffer,0,KEY_READ,&typekey);
					if (result == ERROR_SUCCESS) {
						result = RegOpenKeyEx(typekey,CHAPIHANDLER,0,KEY_READ,&chapikey);
						if (result == ERROR_SUCCESS) {
							int i=0;
							while (1){
								len = MAX_BUFFER;
								if (ERROR_SUCCESS == RegEnumValue(chapikey,i++,valbuf,&len,0,0,0,0)){
									if (!wcscmp(content_handler_id,valbuf)) {
										found = 1;
										break;
									}
								} else break;
							}
							RegCloseKey(chapikey);
						} else {
							len = MAX_BUFFER;
							result = RegQueryValueEx(typekey,EXTENSION,0,&type,(LPBYTE)valbuf,&len);
							if (result == ERROR_SUCCESS) {
								if (suffix_belongs_to_handler(content_handler_id,valbuf)) {
									found = 1;
									break;
								}
							}
						}
						RegCloseKey(typekey);
					}
				}
			}
		}
		RegCloseKey(mimekey);
	}

	if (!found){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	*pos_id=index;
	return result;
}

javacall_result javacall_chapi_get_content_handler_friendly_appname(javacall_const_utf16_string content_handler_id, /*OUT*/ javacall_utf16*  handler_frienfly_appname_out, int* length){
	return read_string(content_handler_id, NULL, buf, length);
}


int get_suite_id(const unsigned short* content_handler_id,  short*  buf, int* length){
	return read_string(content_handler_id, SUITE_ID, buf, length);
}

int get_class_name(const unsigned short* content_handler_id,  short*  buf, int* length){
	return read_string(content_handler_id, CLASS_NAME, buf, length);
}

int get_flag(const unsigned short* content_handler_id,  int* val){
	return read_int(content_handler_id, FLAG, val);
}

javacall_result javacall_chapi_enum_actions(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  action_out, int* length){
	HKEY key,shellkey=0; 
	LONG result;
	DWORD index=(DWORD)*pos_id;
	DWORD len=0;
	DWORD sumlen=0;

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_READ,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	result = RegOpenKeyEx(key,SHELL,0,KEY_READ,&shellkey);
	RegCloseKey(key);

	if (result == ERROR_SUCCESS){
		result = RegEnumKeyEx(shellkey,index++, buffer, length, NULL, NULL, NULL, NULL);
		RegCloseKey(shellkey);
	}

	*pos_id=index;
	return result;
}


javacall_result javacall_chapi_enum_action_locales(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16* locale_out, int* length){
	HKEY key,shellkey=0,actionkey; 
	LONG result;
	DWORD index=(DWORD)*pos_id;
	short abuffer[MAX_BUFFER];
	DWORD len=MAX_BUFFER;
	int kv;
	

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_READ,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	result = RegOpenKeyEx(key,SHELL,0,KEY_READ,&shellkey);
	RegCloseKey(key);

	if (result == ERROR_SUCCESS){
		RegEnumKeyEx(shellkey,0, abuffer, &len, NULL, NULL, NULL, NULL);
		if (result == ERROR_SUCCESS){
			result = RegOpenKeyEx(shellkey,abuffer,0,KEY_READ,&actionkey);
			if (result == ERROR_SUCCESS){
				result = RegQueryInfoKey(actionkey, NULL,NULL,NULL,NULL,NULL,NULL,&kv,NULL,NULL,NULL,NULL);			
				if (result == ERROR_SUCCESS){
					result = RegEnumValue(actionkey,index++,buffer,length,0,0,0,0);
				}
				RegCloseKey(actionkey);
			}
		}
		RegCloseKey(shellkey);
	}

	if (result == ERROR_SUCCESS){
		*pos_id=index;
	} else {
		if (buffer) buffer[0]=0;
		if (result != ERROR_MORE_DATA) *length=0;
	}
	return result;
}

javacall_result javacall_chapi_get_local_action_name(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action, javacall_const_utf16_string locale, javacall_utf16*  local_action_out, int* length){
	HKEY key,shellkey=0,actionkey; 
	LONG result;
	int type;
	

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_READ,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	result = RegOpenKeyEx(key,SHELL,0,KEY_READ,&shellkey);
	RegCloseKey(key);

	if (result == ERROR_SUCCESS){
		result = RegOpenKeyEx(shellkey,action,0,KEY_READ,&actionkey);
		if (result == ERROR_SUCCESS){
			result = RegQueryValueEx(actionkey,locale,0,&type,(LPBYTE)buffer,length);
			RegCloseKey(actionkey);
		}
		RegCloseKey(shellkey);
	}

	return result;
}

javacall_result javacall_chapi_enum_access_allowed_callers(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  access_allowed_out, int* length){
	HKEY key; 
	LONG result;
	DWORD type;
	int index = *pos_id;
	short buf[MAX_BUFFER];
	int i,len=MAX_BUFFER;

	if (!length || (*length && !buffer)) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	result = RegQueryValueEx(key,ACCESSES,0,&type,(LPBYTE)buf,&len);
	RegCloseKey(key);

	if (result == ERROR_SUCCESS){
		short* b = buf;
		for (i=0;i<index;++i){
			b = b+wcslen(b)+1;
			if (!*b) return ERROR_NO_MORE_ELEMENTS;
		}
		len = wcslen(b);
		if (*length < len) {
			*length = len;
			if (buffer) buffer[0]=0;
			return ERROR_MORE_DATA;
		}
		*length = len;
		if (buffer) wcscpy(buffer,b);
	}
	

	return result;
}


int get_key_value(const unsigned short* content_handler_id,  const unsigned short* key_name, short*  buf, int* length){
	return read_string(content_handler_id, key_name, buf, length);
}

static int delete_subkeys(HKEY key){
LONG result;
HKEY subkey;
WCHAR* buf;
int count=0, len, length, i;

	result = RegQueryInfoKey(key,NULL,NULL,NULL,&count,&len,NULL,NULL,NULL,NULL,NULL,NULL);
	if (result != ERROR_SUCCESS) return result;

	len++;
	buf = malloc(len * 2);
	if (!buf) return -1;

	for (i=count-1;i>=0;--i){
		length = len;
		result = RegEnumKeyEx(key, i, buf, &length, NULL, NULL, NULL, NULL);
		if (result != ERROR_SUCCESS) break;

		result = RegOpenKeyEx(key,buf,0,KEY_ALL_ACCESS,&subkey);
		if (result == ERROR_SUCCESS) {
			result = delete_subkeys(subkey);
		}
		RegCloseKey(subkey);
		RegDeleteKey(key, buf);
	}

	free(buf);

	return result;
}

javacall_result javacall_chapi_unregister_handler(javacall_const_utf16_string content_handler_id){
	HKEY key=0,subkey=0,mimekey,typekey, prefixkey, chapikey;
	WCHAR buffer[MAX_BUFFER], name[MAX_BUFFER];
	LONG result;
	DWORD type;
	int count, length, i , kc, kv;
	if (!content_handler_id || !*content_handler_id) return -1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,MIME_CONTENTTYPE_BASE,0,KEY_READ,&mimekey);
	if (result == ERROR_SUCCESS){
		result = RegQueryInfoKey(mimekey, NULL,NULL,NULL,&count,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

		if (result == ERROR_SUCCESS){
			for (i=count-1;i>=0;--i){
				length = MAX_BUFFER;
				result = RegEnumKeyEx(mimekey, i, name, &length, NULL, NULL, NULL, NULL);
				if (result != ERROR_SUCCESS) break;

				result = RegOpenKeyEx(mimekey,name,0,KEY_ALL_ACCESS,&typekey);
				if (result != ERROR_SUCCESS) continue;

				result = RegOpenKeyEx(typekey,CHAPIHANDLER,0,KEY_ALL_ACCESS,&chapikey);
				if (result != ERROR_SUCCESS) continue;

				result = RegDeleteValue(chapikey,content_handler_id);
				if (result == ERROR_SUCCESS){
					result = RegQueryInfoKey(chapikey, NULL,NULL,NULL,NULL,NULL,NULL,&kv,NULL,NULL,NULL,NULL);
					RegCloseKey(chapikey);
					if (kv==0){
						result = RegDeleteKey(typekey,CHAPIHANDLER);
						length = MAX_BUFFER;
						result = RegQueryInfoKey(typekey, NULL,NULL,NULL,&kc,NULL,NULL,&kv,NULL,NULL,NULL,NULL);
						RegCloseKey(typekey);

						if (result == ERROR_SUCCESS && !kc && !kv)  {
							//delete type key
							result = RegDeleteKey(mimekey,name);
						}
					}
				} else {
					RegCloseKey(chapikey);
				}
			}
			RegCloseKey(mimekey);
		}
		
	}


	result = RegQueryInfoKey(HKEY_CLASSES_ROOT, NULL,NULL,NULL,&count,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

	if (result == ERROR_SUCCESS)
	for (i=count-1;i>=0;--i){

		length = MAX_BUFFER;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, i, name, &length, NULL,NULL,NULL,NULL);
		if (result != ERROR_SUCCESS) break;

		if (name[0]!='.') continue;

		result = RegOpenKeyEx(HKEY_CLASSES_ROOT,name,0,KEY_ALL_ACCESS,&prefixkey);
		if (result != ERROR_SUCCESS) continue;

		length = MAX_BUFFER;
		result = RegQueryValueEx(prefixkey,NULL,0,&type,(LPBYTE)buffer,&length);

		if (result == ERROR_SUCCESS && !wcscmp(buffer,content_handler_id)){
			result = RegQueryInfoKey(prefixkey, NULL,NULL,NULL,&kc,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

			if (result == ERROR_SUCCESS && kc > 0){
				//just remove default association
				result = RegDeleteValue(prefixkey,NULL);
			} else {
				//delete key
				result = RegDeleteKey(HKEY_CLASSES_ROOT,name);
			}

		}

		RegCloseKey(prefixkey);
	}

	//delete app key
	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_ALL_ACCESS,&key);
	if (result == ERROR_SUCCESS) {
		delete_subkeys(key);
		RegCloseKey(key);
	}

	result = RegDeleteKey(HKEY_CLASSES_ROOT,content_handler_id);

	return result;
}


javacall_result javacall_chapi_enum_handlers_by_suffix(javacall_const_utf16_string suffix, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
HKEY key=0,progskey;
int index = *pos_handle;
DWORD type;
LONG result;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,suffix,0,KEY_READ,&key);
	if (result != ERROR_SUCCESS) return result;

	if (index == 0){
		result = RegQueryValueEx(key,NULL,0,&type,(LPBYTE)buffer,length);
	} else {
		result = RegOpenKeyEx(key,OPENWITH,0,KEY_READ,&progskey);
		if (result == ERROR_SUCCESS) {
			result = RegEnumValue(progskey, index-1, buffer, length, NULL,NULL,NULL,NULL);
			RegCloseKey(progskey);
		}
	}

	RegCloseKey(key);

	if (result == ERROR_SUCCESS) {
		*pos_handle=index+1;
	} else {
		if (buffer) buffer[0]=0;
		if (result != ERROR_MORE_DATA) *length = 0;
	}
	
	return result;
}

javacall_result javacall_chapi_enum_handlers_by_type(javacall_const_utf16_string content_type, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
HKEY key=0,mimekey,progskey;
int index = *pos_handle;
DWORD type;
LONG result;
unsigned short valbuf[MAX_BUFFER];
int len = MAX_BUFFER;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,MIME_CONTENTTYPE_BASE,0,KEY_READ,&mimekey);
	if (result != ERROR_SUCCESS) return result;

	result = RegOpenKeyEx(mimekey,mimetype,0,KEY_READ,&key);
	RegCloseKey(mimekey);

	if (result != ERROR_SUCCESS) return result;

	if ((index>>16) == 0){
		result = RegOpenKeyEx(key,CHAPIHANDLER,0,KEY_READ,&progskey);
		if (result == ERROR_SUCCESS) {
			result = RegEnumValue(progskey, index++, buffer, length, NULL,NULL,NULL,NULL);
			RegCloseKey(progskey);
			
		}
		if (result != ERROR_SUCCESS) {
			index = 0x10000;
		}
	}

	if ((index>>16) != 0){
		result = RegQueryValueEx(key,EXTENSION,0,&type,(LPBYTE)valbuf,&len);
		if (result == ERROR_SUCCESS) {
			int sufindex = (index>>16) - 1 ;
			result =  enum_handlers_by_suffix(valbuf, &sufindex, buffer, length);
			index = (sufindex + 1) << 16;
		}
	}


	RegCloseKey(key);

	if (result == ERROR_SUCCESS) {
		*pos_handle=index;
	} else {
		if (buffer) buffer[0]=0;
		if (result != ERROR_MORE_DATA) *length = 0;
	}
	
	return result;
}

javacall_result javacall_chapi_enum_handlers_by_action(javacall_const_utf16_string action, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){
HKEY key, shellkey;
LONG result=ERROR_SUCCESS;
DWORD index=(DWORD)*pos_handle;
int found = 0;
int maxlen = *length;

	if (!length || (*length && !buffer)) return -1;

	while  (!found && result != ERROR_NO_MORE_ITEMS){
		*length = maxlen;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, index++, buffer, length, 0, NULL, NULL, NULL);

		if (result == ERROR_SUCCESS) {
			result = RegOpenKeyEx(HKEY_CLASSES_ROOT, buffer,0,KEY_READ,&key);
			if (result == ERROR_SUCCESS) {
				result = RegOpenKeyEx(key,SHELL,0,KEY_READ,&shellkey);
				RegCloseKey(key);
				if (result == ERROR_SUCCESS) {
					result = RegOpenKeyEx(shellkey,action,0,KEY_READ,&key);
					RegCloseKey(shellkey);
					if (result == ERROR_SUCCESS) {
						found =1;
						RegCloseKey(key);
						break;
					}
				}
			}
		}
	}

	if (!found){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	*pos_handle=index;

	return result;
}

javacall_result javacall_chapi_enum_handlers_by_suite_id(
		 int suite_id,
		 int* pos_id, 
		 /*OUT*/ javacall_utf16*  handler_id_out,
        int* length){
HKEY key;
LONG result=ERROR_SUCCESS;
DWORD index=(DWORD)*pos_handle;
int found = 0;
int maxlen = *length;
int type;
unsigned short valbuf[MAX_BUFFER];
int len = MAX_BUFFER;

	if (!length || (*length && !buffer)) return -1;

	while  (!found && result != ERROR_NO_MORE_ITEMS){
		*length = maxlen;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, index++, buffer, length, 0, NULL, NULL, NULL);
		if (result == ERROR_SUCCESS) {
			result = RegOpenKeyEx(HKEY_CLASSES_ROOT, buffer,0,KEY_READ,&key);
			if (result == ERROR_SUCCESS) {
				result = RegQueryValueEx(key,SUITE_ID,0,&type,(LPBYTE)valbuf,&len);
				if (result == ERROR_SUCCESS && !wcscmp(valbuf,suit_id)) {
					found =1;
				}
				RegCloseKey(key);
			}
		}
	}

	if (!found){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	*pos_handle=index;

	return result;
}


javacall_result javacall_chapi_enum_handlers(int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length){

HKEY key, shellkey;
LONG result=ERROR_SUCCESS;
DWORD index=(DWORD)*pos_handle;
int found = 0;
int maxlen = *length;

if (!length || (*length && !buffer)) return -1;

while  (!found && result != ERROR_NO_MORE_ITEMS){
		*length = maxlen;
		result = RegEnumKeyEx(HKEY_CLASSES_ROOT, index++, buffer, length, 0, NULL, NULL, NULL);
		if (result == ERROR_SUCCESS) {
			result = RegOpenKeyEx(HKEY_CLASSES_ROOT, buffer,0,KEY_READ,&key);
			if (result == ERROR_SUCCESS) {
				result = RegOpenKeyEx(key, SHELL,0,KEY_READ,&shellkey);
				if (result == ERROR_SUCCESS) {
					found =1;
					RegCloseKey(shellkey);
				}
				RegCloseKey(key);
			}
		}
	}

	if (!found){
		if (result != ERROR_MORE_DATA) *length = 0;
		if (buffer) *buffer=0;
	}

	*pos_handle=index;

	return result;

}


javacall_result javacall_chapi_get_handler_info(javacall_const_utf16_string content_handler_id,
					/*OUT*/
					int*  suite_id_out,
					javacall_utf16*  classname_out, int* classname_len,
 			        javacall_chapi_handler_registration_type *flag_out)
{


	HKEY key; 
	LONG result;
	DWORD type;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return ERROR_HANDLER_DOESNOT_EXIST;

	if (result == ERROR_SUCCESS && suit_id){
		result = RegQueryValueEx(key,SUITE_ID,0,&type,(LPBYTE)suit_id,suit_id_len);
	}

	if (result == ERROR_SUCCESS && classname){
		result = RegQueryValueEx(key,CLASS_NAME,0,&type,(LPBYTE)classname,classname_len);
	}

	if (result == ERROR_SUCCESS && flag){
		int len = sizeof(DWORD);
		result = RegQueryValueEx(key,FLAG,0,&type,(LPBYTE)flag,&len);
	}

	RegCloseKey(key);

	return result;


}

javacall_bool javacall_chapi_is_access_allowed(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string caller_id)
{

	HKEY key; 
	LONG result;
	DWORD type;
	short accesses[MAX_BUFFER];
	short* a;
	int length = MAX_BUFFER;

	//system caller has all access
	if (!caller_id) return 1;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return 0;

	result = RegQueryValueEx(key,ACCESSES,0,&type,(LPBYTE)accesses,&length);

	// if there is no access value, consider native handler with all access
	if (result != ERROR_SUCCESS) return 1;

	a = accesses;
	while (*a){
		if (!wcsicmp(a,caller_id)) return 1;
		a+=(wcslen(a)+1);
	}

	return 0;
}

javacall_bool javacall_chapi_is_action_supported(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action){
	HKEY key, shellkey; 
	LONG result;
	int found = 0;

	result = RegOpenKeyEx(HKEY_CLASSES_ROOT,content_handler_id,0,KEY_QUERY_VALUE,&key);
	if (result != ERROR_SUCCESS) return 0;

	if (result == ERROR_SUCCESS) {
		result = RegOpenKeyEx(key,SHELL,0,KEY_READ,&shellkey);
		RegCloseKey(key);
		if (result == ERROR_SUCCESS) {
			result = RegOpenKeyEx(shellkey,action,0,KEY_READ,&key);
			RegCloseKey(shellkey);
			if (result == ERROR_SUCCESS) {
				found =1;
				RegCloseKey(key);
			}
		}
	}

	return found;
}


void javacall_chapi_enum_finish(int pos_id){
}
