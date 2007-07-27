// dmalloc.c
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

#if 0
#include "dmalloc.h"
#include "mni.h"
#ifdef FOR_MAC
#include <stdlib.h>
#endif

#define MAXSAVEDPTRS 20

static char *savedPointers[MAXSAVEDPTRS];
static S16 nextPointer = 0;

void *dmalloc(S32 size)
{
	char *p;
	
	if (nextPointer >= MAXSAVEDPTRS)
		return(NULL);
	// Should go through Netscape here... ;;;
#ifdef FOR_MAC
	p = NewPtr(size);
#else
	p = MNI_MALLOC(size);
#endif
	savedPointers[nextPointer++] = p;
	
	return(p);
}

void dfreeall(void)
{
	S16 i;
	
	for (i=0; i<nextPointer; i++)
	{
		if (savedPointers[i])
		{
#ifdef FOR_MAC
			DisposePtr(savedPointers[i]);
#else
			MNI_FREE(savedPointers[i]);
#endif
		}
		savedPointers[i] = NULL;
	}
	nextPointer = 0;
}

#endif
