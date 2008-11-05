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

#include <stdio.h>
#include "javacall_meminfo.h"
#include "javacall_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

mem_alloc_info* memInfoList = NULL;
static meminfo_stat memStat;

/* memInfo List functions*/
void initMemInfo(mem_alloc_info* memInfo);
void add_mem_alloc_info(mem_alloc_info* newMemInfo);
void remove_mem_alloc_info(mem_alloc_info* remMemInfo);

void* javacall_meminfo_malloc(unsigned int size, char* fileName, unsigned int line){
	unsigned char* completeBuffer = NULL;
	unsigned int totalSize = size + sizeof(mem_alloc_info);
	unsigned char* javaRetBuffer = NULL;
	mem_alloc_info* memInfo = NULL;
	
	completeBuffer = javacall_os_malloc(totalSize + sizeof(mem_alloc_info));

	if(completeBuffer != NULL) {
	
		memInfo = (mem_alloc_info*)(completeBuffer);
		initMemInfo(memInfo);
	
		memInfo->size = size;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;
	
		add_mem_alloc_info(memInfo);

		javaRetBuffer = completeBuffer + sizeof(mem_alloc_info);
	}

	return javaRetBuffer;
}

void  javacall_meminfo_free(void* ptr, char* fileName, unsigned int line){
	unsigned char* completeBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	completeBuffer = (unsigned char*)((unsigned char*)ptr - sizeof(mem_alloc_info));
	memInfo = (mem_alloc_info*)((unsigned char*)ptr - sizeof(mem_alloc_info));

	remove_mem_alloc_info(memInfo);

	javacall_os_free(completeBuffer);
}

void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
	javacall_meminfo_free(ptr, fileName, line);

	return javacall_meminfo_malloc(size, fileName, line);
}

void*  javacall_meminfo_calloc(unsigned int numberOfElements, unsigned int elementSize, char* fileName, unsigned int line){
	unsigned char* completeBuffer = NULL;
	unsigned int totalSize = elementSize * numberOfElements + sizeof(mem_alloc_info);
	unsigned int newElementSize = (totalSize + 1)/ numberOfElements;
	mem_alloc_info* memInfo = NULL;

	//amk xxx todo: check the new newElementSize calc
	completeBuffer = javacall_os_calloc(numberOfElements, newElementSize);

	if(completeBuffer != NULL) {

		memInfo = (mem_alloc_info*)(completeBuffer);
		initMemInfo(memInfo);

		memInfo->size = elementSize * numberOfElements;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;

		add_mem_alloc_info(memInfo);
	}

	return completeBuffer + sizeof(mem_alloc_info);
}

#if 0//amk todo: duplicate function need to check why?
void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
	unsigned char completeBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	completeBuffer = (ptr - sizeof(mem_alloc_info));
	memInfo = (mem_alloc_info*)(ptr - sizeof(mem_alloc_info));

	remove_mem_alloc_info(memInfo);

	completeBuffer = javacall_os_realloc(completeBuffer, size + sizeof(mem_alloc_info));

	if(completeBuffer != NULL) {

		memInfo = (mem_alloc_info*)(buffer);
		initMemInfo(memInfo);

		memInfo->size = size;
		OS_STRCPY(memInfo->fileName, fileName);
		memInfo->line = line;

		add_mem_alloc_info(memInfo);
	}

	return completeBuffer + sizeof(mem_alloc_info);
}
#endif

void* javacall_meminfo_strdup(const char* str, char* fileName, unsigned int line){
	int size = OS_STRLEN(str);
	char* newBuffer = javacall_meminfo_malloc(size, fileName,line);

	if(newBuffer != NULL) {
		OS_STRCPY(newBuffer, str);
	}

	return newBuffer;
}

static void initMemInfo(mem_alloc_info* memInfo){
	memInfo->fileName[0] = '\0';
	memInfo->line = 0;
	memInfo->next = NULL;
	memInfo->prev = NULL;
}

static void add_mem_alloc_info(mem_alloc_info* newMemInfo){

	if(memInfoList != NULL){
		mem_alloc_info* prevFirst = memInfoList;
		prevFirst->prev = (struct _malloc_info*)newMemInfo;
		newMemInfo->next = (struct _malloc_info*)prevFirst;
	}

	memInfoList = newMemInfo;

	//update Memory Statistics
	memStat.currentMemeoryUsage += newMemInfo->size;

	if(memStat.currentMemeoryUsage > memStat.maxMemoryUsage) {
		memStat.maxMemoryUsage = memStat.currentMemeoryUsage;
	}

}

static void remove_mem_alloc_info(mem_alloc_info* remMemInfo){

	if(remMemInfo->prev == NULL) {
		memInfoList = (mem_alloc_info*)remMemInfo->next;
	}else{
		mem_alloc_info* prev = (mem_alloc_info*)remMemInfo->prev;

		prev->next = remMemInfo->next;
	}

	if(remMemInfo->next != NULL) {
		mem_alloc_info* next = (mem_alloc_info*)remMemInfo->next;

		next->prev = remMemInfo->prev;
	}

	//update Memory Statistics
	memStat.currentMemeoryUsage -= remMemInfo->size;
}

#ifdef __cplusplus
}
#endif
