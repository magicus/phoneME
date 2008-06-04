/*
 * jvmti hprof
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

/* Simple stack storage mechanism (or simple List). */

/* 
 * Stack is any depth (grows as it needs to), elements are arbitrary
 *   length but known at stack init time.
 *
 * Stack elements can be accessed via pointers (be careful, if stack
 *   moved while you point into stack you have problems)
 *
 * Pointers to stack elements passed in are copied.
 *
 * Since the stack can be inspected, it can be used for more than just
 *    a simple stack.
 *
 */

#include "jvmti_hprof.h"

static void
resize(Stack *stack)
{
    void  *old_elements;
    void  *new_elements;
    int    old_size;
    int    new_size;

    HPROF_ASSERT(stack!=NULL);
    HPROF_ASSERT(stack->elements!=NULL);
    HPROF_ASSERT(stack->size>0);
    HPROF_ASSERT(stack->elem_size>0);
    HPROF_ASSERT(stack->incr_size>0);
    old_size     = stack->size;
    old_elements = stack->elements;
    if ( (stack->resizes % 10) && stack->incr_size < (old_size >> 2) ) {
	stack->incr_size = old_size >> 2; /* 1/4 the old_size */
    }
    new_size = old_size + stack->incr_size;
    new_elements = HPROF_MALLOC(new_size*stack->elem_size);
    (void)memcpy(new_elements, old_elements, old_size*stack->elem_size);
    stack->size     = new_size;
    stack->elements = new_elements;
    HPROF_FREE(old_elements);
    stack->resizes++;
}

Stack *
stack_init(int init_size, int incr_size, int elem_size)
{
    Stack *stack;
    void  *elements;

    HPROF_ASSERT(init_size>0);
    HPROF_ASSERT(elem_size>0);
    HPROF_ASSERT(incr_size>0);
    stack            = (Stack*)HPROF_MALLOC((int)sizeof(Stack));
    elements         = HPROF_MALLOC(init_size*elem_size);
    stack->size      = init_size;
    stack->incr_size = incr_size;
    stack->elem_size = elem_size;
    stack->count       = 0;
    stack->elements  = elements;
    stack->resizes   = 0;
    return stack;
}

void *
stack_element(Stack *stack, int i)
{
    HPROF_ASSERT(stack!=NULL);
    HPROF_ASSERT(stack->elements!=NULL);
    HPROF_ASSERT(stack->count>i);
    HPROF_ASSERT(i>=0);
    return (void*)(((char*)stack->elements) + i * stack->elem_size);
}

void *
stack_top(Stack *stack)
{
    void *element;

    HPROF_ASSERT(stack!=NULL);
    element = NULL;
    if ( stack->count > 0 ) {
	element = stack_element(stack, (stack->count-1));
    }
    return element;
}

int
stack_depth(Stack *stack)
{
    HPROF_ASSERT(stack!=NULL);
    return stack->count;
}

void *
stack_pop(Stack *stack)
{
    void *element;

    element = stack_top(stack);
    if ( element != NULL ) {
	stack->count--;
    }
    return element;
}

void   
stack_push(Stack *stack, void *element)
{
    void *top_element;
    
    HPROF_ASSERT(stack!=NULL);
    if ( stack->count >= stack->size ) {
	resize(stack);
    }
    stack->count++;
    top_element = stack_top(stack);
    (void)memcpy(top_element, element, stack->elem_size);
}

void
stack_term(Stack *stack)
{
    HPROF_ASSERT(stack!=NULL);
    if ( stack->elements != NULL ) {
	HPROF_FREE(stack->elements);
    }
    HPROF_FREE(stack);
}

