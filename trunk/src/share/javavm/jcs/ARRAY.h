/*
 * @(#)ARRAY.h	1.6 06/10/10
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

#ifndef ARRAYH
#define ARRAYH

#include <memory.h>


/* Macros defined:
   DERIVED_ARRAY(atype, etype, bogus_type);
   DERIVED_ARRAYwoC(atype, etype);
   See also "ARRAY2.h" for variants of these macros.
 */

/* Usage:
   
   DERIVED_ARRAY(atype, etype, bogus_type);
   This takes an existing type "etype" (e for element)
   and creates two new types, "atype" (a for array) and
   "bogus_type" ("bogus" meaning not useful in any sense, but a
   distinct type name is needed).  The resulting array will
   behave (syntactically) like a C array, only it grows as
   necessary.

   -- atype objects should have an associated constructor.
   -- atype objects should not have special copy rules, because
      BCOPY is used to move them into new storage when the array
      grows.
   -- bogus_type is just an unused type name required to get
      the constructors run.

   typedef ... etype;
   DERIVED_ARRAY(atype, etype, bogus_type);
   atype A;
   etype x;

   A[i] = x; -- any positive value of i will do, and A is
             -- grown as needed to make it fit.  NEW ELEMENTS
             -- HAVE THEIR CONSTRUCTORS RUN IN ORDER.

  atype has the following methods:

  etype&[i]               Array access -- returns a reference.

  atype()
  atype(unsigned int N)   Constructors.
                          The parameter specifies an initial size.

  ~atype()                Destructor.
                          The allocated array is freed.

  expand(unsigned int N)  Manually expand an array to size >= N.

  unsigned int size()     Returns the smallest index not currently
                          in use.  That is,
                             for (i = 0; i < A.size(); i++)
                                 ... A[i] ...
                          will address every element of the array
                          without provoking an expansion.

  etype get(i)            These are provided for use from a
  etype get_expand(i)     debugger, or by people are are nervous
  atype & put(i, element) about the overloaded use of "[]".  "get_expand"
                          and "put" will always grow the array to the indexed
                          size; "get" will not, and instead calls method
                          "range_error" in the event of an out-of-range access.
			  Currently, this calls assert().

  WARNING: DO NOT TAKE THE ADDRESS OF AN ELEMENT OF A DERIVED ARRAY
  WITHOUT TAKING CARE THAT THE ARRAY DOES NOT EXPAND WHILE THE
  ADDRESS IS IN USE.

  NOTE: IF you use this and the compiler complains about "int not having
  a constructor", then what you really wanted was "DERIVED_ARRAYwoC",
  which is described later in this file, and takes two parameters.
  The rationale for having the "default name" run the destructors is that
  it causes mistakes (running non-existent constructors on integers) to
  be caught by the compiler.  If the names were reversed, mistakes (zeroing
  out new objects instead of running their constructors) would go uncaught.
*/

/* DERIVED_ARRAYwoC(atype, etype);

   Usage:
   
   DERIVED_ARRAYwoC(atype, etype);
   See above for details.  This macro is alike in every respect,
   except that (1) new objects are initialized to zero (instead of
   having constructors run) and (2) no bogus_type is needed.

*/

/* Sample code follows:  
****************************************************************
   #include <stdio.h>
   #include "ARRAY.h"
   static int counter = 0;
   
   class foo {
    public:
     char name[11];
     foo()
       {
         sprintf(name, "%d", counter++);
       }
   
     print()
       {
         printf(" %s", name);
       }
   };
   
   DERIVED_ARRAY(a_foo,foo,bogus_type);
   
   main()
   {
     a_foo A;
     int i,j;
     for (i = 0; i < 10; i++)
       {
         for (j = 0; j < 10; j++)
   	{
   	 A[i + j * 10] . print();
   	}
         printf("\n");
       }
   }
****************************************************************
The output is:

 0 10 20 30 40 50 60 70 80 90
 1 11 21 31 41 51 61 71 81 91
 2 12 22 32 42 52 62 72 82 92
 3 13 23 33 43 53 63 73 83 93
 4 14 24 34 44 54 64 74 84 94
 5 15 25 35 45 55 65 75 85 95
 6 16 26 36 46 56 66 76 86 96
 7 17 27 37 47 57 67 77 87 97
 8 18 28 38 48 58 68 78 88 98
 9 19 29 39 49 59 69 79 89 99

This indicates that the constructors are in fact run exactly once
per item (as it should be), and in a reasonable order (though this
is of course not necessarily guaranteed).
*/

