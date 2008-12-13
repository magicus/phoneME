/*
 *   
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

/**
 * @file
 */

/**
 * JVM native public memory functions.
 *
 * With these functions, we have the ability to manage our own
 * memory pool. This avoids memory fragmentation in the overall
 * system. However, it does not address the potential problem
 * of memory fragmentation within the JVM application itself.
 *
 * The memory pool is one contiguous chunk of memory split up
 * into various free or allocated blocks. All blocks have the
 * same layout.
 *
 * <table border=1>
 * <tr><th scope=col>Contents of the Memory Block</th></tr>
 * <tr><td>magic (value of 0xCAFE)</td></tr>
 * <tr><td>free (value of 0 or 1)</td></tr>
 * <tr><td>reserved/<sup>[*]</sup>guardSize</td></tr>
 * <tr><td>size</td></tr>
 * <tr><td><sup>[*]</sup>filename</td></tr>
 * <tr><td><sup>[*]</sup>lineno</td></tr>
 * <tr><td>1 .. size</td></tr>
 * <tr><td><sup>[*]</sup>1 .. guardSize</td></tr>
 * </table>
 *
 * <p>Items that have the prefix <sup>[*]</sup> are only enabled if memory
 * tracing is enabled.
 *
 * @warning This code is not thread safe.
 */
#include "incls/_precompiled.incl"


#include <stdio.h>
#include <string.h>


#include "JVM_Malloc.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

#if ENABLE_JVM_MALLOC

void *OsMemory_allocate(size_t size);

void OsMemory_free(void *p);

/**
 * Structure to hold memory blocks
 */
typedef struct _jvmMemStruct {
    unsigned short magic;                                    /* magic number */
    char           free;           /* 1 == block is free, 0 == block is used */
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
    char           guardSize;           /* Size of tail guard data; in bytes */
#else
    char           reserved;
#endif 
#else
	char           reserved;
#endif
    unsigned int   size;                                    /* size of block */
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
    char*          filename;         /* filename where allocation took place */
    unsigned int   lineno;        /* line number wehre allocation took place */
    unsigned int   guard;                                    /* memory guard */
#endif
#endif
} _JvmMemHdr, *_JvmMemHdrPtr;

/*
 * Default size of pool usable for allocations; in bytes
 */
#define DEFAULT_POOL_SIZE (256*1024)

/*
 * Byte boundary for word alignment
 */
#define ALIGNMENT     0x00000003                  /* Assumes word is 4-bytes */

/*
 * Constant to verify a header's validity
 */
#define MAGIC         0xCAFE

/*
 * Constants to guard memory
 */
#define GUARD_WORD    0x9A9A9A9A                  /* Assumes word is 4-bytes */
#define GUARD_BYTE    0x9A

/*
 * Minimum number of guard bytes to put at end of the memory block
 */
#define GUARD_SIZE    4

/* #define JVM_MALLOC_USE_STATIC */
#ifdef JVM_MALLOC_USE_STATIC
/* Cannot allocate dynamic memory on the phone. Use static array. */
static char JvmMemory[DEFAULT_POOL_SIZE];       /* Where JVM memory starts */
#else  /* use malloc or similar function provided */
static char* JvmMemory;                         /* Where JVM memory starts */
#endif

static char* JvmMemoryStart = NULL;                /* Aligned start of JVM memory */
static char* JvmMemoryEnd;                         /* End of memory */


int jvm_trace_malloc = 0;
int jvm_tag_malloc = 0;
int jvm_trash_malloc = 0;
int jvm_malloc_total = 0;

 int jvmEndMemory(int* count, int* size);

#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
static int JvmMemoryHighWaterMark;
static int JvmMemoryAllocated;
static int verifyTailGuardData(_JvmMemHdrPtr jvmMemoryHdr);
#endif
#endif

#ifdef REPORT_LEVEL

#if REPORT_LEVEL <= LOG_ERROR
/**
 * An internal helper function.
 * Calls to <code>printAllocation()</code> should be wrapped like this, 
 * as if they were <code>reportToLog()</code> calls.
 *
 * #if REPORT_LEVEL <= severity
 * printAllocation(severity, what, filename, lineno);
 * #endif
 */
 void
printAllocation(int severity, const char* what, char* filename, int lineno) {
    reportToLog(severity, LC_MALLOC, "alloc: %s at %s line %d", 
		what, filename, lineno);
}
#else
#define printAllocation(w,x,y,z)
#endif 

