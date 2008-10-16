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
 * Functions and data structures specific to the native unit
 * tests suite (NUTS). To write tests, include this file and use the
 * APIs.  See the file nuts_example.c for an example of how it is
 * done.
 */

#include <midpNUTS.h>
#include <midp_logging.h>

#if ENABLE_NUTS_FRAMEWORK

/**
 * Max number of tests that could be registered.
 */
#define MAX_NUMBER_OF_TESTS 25

/**
 * test_info will contain the information about the test.<BR>
 * test_name: Name of the test supplied by a tester <BR>
 * fp: Pointer to the test function. It has to have int foo(void)
 * signature <BR>
 * res: Result of the test execution.
 */
typedef struct _test_info {
    char *test_name;
    p_int_func_void fp;
    int res;
} test_info;

/**
 *  Array of pointers to test_info struct 
 *
 */
static test_info tf[MAX_NUMBER_OF_TESTS];

/**
 * Set to > MAX_NUMBER_OF_TESTS to indicate uninitialized state.
 * Will be set to 0 by init_nuts() 
 * and back to MAX_NUMBER_OF_TESTS+1 by finalize_nuts()
 * The main idea for doing like that is to prevent test 
 * registration before initialization
 */
static int number_of_registered_tests = MAX_NUMBER_OF_TESTS+1;

/**
 * Executes a specific test.
 * 
 * @param pf     pointer to the test function
 * @return 0 on fail <BR>
 *         1 on success
 */
static int run_test(int(*pf)(void));

/**
 * Initializes a test framework.<BR>
 * Call this before calling any other functions.<BR>
 *
 * @return MAX_NUMBER_OF_TESTS 
 *          
 */
int init_nuts() {
    number_of_registered_tests = 0;
    return  MAX_NUMBER_OF_TESTS;
}

/**
 * Finalizes a test framework.
 * Call this as a last call to clean up.<BR>
 */
void finalize_nuts() {
    number_of_registered_tests = MAX_NUMBER_OF_TESTS+1;
}

/**
 * Register a test in the framework.
 * Please do not use stack or dynamic allocation for test names! 
 * 
 * @param test_name Test name or any other verbal information provided by the user.<BR>
 * @param pf        Pointer to the test function.<BR>
 * @return On success:   1 <BR>
 *         On failure:   0
 */
int register_test(char* test_name, p_int_func_void pf) {

    if ((number_of_registered_tests >= MAX_NUMBER_OF_TESTS) || 
        (number_of_registered_tests < 0)) {
        return 0;
    }

    tf[number_of_registered_tests].fp = pf;
    tf[number_of_registered_tests].test_name = test_name;
    tf[number_of_registered_tests].res = 0;

    number_of_registered_tests++;

    return 1;
}

/**
 * Runs all the previously registered tests.
 * 
 * @return Number of tests that failed.
 */
int run_tests() {
    int tests_fail = number_of_registered_tests;
    int tests_pass = 0;
    int i;

    REPORT_INFO(LC_CORE, "\n######## TESTS EXECUTION START ############\n");
    for (i=0; i< number_of_registered_tests; i++) {
        tf[i].res = run_test(tf[i].fp);
        tests_fail -= tf[i].res;
        if (!tf[i].res) {
            REPORT_WARN1(LC_CORE, "Test: %s failed.\n",tf[i].test_name);
        }
    } /* end of for */

    tests_pass = number_of_registered_tests - tests_fail;
    REPORT_INFO3(LC_CORE, 
		 "Number of tests is %d, tests_pass = %d, tests_fail = %d\n",
		 number_of_registered_tests, tests_pass, tests_fail);

    REPORT_INFO(LC_CORE, "\n######## TESTS EXECUTION END ############\n");

    return tests_fail;

}

/**
 * Executes a specific test. Test should return a number less then 0 if fail.
 * 
 * @param pf     pointer to the test function
 * @return 0 on fail <BR>
 *         1 on success
 */
static int run_test(int(*pf)(void)) {
    if (pf() < 0) {
        return 0;
    }
    return 1;
}

/**
 * Provides number of tests in the test struct which will be run.
 * 
 * @return number of registered tests
 */
int get_num_tests() {
    return  number_of_registered_tests;
}

/**
 * Provides the size of test registration struct.
 * = maximum number of tests that could be registered.
 * 
 * @return MAX_NUMBER_OF_TESTS
 */
int get_MAX_NUMBER_OF_TESTS() {
    return  MAX_NUMBER_OF_TESTS;
}

#endif /* ENABLE_NUTS_FRAMEWORK */

