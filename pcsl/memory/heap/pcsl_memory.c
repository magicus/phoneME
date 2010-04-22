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

/**
 * @file
 *
 * PCSL memory functions.
 *
 * With these functions, we have the ability to manage our own
 * memory pool. This avoids memory fragmentation in the overall
 * system. However, it does not address the potential problem
 * of memory fragmentation within the PCSL application itself.
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pcsl_memory.h>
#include <pcsl_print.h>
#include <pcsl_memory_port.h>

#ifdef PCSL_DEBUG
/* 
 * define debug macros and function which use pcsl_print() for output 
 */
#define REPORT(msg) report(msg)
#define REPORT1(msg, a1) report(msg, a1)
#define REPORT2(msg, a1, a2) report(msg, a1, a2)
#define REPORT3(msg, a1, a2, a3) report(msg, a1, a2, a3)
#define REPORT4(msg, a1, a2, a3, a4) report(msg, a1, a2, a3, a4)

/*
 * maximum lenght of debug output using report methods
 */
#define RPT_BUF_LEN 200

/*
 * buffer for constructing report strings
 */
static char buf[RPT_BUF_LEN];

/**
 * Report a variable argument message via pcsl_print.
 *
 * The <code>message</code> parameter is treated as a format
 * string to the standard C library call printf would be, with
 * conversion specifications (%s, %d, %c, etc) causing the 
 * conversion and output of each successive argument after
 * <code>message</code>  As with printf, having a conversion
 * character in <code>message</code> without an associated argument
 * following it is an error.
 *
 * To ensure that no character in <code>message</code> is 
 * interpreted as requiring conversion, a safe way to call
 * this method is:
 * <code> report("%s", message); </code>
 * @param message detail message to go with the report
 *                should not be NULL
 */

static void report(char* message, ...){
  
    va_list ap;
    if (message != NULL) {

        va_start(ap, message);
        
#ifdef _WIN32
        _vsnprintf(buf, RPT_BUF_LEN, message, ap);
#else
        vsnprintf(buf, RPT_BUF_LEN, message, ap);
#endif
        pcsl_print(buf);
        
        va_end(ap);
    }
}

/**
 * An internal helper function used when PCSL_DEBUG is defined.
 * 
 * print_alloc( what, filename, lineno);
 */
static void print_alloc(const char* what, const char* filename, int lineno) {
    report("alloc: %s at %s line %d\n", 
           what, filename, lineno);
}

#else  /* PCSL_DEBUG is not defined */

#define REPORT(msg)
#define REPORT1(msg, a1)
#define REPORT2(msg, a1, a2)
#define REPORT3(msg, a1, a2, a3)
#define REPORT4(msg, a1, a2, a3, a4)

#endif 

#ifdef PCSL_DEBUG
/**
 *  If you are interested in very verbose output about every block allocated/coalesced/freed you can enable this define
 *	Keep it zero otherwise
 */
#define PCSL_TRACE_MEMORY  0
#endif 

/**
 * If defined as non-zero, memory allocator will use best-fit algorithm.
 * If defined as zero, memory allocator will use first-fit algorithm.
 *
 * When first-fit algorithm is used, Java heap allocation can fail in SVM mode 
 * even though there is enough free memory in the pool (see CR 6735718).
 * This is because first-fit allocation cause bad fragmentation for allocation
 * request patterns typical for SVM mode.
 */
#define PCSL_BEST_FIT_ALLOCATION 1

/**
 * Structure to hold memory blocks
 */
typedef struct _pcslMemStruct {
    unsigned short magic;                                    /* magic number */
    char           free;           /* 1 == block is free, 0 == block is used */
#ifdef PCSL_DEBUG
    char           guardSize;           /* Size of tail guard data; in bytes */
#else
    char           reserved;
#endif 
    unsigned int   size;                                    /* size of block */
#ifdef PCSL_DEBUG
    const char*    filename;         /* filename where allocation took place */
    unsigned int   lineno;        /* line number wehre allocation took place */
    unsigned int   guard;                                    /* memory guard */
#endif
} _PcslMemHdr, *_PcslMemHdrPtr;

