/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software{ return 0; } you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY{ return 0; } without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work{ return 0; } if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/**
 * @file
 * @brief Content Handler Registry implementation based on POSIX file calls.
 */

#include "inc/javautil_storage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define JAVAUTIL_STOREAGE_PREFIX "jus"

javacall_result javautil_storage_open(const char* name, int flag, /* OUT */ javautil_storage* storage)
{ 
	char* b;
	FILE* f ;

	if (!storage) return JAVACALL_INVALID_ARGUMENT;	
	if (flag == JUS_O_RDONLY){
		b = "r";
	} else if ( flag == JUS_O_RDWR ){
			b = "r+";
	} else if ( flag == (JUS_O_RDWR | JUS_O_CREATE)){
			b = "w+";
	} else {
		return JAVACALL_INVALID_ARGUMENT;
	}
	
	f = fopen(name,b);

	if (!f) {
		*storage = JAVAUTIL_INVALID_STORAGE_HANDLE;
		return JAVACALL_IO_ERROR;
	}
	*storage = (javautil_storage*)f;

	return JAVACALL_OK; 
}

javacall_result javautil_storage_remove(const char* name)
{ 
	return (remove(name) == 0) ? JAVACALL_OK : JAVACALL_IO_ERROR;  
}


javacall_result javautil_storage_close(javautil_storage storage)
{
	return (fclose((FILE*)storage)==0) ? JAVACALL_OK : JAVACALL_IO_ERROR; 
}

javacall_result javautil_storage_gettmpname(char* name, int* len)
{
	return (tmpnam(name) != 0) ? JAVACALL_OK : JAVACALL_IO_ERROR;  
}

javacall_result javautil_storage_rename(const char *oldname, const char *newname)
{
	return ( rename(oldname, newname) == 0) ? JAVACALL_OK : JAVACALL_IO_ERROR;  
}

javacall_result javautil_storage_getsize(javautil_storage storage,/* OUT */ long* size)
{ 
	FILE* f =  (FILE*) storage;
	long l = ftell(f);
	if (fseek(f,0,SEEK_END)) return JAVACALL_IO_ERROR;
	*size = ftell(f);
	if (fseek(f,l,SEEK_SET)) return JAVACALL_IO_ERROR;
	return JAVACALL_OK;
}



javacall_result javautil_storage_setpos(javautil_storage storage, long pos, int flag)
{ 
	FILE* f =  (FILE*) storage;
	return (fseek(f, pos, flag) == 0) ? JAVACALL_OK : JAVACALL_IO_ERROR;
}

javacall_result javautil_storage_getpos(javautil_storage storage, /* OUT */long* pos)
{
	*pos = ftell((FILE*)storage);
	return *pos>=0 ? JAVACALL_OK : JAVACALL_IO_ERROR; 
}

int javautil_storage_read(javautil_storage storage, char* buffer, unsigned int size)
{ 
	return fread(buffer,1, size, (FILE*)storage); 
}

int javautil_storage_write(javautil_storage storage, char* buffer, unsigned int size)
{
	return fwrite(buffer,1, size, (FILE*)storage); 
}
