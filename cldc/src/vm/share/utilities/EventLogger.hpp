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

#if ENABLE_PERFORMANCE_COUNTERS && USE_DEBUG_PRINTING
#define EVENT_LOGGER_RETURN
#define EVENT_LOGGER_RETURN0
#else
#define EVENT_LOGGER_RETURN   {}
#define EVENT_LOGGER_RETURN0  {return 0;}
#endif

class EventLogger : public AllStatic {
public:

#define EVENT_LOGGER_TYPES_DO(template) \
  template(CLASS_LOAD_START) \
  template(CLASS_LOAD_END) \
  template(COMPILER_GC_START) \
  template(COMPILER_GC_END) \
  template(COMPILE_START) \
  template(COMPILE_END) \
  template(GC_START) \
  template(GC_END) \
  template(VERIFY_START) \
  template(VERIFY_END)

#define DECLARE_EVENT_LOGGER_TYPE(x) x,

  enum EventType {
    EVENT_LOGGER_TYPES_DO(DECLARE_EVENT_LOGGER_TYPE)
    _number_of_event_types
  };
  enum {
    GC_TYPE_YOUNG,
    GC_TYPE_FULL,
    GC_TYPE_COMPILER_AREA,
    _last_event_arg_
  };

  static void initialize()                         EVENT_LOGGER_RETURN;
  static void log(EventType /*type*/)              EVENT_LOGGER_RETURN;
  static void log(EventType /*type*/, int /*arg*/) EVENT_LOGGER_RETURN;
  static void dump()                               EVENT_LOGGER_RETURN;
  static void dump(Stream *)                       EVENT_LOGGER_RETURN;
  static void dispose()                            EVENT_LOGGER_RETURN;

  enum {
    BLOCK_SIZE = 1024
  };
  struct LogEntry {
    int       _hrtick_delta;  // Number of hrticks from the last recorded event
    EventType _type;
  };

  struct LogEntryBlock {
    LogEntryBlock * _next;
    int             _used_count;
    LogEntry        _entries[1];
  };

private:
  static jlong          _last_hrticks;
  static LogEntryBlock *_head;
  static LogEntryBlock *_tail;

  static LogEntry *allocate() EVENT_LOGGER_RETURN0;
};
