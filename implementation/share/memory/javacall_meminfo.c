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

#include "javacall_meminfo.h"

#ifdef __cplusplus
extern "C" {
#endif

static char* memInfoList = NULL;
static meminfo_stat;

/* memInfo List functions*/
void initMemInfo(malloc_info* memInfo);
void add_memory_info(malloc_info* newMemInfo);
void remove_memory_info(malloc_info* remMemInfo);

void* javacall_meminfo_malloc(unsigned int size, char* fileName, unsigned int line){
	unsigned char completeBuffer = NULL;
	unsigned int totalSize = size + sizeof(memory_info);
	memory_info* memInfo = NULL;

	completeBuffer = javacall_os_malloc(totalSize) + sizeof(memory_info);

	if(completeBuffer != NULL) {
	
		memInfo = (memory_info*)(buffer);
		initMemInfo(memInfo);
	
		memInfo->size = size;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;
	
		add_memory_info(memInfo);

		completeBuffer += sizeof(memory_info);

		meminfo_stat.currentMemeoryUsage += size;
	}

	return completeBuffer
}

void  javacall_meminfo_free(void* ptr, char* fileName, unsigned int line){
	unsigned char completeBuffer = NULL;
	memory_info* memInfo = NULL;

	completeBuffer = (ptr - sizeof(memory_info));
	memInfo = (memory_info*)(ptr - sizeof(memory_info));

	remove_memory_info(memInfo);

	meminfo_stat.currentMemeoryUsage -= memInfo.size;
	javacall_os_free(completeBuffer);
}

void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
	javacall_meminfo_free(ptr, fileName, line);

	return javacall_meminfo_malloc(size, fileName, line);
}

javacall_meminfo_calloc(nunsigned int numberOfElements, unsigned int elementSize, char* fileName, unsigned int line){
	unsigned char completeBuffer = NULL;
	unsigned int totalSize = size + sizeof(memory_info);
	memory_info* memInfo = NULL;

	completeBuffer = javacall_os_calloc(totalSize) + sizeof(memory_info);

	if(completeBuffer != NULL) {

		memInfo = (memory_info*)(buffer);
		initMemInfo(memInfo);

		memInfo->size = size;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;

		add_memory_info(memInfo);
	}

	return buffer + sizeof(memory_info);
}

javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
	unsigned char completeBuffer = NULL;
	memory_info* memInfo = NULL;

	completeBuffer = (ptr - sizeof(memory_info));
	memInfo = (memory_info*)(ptr - sizeof(memory_info));

	remove_memory_info(memInfo);

	completeBuffer = javacall_os_realloc(completeBuffer, size + sizeof(memory_info));

	if(completeBuffer != NULL) {

		memInfo = (memory_info*)(buffer);
		initMemInfo(memInfo);

		memInfo->size = size;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;

		add_memory_info(memInfo);
	}

	return completeBuffer + sizeof(memory_info);
}

javacall_meminfo_strdup(const char* str, char* fileName, unsigned int line){
}

static void initMemInfo(malloc_info* memInfo){
	memInfo->fileName[0] = \0;
	memInfo->line = 0;
	memInfo->next = NULL;
	memInfo->prev = NULL;
}

static void add_memory_info(malloc_info* newMemInfo){

	if(memInfoList != NULL){
		malloc_info* prevFirst = memInfoList;
		prevFirst->prev = newMemInfo;
		newMemInfo->next = prevFirst;
	}

	memInfoList = newMemInfo;

}

static void remove_memory_info(malloc_info* remMemInfo){

	if(remMemInfo->prev == NULL) {
		memInfoList = remMemInfo->next;
	}else{
		malloc_info* prev = remMemInfo->prev;

		prev->next = remMemInfo->next;
	}

	if(remMemInfo->next != NULL) {
		malloc_info* next = remMemInfo->next;

		next->prev = remMemInfo->prev;
	}
}

#ifdef __cplusplus
}
#endif
