/*
 * @(#)POINTERLIST.h	1.7 06/10/10
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

#ifndef __POINTERLISTH__
#define __POINTERLISTH__

/*
 * This file declares these classes:
 *	POINTERLIST
 *	POINTERLIST_ITERATOR
 * and the macros
 *	DERIVED_POINTERLIST_CLASS
 * These classes are not intended to be used directly
 * in "applications" code, but are to be fairly generic
 * foundations from which other, more useful, classed
 * may be derived. (To mix a metaphor.) The macros provide
 * a straightforward way of doing this.
 */

/*
 * "POINTERLIST" and
 * "DERIVED_POINTERLIST_CLASS"
 * We make assumption, safe under the rules of (ANSI) C,
 * I believe, that a (void *) can represent a pointer to
 * anything, and, consequently, that a pointer-to-anything
 * can be assigned to a (void*) and back without loss of
 * information. Thus we can define a class implementing
 * a (void *) list, and with just a little type casting,
 * derive from it a class implementing a pointer-to-anything
 * class.
 *
 * Here we describe the properties of a POINTERLIST, which will also be
 * the properties of ANY DERIVED CLASS.  A POINTERLIST can be added to at
 * end, using POINTERLIST::add. This function make no check for
 * uniqueness, thus an item (pointer) may appear on a list multiple times
 * unless some checking is done at a higher level.
 *
 * IMPLEMENTATION NOTES: A POINTERLIST is a descriptor plus a malloc'd
 * array. Simple assignment will copy the descriptor, but not the array.
 * Although addition is always done at the end of the list, addition may
 * cause realloc'ation of the array. See the functions copy and destruct
 * below for ways to manage heap memory usage.  CAUTION: As a side effect
 * of this implementation, it is not wise to iterate through a POINTERLIST
 * while manipulating it. See the copy routine below.
 *
 * Usage:
 *	class foo;
 *	foo * a;
 *	int n;
 *	DERIVED_POINTERLIST_CLASS( foolist, (foo *), foolist_iterator ); // <== note ; here !!
 *	foolist fooList;
 *	fooList.add( a );
 *	assert( fooList.n() == 1 );
 */

class POINTERLIST {
	friend class POINTERLIST_ITERATOR;
	int	pointer_n;
	int	pointer_maxn;
	void **	pointer_array;

	void expand( void );

public:
	/*
	 * the null constructor.
	 */
	POINTERLIST(){
		pointer_n = 0; pointer_maxn = 0; pointer_array=0;
	}

	/*
	 * discover the number of states on a list.
	 * Usage:
	 *      POINTERLIST pl;
	 *      int n;
	 *      n = pl.n();
	 */
	int n() const	{ return pointer_n; }

	/*
	 * Add an element to a list, without regard to uniqueness.
	 * Usage:
	 *      POINTERLIST * slp;
	 *      void     * sp;
	 *
	 *      slp->add( sp );
	 */
	void add( void * e )
	{
		if (pointer_n >= pointer_maxn)
			expand();
		pointer_array[ pointer_n++ ] = e;
	}

	/*
	 * Subtract the first occurance of an element from the list
	 * This is likely to be quite slow.
	 */
	void subtract( void * e );

	/*
	 * Return the first element of the list
	 */
	void *first( void ) const;

	/*
	 * Return the last element of the list
	 */
	void *last( void ) const;

	/*
	 * Remove the last element of the list
	 */
	void remove_last( void );

	/*
	 * deallocate the malloc'd part of a list,
	 * set the descriptor to empty. Does not deallocate
	 * the descriptor. You may wish to do so yourself.
	 */
	void destruct( void );

	/*
	 * Does not deallocate malloc'd portion of list, but does
	 * reset the used pointer to zero.
	 */
	void reset( void )	{ pointer_n = 0; }

	/*
	 * copy a POINTERLIST.
	 * This is different from POINTERLIST assignment, which is
	 * rather shallow, in that assignment doesn't make a copy of the list part,
	 * only of the descriptor of it. This is a deeper copy. It does NOT
	 * however copy the things pointed to.
	 * This must be used whenever we want to iterate through a list
	 * while operating on it, as maintaining sorted order results
	 * in shifting elements in the list around. This is an artifact
	 * of the implementation.
	 * Too bad we cannot just overload operator=, but we sometimes
	 * want to use the shallow semantics too, don't we?
	 *
	 * Usage:
	 *	POINTERLIST a;
	 *	POINTERLIST b;
	 *	POINTERLIST_ITERATOR sli;
	 *	void * sp;
	 *	void * sp2;
	 *
	 *	b = a.copy();
	 *	sli = b;
	 *	while( (sp = sli.next() != 0 ){
	 *		...
	 *		a.add( sp2 );
	 *	}
	 *	b.destruct();
	 */
	POINTERLIST copy(void) const;

}; // end class POINTERLIST

