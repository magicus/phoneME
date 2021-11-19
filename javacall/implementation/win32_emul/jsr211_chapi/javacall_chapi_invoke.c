/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
 * @brief Content Handler Invocation stubs.
 */


#include "javautil_unicode.h"
#include "javacall_file.h"
#include "javacall_dir.h"
#include "javacall_string.h"
#include "javacall_chapi_invoke.h"

#include "windows.h"

/**
 * Asks NAMS to launch specified MIDlet. <BR>
 * It can be called by JVM when Java content handler should be launched
 * or when Java caller-MIDlet should be launched.
 * @param suite_id suite Id of the requested MIDlet
 * @param class_name class name of the requested MIDlet
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 *   This should be used when there is no free isolate to run content handler (e.g. in SVM mode).
 * @return result of operation.
 */
javacall_result javacall_chapi_ams_launch_midlet(int suite_id,
        javacall_const_utf16_string class_name,
        /* OUT */ javacall_bool* should_exit)
{
    (void)suite_id;
    (void)class_name;
    (void)should_exit;
    return JAVACALL_NOT_IMPLEMENTED;
}



/**
 * Sends invocation request to platform handler. <BR>
 * This is <code>Registry.invoke()</code> substitute for Java->Platform call.
 * @param invoc_id requested invocation Id for further references
 * @param handler_id target platform handler Id
 * @param invocation filled out structure with invocation params
 * @param without_finish_notification if <code>JAVACALL_TRUE</code>, then 
 *       @link javanotify_chapi_platform_finish() will not be called after 
 *       the invocation's been processed.
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
javacall_result javacall_chapi_platform_invoke(int invoc_id,
        const javacall_utf16_string handler_id, 
        javacall_chapi_invocation* invocation, 
        /* OUT */ javacall_bool* without_finish_notification, 
        /* OUT */ javacall_bool* should_exit)
{
    int res;
    (void)invoc_id;
    (void)handler_id;
    *without_finish_notification = 1;
    *should_exit = 0;

    res = (int) ShellExecuteW( NULL, invocation->action, invocation->url, NULL, NULL, SW_SHOWNORMAL);
    if ( res > 32 )
        return JAVACALL_OK;
    return JAVACALL_FAIL;
}

/*
 * Called by Java to notify platform that requested invocation processing
 * is completed by Java handler.
 * This is <code>ContentHandlerServer.finish()</code> substitute 
 * for Platform->Java call.
 * @param invoc_id processed invocation Id
 * @param url if not NULL, then changed invocation URL
 * @param argsLen if greater than 0, then number of changed args
 * @param args changed args if @link argsLen is greater than 0
 * @param dataLen if greater than 0, then length of changed data buffer
 * @param data the data
 * @param status result of the invocation processing. 
 * @param should_exit if <code>JAVACALL_TRUE</code>, then calling MIDlet 
 *   should voluntarily exit to allow pending invocation to be handled.
 * @return result of operation.
 */
/*
  javacall_result javacall_chapi_java_finish(int invoc_id, 
  javacall_const_utf16_string url,
  int argsLen, javacall_const_utf16_string* args,
  int dataLen, void* data, javacall_chapi_invocation_status status,
  javacall_bool* should_exit)
  {
  (void)invoc_id;
  (void)url;
  (void)argsLen;
  (void)args;
  (void)dataLen;
  (void)data;
  (void)status;
  (void)should_exit;
  return JAVACALL_NOT_IMPLEMENTED;
  }
*/

static javacall_utf16 buffer[ 0x100 ];
static int pos;

