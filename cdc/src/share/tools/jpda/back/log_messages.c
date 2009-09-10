/*
 * @(#)log_messages.c	1.19 06/10/26
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


#include <stdarg.h>
#include <errno.h>

#include "util.h"
#include "proc_md.h"
#include "typedefs.h"
#include "log_messages.h"

#ifdef JDWP_LOGGING

#define MAXLEN_INTEGER          20
#define MAXLEN_FILENAME         256
#define MAXLEN_TIMESTAMP        80
#define MAXLEN_LOCATION         (MAXLEN_FILENAME+MAXLEN_INTEGER+16)
#define MAXLEN_MESSAGE          256
#define MAXLEN_EXEC             (MAXLEN_FILENAME*2+MAXLEN_INTEGER+16)

static MUTEX_T my_mutex;

/* Static variables (should be protected with mutex) */
static int logging;
static int log_file;
static char logging_filename[MAXLEN_FILENAME+1+6];
static char location_stamp[MAXLEN_LOCATION+1];
static PID_T processPid;
static int open_count;

/* Ascii id of current native thread. */
static void
get_time_stamp(char *tbuf, size_t ltbuf)
{
    char format[MAXLEN_TIMESTAMP+1];
    unsigned millisecs = 0;
    time_t t = 0;
    
    GETMILLSECS(millisecs);
    if ( time(&t) == (time_t)(-1) )
        t = 0;
    (void)CVMformatTime(format, sizeof(format), t);
    (void)snprintf(tbuf, ltbuf, format, (int)(millisecs));
}

/* Get basename of filename */
static const char *
file_basename(const char *file)
{
    char *p1;
    char *p2;

    if ( file==NULL )
        return "unknown";
    p1 = strrchr(file, '\\');
    p2 = strrchr(file, '/');
    p1 = ((p1 > p2) ? p1 : p2);
    if (p1 != NULL) {
        file = p1 + 1;
    }
    return file;
}

/* Fill in the exact source location of the LOG entry. */
static void
fill_location_stamp(const char *flavor, const char *file, int line)
{
    (void)snprintf(location_stamp, sizeof(location_stamp), 
                    "%s:\"%s\":%d;", 
                    flavor, file_basename(file), line);
    location_stamp[sizeof(location_stamp)-1] = 0;
}

/* Begin a log entry. */
void
log_message_begin(const char *flavor, const char *file, int line)
{
    MUTEX_LOCK(my_mutex); /* Unlocked in log_message_end() */
    if ( logging ) {
        location_stamp[0] = 0;
        fill_location_stamp(flavor, file, line);
    }
}

/* Standard Logging Format Entry */
static void
standard_logging_format(int fd,
        const char *datetime,
        const char *level,
        const char *product,
        const char *module,
        const char *optional,
        const char *messageID,
        const char *message)
{
    const char *format; 
        
    /* "[#|Date&Time&Zone|LogLevel|ProductName|ModuleID|
     *     OptionalKey1=Value1;OptionalKeyN=ValueN|MessageID:MessageText|#]\n"
     */
    
    format="[#|%s|%s|%s|%s|%s|%s:%s|#]\n";
    
    print_message(fd, "", "", format,
            datetime,
            level,
            product,
            module,
            optional,
            messageID,
            message);
}

/* End a log entry */
void
log_message_end(const char *format, ...)
{
    if ( logging ) {
        va_list ap;
        THREAD_T tid;
        char datetime[MAXLEN_TIMESTAMP+1];
        const char *level;
        const char *product;
        const char *module;
        char optional[MAXLEN_INTEGER+6+MAXLEN_INTEGER+6+MAXLEN_LOCATION+1];
        const char *messageID;
        char message[MAXLEN_MESSAGE+1];

        /* Grab the location, start file if needed, and clear the lock */
        if ( log_file == 0 && open_count == 0 && logging_filename[0] != 0 ) {
            open_count++;
            log_file = md_creat(logging_filename);
            if ( log_file < 0 ) {
                logging = 0;
            }
        }
        
        if ( log_file > 0 ) {
            
            /* Get the rest of the needed information */
            tid = GET_THREAD_ID();
            level = "FINEST"; /* FIXUP? */
            product = "CVM"; /* FIXUP? */
            module = "jdwp"; /* FIXUP? */
            messageID = ""; /* FIXUP: Unique message string ID? */
            (void)snprintf(optional, sizeof(optional), 
                        "LOC=%s;PID=%d;THR=t@%d", 
                        location_stamp,
                        (int)processPid,
                        (int)tid);
            
            /* Construct message string. */
            va_start(ap, format);
            (void)vsnprintf(message, sizeof(message), format, ap);
            va_end(ap);

            get_time_stamp(datetime, sizeof(datetime));
            
            /* Send out standard logging format message */
            standard_logging_format(log_file,
                datetime,
                level,
                product,
                module,
                optional,
                messageID,
                message);
        }
        location_stamp[0] = 0;
    }
    MUTEX_UNLOCK(my_mutex); /* Locked in log_message_begin() */
}

#endif

/* Set up the logging with the name of a logging file. */
void
setup_logging(const char *filename, unsigned flags)
{
#ifdef JDWP_LOGGING
    my_mutex = MUTEX_INIT;
    /* Turn off logging */
    logging = 0;
    gdata->log_flags = 0;
   
    /* Just return if not doing logging */
    if ( filename==NULL || flags==0 )
        return;
    
    /* Create potential filename for logging */
    processPid = GETPID();
    (void)snprintf(logging_filename, sizeof(logging_filename), 
                    "%s.%d", filename, (int)processPid);
    
    /* Turn on logging (do this last) */
    logging = 1;
    gdata->log_flags = flags;

#endif
}

/* Finish up logging, flush output to the logfile. */
void
finish_logging(int exit_code)
{
#ifdef JDWP_LOGGING
    MUTEX_LOCK(my_mutex);
    if ( logging ) {
        logging = 0;
        if ( log_file >= 0 ) {
            (void)md_close(log_file);
            log_file = -1;
        }
    }
    MUTEX_UNLOCK(my_mutex);
#endif
}

