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

# include "incls/_precompiled.incl"
# include "incls/_EventLogger.cpp.incl"

#if ENABLE_PERFORMANCE_COUNTERS && USE_DEBUG_PRINTING

EventLogger::LogEntryBlock* EventLogger::_head;
EventLogger::LogEntryBlock* EventLogger::_tail;
jlong                       EventLogger::_last_hrticks;

void EventLogger::initialize() {
  if (UseEventLogger) {
    _head = _tail = NULL;
    _last_hrticks = Os::elapsed_counter();
  }
}

void EventLogger::log(EventType type) {
  if (!UseEventLogger) {
    return;
  }
  if (_tail == NULL || _tail->_used_count >= BLOCK_SIZE) {
    size_t size = sizeof(LogEntryBlock) + sizeof(LogEntry[BLOCK_SIZE-1]);
    LogEntryBlock *blk = (LogEntryBlock*)OsMemory_allocate(size);
    if (_head == NULL) {
      _head = _tail = blk;
    } else {
      _tail->_next = blk;
      _tail = blk;
    }
    blk->_used_count = 0;
    blk->_next = NULL;
  }

  GUARANTEE(_tail != NULL && _tail->_used_count < BLOCK_SIZE, "sanity");
  LogEntry *entry = &_tail->_entries[_tail->_used_count++];
  jlong now_hrticks = Os::elapsed_counter();
  entry->_hrtick_delta = (int)(now_hrticks - _last_hrticks);
  entry->_type = type;
  _last_hrticks = now_hrticks;
}

#define CASE_OF_EVENT_LOGGER_TYPE(x) \
    case x: name = STR(x); break;

void EventLogger::dump() {
  if (!UseEventLogger) {
    return;
  }
  if (LogEventsToFile) {
    static const JvmPathChar filename[] = {
      'e','v','e','n','t','.','l','o','g',0
    };
    FileStream s(filename, 200);
    dump(&s);
  } else {
    dump(tty);
  }
}
  
void EventLogger::dump(Stream *s) {
  jlong freq = Os::elapsed_frequency();
  bool use_usec = (freq > 100 * 1000);

  if (use_usec) {
    s->print("      msec");
  } else {
    s->print("  msec");
  }
  s->print_cr("   hrtick event");
  s->print_cr("=======================================");

  jlong last_hrticks = 0;
  for (LogEntryBlock *blk = _head; blk; blk = blk->_next) {
    for (int i=0; i<blk->_used_count; i++) {
      LogEntry *entry = &blk->_entries[i];
      const char *name = "??";
      switch (entry->_type) {
        EVENT_LOGGER_TYPES_DO(CASE_OF_EVENT_LOGGER_TYPE);
      default:
        SHOULD_NOT_REACH_HERE();
        break;
      }
      jlong time = last_hrticks + entry->_hrtick_delta;
      jlong usec = time * 1000 * 1000 / freq;
      jlong msec = usec / 1000;
      usec = usec % 1000;
      last_hrticks = time;

      s->print("%6d", (jint)msec);
      if (use_usec) {
        s->print(".");
        if (usec < 100) {
          s->print("0");
        }
        if (usec < 10) {
          s->print("0");
        }
        s->print("%d", usec);
      }
      s->print_cr(" %8d %s", (jint)time, name);
    }
    s->print_cr("=======================================");
  }
}

void EventLogger::dispose() {
  if (!UseEventLogger) {
    return;
  }

  LogEntryBlock *blk, *next;
  for (blk = _head; blk; ) {
    next = blk->_next;
    OsMemory_free((void*)blk);
    blk = next;
  }
}

#endif // ENABLE_PERFORMANCE_COUNTERS
