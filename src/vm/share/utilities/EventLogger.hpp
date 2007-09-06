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

#if USE_EVENT_LOGGER
#  define EVENT_LOGGER_RETURN ;
#else
#  define EVENT_LOGGER_RETURN {}
#endif

class EventLogger : public AllStatic {
public:

#define EVENT_LOGGER_TYPES_DO(template) \
  template(SCREEN_UPDATE_START )\
  template(SCREEN_UPDATE_END   )\
  template(CLASS_LOAD_START    )\
  template(CLASS_LOAD_END      )\
  template(COMPILER_GC_START   )\
  template(COMPILER_GC_END     )\
  template(COMPILE_START       )\
  template(COMPILE_END         )\
  template(GC_START            )\
  template(GC_END              )\
  template(VERIFY_START        )\
  template(VERIFY_END          )\

#define DECLARE_EVENT_LOGGER_TYPE(x) x,
  enum EventType {
    EVENT_LOGGER_TYPES_DO(DECLARE_EVENT_LOGGER_TYPE)
    _number_of_event_types
  };
#undef DECLARE_EVENT_LOGGER_TYPE

#if ENABLE_EXTENDED_EVENT_LOGGER
  static const char* _event_names[];
  static int add_event_type( const char name[] );
#else
  static const char* const _event_names[];
#endif

  static const char* name( const EventType type ) {
    return _event_names[ type ];
  }

  static void initialize( void )             EVENT_LOGGER_RETURN
  static void log( const EventType /*type*/) EVENT_LOGGER_RETURN
  static void dump( void )                   EVENT_LOGGER_RETURN
  static void dump( Stream* )                EVENT_LOGGER_RETURN
  static void dispose( void )                EVENT_LOGGER_RETURN

  struct Entry {
    enum {
      delta_bits = 24,
#if ENABLE_EXTENDED_EVENT_LOGGER
      event_type_bits = 32 - delta_bits,
      max_event_types = 1 << event_type_bits,
#endif
      delta_mask = (1 << delta_bits) - 1
    };

    unsigned _packed_data;

    // Number of hrticks from the last recorded event
    int delta       ( void ) const { return _packed_data & delta_mask; }
    EventType type  ( void ) const { return EventType(_packed_data >> delta_bits); }
    const char* name( void ) const { return EventLogger::name( type() ); }    
    void set ( const EventType type, const jlong time );
    jlong dump( Stream* s, jlong time ) const;

    static jlong now ( void );
    static void initialize ( void );
    static void terminate  ( void ) {}
    static bool  use_usec  ( void ) { return _use_usec; }

    static jlong _last;
    static jlong _freq;
    static bool  _use_usec;
  };

  struct Block {
    enum { size = 4096 };

    Block* _next;
    Entry  _entries[size];

    int used ( void ) const;

    static Block*  _head;
    static Block** _tail;
    static int     _used;

    static void initialize ( void );
    static void terminate  ( void );

    static void overflow   ( void );
    static void allocate   ( void );
    static void log ( const EventType type );

    jlong dump( Stream* s, jlong time ) const;
  };
};

#undef EVENT_LOGGER_RETURN
