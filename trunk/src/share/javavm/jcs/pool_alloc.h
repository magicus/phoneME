/*
 * @(#)pool_alloc.h	1.6 06/10/10
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

#ifndef POOLALLOCH
#define POOLALLOCH

/* Allocation pools are intended for use in this style:
 *
 *   <allocate small things, many many times>
 *   <throw every last one of them away>
 *
 *   Destructors don't get run, but since most
 *   destructors appear to exist solely for the
 *   purpose of memory management, this is
 *   probably not a big deal.  But, be aware
 *   of the restriction.
 */

#define POOL_ALLOC_POOL_SIZE 4000
#define GUARD_SIZE 16

class pool_alloc_pool {
  friend class pool_alloc;
  pool_alloc_pool * next;
  unsigned int size;
  char memory[POOL_ALLOC_POOL_SIZE];
  char guard[GUARD_SIZE];
};

class pool_alloc {
 private:
  pool_alloc_pool * current;
  unsigned int size;
  char * next_address;
  char * limit_address;

  char * more(int size, int zeroed=1);

 public:

  int verify();

  void * nonnull(void *&p)
    {
      void * q = p;
      if (q == 0)
	{
	  q = mem();
	  p = q;
	}
      return q;
    }

	/*
	 * Allocate a specified amount of memory,
	 * hold the zero-fill.
	 */
  void * mem_nonzero(unsigned int item_size)
    {
      char * next_next_address;
      item_size = (item_size + 7) & ~7;
      next_next_address = next_address + item_size;
      if (next_next_address > limit_address)
	next_next_address = more(item_size, 0) + item_size;
      next_address = next_next_address;
      return (void *) (next_next_address - item_size);
    }

	/*
	 * Allocate a specified amount of memory.
	 */
  void * mem(unsigned int item_size)
    {
      char * next_next_address;
      item_size = (item_size + 7) & ~7;
      next_next_address = next_address + item_size;
      if (next_next_address > limit_address)
	next_next_address = more(item_size) + item_size;
      next_address = next_next_address;
      return (void *) (next_next_address - item_size);
    }

	/*
	 * Allocate the default amount of memory.
	 * See pool_alloc::pool_alloc(size) below.
	 */
  void * mem()
    {
      unsigned int item_size = size;
      char * next_next_address = next_address + item_size;
      if (next_next_address > limit_address)
	next_next_address = more(item_size) + item_size;
      next_address = next_next_address;
      return (void *) (next_next_address - item_size);
    }

	/*
	 * Return the total number of bytes allocated in
	 * the pool, including overhead.
	 */
  unsigned int pool_size();

	/*
	 * Free all the pool memory and re-initialize.
	 */
  void destruct();

	/*
	 * Constructor and destructor.
	 */
  pool_alloc(unsigned int item_size);
  ~pool_alloc();

};
#endif
