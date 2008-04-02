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

/**
 * @file
 */

/**
 * Functions and data structures definition of RMFS
 *
 * @warning This code is not thread safe
 */

#ifndef _RMFS_ALLOC_H_
#define _RMFS_ALLOC_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//#include <logging.h>
//#include <log_channels.h>


//#define TRACE_STORAGE

#define RMFS_FIRST_BLOCK             0x00;	/* The first data block of one RMFS file */
#define RMFS_SUCCEEDING_BLOCK            0x80;	/* The concatenate data block belongs one RMFS file, but it is not the first one */

/* Define RMFS Data Block Type: The second,third and fourth bit in Flag byte */
#define RMFS_JAD_FILE   0x00	/* suffix of JAD filename is: ".jad" */
#define RMFS_JAR_FILE   0x02	/* suffix of JAR filename is: ".jar" */
#define RMFS_MIDLET_SETTING 0x04	/* suffix of MIDlet suites settings filename: ".ss" */
#define RMFS_MIDLET_PROR    0x06	/* suffix of MIDlet application properties filename: ".ap" */
#define RMFS_MIDLET_INSTALL 0x08	/* suffix of MIDlet install information filename: ".ii" */
#define RMFS_MIDLET_DEL_NOTIFY 0x0A	/* suffix of the filename to save the delete notification URLs: "delete_notify.dat" */
#define RMFS_MIDLET_SUITE_LIST 0x0C	/* suffix of the filename of the list of installed MIDlet suites.   "_suites.dat" */
#define RMFS_RMS_RECORD        0x0E	/* suffix of the filename of the RMS Record: ".db" */
#define RMFS_INSTALL_TEMP      0x10	/* suffix of the Midlet Installer file: ".tmp" */
#define RMFS_RMS_RECORD_INDEX  0x12	/* suffix of the filename of the RMS Record Index: ".idx" */
#define RMFS_NORMAL_FILE       0x14	/* common file */


#define MAX_BLOCKSIZE_4_SPECFILE 0x800	/* 2K bytes */
#define MAX_BLOCKSIZE_4_INSTALL_INFO 0x400	/* 1K bytes */
#define MAX_BLOCKSIZE_4_RMS   0x2000	/* 8K bytes */
#define MAX_BLOCKSIZE_4_RMS_INDEX   0x800	/* 1K bytes */
#define MAX_BLOCKSIZE_4_MIDLET_PROR    0x1800	/* 6K bytes */
#define MAX_BLOCKSIZE_4_INSTALL_TEMP   0xF000	/* 60K bytes */
#define   MAX_DATABLOCK_NUMBER   40

typedef unsigned char u8;
typedef signed char s8;
typedef signed short s16;
typedef unsigned short u16;

typedef unsigned char RMFS_BOOLEAN;

#define RMFS_TRUE   0x01
#define RMFS_FALSE  0x00


/**
 * Structure of RMFS Name Table Head  
 */
typedef struct _rmfsNameTableHdrStruct {
  unsigned short nameLen;	/* the length of filename string */
  unsigned short identifier;	/* file identifier */
  int size;			/* the size of the file */
} _RmfsNameTabHdr, *_RmfsNameTabHdrPtr;


/**
 * Structure of RMFS Name Table  
 */
typedef struct _rmfsNameTableStruct {
  unsigned short nameLen;	/* the length of filename string */
  unsigned short identifier;	/* file identifier */
  int size;			/* size of the file */
  char *filename;		/* filename where allocation took place */

} _RmfsNameTab, *_RmfsNameTabPtr;


/* Structure of RMFS JAD/JAR/RMS Data Block Head in Heap */
typedef struct _rmfsDataBlockHdrStruct {
  unsigned char flag;		/* Data Block is free or used, data type */
  unsigned short identifier;	/* JAD/JAR file or RMS RecordStore ID */
  unsigned int offset;		/* The position in Storage */
  unsigned int dataSize;	/* size of data in this block, dataSize <= blockSize */
  unsigned int size;		/* The size of the block */
  unsigned int next;		/* The offset of the next block */
} _RmfsDataBlockHdr, *_RmfsDataBlockHdrPtr;

/**
 * Structure of RMFS Block Head 
 */
typedef struct _rmfsBlockStruct {
  unsigned char flag;		/* 1 == block is free, 0 == block is used */
  unsigned short nameID;	/* The identifier of the file name */
  unsigned int dataSize;	/* size of data in this block, dataSize <= blockSize */
  unsigned int blockSize;	/* size of the block */
  unsigned int next;		/* the offset of the next block */

} _RmfsBlockHdr, *_RmfsBlockHdrPtr;

/**
 * Structure of RMFS Memory Block Head 
 */
typedef struct _rmfsStruct {
  unsigned short magic;		/* magic number */
  unsigned short maxFileID;	/* Record the max file name identifier */
  unsigned int nameTableEndPos;	/* End Address of RMFS Name Table */
  unsigned int dataBufferEndPos;	/* End Address of RMFS JAD/JAR/RMS data buffer */

} _RmfsHdr, *_RmfsHdrPtr;

/**
 * FUNCTION:      ReadDataFromStorage()
 * TYPE:          public operation
 * OVERVIEW:      Read data buffer from RMFS raw memory storage
 *                 NOTE: This function should be replaced by Raw Memory interface provided by vendor
 * INTERFACE:
 *   parameters:  buffer   put the data into this buffer.
 *                addr     start address in Raw memory storage to be read
 *                size     the byte number 
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*int ReadDataFromStorage(int start, int size) */

int ReadDataFromStorage(void *buffer, int addr, unsigned int size);

