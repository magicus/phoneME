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
#include "javacall_logging.h"
#include "javacall_meminfo.h"
#include "javacall_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

mem_alloc_info* memInfoList = NULL;
static meminfo_stat memStat;

/* memInfo List functions*/
void initMemInfo(mem_alloc_info* memInfo);
static mem_alloc_info* buffer_2_meminfo(void* buffer);
void add_mem_alloc_info(mem_alloc_info* newMemInfo);
void remove_mem_alloc_info(mem_alloc_info* remMemInfo);

void* javacall_meminfo_memory_heap_allocate(long size, /*OUT*/ long* outSize){
	javacall_os_memory_heap_allocate(size,outSize);
}

void javacall_meminfo_memory_heap_deallocate(void* heap){
	javacall_os_memory_heap_deallocate(heap);
	print_memory_alloc_report();
}

void* javacall_meminfo_malloc(unsigned int size, char* fileName, unsigned int line){
	unsigned char* retBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	memInfo = javacall_os_malloc(sizeof(mem_alloc_info));

	if(memInfo != NULL) {
	
		initMemInfo(memInfo);

		memInfo->buffer = javacall_os_malloc(size);

		if(memInfo->buffer != NULL) {
			memInfo->size = size;
			OS_STRCPY(memInfo->fileName, fileName);
			memInfo->line = line;
		
			add_mem_alloc_info(memInfo);
	
			retBuffer = memInfo->buffer;
		}else{
			// malloc failed free meminfo structure
			javacall_free(memInfo);
		}
	}

	return retBuffer;
}

void  javacall_meminfo_free(void* ptr, char* fileName, unsigned int line){
	unsigned char* completeBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	memInfo = buffer_2_meminfo(ptr);

	remove_mem_alloc_info(memInfo);

	javacall_os_free(memInfo->buffer);
	javacall_os_free(memInfo);
}

void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
	javacall_meminfo_free(ptr, fileName, line);

	return javacall_meminfo_malloc(size, fileName, line);
}

void*  javacall_meminfo_calloc(unsigned int numberOfElements, unsigned int elementSize, char* fileName, unsigned int line){
	void* retBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	memInfo = javacall_os_malloc(sizeof(mem_alloc_info));

	if(memInfo != NULL) {

		initMemInfo(memInfo);

		memInfo->buffer = javacall_os_calloc(numberOfElements, elementSize);

		if(memInfo->buffer != NULL) {
			retBuffer = memInfo->buffer;
			memInfo->size = elementSize * numberOfElements;
			OS_STRCPY(memInfo->fileName, fileName);
			memInfo->line = line;
	
			add_mem_alloc_info(memInfo);
		}else{
			// malloc failed free meminfo structure
			javacall_free(memInfo);
		}
	}

	return retBuffer;
}

#if 0//amk todo: duplicate function need to check why?
void* javacall_meminfo_realloc(void* ptr, unsigned int size, char* fileName, unsigned int line){
}
#endif

void* javacall_meminfo_strdup(const char* str, char* fileName, unsigned int line){
	void* retBuffer = NULL;
	mem_alloc_info* memInfo = NULL;

	memInfo = javacall_os_malloc(sizeof(mem_alloc_info));

	if(memInfo != NULL) {

		initMemInfo(memInfo);

		memInfo->buffer = javacall_os_strdup(str);

		if(memInfo->buffer != NULL) {
			retBuffer = memInfo->buffer;
			memInfo->size = OS_STRLEN(str);
			OS_STRCPY(memInfo->fileName, fileName);
			memInfo->line = line;

			add_mem_alloc_info(memInfo);
		}else{
			// malloc failed free meminfo structure
			javacall_free(memInfo);
		}
	}

	return retBuffer;
}

static void initMemInfo(mem_alloc_info* memInfo){
	memInfo->fileName[0] = '\0';
	memInfo->line = 0;
	memInfo->next = NULL;
	memInfo->prev = NULL;
}

static mem_alloc_info* buffer_2_meminfo(void* buffer){
	mem_alloc_info* retMemInfo = NULL;

	retMemInfo = memInfoList;

	while(retMemInfo != NULL) {
		if(retMemInfo->buffer == buffer) {
			break;
		}

		retMemInfo = (mem_alloc_info*)retMemInfo->next;
	}

	return retMemInfo;
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

void print_memory_alloc_report(){
	mem_alloc_info* memInfo = NULL;
	char line[1024];

	javacall_print("\n------------------------\n\0");

	sprintf(line, "Max Memory usage was %d\n\0", memStat.maxMemoryUsage);
	javacall_print(line);

	sprintf(line, "Current Memory usage is %d\n\0", memStat.currentMemeoryUsage);
	javacall_print(line);

	if(memStat.currentMemeoryUsage != 0) {

		javacall_print("Memory Leak from:\n\0");

		memInfo = memInfoList;

		while(memInfo != NULL) {

			sprintf(line, "\t%s:%d\n\0", memInfo->fileName, memInfo->line);
			javacall_print(line);

			memInfo = memInfo->next;
		}
	}

	javacall_print("------------------------\n\0");
}

#ifdef __cplusplus
}
#endif
