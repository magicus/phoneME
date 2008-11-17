/*
 * @(#)pool_alloc.cc	1.6 06/10/10
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

// @(#)pool_alloc.cc	1.10	95/02/28

#include "pool_alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static char guard_data[16] = "0123456789abcde";

int pool_alloc_debug = 0;

char * pool_alloc::more(int P_size, int zeroed)
{
  pool_alloc_pool * p = 0;
  if (P_size <= POOL_ALLOC_POOL_SIZE)
    {
      //p = new pool_alloc_pool;
      p = (pool_alloc_pool *) malloc( sizeof(pool_alloc_pool) );
      assert(p != NULL);
      p -> next = current;
      p -> size = 0;

      strncpy(p -> guard, guard_data, GUARD_SIZE - 1);
      p -> guard[GUARD_SIZE - 1] = 0;
      current = p;
      if (zeroed) {
	  memset(p -> memory, 0, POOL_ALLOC_POOL_SIZE);
	  memset(p -> memory, 0, POOL_ALLOC_POOL_SIZE);
	}
      limit_address = p -> memory + POOL_ALLOC_POOL_SIZE;
    }
  else /* Gigantic single element pool */
    {
      p = (pool_alloc_pool *) malloc( sizeof(pool_alloc_pool)
			+ (P_size - sizeof(char)*POOL_ALLOC_POOL_SIZE) );
      assert(p != NULL);
      p -> next = current;
      p -> size = P_size;

      strncpy(p -> memory + P_size, guard_data, GUARD_SIZE - 1);
      (p -> memory + P_size)[GUARD_SIZE - 1] = 0;
      current = p;

      if (zeroed)
	{
	  memset(p -> memory, 0, P_size);
        }
      limit_address = p -> memory + P_size;
    }

  return p -> memory;
}

void
pool_alloc::destruct()
{
  pool_alloc_pool * n = current;
  assert(verify());
  if (pool_alloc_debug)
    {
      printf("pool_alloc size(0x%x) = %d bytes\n",
	this, this->pool_size());
    }
  while (n != 0)
    {
      pool_alloc_pool * next_n = n -> next;
      free( (char *) n );
      n = next_n;
    }
  //size = (size + 7) & ~7;
  current = 0;
  next_address = 0;
  limit_address = 0;
}

/*
 * Return the total number of bytes allocated in
 * the pool, including overhead.
 */
unsigned int pool_alloc::pool_size()
{
  unsigned int sumsizes = 0;
  pool_alloc_pool * p = current;
  while (p != 0)
    {
      if (p -> size)
	{
	  sumsizes += p -> size;
	}
      else
	{
	  sumsizes += POOL_ALLOC_POOL_SIZE;
	}
      p = p -> next;
    }
  return sumsizes;
}

pool_alloc::~pool_alloc()
{
  destruct();
}

int pool_alloc::verify()
{
  pool_alloc_pool * n = current;
  while (n != 0)
    {
      if (
	  (n -> size == 0 &&
	   strncmp(n -> guard, guard_data, GUARD_SIZE) != 0)
	  ||
	  (n -> size > 0 &&
	   strncmp(n -> memory + n -> size, guard_data, GUARD_SIZE) != 0)
	  )
	return 0;
      n = n -> next;
    }
  return 1;
}

pool_alloc::pool_alloc(unsigned int P_size) :
     size((P_size + 7) & ~7), current(0), next_address(0), limit_address(0)
{
}

