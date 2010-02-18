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
 * @ingroup system
 *
 * @brief Interface to the native unit tests suite (NUTS)
 * 
 * <p>To write tests using this API, see the file 
 * <tt>nuts_example.c</tt> for an example of how it is done.
 */

#ifndef _MIDP_NUTS_H_
#define _MIDP_NUTS_H_

#if ENABLE_NUTS_FRAMEWORK

/**
 * Defines a pointer to a function that returns an int and takes no
 * parameters.
 *
 * <p>The signature of the tests should be:
 * <pre>@return 0 or greater if passed; a negative number if failed.</pre>
 */
typedef int (*p_int_func_void)(void);


/**
 * Initializes the test framework. Call this function before calling
 * any other functions.
 *
 * @return MAX_NUMBER_OF_TESTS 
 */
int init_nuts();

/**
 * Takes any actions necessary to safely terminate the test
 * framework. Call this function last, to clean up.
 */
void finalize_nuts();

/**
 * Registers a test in the framework. Do <em>not</em> use the stack or
 * dynamic allocation for test names!
 * 
 * @param test_name name of the test, or any other verbal information
 *        from the user.
 * @param pf pointer to the test function.
 *
 * @return 1 if successful; 0 otherwise.
 */
int register_test(char* test_name, p_int_func_void pf);

/**
 * Runs all of the previously registered tests.
 * 
 * @return the number of tests that failed.
 */
int run_tests();

/**
 * Returns the number of tests that will be run.
 * 
 * @return number of registered tests
 */
int get_num_tests();

/**
 * Returns the size of test-registration struct structure (that is,
 * the maximum number of tests that can be registered).
 * 
 * @return MAX_NUMBER_OF_TESTS
 */
int get_MAX_NUMBER_OF_TESTS();

#endif /* ENABLE_NUTS_FRAMEWORK */
#endif /* _MIDP_NUTS_H_ */
