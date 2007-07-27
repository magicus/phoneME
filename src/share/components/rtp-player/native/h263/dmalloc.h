// dmalloc.h
// "Decoder malloc"
//
// The decoders keep several large static tables to speed up calculations.
// On the Mac, these tables (totalling about a megabyte) slow plugin loading
// and make the plugin large because, for plugins, uninitialized static
// data consumes space in the executable. At load time, the extra meg is
// copied into memory and stashed in the virtual memory file. To speed
// things up and keep the plugin smaller, some of the largest of these tables
// are now allocated dynamically, via dmalloc.
//
// dmalloc is NOT a general-purpose replacement for malloc! It is specifically
// for allocating a few large blocks of memory which persist for the life of
// the plugin instance (or application). (Code above the decoder layer uses
// VWMALLOC.)
//
// Since decoders aren't objects and don't have destructors, I found it
// easier to simply remember the allocated pointers and add a dfreeall
// function which frees all of them at once. dfreeall is called from the
// plugin's destructor. The array used to save the pointers has room for
// only a few objects.
//
// Although this is really a workaround for a linker problem on the Mac,
// there is no harm in allocating this memory at runtime under Windows as
// well, so both implementations work the same way.
//
// tkent, 8/17/96
//
// Copyright (c) 1996-1997 Vivo Software, Inc.  All Rights Reserved.

#ifndef _DMALLOC_H
#define _DMALLOC_H

#include "machine.h"

#ifdef __cplusplus
extern "C" {
#endif

void *dmalloc(S32 size);
void dfreeall(void);

#ifdef __cplusplus
}
#endif

#endif /* _DMALLOC_H */