#endif

#if ENABLE_JVM_MEM_API
/***
* FUNCTION:      JVM_Memset()
* TYPE:          public operation*
*
*OVERVIEW:
*       Sets the first "count" bytes of the memory starting
*       at "dstPtr" to the character value "val".
*
*INTERFACE:
*  parameter:
*       void *dstPtr - pointer to memory to fill with val
*       int val   - value to put in dstPtr bytes
*       int count - number of bytes of dstPtr to fill
*
*  return:
*       returns dstPtr, with filled bytes
*
*******************************************************************************/

void * JVM_Memset (
    void *dst,
    int val,
    int count
    )
{
    void *start = dst;
    while (count--) {
        *(char *)dst = (char)val;
        dst = (char *)dst + 1;
    }

    return(start);
}

/***
* FUNCTION:      JVM_Memcpy()
* TYPE:          public operation*
*OVERVIEW:
*       JVM_Memcpy() copies a source memory buffer to a destination memory buffer.
*   
*INTERFACE:
* parameters:
*       void *dstPtr = pointer to destination buffer
*       const void *srcPtr = pointer to source buffer
*       int count = number of bytes to copy
*
* return:
*       Returns a pointer to the destination buffer
*
*******************************************************************************/

void * JVM_Memcpy (
    void * dstPtr,
    const void * srcPtr,
    int count
    )
{
    void * ret = dstPtr;


    while (count--) {
        *(char *)dstPtr = *(char *)srcPtr;
        dstPtr = (char *)dstPtr + 1;
        srcPtr = (char *)srcPtr + 1;
    }


    return(ret);
}

#endif

/**
 * @internal
 *
 * FUNCTION:      file_tail()
 * TYPE:          public operation
 * OVERVIEW:      strip the path from the filename
 *                 
 * INTERFACE:
 *   parameters:  char* file: filename
 *
 *   returns:     0 on succes; != 0 on failure
 *                
 */
static char * file_tail(char *file) 
{
  char *p, *q;
  q = file;

  if(q == NULL) {
    return q;
  }

  for (p=file; *p; p++) {
    if ((p[0] == '/' || p[0] == '\\') && p[1] != 0) {
      q = p+1;
    }
  }
  return q;
}

char * jvm_Strdup(const char *str) 
{

  int len = strlen(str);
  char * p = (char*)jvmMallocImpl(len + 1, __FILE__, __LINE__);
  if (p){
    strncpy(p, str, len);
    p[len] = '\0';
  }
  return p; /* null if p could not be allocated */
}

/**
 * FUNCTION:      JVM_InitializeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the JVM memory pool
 *                 NOTE: This must only be called once
 * INTERFACE:
 *   parameters:  size   Size of memory pool to use; if size is '-1',
 *                        the default memory pool size will be used
 *   returns:     0 on succes; != 0 on failure
 *                
 */

int
JVM_InitializeMemory(char* start, int size) {
    _JvmMemHdrPtr jvmMemoryHdr;

    if (JvmMemoryStart != NULL) {
        /* avoid a double init */
        return 0;
    }

    if (size < 0) {
        /* size not specified, use the default */
        size = DEFAULT_POOL_SIZE;
    }

    if(jvm_trace_malloc) {
        tty->print_cr("size = %d", size);
    }

    if(start == NULL)  {
        /* Need malloc the memory pool dynamically */

        /* allocate the chunk of memory to C heap */
#ifdef JVM_MALLOC_USE_CLDC_HI
        JvmMemory = (char*) OsMemory_allocate(size);
#else
        JvmMemory = (char*)malloc(size);
#endif
        if (JvmMemory == NULL) {
            return -1;
        }

     } else {
        
#define JVM_MALLOC_USE_STATIC
        JvmMemory = start;
     }

    JvmMemoryStart = JvmMemory;
    JvmMemoryEnd   = JvmMemory + size - sizeof(_JvmMemHdr);

    /* Word alignment */
    while (((long)JvmMemoryStart & ALIGNMENT) != 0) {
        JvmMemoryStart++;
    }

    jvmMemoryHdr = (_JvmMemHdrPtr)JvmMemoryStart;
    jvmMemoryHdr->magic = MAGIC;
    jvmMemoryHdr->free  = 1;
    jvmMemoryHdr->size  = (JvmMemory - JvmMemoryStart)
                           + size - sizeof(_JvmMemHdr);
    return size;
}