/*
 * Default size of pool usable for allocations; in bytes
 */
#define DEFAULT_POOL_SIZE (8024*1024)

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

#ifdef PCSL_MEMORY_USE_STATIC
/* Cannot allocate dynamic memory on the phone. Use static array. */
static char PcslMemory[DEFAULT_POOL_SIZE];       /* Where PCSL memory starts */
#else  /* use malloc or similar function provided */
static char* PcslMemory;                         /* Where PCSL memory starts */
#endif

static char* PcslMemoryStart;                /* Aligned start of PCSL memory */
static char* PcslMemoryEnd;                                 /* End of memory */

static int PcslMemoryHighWaterMark;
static int PcslMemoryAllocated;

static int pcsl_end_memory(int* count, int* size);

static int verify_tail_guard_data(_PcslMemHdrPtr pcslMemoryHdr);

/**
 * @internal
 *
 * FUNCTION:      pcsl_end_memory()
 * TYPE:          private operation
 * OVERVIEW:      Finalize the PCSL memory pool
 * INTERFACE:
 *   parameters:  count   address to store memory leak count
 *                size    address to store totol bytes of memory leaked
 *   returns:     the number of memory leaks detected
 *                
 */
static int
pcsl_end_memory(int* count, int* size) {
    _PcslMemHdrPtr pcslMemoryHdr;
    char*          pcslMemoryPtr;

    *count = 0;
    *size  = 0;

    for (pcslMemoryPtr = PcslMemoryStart; 
         pcslMemoryPtr < PcslMemoryEnd;
         pcslMemoryPtr += pcslMemoryHdr->size + sizeof(_PcslMemHdr)) {

        pcslMemoryHdr = (_PcslMemHdrPtr)pcslMemoryPtr;

        if (pcslMemoryHdr->magic != MAGIC) {
            REPORT1("ERROR: Corrupted start of memory header: 0x%p\n", 
                    pcslMemoryPtr);
            return -1;
        }
#ifdef PCSL_DEBUG
        if (pcslMemoryHdr->guard != GUARD_WORD) {
            report("ERROR: Corrupted end of memory header: 0x%p\n",
                   pcslMemoryPtr);
            return -1;
        }

        /* The memory block header is valid, now check the guard data */
        if (verify_tail_guard_data(pcslMemoryHdr)) {
            report("ERROR: Memory overrun: 0x%p\n",
                   pcslMemoryPtr);
            print_alloc("allocated", 
                        pcslMemoryHdr->filename, 
                        pcslMemoryHdr->lineno);
        }
#endif 

        if (pcslMemoryHdr->free != 1) {

#ifdef PCSL_DEBUG
            report("WARNING: memory leak: size= %d  address= 0x%p\n",
                   pcslMemoryHdr->size,
                   (void*)((char*)pcslMemoryHdr + sizeof(_PcslMemHdr)));
            print_alloc("allocated", 
                        pcslMemoryHdr->filename, pcslMemoryHdr->lineno);
#endif
            pcsl_mem_free((void*)((char*)pcslMemoryHdr + sizeof(_PcslMemHdr)));
            *count += 1;
            *size  += pcslMemoryHdr->size;
        }
    }
    return *count;
}


/**
 * FUNCTION:      pcsl_mem_initialize_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the PCSL memory pool
 *                 NOTE: This must only be called once
 * INTERFACE:
 * parameters:  
 *                 startAddr Starting address of memory pool. If NULL, it will
 *                           be either dynamically or statically allocated.
 *                 size   Size of memory pool to use; if size is <= 0,
 *                        the default memory pool size will be used
 *   returns:     0 on succes; != 0 on failure
 *                
 */
