/*
 *   
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
 */

# include "incls/_precompiled.incl"
# include "incls/_EventLogger.cpp.incl"

#if USE_EVENT_LOGGER

const char* 
#if ENABLE_EXTENDED_EVENT_LOGGER
  EventLogger::_event_names[EventLogger::Entry::max_event_types+1] =
#else
  const EventLogger::_event_names[] =
#endif
{
# define NAME_EVENT_LOGGER_TYPE(x) #x,
  EVENT_LOGGER_TYPES_DO(NAME_EVENT_LOGGER_TYPE)
# undef NAME_EVENT_LOGGER_TYPE
};

#if ENABLE_EXTENDED_EVENT_LOGGER
inline bool EventLogger::validate_event_type( const char c ) {
  return ('A' <= c && c <= 'Z') ||
         ('a' <= c && c <= 'b') ||
         ('0' <= c && c <= '9') || c == '_';
}

inline bool EventLogger::validate_event_type( const char name[] ) {
  if( *name ) {
    const char* p = name;
    for( ; validate_event_type( *p ); p++ ) {
      if( (p - name) > JVM_MAX_EVENT_NAME_LENGTH ) {
        return false; 
      }
    }
    if( *p == 0 ) {
      return true; 
    } 
  }
  return false;
}

inline int EventLogger::add_event_type( const char name[] ) {
  enum { invalid_type = -1 };

  if( !UseEventLogger || !validate_event_type( name ) ) {
    return invalid_type;
  }

  const char** p = _event_names;
  const char* event_name;
  for( ; (event_name = *p) != NULL && jvm_strcmp(event_name, name) != 0; p++ );
  if( event_name == NULL ) {
    if( p == _event_names + Entry::max_event_types ) {
      return invalid_type;
    }
    *p = name;
  }
  return p - _event_names;
}
#endif

jlong EventLogger::Entry::_last;
jlong EventLogger::Entry::_freq;
bool  EventLogger::Entry::_use_usec;
#if USE_EVENT_LOG_TIMER_DOWNSAMPLING
jbyte EventLogger::Entry::_shift;
#endif

inline jlong EventLogger::Entry::now( void ) {
#if USE_EVENT_LOG_TIMER_DOWNSAMPLING
  return julong(Os::elapsed_counter()) >> _shift;
#else
  return Os::elapsed_counter();
#endif
}

inline void EventLogger::Entry::initialize( void ) {
  julong freq = Os::elapsed_frequency();
  GUARANTEE( jlong(freq) > 0, "Invalid high-resolution timer frequency");
#if USE_EVENT_LOG_TIMER_DOWNSAMPLING
  {
    enum { max_freq = 1 << (delta_bits-2) };
    jubyte shift = 0;
    for( ; freq > max_freq; freq >>= 1 ) {
      shift++;
    }
    _shift = shift;
  }
#endif
  _freq = freq;
  _use_usec = freq > 100 * 1000;
  GUARANTEE( freq != 0, "Sanity" );

  _last = now();
}

inline void EventLogger::Entry::set ( const unsigned type, const jlong time ) {
  const unsigned delta = unsigned(time - _last);
  GUARANTEE( (delta >> delta_bits) == 0, "delta overflow" );
  _packed_data = (type << delta_bits) | delta;
  _last = time;
}

inline jlong
EventLogger::Entry::dump( Stream* s, jlong time ) const {
  time += delta();
  jlong usec = time * 1000 * 1000 / _freq;
  const jlong msec = usec / 1000;
  s->print( "%6d", jint(msec) );
  if( _use_usec ) {
    usec %= 1000;
    s->print(".");
    if( usec < 100 ) {
      s->print("0");
    }
    if( usec < 10 ) {
      s->print("0");
    }
    s->print("%d", usec);
  }
  s->print_cr(" %8d %s %s", jint(time), kind(), name() );
  return time;
}


EventLogger::Block*  EventLogger::Block::_head;
EventLogger::Block** EventLogger::Block::_tail;
int                  EventLogger::Block::_used;

inline void EventLogger::Block::initialize ( void ) {
  _head = NULL;
  _tail = &_head;
  _used = size;
}

inline void EventLogger::Block::terminate ( void ) {
  for( Block* block = _head; block; ) {
    Block* next = block->_next;
    OsMemory_free( block );
    block = next;
  }
}

inline void EventLogger::Block::allocate ( void ) {
  Block* block = (Block*)OsMemory_allocate( sizeof( Block ) );
  block->_next = NULL;
  *_tail = block;
  _used = 0;
}

inline void EventLogger::Block::overflow( void ) {
  allocate();
}

inline int EventLogger::Block::used ( void ) const {
  return _next ? size : _used;
}

inline void EventLogger::Block::log ( const unsigned type ) {
  if( _used == size ) {
    overflow();
  }
  const jlong time = EventLogger::Entry::now();
  (*_tail)->_entries[_used++].set( type, time );
}

inline jlong
EventLogger::Block::dump( Stream* s, jlong time ) const {
  const int count = used();
  for( int i = 0; i < count; i++) {
    time = _entries[i].dump( s, time );
  }
  return time;
}


void EventLogger::initialize( void ) {
#if ENABLE_EXTENDED_EVENT_LOGGER
  _event_names[_number_of_event_types] = NULL;
#endif
  EventLogger::Entry::initialize();
  EventLogger::Block::initialize();
}

void EventLogger::log(const unsigned type) {
  if( UseEventLogger ) {
    EventLogger::Block::log( type );
  }
}

void EventLogger::dump( void ) {
  if (!UseEventLogger) {
    return;
  }
  if( LogEventsToFile ) {
    static const JvmPathChar filename[] = {
      'e','v','e','n','t','.','l','o','g',0
    };
    FileStream s(filename, 200);
    dump(&s);
  } else {
    dump(tty);
  }
}
  
void EventLogger::dump( Stream* s ) {
  s->print_cr("*** Event log, hrfreq = %ld", EventLogger::Entry::_freq );
  s->print( Entry::use_usec() ? "      msec" : "  msec" );
  s->print_cr("   hrtick event");
  s->print_cr("=======================================");

  jlong time = 0;
  for( const Block* block = Block::_head; block; block = block->_next ) {
    time = block->dump( s, time );
  }
  s->print_cr("=======================================");
}

void EventLogger::dispose( void ) {
  EventLogger::Block::terminate();
  EventLogger::Entry::terminate();
}

static jboolean JVM_LogEvent(int type, EventLogger::EventKind kind) {
  enum {
#if ENABLE_EXTENDED_EVENT_LOGGER
    max_event_types = EventLogger::Entry::max_event_types
#else
    max_event_types = EventLogger::_number_of_event_types
#endif
  };
  if( unsigned(type) >= max_event_types ) {
    return KNI_FALSE;
  }
  EventLogger::log( EventLogger::EventType( type ), kind );
  return KNI_TRUE;
}
#endif // USE_EVENT_LOGGER

extern "C" jboolean JVM_LogEventStart(int type) {
#if USE_EVENT_LOGGER
  return JVM_LogEvent( type, EventLogger::START );
#else
  return KNI_FALSE;
#endif
}

extern "C" jboolean JVM_LogEventEnd(int type) {
#if USE_EVENT_LOGGER
  return JVM_LogEvent( type, EventLogger::END );
#else
  return KNI_FALSE;
#endif
}

extern "C" int JVM_RegisterEventType(const char* name) {
#if USE_EVENT_LOGGER && ENABLE_EXTENDED_EVENT_LOGGER
  return EventLogger::add_event_type( name );
#else
  (void)name;
  return -1;
#endif
}
