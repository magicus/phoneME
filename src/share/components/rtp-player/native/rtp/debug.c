/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
 
/** 
 * @file debug.c
 *
 * RTP debug module.
 */


#include <stdio.h>

/**
 * @var FILE dout.
 *
 * File pointer for the debug output file.
 */

static FILE *dout; 

/** 
 *  Opens the debug output file.
 *
 * @param path         Full path of the output file.
 */

void rtp_start_debug( char *path) {
    dout = fopen( path, "w");
}


/** 
 *  Writes a message to the debug output file.
 *
 * @param msg          The debug message to be written.
 */

void rtp_write_debug( char *msg) {
    fprintf(dout, msg);
    fflush(dout);
}


/** 
 *  Closes the RTP debug file.
 *
 */

void rtp_stop_debug() {
    fclose(dout);
}