/**
 * FUNCTION:      JVM_FinalizeMemory()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the JVM memory pool
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *                
 */
  void
JVM_FinalizeMemory(void) {
    int count, size, ret;

    ret = jvmEndMemory(&count, &size);

#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
    if (ret > 0) {
        reportToLog(LOG_WARNING, LC_MALLOC, 
		  "WARNING: %d memory leak(s); %d bytes!\n",
		  count, size);
    }
    reportToLog(LOG_WARNING, LC_MALLOC, 
		"** Total memory: %d\n** Highwater mark:%d",
		JVM_GetTotalHeap(), JvmMemoryHighWaterMark);
#endif 
#endif

#ifndef JVM_MALLOC_USE_STATIC
#ifdef JVM_MALLOC_USE_CLDC_HI
    OsMemory_free(JvmMemory);
#else
    free(JvmMemory);
#endif
    JvmMemory = NULL;
#endif

    JvmMemoryStart = NULL;
    JvmMemoryEnd = NULL;
}


/**
 * @internal
 *
 * FUNCTION:      jvmEndMemory()
 * TYPE:          private operation
 * OVERVIEW:      Finalize the JVM memory pool
 * INTERFACE:
 *   parameters:  count   address to store memory leak count
 *                size    address to store totol bytes of memory leaked
 *   returns:     the number of memory leaks detected
 *                
 */
 int
jvmEndMemory(int* count, int* size) {
    _JvmMemHdrPtr jvmMemoryHdr;
    char*          jvmMemoryPtr;

    *count = 0;
    *size  = 0;

    for (jvmMemoryPtr = JvmMemoryStart; 
         jvmMemoryPtr < JvmMemoryEnd;
         jvmMemoryPtr += jvmMemoryHdr->size + sizeof(_JvmMemHdr)) {

        jvmMemoryHdr = (_JvmMemHdrPtr)jvmMemoryPtr;

        if (jvmMemoryHdr->magic != MAGIC) {
            tty->print_cr( 
			  "ERROR: memory corruption at 0x%p", 
			  jvmMemoryPtr);
            return -1;
        } else if (jvmMemoryHdr->free != 1) {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
	    reportToLog(LOG_WARNING, LC_MALLOC, 
			"WARNING: memory leak: size= %d  address= 0x%p",
			jvmMemoryHdr->size,
			(void*)((char*)jvmMemoryHdr + sizeof(_JvmMemHdr)));
            printAllocation(LOG_WARNING, "allocated", 
                            jvmMemoryHdr->filename, jvmMemoryHdr->lineno);
#endif
#endif
            JVM_Free((void*)((char*)jvmMemoryHdr + sizeof(_JvmMemHdr)));
            *count += 1;
            *size  += jvmMemoryHdr->size;
        }
    }
    return *count;
}


/**
 * FUNCTION:      jvmMallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private JVM memory pool
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated memory
 *                
 */
 void*
