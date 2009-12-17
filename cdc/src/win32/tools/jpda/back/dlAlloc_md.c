/*
 * @(#)dlAlloc_md.c	1.6 06/10/10
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
 *
 */
/*
 * Platform-specific implementation for Doug Lea's malloc. See dlAlloc.c for details.
 */
 
/* 
  Emulation of sbrk for WIN32
  All code within the ifdef WIN32 is untested by me.
*/
#include "dlAlloc_md.h"

#define AlignPage(add) (((add) + (malloc_getpagesize-1)) & ~(malloc_getpagesize-1))

/* resrve 64MB to insure large contiguous space */ 
#define RESERVED_SIZE (1024*1024*64)
#define NEXT_SIZE (2048*1024)
#define TOP_MEMORY ((unsigned long)2*1024*1024*1024)

struct GmListElement;
typedef struct GmListElement GmListElement;

struct GmListElement 
{
        GmListElement* next;
        void* base;
};

static GmListElement* head = 0;
static unsigned int gNextAddress = 0;
static unsigned int gAddressBase = 0;
static unsigned int gAllocatedSize = 0;

static
GmListElement* makeGmListElement (void* bas)
{
        GmListElement* this;
        this = (GmListElement*)(void*)LocalAlloc (0, sizeof (GmListElement));
        ASSERT (this);
        if (this)
        {
                this->base = bas;
                this->next = head;
                head = this;
        }
        return this;
}

void gcleanup ()
{
        BOOL rval;
        ASSERT ( (head == NULL) || (head->base == (void*)gAddressBase));
        if (gAddressBase && (gNextAddress - gAddressBase))
        {
                rval = VirtualFree ((void*)gAddressBase, 
                                                        gNextAddress - gAddressBase, 
                                                        MEM_DECOMMIT);
        ASSERT (rval);
        }
        while (head)
        {
                GmListElement* next = head->next;
                rval = VirtualFree (head->base, 0, MEM_RELEASE);
                ASSERT (rval);
                LocalFree (head);
                head = next;
        }
}
                
static
void* findRegion (void* start_address, unsigned long size)
{
        MEMORY_BASIC_INFORMATION info;
        while ((unsigned long)start_address < TOP_MEMORY)
        {
                VirtualQuery (start_address, &info, sizeof (info));
                if (info.State != MEM_FREE)
                        start_address = (char*)info.BaseAddress + info.RegionSize;
                else if (info.RegionSize >= size)
                        return start_address;
                else
                        start_address = (char*)info.BaseAddress + info.RegionSize; 
        }
        return NULL;
        
}


void* wsbrk (long size)
{
        void* tmp;
        if (size > 0)
        {
                if (gAddressBase == 0)
                {
                        gAllocatedSize = max (RESERVED_SIZE, AlignPage (size));
                        gNextAddress = gAddressBase = 
                                (unsigned int)VirtualAlloc (NULL, gAllocatedSize, 
                                                                                        MEM_RESERVE, PAGE_NOACCESS);
                } else if (AlignPage (gNextAddress + size) > (gAddressBase +
gAllocatedSize))
                {
                        long new_size = max (NEXT_SIZE, AlignPage (size));
                        void* new_address = (void*)(gAddressBase+gAllocatedSize);
                        do 
                        {
                                new_address = findRegion (new_address, new_size);
                                
                                if (new_address == 0)
                                        return (void*)-1;

                                gAddressBase = gNextAddress =
                                        (unsigned int)VirtualAlloc (new_address, new_size,
                                                                                                MEM_RESERVE, PAGE_NOACCESS);
                                // repeat in case of race condition
                                // The region that we found has been snagged 
                                // by another thread
                        }
                        while (gAddressBase == 0);

                        ASSERT (new_address == (void*)gAddressBase);

                        gAllocatedSize = new_size;

                        if (!makeGmListElement ((void*)gAddressBase))
                                return (void*)-1;
                }
                if ((size + gNextAddress) > AlignPage (gNextAddress))
                {
                        void* res;
                        res = VirtualAlloc ((void*)AlignPage (gNextAddress),
                                                                (size + gNextAddress - 
                                                                 AlignPage (gNextAddress)), 
                                                                MEM_COMMIT, PAGE_READWRITE);
                        if (res == 0)
                                return (void*)-1;
                }
                tmp = (void*)gNextAddress;
                gNextAddress = (unsigned int)tmp + size;
                return tmp;
        }
        else if (size < 0)
        {
                unsigned int alignedGoal = AlignPage (gNextAddress + size);
                /* Trim by releasing the virtual memory */
                if (alignedGoal >= gAddressBase)
                {
                        VirtualFree ((void*)alignedGoal, gNextAddress - alignedGoal,  
                                                 MEM_DECOMMIT);
                        gNextAddress = gNextAddress + size;
                        return (void*)gNextAddress;
                }
                else 
                {
                        VirtualFree ((void*)gAddressBase, gNextAddress - gAddressBase,
                                                 MEM_DECOMMIT);
                        gNextAddress = gAddressBase;
                        return (void*)-1;
                }
        }
        else
        {
                return (void*)gNextAddress;
        }
}


