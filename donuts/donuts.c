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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "donuts.h"
#include "donutsImpl.h"

extern DonutsPair tests[];
extern int numTests;

/**
 * Checks if the assert is true.  If it is not, the message is output 
 * and the unit test framework stops executing.
 *
 * @param message message to print if the assert is false
 * @param assertion the assertion to test
 * @param filename the name of the file where the method was invoked
 * @param lineNumber the line of the file where the method was invoked
 */
void assertTrueImpl(char *message, int assertion, char *filename, int lineNumber) {
    if (assertion == 0) {
        printf("TEST FAILURE: \"%s\" at %s line %d\n", message, 
	       filename, lineNumber);
	exit(EXIT_FAILURE);
    }
}

void runTest(char *testName) {
    int i;
    for (i = 0; i < numTests; i++) {
        if (testName == NULL || strcmp(testName, tests[i].name) == 0) {
	    printf("run %s\n", tests[i].name);
	    (*(tests[i].test))();
	    if (testName != NULL) {
	        break;
	    }
	}
    }
}

int main(int argc, char **argv) {
    int i;

    printf("start donuts...\n");
    if (argc < 2) {
        runTest(NULL);
    } else {
        for (i=0; i < argc; i++) {
            printf("arg[%d] = %s\n", i, argv[i]);
            if (strcmp(argv[i], "-list") == 0) {
	        int j;
	        for (j = 0; j < numTests; j++) {
	            printf("%s\n", tests[j].name);
	        }
	    } else {
	        runTest(argv[i]);
	    }
        }
    }

    return 0;
}