jvmMallocImpl(unsigned int size, char* filename, int lineno) {
    unsigned int   numBytesToAllocate = size;
    void*          loc     = NULL;
    _JvmMemHdrPtr tempHdr = NULL;
    char*          temp    = NULL;
    char*          jvmMemoryPtr;
    char*          jvmMemoryLast;
    _JvmMemHdrPtr jvmMemoryHdr;
    int   guardSize = 0;
    void* guardPos;


    if (jvm_tag_malloc) {
        guardSize = 4;
    } else {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
        guardSize = GUARD_SIZE;
#endif
#endif
    }

    while ( (numBytesToAllocate & ALIGNMENT) != 0 ) {
        numBytesToAllocate++;
	size++;
    }

    numBytesToAllocate += guardSize;
    jvmMemoryLast = JvmMemoryStart;

    /* find a free slot */
    for (jvmMemoryPtr = JvmMemoryStart;
         jvmMemoryPtr < JvmMemoryEnd;
         jvmMemoryLast = jvmMemoryPtr,
         jvmMemoryPtr += jvmMemoryHdr->size + sizeof(_JvmMemHdr)) {
        
        jvmMemoryHdr = (_JvmMemHdrPtr)jvmMemoryPtr;
        if (jvmMemoryHdr->magic != MAGIC) {
	    tty->print_cr("ERROR: Memory corruption at 0x%p, in chunk 0x%p", 
			  jvmMemoryPtr, jvmMemoryLast); 
            return((void *) 0);
        } else {
            while ( 1 ) {
                /* coalescing */
                if (jvmMemoryHdr->free == 1) {
                    /* if current block is free */
                    temp = (char*)jvmMemoryHdr;
                    temp += jvmMemoryHdr->size + sizeof(_JvmMemHdr);
                    tempHdr = (_JvmMemHdrPtr)temp;

                    if ((temp < JvmMemoryEnd) && 
			(tempHdr->free == 1) && (tempHdr->magic == MAGIC)) {
		        /* and the next block is free too */
		        /* then coalesce */
		        jvmMemoryHdr->size += tempHdr->size
			    + sizeof(_JvmMemHdr);
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_INFORMATION
                        reportToLog(LOG_INFORMATION, LC_MALLOC,
                           "DEBUG: Coalescing blocks 0x%p and 0x%p",
				     jvmMemoryHdr, tempHdr);
#endif
#endif

                    } else {
		        break;
                    }
                } else {
		    break;
                }
            } /* while */

            /* allocating */
            if ((jvmMemoryHdr->free == 1) && 
                (jvmMemoryHdr->size >= numBytesToAllocate)) {
                if (jvmMemoryHdr->size > (numBytesToAllocate 
                                              + sizeof(_JvmMemHdr) + 4)) {
                    /* split block */
                    _JvmMemHdrPtr nextHdr;
                    nextHdr = (_JvmMemHdrPtr)((char *)jvmMemoryPtr
                                               + numBytesToAllocate
                                               + sizeof(_JvmMemHdr));
                    nextHdr->magic = MAGIC;
                    nextHdr->free = 1;
                    nextHdr->size = jvmMemoryHdr->size 
                                    - numBytesToAllocate 
                                    - sizeof(_JvmMemHdr);
                    jvmMemoryHdr->size     = numBytesToAllocate;
                } else
                {
                    size = jvmMemoryHdr->size - guardSize;
                }
                jvmMemoryHdr->free     = 0;
                loc = (void*)((char*)jvmMemoryHdr + sizeof(_JvmMemHdr));

                if (jvm_tag_malloc) {
                    /* Add tail guard */
	            guardPos = (void*)((char*)loc + jvmMemoryHdr->size - guardSize);
		    ((unsigned char*)guardPos)[0] = (char)0x01;
                    ((unsigned char*)guardPos)[1] = (char)0xcd;
                    ((unsigned char*)guardPos)[2] = (char)0x02;
                    ((unsigned char*)guardPos)[3] = (char)0xef;
		} else {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
                  jvmMemoryHdr->guard    = GUARD_WORD;      /* Add head guard */
                  jvmMemoryHdr->filename = filename;
                  jvmMemoryHdr->lineno   = lineno;

                  /* Add tail guard */
                  guardSize = jvmMemoryHdr->size - size;

                  jvmMemoryHdr->guardSize = guardSize;
                  guardPos = (void*)((char*)loc + jvmMemoryHdr->size 
                                   - guardSize);
                  for(i=0; i<guardSize; i++) {
                      ((unsigned char*)guardPos)[i] = GUARD_BYTE;
                  }
                
                  JvmMemoryAllocated += numBytesToAllocate;
                  if (JvmMemoryAllocated > JvmMemoryHighWaterMark) {
                      JvmMemoryHighWaterMark = JvmMemoryAllocated;
                  }
#endif 
#endif
		}
               
	 	if (loc != NULL && jvm_tag_malloc) {
		    jvm_malloc_total += size;
		}

                if (jvm_trace_malloc) {
                    tty->print_cr("|%x| Malloc(%d): total=%d, %s:%d ", loc, size, 
                    jvm_malloc_total, file_tail(filename), lineno);
                }

                if (jvm_trash_malloc && loc != NULL) {
		   JVM_Memset(loc, 0xab, size);
		}
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_INFORMATION
                reportToLog(LOG_INFORMATION, LC_MALLOC, 
			    "DEBUG: Requested %d provided %d at 0x%p",
			    numBytesToAllocate, jvmMemoryHdr->size, loc);
                printAllocation(LOG_INFORMATION, "allocated", filename, lineno);
#else
                (void)filename; /* No-op */
                (void)lineno;   /* No-op */
#endif
#endif
                return(loc);
            } /* end of allocating */
        } /* end of else */
    } /* end of for */
    tty->print_cr("DEBUG: Unable to allocate %d bytes", 
		 numBytesToAllocate);
    return((void *)0);
}