int
pcsl_mem_initialize_impl0(void *startAddr, int size) {
    _PcslMemHdrPtr pcslMemoryHdr;

    if (PcslMemoryStart != NULL) {
        /* avoid a double init */
        return 0;
    }

    if (size <= 0) {
        /* size not specified, use the default */
        size = DEFAULT_POOL_SIZE;
    }

    if (startAddr != NULL) {
        PcslMemory = (char *)startAddr;
    } else {
        
#ifndef PCSL_MEMORY_USE_STATIC

        /* allocate the chunk of memory to C heap */
        PcslMemory = (char*)pcsl_heap_allocate_port(size,&size);
        if (PcslMemory == NULL) {
            return -1;
        }

#endif /* ! PCSL_MEMORY_USE_STATIC */
    }

    PcslMemoryStart = PcslMemory;
    PcslMemoryEnd   = PcslMemory + size - sizeof(_PcslMemHdr);

    /* Word alignment */
    while (((long)PcslMemoryStart & ALIGNMENT) != 0) {
        PcslMemoryStart++;
    }

    pcslMemoryHdr = (_PcslMemHdrPtr)PcslMemoryStart;
    pcslMemoryHdr->magic = MAGIC;
    pcslMemoryHdr->free  = 1;
    pcslMemoryHdr->size  = (PcslMemory - PcslMemoryStart)
                           + size - sizeof(_PcslMemHdr);
#ifdef PCSL_DEBUG
    pcslMemoryHdr->guard = GUARD_WORD;
    pcslMemoryHdr->guardSize = 0;
#endif
    return 0;
}


/**
 * FUNCTION:      pcsl_mem_finalize_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the PCSL memory pool
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     <nothing>
 *                
 */
void
pcsl_mem_finalize_impl0() {
    int count, size, ret;

    ret = pcsl_end_memory(&count, &size);

#ifdef PCSL_DEBUG
    if (ret > 0) {
      report("WARNING: %d memory leak(s); %d bytes!\n",
             count, size);
    }
    report("** Total memory: %d\n** Highwater mark:%d",
           pcsl_mem_get_total_heap_impl0(), PcslMemoryHighWaterMark);
#endif 

#ifndef PCSL_MEMORY_USE_STATIC       
    pcsl_heap_deallocate_port(PcslMemory);
    PcslMemory = NULL;
#endif

    PcslMemoryStart = NULL;
    PcslMemoryEnd = NULL;
}

/**
 * FUNCTION:      pcsl_mem_malloc_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private PCSL memory pool
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated memory
 *                
 */