#define DERIVED_ARRAY(array_type, element_type, bogus_type) \
  class bogus_type : public element_type \
{ \
    friend class array_type; \
    	void *operator new(unsigned, void*vp) { return vp; }; \
	void *operator new(unsigned s) { return ::operator new(s); } \
	void operator delete(void *) {} \
}; \
  class array_type { \
    element_type * xxelements; \
    unsigned int max_index; \
  public: \
    array_type() : max_index(0)  \
      { \
	xxelements = 0; \
      } \
    array_type(unsigned int N) : max_index(N) \
      { \
	xxelements = new element_type [N]; \
      } \
    ~array_type() \
      { \
      } \
    void destruct() \
      { \
	  reset(); \
      } \
    void reset() \
      { \
	  unsigned int i; \
	  for (i = 0; i < max_index; i++) \
            xxelements[i].destruct(); \
	  if (xxelements != 0) free((char *) xxelements); \
	  xxelements = 0; \
	  max_index = 0; \
      } \
    void expand(unsigned int I) \
      { \
	  unsigned int new_max_index = ((I+1)*3)/2; \
	  unsigned int ii; \
	  element_type * temp = (element_type *) malloc  \
	    (new_max_index * sizeof(element_type)); \
	  memcpy(temp, maxelements, max_index * sizeof(element_type)); \
	  for (ii = max_index; ii < new_max_index; ii++) \
	    { \
		(void) new (temp+ii) bogus_type ; \
	    } \
	  if (xxelements != 0) free((char *) xxelements); \
	  xxelements = temp; \
	  max_index = new_max_index; \
      } \
    unsigned int size() { return max_index; } \
    element_type & operator[](unsigned int I) \
      { \
	if (I >= max_index) \
	  expand(I); \
	return xxelements[I]; \
      } \
    element_type get (unsigned int J) \
      { \
	if (J >= max_index) \
	  range_error(J); \
	return xxelements[J]; \
      } \
    element_type get_expand (unsigned int J) \
      { \
	if (J >= max_index) \
	  expand(J); \
	return xxelements[J]; \
      } \
    array_type & put (unsigned int J, const element_type & x) \
      { \
	if (J >= max_index) \
	  expand(J); \
	xxelements[J] = x; \
	return *this; \
      } \
    void range_error(unsigned int) \
      { \
	assert(0); \
      } \
  }
#endif

#ifndef ARRAYwoCH
#define ARRAYwoCH

#define DERIVED_ARRAYwoC(array_type, element_type) \
  class array_type { \
    element_type * xxelements; \
    unsigned int max_index; \
  public: \
    array_type() : max_index(0)  \
      { \
	xxelements = 0; \
      } \
    array_type(unsigned int N) : max_index(N) \
      { \
	xxelements = new element_type [N]; \
	memset(xxelements, 0, sizeof(element_type) * max_index); \
      } \
    ~array_type() \
      { \
	  delete [] xxelements; \
      } \
    void reset() \
      { \
	  delete [] xxelements; \
	  xxelements = 0; \
	  max_index = 0; \
      } \
    void expand(unsigned int I) \
      { \
	  unsigned int new_max_index = ((I+1)*3)/2; \
	  element_type * temp = new element_type [new_max_index]; \
	  memcpy(temp, xxelements, max_index * sizeof(element_type)); \
	  memset(temp + max_index, 0, sizeof(element_type) * (new_max_index - max_index)); \
	  delete [] xxelements; \
	  xxelements = temp; \
	  max_index = new_max_index; \
      } \
    unsigned int size() { return max_index; } \
    element_type & operator[](unsigned int I) \
      { \
	if (I >= max_index) \
	  expand(I); \
	return xxelements[I]; \
      } \
    element_type get (unsigned int J) \
      { \
	if (J >= max_index) \
	  range_error(J); \
	return xxelements[J]; \
      } \
    element_type get_expand (unsigned int J) \
      { \
	if (J >= max_index) \
	  expand(J); \
	return xxelements[J]; \
      } \
    array_type & put (unsigned int J, const element_type & x) \
      { \
	if (J >= max_index) \
	  expand(J); \
	xxelements[J] = x; \
	return *this; \
      } \
    void range_error(unsigned int) \
      { \
	assert(0); \
      } \
  }

#endif