/**
 * FUNCTION:      jvmCallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private JVM memory pool,
 *                 memory contents are cleared
 * INTERFACE:
 *   parameters:  nelem      Number of elements to allocate
 *                elsize     Size of one element
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated and cleared memory
 *                
 */
 void*
jvmCallocImpl(unsigned int nelem, unsigned int elsize, 
               char* filename, int lineno) {
    void *loc = NULL;
    
    if (jvm_trace_malloc) {
      tty->print_cr("Calloc %s:%d: ", file_tail(filename), lineno);
    }

    if ((loc = jvmMallocImpl((nelem) * (elsize), filename, lineno)) != NULL) {
        JVM_Memset(loc, 0, nelem * elsize);
    }
    return loc;
}

/**
 * FUNCTION:      jvmReallocImpl()
 * TYPE:          public operation
 * OVERVIEW:      Re-allocate memory from the private JVM memory pool
 * INTERFACE:
 *   parameters:  ptr        Original memory pointer
 *                size       New size
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the re-allocated memory
 *                
 */
 void*
jvmReallocImpl(void* ptr, unsigned int size, char* filename, int lineno) {
    void*          newPtr = NULL;
    _JvmMemHdrPtr memHdr;
    int           guardSize = 0;

    if(jvm_tag_malloc) {
	guardSize = 4;
    }

    if (ptr == NULL && size == 0) {
	return NULL;
    }

    if (jvm_trace_malloc) {
       tty->print_cr("Realloc: ");
    }

    /* If ptr is NULL, realloc() behaves like malloc() for the given size. */
    if (ptr == NULL) {
        ptr = jvmMallocImpl(size, filename, lineno);
        return ptr;
    }

    memHdr = (_JvmMemHdrPtr)((char*)ptr - sizeof(_JvmMemHdr));

    if (memHdr->size != size) {
        if (size != 0) {
            newPtr = jvmMallocImpl(size, filename, lineno);
            if (newPtr != NULL) {
                if (memHdr->size < size) {
                    JVM_Memcpy(newPtr, ptr, memHdr->size);
                } else {
                    JVM_Memcpy(newPtr, ptr, size);
                }
                jvmFreeImpl(ptr, filename, lineno);
            }
        } else {
            /* When size == 0, realloc() acts just like free() */
            jvmFreeImpl(ptr, filename, lineno);
        }
    } else {
        /* sizes are the same, just return the same pointer */
        newPtr = ptr;
    }
        
    return newPtr;
}


/**
 * FUNCTION:      jvmStrdupImpl()
 * TYPE:          public operation
 * OVERVIEW:      Duplicate the given string
 * INTERFACE:
 *   parameters:  s1         String to duplicate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the duplicate string
 *                
 */
 char*
jvmStrdupImpl(const char *s1, char* filename, int lineno) {
    char *p = (char *)jvmMallocImpl(strlen(s1) + 1, filename, lineno);

    if ( p != NULL ) {
        strcpy(p, s1);
    }
    return(p);
}


/**
 * FUNCTION:      jvmFreeImpl()
 * TYPE:          public operation
 * OVERVIEW:      Free memory allocated from the private JVM memory pool
 * INTERFACE:
 *   parameters:  ptr        Pointer to allocated memory
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     <nothing>
 *                
 */
 void
