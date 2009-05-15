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

/** @file
 *
 * This file implements all the necessary PCSL interfaces for the print service.
 */
#include <stdio.h>
#include <string.h>
#include <pcsl_file.h>

/** Name of the file where pcsl_print() will direct output. */

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(fileName)
  {'/','t','m','p','/','o','u','t','p','u','t','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(fileName);

/**
 * File descriptor that points to the output file used by pcsl_print().
 * Its name is specified by OUTPUT_FILE_NAME.
 */
int outputFd;

/**
 * Print contents of buffer to stdout.
 *
 */
void pcsl_print_chars(const char* s, int length) {
  const int stat = pcsl_file_open(&fileName, 
                      PCSL_FILE_O_RDWR | PCSL_FILE_O_CREAT | PCSL_FILE_O_APPEND,
                      (void **)(&outputFd));

  if (stat == 0) {
    pcsl_file_write((void *)outputFd, (unsigned char*)s, length);
    pcsl_file_commitwrite((void *)outputFd);

    pcsl_file_close((void *)outputFd);
  }

}

void pcsl_print(const char* s) {
  pcsl_print_chars(s, strlen(s));
}