/*
 * "POINTERLIST_ITERATOR"
 *
 * In order to help hide the implementation of a POINTERLIST,
 * POINTERLIST_ITERATOR is provided.
 * Usage:
 *	class foo;
 *	DERIVED_POINTERLIST_CLASS( foolist, (foo *), foolist_iterator );
 *
 *      foo *			fp;
 *      foolist			fl;
 *      foolist_iterator	fl_i;
 *
 *      fl_i = fl;
 *      while( (fp = fl_i.next() ) != 0 )
 *	      ...
 * CAUTION: As a side effect of the implementation, it is not wise
 * to iterate through a statelist while manipulating it. Although
 * addition is done strictly at the end of a list, the list data
 * may be realloc'd by this action, which would cause the iterator
 * internal data to address free'd storage. Not good.
 */
class POINTERLIST_ITERATOR {
	int	list_nleft;
	void **	list_next;

public:
	/*
	 * null constructor.
	 */
	POINTERLIST_ITERATOR( ){
		list_nleft = 0;
		list_next = 0;
	}

	/*
	 * constructor with initialization. Equivalent to
	 * null construction, then assignment.
	 * Usage:
	 *      POINTERLIST               pl;
	 *      POINTERLIST_ITERATOR      pl_i = pl;
	 * (OR) POINTERLIST_ITERATOR      pl_i( pl );
	 */
	POINTERLIST_ITERATOR( const POINTERLIST &p ){
		list_nleft = p.pointer_n;
		list_next  = p.pointer_array;
	}

	/*
	 * Explicitly initialize this iterator with a pointer
	 * list:
	 */
	void initialize( const POINTERLIST &p ){
		list_nleft = p.pointer_n;
		list_next  = p.pointer_array;
	}

	/*
	 * Fetch an item from the list.
	 * Returns a pointer from the list,
	 * or zero if all items have already been inspected.
	 * Implementation note: because of the implementation
	 * of a list, and of an iterator, the
	 * "next" ordering is insertion ordering.
	 * It may not be safe to count on this fact.
	 * Usage:
	 *      see above example of POINTERLIST_ITERATOR class use.
	 * Returns:
	 *      0 -- all items have been examined
	 *      else -- a pointer from the list
	 */
	void * next(){ return (list_nleft--<=0) ? 0: *(list_next++); }

	void replace_current(void * x);

}; // end POINTERLIST_ITERATOR


#define DERIVED_POINTERLIST_CLASS( listname, ptrtype, iterator_name ) \
    class listname { \
	    friend class iterator_name; \
	    POINTERLIST pl; \
    public: \
	    listname(void):pl(){} \
	    int n(void) const { return pl.n(); } \
	    void add( const ptrtype p ){ pl.add( (void *) p ); } \
	    void subtract( const ptrtype p ){ pl.subtract( (void *) p ); } \
	    ptrtype first(void) const { return (ptrtype) pl.first(); } \
	    ptrtype last(void) const { return (ptrtype) pl.last(); } \
	    void destruct(void){ pl.destruct();} \
	    void reset(void){ pl.reset();} \
	    listname copy(void) const { listname r; r.pl = pl.copy(); return r;} \
    }; \
    class iterator_name { \
	    POINTERLIST_ITERATOR pli; \
    public: \
	    iterator_name(): pli() {} \
	    iterator_name( const listname & p ): pli( p.pl ){} \
	    void initialize( const listname & p ) { pli.initialize(p.pl); } \
	    ptrtype next(void){ return (ptrtype) (pli.next()); } \
	    void replace_current(ptrtype x){ pli.replace_current((void *) x);} \
    }


#define DERIVED_POINTERSTACK_CLASS( stackname, ptrtype, iterator_name )\
    class stackname { \
	    friend class iterator_name; \
	    POINTERLIST pl; \
    public: \
	    stackname(void):pl(){} \
	    int n(void) const { return pl.n(); } \
	    void push( ptrtype p ){ pl.add( (void *)p); } \
	    ptrtype bottom(void) const { return (ptrtype) pl.first(); } \
	    ptrtype top(void) const { return (ptrtype) pl.last(); } \
	    void pop(void){ pl.remove_last(); } \
	    void subtract( ptrtype p ){ pl.subtract( (void *) p ); } \
	    void destruct(void){ pl.destruct();} \
	    void reset(void){ pl.reset();} \
	    stackname copy(void) const { stackname r; r.pl = pl.copy(); return r;} \
    }; \
    class iterator_name { \
	    POINTERLIST_ITERATOR pli; \
    public: \
	    iterator_name(): pli() {} \
	    iterator_name( const stackname & p ): pli( p.pl ){} \
	    ptrtype next(void){ return (ptrtype) (pli.next()); } \
	    void replace_current(ptrtype x){ pli.replace_current((void *) x);} \
    }

#endif // ! __POINTERLISTH__
