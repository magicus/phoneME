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

/**
 * @file
 * @brief Content Handler Registry implementation based on POSIX file calls.
 */

#include "inc/javautil_storage.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>


javacall_result javautil_storage_open(const char* name, int flag, /* OUT */ javautil_storage* storage)
{ 
    char* b;
    FILE* f = 0;
    
    if (!storage) return JAVACALL_INVALID_ARGUMENT;    
    if (flag == JUS_O_RDONLY){
        b = "rb";
    } else if ( flag == JUS_O_RDWR || flag == (JUS_O_RDWR | JUS_O_CREATE)){
        b = "rb+";
    } else if (flag == JUS_O_APPEND || flag == (JUS_O_APPEND | JUS_O_CREATE)){
        b = "ab";
    } else {
        return JAVACALL_INVALID_ARGUMENT;
    }
    
    f = fopen(name,b);

    if ((flag & JUS_O_CREATE)!=0 && !f){
        f = fopen(name,"wb");
    }

    if (f == 0) {
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
    return  fread(buffer, 1, size, (FILE*)storage); 
}

int javautil_storage_write(javautil_storage storage, char* buffer, unsigned int size)
{
    int ret = fwrite(buffer,1, size, (FILE*)storage); 
    return ret;
}

javacall_result  javautil_storage_flush(javautil_storage storage)
{
    return (fflush((FILE*)storage) == 0) ? JAVACALL_OK : JAVACALL_IO_ERROR;
}

javacall_result  javautil_storage_lock(javautil_storage storage)
{
    return JAVACALL_OK;
}

javacall_result  javautil_storage_unlock(javautil_storage storage)
{
    return JAVACALL_OK;
}

javacall_result  javautil_storage_get_modified_time(const char* fname, unsigned long* modified_time)
{
    struct stat st={0};
    if (0!=stat(fname,&st)) return JAVACALL_IO_ERROR;
    if (modified_time) *modified_time = st.st_mtime;
    return JAVACALL_OK;
}