jvmFreeImpl(void *ptr, char *filename, int lineno) {
    _JvmMemHdrPtr jvmMemoryHdr;
    int guardSize = 0;

    if(jvm_tag_malloc) {
       guardSize = 4;
    }

    if (ptr == NULL) {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_INFORMATION
        reportToLog(LOG_INFORMATION, LC_MALLOC, 
		    "DEBUG: Attempt to free NULL pointer");
        printAllocation(LOG_INFORMATION, "freed", filename, lineno);
#endif
#endif
    } else if (((char*)ptr > JvmMemoryEnd) || 
               ((char*)ptr < JvmMemoryStart)) {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_ERROR
        reportToLog(LOG_ERROR, LC_MALLOC, 
		    "ERROR: Attempt to free memory out of scope: 0x%p", ptr);
	printAllocation(LOG_ERROR, "freed", filename, lineno);
#endif
#endif
    } else {

	jvmMemoryHdr = (_JvmMemHdrPtr)((char*)ptr -sizeof(_JvmMemHdr));

        if (jvm_trace_malloc) {
            tty->print("|%x| Free, total=%d, %s:%d    ", ptr, jvm_malloc_total,
              file_tail(filename), lineno);
        }
		
        if (jvmMemoryHdr->magic != MAGIC) {

	    if(jvm_tag_malloc) {
		tty->print_cr(" **** BAD TAG (0x%x) **** \n", jvmMemoryHdr->magic);
	    }
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_ERROR
	    reportToLog(LOG_ERROR, LC_MALLOC, 
			"ERROR: Attempt to free corrupted memory: 0x%p", 
			ptr);
            printAllocation(LOG_ERROR, "freed", filename, lineno);
#endif
#endif
        } else if (jvmMemoryHdr->free != 0) {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_ERROR
	    reportToLog(LOG_ERROR, LC_MALLOC, 
			"ERROR: Attempt to free memory twice: 0x%p", ptr);
            printAllocation(LOG_ERROR, "freed", filename, lineno);
#endif
#endif
        } else {
	    if(jvm_tag_malloc) {
	       if (*(((char*)ptr) + jvmMemoryHdr->size - guardSize + 0) != (char)0x01 ||
                 *(((char*)ptr) + jvmMemoryHdr->size - guardSize + 1) != (char)0xcd ||
                 *(((char*)ptr) + jvmMemoryHdr->size - guardSize + 2) != (char)0x02 ||
                 *(((char*)ptr) + jvmMemoryHdr->size - guardSize + 3) != (char)0xef) {
                 tty->print_cr(" **** BAD POST TAG **** ");
		 }
	    } else {
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
                JvmMemoryAllocated -= jvmMemoryHdr->size;
                /* The memory block header is valid, now check the guard data */
                if (jvmMemoryHdr->guard != GUARD_WORD) {
	            reportToLog(LOG_WARNING, LC_MALLOC, 
			        "ERROR: Possible memory underrun: 0x%p", ptr);
                    printAllocation(LOG_WARNING, "allocated", 
				    jvmMemoryHdr->filename, 
				    jvmMemoryHdr->lineno);
                    printAllocation(LOG_WARNING, "freed", filename, lineno);
                } else if (verifyTailGuardData(jvmMemoryHdr)) {
                   reportToLog(LOG_WARNING, LC_MALLOC, 
			       "ERROR: Possible memory overrun: 0x%p", ptr);
                    printAllocation(LOG_WARNING, "allocated", 
				   jvmMemoryHdr->filename, 
				   jvmMemoryHdr->lineno);
                    printAllocation(LOG_WARNING, "freed", filename, lineno);
                }
#endif 
#endif
	    }
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_INFORMATION
            reportToLog(LOG_INFORMATION, LC_MALLOC, 
			"DEBUG: free %d bytes: 0x%p", 
			jvmMemoryHdr->size, ptr);
            printAllocation(LOG_INFORMATION, "allocated", 
                            jvmMemoryHdr->filename, jvmMemoryHdr->lineno);
            printAllocation(LOG_INFORMATION, "freed", filename, lineno);
#else
            (void)filename; /* No-op */
            (void)lineno;   /* No-op */
#endif
#endif
            jvmMemoryHdr->free = 1;
	    if(jvm_tag_malloc) {
	       jvm_malloc_total -= (jvmMemoryHdr->size - guardSize);
            }
	    if (jvm_trace_malloc) {
              tty->print_cr("OK");
            }
        }
	if (jvm_trash_malloc)
	{
          if (jvmMemoryHdr->size < 1024)
	  {
             JVM_Memset(ptr, 0xaf, jvmMemoryHdr->size);
          } 
	  
        }

    } /* end of else */

}

#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING

/**
 * @internal
 *
 * FUNCTION:      verifyTailGuardData()
 * TYPE:          private operation
 * OVERVIEW:      Verify guard data at the end of the memory is valid
 * INTERFACE:
 *   parameters:  jvmMemoryHdr   Pointer to memory block header
 *   returns:     0 if guard data is valid; otherwise, the byte position
 *                 of the first incorrect guard data byte
 *                
 */
 static int