/**
 * FUNCTION:      WriteDataToStorage()
 * TYPE:          public operation
 * OVERVIEW:      Write data buffer to RMFS raw memory storage
 *                 NOTE: This function should be replaced by Raw Memory interface provided by vendor
 * INTERFACE:
 *   parameters:  buffer   put this data into RMFS storage.
 *                addr     start address in Raw memory storage to be written
 *                size     the byte number 
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*int WriteDataToStorage(int start, int size) */

int WriteDataToStorage(const void *buffer, int addr, unsigned int size);


/**
 * FUNCTION:      rmfsInitialize()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the RMFS raw memory storage
 *                 NOTE: This must only be called once
 * INTERFACE:
 *   parameters:  size   Size of Raw Memory Storage to use
 *                start  start address of the Raw memory storage
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*int rmfsInitialize(int start, int size) */

int rmfsInitialize(int start, int size);


/**
 * FUNCTION:      rmfsFinalizeHead()
 * TYPE:          public operation
 * OVERVIEW:      Finalize the Raw memory head
 *                 NOTE: This must only be called once before KVM exist
 * INTERFACE:
 *   None   
 *
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*int rmfsFinalizeHead( ) */

int rmfsFinalizeHead();


/**
 * FUNCTION:      rmfsNewNameTableByString()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity whose filename is equal to input string in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, unsigned int *identifier>
 *   returns:     < int position>
 *                
 */


int searchNameTabByID(char **filename, unsigned int identifier);


/**
 * FUNCTION:      rmfsNewNameTable()
 * TYPE:          public operation
 * OVERVIEW:      Create one new Name Table entity in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename>
 *   returns:     <unsigned int identifier>
 *                
 */

unsigned short rmfsNewNameTable(const char *filename);

/**
 * FUNCTION:      rmfsNewNameTableByString()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity whose filename is equal to input string in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, unsigned int *identifier>
 *   returns:     < int position>
 *                
 */

int searchNameTabByString(const char *filename, unsigned short *identifier);

/**
 * FUNCTION:      rmfsNewNameTabStartWith()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity whose filename starts with the input string;
 * INTERFACE:
 *   parameters:  <char* filename, unsigned int *identifier>
 *   returns:     < int position>
 *                
 */

int searchNameTabStartWith(const char *filename, unsigned short *identifier);

/**
 * FUNCTION:      delNameTabByID()
 * TYPE:          public operation
 * OVERVIEW:      Delete one entity whose identifier is equal to input integer in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, unsigned int identifier>
 *   returns:     < int position>
 *                
 */


RMFS_BOOLEAN delNameTabByID(unsigned int identifier);

/**
 * FUNCTION:      delNameTabByString()
 * TYPE:          public operation
 * OVERVIEW:      Delete one entity whose identifier is equal to input integer in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, unsigned int identifier>
 *   returns:     < int position>
 *                
 */


RMFS_BOOLEAN delNameTabByString(const char *filename);

/**
 * FUNCTION:      rmfsTruncateNameTable()
 * TYPE:          public operation
 * OVERVIEW:      Eliminate all holes in Name Table Space
 *                Call once before exits
 * INTERFACE:
 *   parameters:  None
 *
 *   returns:     0: Success;   Other values: Failed
 *                
 */
int rmfsTruncateNameTable();

/**
 * FUNCTION:      rmfsInitDataBlockHdrArray()
 * TYPE:          public operation
 * OVERVIEW:      Allocate space from the raw memory block for one file 
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated memory
 *                
 */
void rmfsInitDataBlockHdrArray();


/**
 * FUNCTION:      rmfsFileAlloc()
 * TYPE:          public operation
 * OVERVIEW:      Allocate space from the raw memory block for one file 
 * INTERFACE:
 *   parameters:  size       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                lineno     Line number where allocation occured
 *   returns:     pointer to the newly allocated memory
 *                
 */
int
rmfsFileAlloc(unsigned int numBytesToAllocate, const char *filename,
	      const void *dataBuffer, unsigned char blockType);



/**
 * FUNCTION:      rmfsFreeDataBlock()
 * TYPE:          public operation
 * OVERVIEW:      Free one Data Block in JAD/JAR/RMS space.
 *                 
 * INTERFACE:
 *   parameters:  int        the index of the data block (From 0 to curDataBlockNum )
 *                
 *   returns:     pointer to newly cleared storage data block
 *                
 */
int rmfsFreeDataBlock(int index);


/**
 * FUNCTION:      rmfsDelFile(const char* filename)
 * TYPE:          public operation
 * OVERVIEW:      Delete all blocks assigned for one file
 * INTERFACE:
 *   parameters:  const char* filename   Filename 
 *                
 *                
 */
void rmfsDelFile(const char *filename);

/**
 * FUNCTION:      rmfsFree(int start, int size )
 * TYPE:          public operation
 * OVERVIEW:      Free the whole RMFS storage, just rewrite the MAGIC and Head
 *                
 * INTERFACE:
 *   parameters:  size   Size of Raw Memory Storage to use
 *                start  start address of the Raw memory storage
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*int rmfsFree(int start, int size) */

int rmfsFree(int start, int size);

int rmfsSplitBlock(int index, unsigned int offset);


int rmfsWriteDataToBlock(int index, const void *buffer, int size, int offset);

int rmfsAllocLinkedBlock(int index, const void *buffer, int size);

int rmfsReadBlock(int index, void *buffer, int size, int offset);

int rmfsSearchBlockByPos(unsigned short *index, int address);

#ifdef __cplusplus
}
#endif

#endif				/* _RMFS_ALLOC_H_ */
