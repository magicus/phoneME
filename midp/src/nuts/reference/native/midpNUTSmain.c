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
#if ENABLE_NUTS_FRAMEWORK

#include <stdio.h>
#include <midpAMS.h>
#include <midpNUTS.h>
#include <midpMalloc.h>

#include <midp_logging.h>

/**
 * This file supposed to be an example of how to use native unit test suite
 * To write your own tests include nuts.h and use the APIs there.
 */

/**
 * All the tests supposed to do whatever they do.<BR>
 * Tests may provide some information by using report function
 * Test signature must be int test_name(void)
 * 
 * @return >= 0 on success <BR>
 *         < 0  on failure
 */

int test1(void) {
    REPORT_INFO(LC_CORE, "Test1: Failed.\n");
    return -1;
}

int test2(void) {
    REPORT_INFO(LC_CORE, "Test2: Passed.\n");
    return 0;
}

int test3(void) {
    REPORT_INFO(LC_CORE, "Test3: Failed.\n");
    return -2;
}

int test4(void) {
    REPORT_INFO(LC_CORE, "Test4: Passed.\n");
    return 3;
}

int register_a_bunch_of_tests() {
    /*
     * Please do not use stack or dynamic allocation for test names!
     */
    if (!register_test("test1",test1)) {
        REPORT_WARN1(LC_CORE, "Registration of test %s failed.\n","test1");
    }

    if (!register_test("test2",test2)) {
        REPORT_WARN1(LC_CORE, "Registration of test %s failed.\n","test2");
    }

    if (!register_test("test3",test3)) {
        REPORT_WARN1(LC_CORE, "Registration of test %s failed.\n","test3");
    }

    if (!register_test("test4",test4)) {
        REPORT_WARN1(LC_CORE, "Registration of test %s failed.\n","test4");
    }
    return get_num_tests();
}

extern char* getApplicationDir(char *cmd);

extern int registerFileInstallerTests();

int main(int argc, char* argv[]) {

    int max_number_of_tests = 0;

    if (argc > 1) {
        REPORT_ERROR(LC_CORE, "Too many arguments given\n");
        return -1;
    }

    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_CORE, "Too many arguments given\n");
        return -1;
    }

#ifndef ARM
    {
        char* appDir;

        /* For development platforms MIDP_HOME is dynamic. */
        appDir = getApplicationDir(argv[0]);
        if (appDir == NULL) {
            return -1;
        }

        midpSetAppDir(appDir);
    }
#endif

    /**  just in case somebody will need the MAX_NUMBER_OF_TESTS value */
    max_number_of_tests = get_MAX_NUMBER_OF_TESTS();

    /* initialize tests framework */
    init_nuts();

    /* if (register_a_bunch_of_tests() > 0) {
     *     run_tests();
     * }
     */
/*************************FILE INSTALLER TESTS HERE *************************/
/* we can put here some ifdef or if to choose which sub tests to run */    
    if (registerFileInstallerTests() > 0) {
        run_tests();  /* run the tests */
    }
/****************************************************************************/
    
    /* Finalize test frame work */
    finalize_nuts();

    midpFinalize();

    return 1;
}

#endif /* ENABLE_NUTS_FRAMEWORK */