verifyTailGuardData(_JvmMemHdrPtr jvmMemoryHdr) {
    void* guardPos;
    int   guardSize;
    int   i;

    guardSize = jvmMemoryHdr->guardSize;
    guardPos = (void*)((char*)jvmMemoryHdr
                       + sizeof(_JvmMemHdr)
                       + jvmMemoryHdr->size - guardSize - 1);
    for(i = 1; i <= guardSize; i++) {
        if (((unsigned char*)guardPos)[i] != GUARD_BYTE) {
            return i;
        }
    }
    return 0;
}
#endif
#endif

/**
 *
 * FUNCTION:      JVM_GetTotalHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the total amount of available heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The total amount of available heap
 *                
 */
 int
JVM_GetTotalHeap() {
    return (JvmMemoryEnd - JvmMemoryStart);
}


/**
 * FUNCTION:      JVM_GetFreeHeap()
 * TYPE:          public operation
 * OVERVIEW:      Get the current amount of unused heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The current amount of unused heap
 *                
 */
 int
JVM_GetFreeHeap() {
    _JvmMemHdrPtr jvmMemoryHdr;
    char*          jvmMemoryPtr;
    int            size = 0;

    for (jvmMemoryPtr = JvmMemoryStart; 
         jvmMemoryPtr < JvmMemoryEnd;
         jvmMemoryPtr += jvmMemoryHdr->size + sizeof(_JvmMemHdr)) {

        jvmMemoryHdr = (_JvmMemHdrPtr)jvmMemoryPtr;

        if (jvmMemoryHdr->free != 1) {
            size += jvmMemoryHdr->size;
        }
    }
    return (JVM_GetTotalHeap() - size);
}



/* Set countMemoryLeaksOnly = 0 in order to get more verbose information */
 int JVM_MallocDumpMemory(int countMemoryLeaksOnly)
{
    char *localjvmMallocMemPtr = NULL;
    char *localjvmMallocMemStart = JvmMemoryStart;
    char *localjvmMallocMemEnd = JvmMemoryEnd;
    _JvmMemHdrPtr localjvmMallocMemHdr = NULL;

    int numberOfAllocatedBlocks = 0;

    tty->print_cr(
		 "JvmMemory=0x%p JvmMemoryStart=0x%p JvmMemoryEnd=0x%p", 
		 JvmMemory, JvmMemoryStart, JvmMemoryEnd);

    for (localjvmMallocMemPtr = localjvmMallocMemStart; 
        localjvmMallocMemPtr < localjvmMallocMemEnd;
        localjvmMallocMemPtr += localjvmMallocMemHdr->size + sizeof(_JvmMemHdr)) {

        localjvmMallocMemHdr = (_JvmMemHdrPtr) localjvmMallocMemPtr;
        if (localjvmMallocMemHdr->magic != MAGIC) {
            tty->print_cr( 
			  "ERROR: memory corruption at 0x%p", 
			  localjvmMallocMemPtr);
            return -1;
        } else {

            if (countMemoryLeaksOnly == 0) {
                tty->print_cr( 
			     "hdr 0x%p free=%d size=%d address=0x%p",
			     localjvmMallocMemHdr, 
			     localjvmMallocMemHdr->free, 
			     localjvmMallocMemHdr->size,
			     (void *)(((char *)localjvmMallocMemHdr) + 
				      sizeof(_JvmMemHdr)));
            }

            if (localjvmMallocMemHdr->free != 1) {
                numberOfAllocatedBlocks += 1;
#ifdef REPORT_LEVEL
#if REPORT_LEVEL <= LOG_WARNING
		reportToLog(LOG_WARNING, LC_MALLOC, 
			    "WARNING: memory leak: size=%d  address=0x%p",
			    localjvmMallocMemHdr->size, 
			    (void*)((char*)localjvmMallocMemHdr + 
				    sizeof(_JvmMemHdr)));
		printAllocation(LOG_WARNING, "allocated", 
				localjvmMallocMemHdr->filename, 
				localjvmMallocMemHdr->lineno);
#endif
#endif
            }
        }
    }
    return numberOfAllocatedBlocks;
}


#endif  /* ENABLE_JVM_MALLOC*/

#ifdef __cplusplus
}
#endif