static void fill_buffer( javacall_handle hhp ){
    static unsigned char utf8buf[ 0x80 ];
    static int utf8pos = sizeof(utf8buf);

    long count8, count16;
    do {
        int freepos = utf8pos;
        javacall_int32 len8, len16;

        memcpy( utf8buf, &utf8buf[utf8pos], sizeof(utf8buf) - utf8pos );
        utf8pos = 0;
        count8 = javacall_file_read( hhp, &utf8buf[ sizeof(utf8buf) - freepos ], freepos );
        if( count8 < freepos ){
            utf8pos = freepos - count8;
            memmove( &utf8buf[ utf8pos ], utf8buf, sizeof(utf8buf) - utf8pos );
        }

        // convert to utf16
        freepos = pos;
        memcpy( buffer, &buffer[pos], sizeof(buffer) - pos * sizeof(buffer[0]) );
        pos = 0;

        len8 = sizeof(utf8buf) - utf8pos;
        len16 = freepos;
        count16 = 0;
        if( JAVACALL_OK == javautil_unicode_get_max_lengths( utf8buf, &len8, &len16 ) ){
            if( JAVACALL_OK == javautil_unicode_utf8_to_utf16( &utf8buf[ utf8pos ], len8, 
                                        &buffer[ sizeof(buffer)/sizeof(buffer[0]) - freepos ], len16, &len16 ) ){
                count16 = len16;
                utf8pos += len8;
            }
        }
        if( count16 < freepos ){
            pos = freepos - count16;
            memmove( &buffer[ pos ], buffer, sizeof(buffer) - pos * sizeof(buffer[0]) );
        }
    } while( pos > 0 && count8 > 0 );
}

static void skip_ws( javacall_handle hhp ){
    fill_buffer( hhp );
    while( pos < sizeof(buffer)/sizeof(buffer[0]) ){
        switch( buffer[ pos ] ){
            case ' ': case '\t':
                pos++;
                continue;
        }
        break;
    }
}

static int skip_to( const javacall_utf16 * delim, javacall_handle hhp ){
    do {
        while( pos < sizeof(buffer)/sizeof(buffer[0]) ){
            const javacall_utf16 * d = delim;
            while( *d ){
                if( *d++ == buffer[pos] ) return( -1 ); // true
            }
            pos++;
        }
        if( hhp != NULL )
            fill_buffer( hhp );
    } while( pos < sizeof(buffer)/sizeof(buffer[0]) );
    return( 0 ); // false
}

static int equals( int s, const javacall_utf16 * a ){
    if( a == NULL )
        return( 0 ); // false
    while( s < pos && *a++ == buffer[ s ] ) s++;
    return( s == pos && *a == L'\0' );
}

javacall_result javacall_chapi_select_handler( javacall_const_utf16_string action, int count, const javacall_chapi_handler_info * list, 
                                               /* OUT */ int * handler_idx ){
    static const javacall_utf16 hpName[] = L"\\handlers_precedence.txt";
    javacall_handle hhp;
    javacall_utf16 fileName[ JAVACALL_MAX_FILE_NAME_LENGTH ];
    int length;

    *handler_idx = 0;
    // open 'handlers_precedence' file
    length = sizeof(fileName)/sizeof(fileName[0]);
    javacall_dir_get_root_path( fileName, &length );
    javautil_wcsncpy( fileName + length, hpName, JAVACALL_MAX_FILE_NAME_LENGTH - length );
    if( JAVACALL_OK == javacall_file_open( fileName, length + sizeof(hpName)/sizeof(hpName[0]), JAVACALL_FILE_O_RDONLY, &hhp) ){
        int s;
        // fileformat :: (<emptyline>|<line>)* [<lastline> <emptyline>*]
        // emptyline :: '\n'
        // line :: <action>':' <handlerslist> '\n'
        // handlerslist :: <handlerID> | <handlerID> ',' <handlerslist>
        // lastline :: ':' <handlerslist> '\n'
        pos = sizeof(buffer)/sizeof(buffer[0]);
        do {
            // process line
            skip_ws( hhp );
            if( pos >= sizeof(buffer)/sizeof(buffer[0]) )
                break;
            if( buffer[pos] == L'\n' ) 
                pos++; // go to new line parsing
            else {
                s = pos;
                if( !skip_to( L":", NULL ) ) break;
                // compare actions
                if( s == pos || equals( s, action ) )
                    break;
                if( skip_to( L"\n", hhp ) ) pos++;
            }
        } while( -1 );

        // action is found
        while( pos < sizeof(buffer)/sizeof(buffer[0]) ) {
            pos++; // skip delimiter
            // extract <handlerID>
            skip_ws( hhp );
            if( pos >= sizeof(buffer)/sizeof(buffer[0]) || buffer[ pos ] == L'\n' )
                break;
            s = pos;
            skip_to( L",\n", NULL );
            // handlerID : buffer[ s : pos-1 ]
            if( pos > s ){
                int i;
                for( i = 0; i < count && !equals( s, list[ i ].handler_id ); i++);
                if( i < count ){
                    *handler_idx = i;
                    break;
                }
            }
        }
        javacall_file_close( hhp );
    }
    return( JAVACALL_OK );
}