#ifdef PCSL_DEBUG
void*
pcsl_mem_malloc_impl0(unsigned int size, const char* filename, int lineno) {
#else
void*
pcsl_mem_malloc_impl0(unsigned int size) {
#endif
    unsigned int   numBytesToAllocate = size;
    void*          loc     = NULL;
    _PcslMemHdrPtr tempHdr = NULL;
    char*          temp    = NULL;
    char*          pcslMemoryPtr;
    _PcslMemHdrPtr pcslMemoryHdr;
    _PcslMemHdrPtr fitBlockHdr = NULL;

#ifdef PCSL_DEBUG
    int   guardSize = 0;
    void* guardPos = NULL;
    int   i = 0;
    numBytesToAllocate += GUARD_SIZE;
#endif

    while ( (numBytesToAllocate & ALIGNMENT) != 0 ) {
        numBytesToAllocate++;
    }

    /* find a free slot */
    for (pcslMemoryPtr = PcslMemoryStart;
         pcslMemoryPtr < PcslMemoryEnd;
         pcslMemoryPtr += pcslMemoryHdr->size + sizeof(_PcslMemHdr)) {
        
        pcslMemoryHdr = (_PcslMemHdrPtr)pcslMemoryPtr;
        if (pcslMemoryHdr->magic != MAGIC) {
            REPORT1("ERROR: Memory corruption at 0x%p\n", pcslMemoryPtr); 
            return((void *) 0);
        } else {
            while ( 1 ) {
                /* coalescing */
                if (pcslMemoryHdr->free == 1) {
                    /* if current block is free */
                    temp = (char*)pcslMemoryHdr;
                    temp += pcslMemoryHdr->size + sizeof(_PcslMemHdr);
                    tempHdr = (_PcslMemHdrPtr)temp;

                    if ((temp < PcslMemoryEnd) && 
                        (tempHdr->free == 1) && (tempHdr->magic == MAGIC)) {
                        /* and the next block is free too */
                        /* then coalesce */
                        pcslMemoryHdr->size += tempHdr->size
                            + sizeof(_PcslMemHdr);
#ifdef PCSL_DEBUG
                        pcslMemoryHdr->guardSize = 0;
#if PCSL_TRACE_MEMORY
                        REPORT2("DEBUG: Coalescing blocks 0x%p and 0x%p\n",
                                pcslMemoryHdr, tempHdr);
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
            if ((pcslMemoryHdr->free == 1) && 
                (pcslMemoryHdr->size >= numBytesToAllocate)) {
#if PCSL_BEST_FIT_ALLOCATION
	      /* Keep looking for the best fit */
	      if (fitBlockHdr == NULL || 
		  fitBlockHdr->size > pcslMemoryHdr->size) {
		fitBlockHdr = pcslMemoryHdr;
		/* Found exact match */
		if (pcslMemoryHdr->size == numBytesToAllocate) {
		  break;
		}
	      }
#else
	      fitBlockHdr = pcslMemoryHdr;
	      break;
#endif
            } /* end of allocating */
        } /* end of else */
    } /* end of for */

    if (fitBlockHdr != NULL) {
      pcslMemoryHdr = fitBlockHdr;
      if (pcslMemoryHdr->size > (numBytesToAllocate 
				 + sizeof(_PcslMemHdr) + 4)) {
	/* split block */
	_PcslMemHdrPtr nextHdr;
	nextHdr = (_PcslMemHdrPtr)((char *)pcslMemoryHdr
				   + numBytesToAllocate
				   + sizeof(_PcslMemHdr));
	nextHdr->magic = MAGIC;
	nextHdr->free = 1;
	nextHdr->size = pcslMemoryHdr->size 
	  - numBytesToAllocate 
	  - sizeof(_PcslMemHdr);
#ifdef PCSL_DEBUG
	nextHdr->guard    = GUARD_WORD;
	nextHdr->guardSize = 0;
#endif
	pcslMemoryHdr->size     = numBytesToAllocate;
      }
      pcslMemoryHdr->free     = 0;
      loc = (void*)((char*)pcslMemoryHdr + sizeof(_PcslMemHdr));

#ifdef PCSL_DEBUG
      pcslMemoryHdr->guard    = GUARD_WORD;      /* Add head guard */
      pcslMemoryHdr->filename = filename;
      pcslMemoryHdr->lineno   = lineno;

      /* Add tail guard */
      guardSize = pcslMemoryHdr->size - size;

      pcslMemoryHdr->guardSize = guardSize;
      guardPos = (void*)((char*)loc + pcslMemoryHdr->size 
			 - guardSize);
      for(i=0; i<guardSize; i++) {
	((unsigned char*)guardPos)[i] = GUARD_BYTE;
      }
                
      PcslMemoryAllocated += numBytesToAllocate;
      if (PcslMemoryAllocated > PcslMemoryHighWaterMark) {
	PcslMemoryHighWaterMark = PcslMemoryAllocated;
      }
#if PCSL_TRACE_MEMORY
      report("DEBUG: Requested %d provided %d at 0x%p\n",
	     numBytesToAllocate, pcslMemoryHdr->size, loc);
      print_alloc("allocated", filename, lineno);
#endif /* of PCSL_TRACE_MEMORY */

#endif /* of PCSL_DEBUG */
      return(loc);
    }

    REPORT1("DEBUG: Unable to allocate %d bytes\n", numBytesToAllocate);
    return((void *)0);
}

/**
 * FUNCTION:      pcsl_mem_calloc_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Allocate memory from the private PCSL memory pool,
 *                 memory contents are cleared
 * INTERFACE:
 *   parameters:  nelem      Number of elements to allocate
 *                elsize     Size of one element
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated and cleared memory
 *                
 */
#ifdef PCSL_DEBUG
void*
pcsl_mem_calloc_impl0(unsigned int nelem, unsigned int elsize, 
                     const char* filename, int lineno) {
    void *loc = NULL;

    if ((loc = pcsl_mem_malloc_impl0((nelem) * (elsize), filename, lineno)) != NULL) {
        memset(loc, 0, nelem * elsize);
    }
    return loc;
}

#else

void*
pcsl_mem_calloc_impl0(unsigned int nelem, unsigned int elsize) { 
    void *loc = NULL;

    if ((loc = pcsl_mem_malloc_impl0((nelem) * (elsize))) != NULL) {
        memset(loc, 0, nelem * elsize);
    }
    return loc;
}

#endif

/**
 * FUNCTION:      pcsl_mem_realloc_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Re-allocate memory from the private PCSL memory pool
 * INTERFACE:
 *   parameters:  ptr        Original memory pointer
 *                size       New size
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the re-allocated memory
 *                
 */
#ifdef PCSL_DEBUG
void*
pcsl_mem_realloc_impl0(void* ptr, unsigned int size, const char* filename, int lineno) {
#else
void*
pcsl_mem_realloc_impl0(void* ptr, unsigned int size) {
#endif
    void*          newPtr = NULL;
    _PcslMemHdrPtr memHdr;

    /* If ptr is NULL, realloc() behaves like malloc() for the given size. */
    if (ptr == NULL) {
#ifdef PCSL_DEBUG
        ptr = pcsl_mem_malloc_impl0(size, filename, lineno);
#else
        ptr = pcsl_mem_malloc_impl0(size);
#endif
        return ptr;
    }

    memHdr = (_PcslMemHdrPtr)((char*)ptr - sizeof(_PcslMemHdr));

    if (memHdr->size != size) {
        if (size != 0) {
#ifdef PCSL_DEBUG
            newPtr = pcsl_mem_malloc_impl0(size, filename, lineno);
#else
            newPtr = pcsl_mem_malloc_impl0(size);
#endif
            if (newPtr != NULL) {
                if (memHdr->size < size) {
                    memcpy(newPtr, ptr, memHdr->size);
                } else {
                    memcpy(newPtr, ptr, size);
                }
#ifdef PCSL_DEBUG
                pcsl_mem_free_impl0(ptr, filename, lineno);
#else
                pcsl_mem_free_impl0(ptr);
#endif
            }
        } else {
            /* When size == 0, realloc() acts just like free() */
#ifdef PCSL_DEBUG
            pcsl_mem_free_impl0(ptr, filename, lineno);
#else
            pcsl_mem_free_impl0(ptr);
#endif
        }
    } else {
        /* sizes are the same, just return the same pointer */
        newPtr = ptr;
    }
        
    return newPtr;
}

/**
 * FUNCTION:      pcsl_mem_strdup_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Duplicate the given string
 * INTERFACE:
 *   parameters:  s1         String to duplicate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the duplicate string
 *                
 */

#ifdef PCSL_DEBUG

char*
pcsl_mem_strdup_impl0(const char *s1, const char* filename, int lineno) {

    char *p = (char *)pcsl_mem_malloc_impl0(strlen(s1) + 1, filename, lineno);

    if ( p != NULL ) {
        strcpy(p, s1);
    }
    return(p);
}

#else

char*
pcsl_mem_strdup_impl0(const char *s1) {

    char *p = (char *)pcsl_mem_malloc_impl0(strlen(s1) + 1);

    if ( p != NULL ) {
        strcpy(p, s1);
    }
    return(p);
}

#endif

/**
 * FUNCTION:      pcsl_mem_free_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Free memory allocated from the private PCSL memory pool
 * INTERFACE:
 *   parameters:  ptr        Pointer to allocated memory
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     <nothing>
 *                
 */

#ifdef PCSL_DEBUG
	
void
pcsl_mem_free_impl0(void *ptr, const char *filename, int lineno) {
    _PcslMemHdrPtr pcslMemoryHdr;

    if (ptr == NULL) {
        report("WARNING: Attempt to free NULL pointer\n");
        print_alloc("freed", filename, lineno);
    } else if (((char*)ptr > PcslMemoryEnd) || 
               ((char*)ptr < PcslMemoryStart)) {
        report("ERROR: Attempt to free memory out of scope: 0x%p\n", ptr);
        print_alloc("freed", filename, lineno);
    } else {
        pcslMemoryHdr = (_PcslMemHdrPtr)((char*)ptr -sizeof(_PcslMemHdr));
        if (pcslMemoryHdr->magic != MAGIC) {
            report("ERROR: Attempt to free corrupted memory: 0x%p\n", ptr);
            print_alloc("freed", filename, lineno);
        } else if (pcslMemoryHdr->free != 0) {
            report("ERROR: Attempt to free memory twice: 0x%p\n", ptr);
            print_alloc("freed", filename, lineno);
        } else {
            PcslMemoryAllocated -= pcslMemoryHdr->size;
            /* The memory block header is valid, now check the guard data */
            if (pcslMemoryHdr->guard != GUARD_WORD) {
                report("ERROR: Possible memory underrun: 0x%p\n", ptr);
                print_alloc("allocated", 
                            pcslMemoryHdr->filename, 
                            pcslMemoryHdr->lineno);
                print_alloc("freed", filename, lineno);
            } else if (verify_tail_guard_data(pcslMemoryHdr)) {
                report("ERROR: Possible memory overrun: 0x%p\n", ptr);
                print_alloc("allocated", 
                            pcslMemoryHdr->filename, 
                            pcslMemoryHdr->lineno);
                print_alloc("freed", filename, lineno);
            }

#if PCSL_TRACE_MEMORY
            report("DEBUG: free %d bytes: 0x%p\n", pcslMemoryHdr->size, ptr);
            print_alloc("allocated", 
                        pcslMemoryHdr->filename, pcslMemoryHdr->lineno);
            print_alloc("freed", filename, lineno);
#endif 

            pcslMemoryHdr->free = 1;
        }
    } /* end of else */
}

#else

void
pcsl_mem_free_impl0(void *ptr) {
    _PcslMemHdrPtr pcslMemoryHdr;

    if (ptr == NULL) {
    } else if (((char*)ptr > PcslMemoryEnd) || 
               ((char*)ptr < PcslMemoryStart)) {
    } else {
        pcslMemoryHdr = (_PcslMemHdrPtr)((char*)ptr -sizeof(_PcslMemHdr));
        if (pcslMemoryHdr->magic != MAGIC) {
        } else if (pcslMemoryHdr->free != 0) {
        } else {
            pcslMemoryHdr->free = 1;
        }
    } /* end of else */
}
#endif

#ifdef PCSL_DEBUG

/**
 * @internal
 *
 * FUNCTION:      verify_tail_guard_data()
 * TYPE:          private operation
 * OVERVIEW:      Verify guard data at the end of the memory is valid
 * INTERFACE:
 *   parameters:  pcslMemoryHdr   Pointer to memory block header
 *   returns:     0 if guard data is valid; otherwise, the byte position
 *                 of the first incorrect guard data byte
 *                
 */
static int
verify_tail_guard_data(_PcslMemHdrPtr pcslMemoryHdr) {
    void* guardPos;
    int   guardSize;
    int   i;

    guardSize = pcslMemoryHdr->guardSize;
    guardPos = (void*)((char*)pcslMemoryHdr
                       + sizeof(_PcslMemHdr)
                       + pcslMemoryHdr->size - guardSize - 1);
    for(i = 1; i <= guardSize; i++) {
        if (((unsigned char*)guardPos)[i] != GUARD_BYTE) {
            return i;
        }
    }
    return 0;
}
#endif

/**
 *
 * FUNCTION:      pcsl_mem_get_total_heap_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Get the total amount of available heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The total amount of available heap
 *                
 */
int
pcsl_mem_get_total_heap_impl0() {
    return (PcslMemoryEnd - PcslMemoryStart);
}


/**
 * FUNCTION:      pcsl_mem_get_free_heap_impl0()
 * TYPE:          public operation
 * OVERVIEW:      Get the current amount of unused heap
 * INTERFACE:
 *   parameters:  <none>
 *   returns:     The current amount of unused heap
 *                
 */
int
pcsl_mem_get_free_heap_impl0() {
    _PcslMemHdrPtr pcslMemoryHdr;
    char*          pcslMemoryPtr;
    int            size = 0;

    for (pcslMemoryPtr = PcslMemoryStart; 
         pcslMemoryPtr < PcslMemoryEnd;
         pcslMemoryPtr += pcslMemoryHdr->size + sizeof(_PcslMemHdr)) {

        pcslMemoryHdr = (_PcslMemHdrPtr)pcslMemoryPtr;

#ifdef PCSL_DEBUG
        if (pcslMemoryHdr->magic != MAGIC) {
            report("ERROR: Corrupted start of memory header: 0x%p\n", 
                   pcslMemoryPtr);
            return -1;
        }
        
        if (pcslMemoryHdr->guard != GUARD_WORD) {
            report("ERROR: Corrupted end of memory header: 0x%p\n",
                   pcslMemoryPtr);
            return -1;
        }

        /* The memory block header is valid, now check the guard data */
        if (verify_tail_guard_data(pcslMemoryHdr)) {
            report("ERROR: Memory overrun: 0x%p\n", pcslMemoryPtr);
            print_alloc("allocated", 
                        pcslMemoryHdr->filename, 
                        pcslMemoryHdr->lineno);
        }
#endif

        if (pcslMemoryHdr->free != 1) {
            size += pcslMemoryHdr->size;
        }
    }
    return (pcsl_mem_get_total_heap_impl0() - size);
}


/* Set countMemoryLeaksOnly = 0 in order to get more verbose information */
int pcsl_mem_malloc_dump_impl0(int countMemoryLeaksOnly)
{
    char *localpcslMallocMemPtr = NULL;
    char *localpcslMallocMemStart = PcslMemoryStart;
    char *localpcslMallocMemEnd = PcslMemoryEnd;
    _PcslMemHdrPtr localpcslMallocMemHdr = NULL;

    int numberOfAllocatedBlocks = 0;

    REPORT3("PcslMemory=0x%p PcslMemoryStart=0x%p PcslMemoryEnd=0x%p\n", 
            PcslMemory, PcslMemoryStart, PcslMemoryEnd);

    for (localpcslMallocMemPtr = localpcslMallocMemStart; 
        localpcslMallocMemPtr < localpcslMallocMemEnd;
        localpcslMallocMemPtr += localpcslMallocMemHdr->size + sizeof(_PcslMemHdr)) {

        localpcslMallocMemHdr = (_PcslMemHdrPtr) localpcslMallocMemPtr;
        if (localpcslMallocMemHdr->magic != MAGIC) {
            REPORT1("ERROR: memory corruption at 0x%p\n", 
                    localpcslMallocMemPtr);
            return -1;
        } else {

            if (countMemoryLeaksOnly == 0) {
                REPORT4("hdr 0x%p free=%d size=%d address=0x%p\n",
                        localpcslMallocMemHdr, 
                        localpcslMallocMemHdr->free, 
                        localpcslMallocMemHdr->size,
                        (void *)(((char *)localpcslMallocMemHdr) + 
                                 sizeof(_PcslMemHdr)));
            }

            if (localpcslMallocMemHdr->free != 1) {
                numberOfAllocatedBlocks += 1;
#ifdef PCSL_DEBUG
                report("WARNING: memory leak: size=%d  address=0x%p\n",
                       localpcslMallocMemHdr->size, 
                       (void*)((char*)localpcslMallocMemHdr + 
                               sizeof(_PcslMemHdr)));
                print_alloc("allocated", 
                            localpcslMallocMemHdr->filename, 
                            localpcslMallocMemHdr->lineno);
#endif
            }
        }
    }
    return numberOfAllocatedBlocks;
}
