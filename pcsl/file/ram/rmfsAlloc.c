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

/**
 * @file
 */

/**
 * RMFS storage management and file allocation functions.
 *
 * With these functions, we have the ability to manage RMFS
 * raw memory pool. Please refer to the RMFS design document for details
 *
 * warning This code is not thread safe.
 *
 */

#include <stdio.h>
#include <string.h>

#include <rmfsAlloc.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define   MIN_RAW_MEMORY_ADDR    0x00000000
#define   MAX_RAW_MEMORY_ADDR    0xFFFFFFFF
#define   ALIGNMENT_BYTE         0x04


/*
 * Constant to verify a RMFS storage's validity
 */
#define MAGIC         0xCAFE
//#define TRACE_STORAGE 1

static jint RmfsGlobalNameID = 0x01;


/* Initialize the Open File Table */
extern void rmfsInitOpenFileTab();

static char *prevFilename = NULL;

_RmfsDataBlockHdr rmfsDataBlockHdrArray[MAX_DATABLOCK_NUMBER];
jint curDataBlockNum = 0;


static jint RmfsMemoryStart;	/* Start Address of RMFS memory space */
static jint RmfsNameTableEnd;	/* End Address of RMFS Name Table Space */
static jint RmfsDataBufferEnd;	/* End Address of RMFS DataBufferEnd */
static jint RmfsMemoryEnd;	/* End Address of RMFS memory space */

static jint totalStorageSpace = -1;

unsigned char tempBuf[300];      /* This buffer is used to avoid jvmMalloc and jvmFree */

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
/*jint ReadDataFromStorage(jint start, jint size) */

jint ReadDataFromStorage(void *buffer, jint addr, jint size)
{
  if (((jint) buffer == 0) || (addr == 0)) {
#ifdef TRACE_STORAGE
    printf("Exception Happens: buffer: %d; addr: %d; size: %d \n",
	   (jint) buffer, addr, size);
#endif
    return -1;
  }
  memcpy(buffer, (void *) addr, size);
  return 1;
}

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
 *//*
   jint WriteDataToStorage(jint start, jint size) 
 */
jint WriteDataToStorage(const void *buffer, jint addr, jint size)
{
  if (((jint) buffer == 0) || (addr == 0)) {
#ifdef TRACE_STORAGE
    printf("Exception Happens: buffer: %d; addr: %d size: %d\n",
	   (jint) buffer, addr, size);
#endif

    return -1;
  }

  memcpy((void *) addr, buffer, size);
  return 1;
}


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
/*jint rmfsInitialize(jint start, jint size) */
jint rmfsInitialize(jint start, jint size)
{
  _RmfsHdr rmfsHdr;


#ifdef TRACE_STORAGE
  printf("rmfsInitiallize: start: %d, size: %d\n", start, size);
#endif

  memset(&rmfsHdr, 0, sizeof(_RmfsHdr));
  if ((unsigned int)size < sizeof(_RmfsBlockHdr) + sizeof(_RmfsHdr)) {
#ifdef TRACE_STORAGE
    printf("Can't initialize. Not enough memory.\n");
#endif
    return -1;
  }

  if ((start < MIN_RAW_MEMORY_ADDR)
      || ((unsigned int)(start + size) > MAX_RAW_MEMORY_ADDR)) {
#ifdef TRACE_STORAGE
    printf("Start address of the memory block is invalid \n");
#endif
    return -1;
  }

  /*
     assign the value for the Memory Block Head 
   */

  /* For RAM RMFS no need to read next sentence */
  // ReadDataFromStorage(&rmfsHdr, start, sizeof(_RmfsHdr));

  RmfsMemoryStart = start;
  RmfsMemoryEnd = start + size;

  if (rmfsHdr.magic != MAGIC) {
    rmfsHdr.magic = MAGIC;
    rmfsHdr.dataBufferEndPos = start + sizeof(_RmfsHdr);
    RmfsDataBufferEnd = rmfsHdr.dataBufferEndPos;

    rmfsHdr.nameTableEndPos = start + size;
    RmfsNameTableEnd = rmfsHdr.nameTableEndPos;


    WriteDataToStorage(&rmfsHdr, start, sizeof(_RmfsHdr));
  } else {
    RmfsGlobalNameID = rmfsHdr.maxFileID;
    RmfsDataBufferEnd = rmfsHdr.dataBufferEndPos;
    RmfsNameTableEnd = rmfsHdr.nameTableEndPos;
  }

  /*
     Initialize Open File Table 
   */
  rmfsInitOpenFileTab();

  /*
     Initialize the Data Block Array in RAM 
   */
  rmfsInitDataBlockHdrArray();

  return 0;
}

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
/*jint rmfsFinalizeHead( ) */
jint rmfsFinalizeHead()
{
  _RmfsHdr rmfsHdr;

#ifdef TRACE_STORAGE
  printf("rmfsFinalizeHead\n");
#endif

  rmfsHdr.magic = MAGIC;
  rmfsHdr.maxFileID = RmfsGlobalNameID;

  rmfsHdr.dataBufferEndPos = RmfsDataBufferEnd;

  rmfsHdr.nameTableEndPos = RmfsNameTableEnd;

  WriteDataToStorage(&rmfsHdr, RmfsMemoryStart, sizeof(_RmfsHdr));
  return 0;
}

/**
 * FUNCTION:      getUsedSpace()
 * TYPE:          public operation
 * OVERVIEW:      Get the used space size in RMFS
 * INTERFACE:
 *   None   
 *
 *   returns:     the byte numer was used
 *                
 */
jint getUsedSpace() {
    int i; 
    int usedSpace = 0;

    for (i = 0; i < curDataBlockNum; i++) {
      if ((rmfsDataBlockHdrArray[i].flag & 0x01) == 1) {
	usedSpace += rmfsDataBlockHdrArray[i].size;
      }
    }
    usedSpace += (RmfsMemoryEnd - RmfsNameTableEnd);

    return usedSpace;
}


/**
 * FUNCTION:      getFreeSpace()
 * TYPE:          public operation
 * OVERVIEW:      Get the free space size in RMFS
 * INTERFACE:
 *   None   
 *
 *   returns:     the byte numer was used
 *                
 */
jint getFreeSpace()
{
    int i;
    int freeSpace = 0;

    for (i = 0; i < curDataBlockNum; i++) {
      if ((rmfsDataBlockHdrArray[i].flag & 0x01) == 0) {
	freeSpace += rmfsDataBlockHdrArray[i].size;
      }
    }

    if ((unsigned int)(RmfsNameTableEnd - RmfsDataBufferEnd) > sizeof(_RmfsBlockHdr)) {
      freeSpace +=
	  RmfsNameTableEnd - RmfsDataBufferEnd - sizeof(_RmfsBlockHdr);
    }

    //return RmfsNameTableEnd - RmfsDataBufferEnd;
    return freeSpace;


}

/**
 * FUNCTION:      searchNameTabByID()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity whose filename is equal to input string in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, jint *identifier>
 *   returns:     < jint position>
 *                
 */
jint searchNameTabByID(char **filename, jint identifier)
{
  char *nameTabBuffer;
  char *tempNameStr;
  _RmfsNameTab nameTab;
  _RmfsNameTabHdrPtr nameTabHdr;

  jint tabLength = 0;
  jint curLength = 0;
  RMFS_BOOLEAN done = RMFS_FALSE;

#ifdef TRACE_STORAGE
  printf("searchNameTabByID: identifier %d\n", identifier);
#endif
  tabLength = RmfsMemoryEnd - RmfsNameTableEnd;

  if ((unsigned int)tabLength < sizeof(_RmfsNameTabHdr)) {
#ifdef TRACE_STORAGE
    printf("No entity in Name Table\n");
#endif

    return -1;
  }

  nameTabBuffer = (char *) RmfsNameTableEnd;
  if (nameTabBuffer == NULL) {
#ifdef TRACE_STORAGE
    printf("Name Table is not ready \n");
#endif
    return -1;
  }


  while (!done) {
    nameTabHdr = (_RmfsNameTabHdrPtr) (nameTabBuffer + curLength);

    curLength += sizeof(_RmfsNameTabHdr);

    if ((curLength + nameTabHdr->nameLen) > tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif
      return -1;
    }

    nameTab.filename = (char *) (nameTabBuffer + curLength);

    if (nameTabHdr->identifier == identifier) {
      tempNameStr = (char *) tempBuf;
      if (tempNameStr == NULL) {
#ifdef TRACE_STORAGE
	printf("Memory Allocation Failed \n");
#endif

	return -1;
      }
      strcpy(tempNameStr, nameTab.filename);

      *filename = tempNameStr;
      curLength = curLength - sizeof(_RmfsNameTabHdr);
      done = RMFS_TRUE;

      break;
    }

    curLength += nameTabHdr->nameLen;

    if ((curLength + sizeof(_RmfsNameTabHdr)) >= (unsigned int)tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif

      return -1;
    }
  }


  return RmfsNameTableEnd + curLength;


}


/**
 * FUNCTION:      rmfsNewNameTable()
 * TYPE:          public operation
 * OVERVIEW:      Create one new Name Table entity in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename>
 *   returns:     <jint identifier>
 *                
 */

uchar rmfsNewNameTable(const char *filename)
{
  _RmfsNameTabHdrPtr nameTab;
  char *tempName;
  uchar fileID = 0;
  jint entityPos;


#ifdef TRACE_STORAGE
  printf("rmfsNewNameTable: filename %s\n", filename);
#endif

  RmfsGlobalNameID++;
  if ((entityPos = searchNameTabByString(filename, &fileID)) > 0) {
    if (fileID == 0) {
      fileID = RmfsGlobalNameID;
      WriteDataToStorage(&fileID, entityPos + sizeof(uchar),
			 sizeof(uchar));
      return fileID;
    }

    return fileID;
  }

  /* nameTab = (_RmfsNameTabHdrPtr) jvmMalloc(strlen(filename) + 1 +
					   sizeof(_RmfsNameTabHdr));
					   */
  nameTab = (_RmfsNameTabHdrPtr)tempBuf;

  if (nameTab == NULL) {
#ifdef TRACE_STORAGE
    printf("Memory Allocation Failed \n");
#endif
    return 0;
  }

  memset(nameTab, 0x00, (strlen(filename) + sizeof(_RmfsNameTabHdr) + ALIGNMENT_BYTE));

  nameTab->nameLen = (strlen(filename) / ALIGNMENT_BYTE + 1) * ALIGNMENT_BYTE;

  tempName = (char *) (nameTab + 1);
  strcpy(tempName, filename);

  nameTab->identifier = RmfsGlobalNameID;

  if (((unsigned int)RmfsNameTableEnd - RmfsDataBufferEnd) <=
      (((unsigned int)nameTab->nameLen) + sizeof(_RmfsNameTabHdr))) {
#ifdef TRACE_STORAGE
    printf("Not enough space in Raw Storage \n");
#endif

    // jvmFree(nameTab);
    return 0;
  }

  RmfsNameTableEnd =
      RmfsNameTableEnd - sizeof(_RmfsNameTabHdr) - nameTab->nameLen;

  if (WriteDataToStorage
      (nameTab, RmfsNameTableEnd,
       sizeof(_RmfsNameTabHdr) + nameTab->nameLen) < 0) {
#ifdef TRACE_STORAGE
    printf("Persistent Storage Write failed \n");
#endif

    // jvmFree(nameTab);

    RmfsNameTableEnd =
	RmfsNameTableEnd + sizeof(_RmfsNameTabHdr) + nameTab->nameLen;
    return 0;
  }

  fileID = nameTab->identifier;

  return fileID;
}

/**
 * FUNCTION:      searchNameTabByString()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity whose filename is equal to input string in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, jint *identifier>
 *   returns:     < jint position>
 *                
 */

jint searchNameTabByString(const char *filename, uchar *identifier)
{
  char *nameTabBuffer;
  _RmfsNameTab nameTab;
  _RmfsNameTabHdrPtr nameTabHdr = NULL;
  RMFS_BOOLEAN done = RMFS_FALSE;
  jint tabLength = 0;
  jint curLength = 0;

#ifdef TRACE_STORAGE
  printf("searchNameTabByString: filename %s\n", filename);
#endif

  tabLength = RmfsMemoryEnd - RmfsNameTableEnd;

  if ((unsigned int)tabLength < sizeof(_RmfsNameTabHdr)) {
#ifdef TRACE_STORAGE
    printf("No entity in Name Table\n");
#endif

    return -1;
  }

  nameTabBuffer = (char *) RmfsNameTableEnd;
/*
    nameTabBuffer = (char *) jvmMalloc (RmfsMemoryEnd - RmfsNameTableEnd);

    if (nameTabBuffer == NULL) {
#ifdef TRACE_STORAGE
      printf ("Memory Allocation Failed \n");
#endif
      return -1;
    }
	

    if (ReadDataFromStorage (nameTabBuffer, RmfsNameTableEnd, tabLength) < 0) {
      jvmFree (nameTabBuffer);
      return -1;
    }
*/
  while (!done) {
    nameTabHdr = (_RmfsNameTabHdrPtr) (nameTabBuffer + curLength);

    if((jint)nameTabHdr != (jint)(nameTabBuffer + curLength)) {
#ifdef TRACE_STORAGE
    printf("address alignment:%d \n",(int)nameTabHdr);
#endif
    }
 
    curLength += sizeof(_RmfsNameTabHdr);

    if ((curLength + nameTabHdr->nameLen) > tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif
      return -1;
    }

    nameTab.filename = nameTabBuffer + curLength;

    if ((strcmp(filename, nameTab.filename) == 0)) {
      curLength = curLength - sizeof(_RmfsNameTabHdr);
      done = RMFS_TRUE;
      break;
    }

    curLength += nameTabHdr->nameLen;

    if ((curLength + sizeof(_RmfsNameTabHdr)) > (unsigned int)tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif

      // jvmFree (nameTabBuffer);
      return -1;
    }
  }


  *identifier = nameTabHdr->identifier;

  // jvmFree (nameTabBuffer);
  return RmfsNameTableEnd + curLength;


}


/**
 * FUNCTION:      searchNameTabStartWith()
 * TYPE:          public operation
 * OVERVIEW:      Search one entity NameTable Space, whose filename starts with one string;
 * INTERFACE:
 *   parameters:  <char* filename, jint *identifier>
 *   returns:     < jint position>
 *                
 */

jint searchNameTabStartWith(const char *filename, uchar *identifier)
{
  char *nameTabBuffer;
  _RmfsNameTab nameTab;
  _RmfsNameTabHdrPtr nameTabHdr = NULL;
  RMFS_BOOLEAN done = RMFS_FALSE;
  RMFS_BOOLEAN findNext = RMFS_FALSE;
  jint tabLength = 0;
  jint curLength = 0;
  jint matchLen = 0;

  matchLen = strlen(filename);

#ifdef TRACE_STORAGE
  printf("searchNameTabByStartWith: filename %s\n", filename);
#endif

  tabLength = RmfsMemoryEnd - RmfsNameTableEnd;

  if ((unsigned int)tabLength < sizeof(_RmfsNameTabHdr)) {
#ifdef TRACE_STORAGE
    printf("No entity in Name Table\n");
#endif

    return -1;
  }

  nameTabBuffer = (char *) RmfsNameTableEnd;
  /*
     nameTabBuffer = (char *) jvmMalloc (RmfsMemoryEnd - RmfsNameTableEnd);

     if (nameTabBuffer == NULL) {
#ifdef TRACE_STORAGE
     printf ("Memory Allocation Failed \n");
#endif
     return -1;
     }

     if (ReadDataFromStorage (nameTabBuffer, RmfsNameTableEnd, tabLength) < 0) {
     jvmFree (nameTabBuffer);
     return -1;
     }
   */
  while (!done) {
    nameTabHdr = (_RmfsNameTabHdrPtr) (nameTabBuffer + curLength);

    curLength += sizeof(_RmfsNameTabHdr);

    if ((curLength + nameTabHdr->nameLen) > tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif
      // jvmFree (nameTabBuffer);
      return -1;
    }

    nameTab.filename = nameTabBuffer + curLength;

#ifdef TRACE_STORAGE
    printf("filename: %s; nameTab.filename: %s, matchlen: %d \n", filename,
	   nameTab.filename, matchLen);
#endif
    if (strncmp(filename, nameTab.filename, matchLen) == 0) {
      if ((prevFilename == NULL) || (findNext == RMFS_TRUE)) {
	curLength = curLength - sizeof(_RmfsNameTabHdr);
	done = RMFS_TRUE;
	break;
      } else if (strcmp(prevFilename, nameTab.filename) == 0) {
	findNext = RMFS_TRUE;
      }
    }

    curLength += nameTabHdr->nameLen;

    if ((curLength + sizeof(_RmfsNameTabHdr)) > (unsigned int)tabLength) {
#ifdef TRACE_STORAGE
      printf("No entity match \n");
#endif

      prevFilename = NULL;
      // jvmFree (nameTabBuffer);
      return -1;
    }
  }


  *identifier = nameTabHdr->identifier;

  // jvmFree (nameTabBuffer);

  prevFilename =
      (char *) (RmfsNameTableEnd + curLength + sizeof(_RmfsNameTabHdr));

#ifdef TRACE_STORAGE
  printf("preFilename: %s; RmfsNameTableEnd %d \n", prevFilename,
	 RmfsNameTableEnd);
#endif
  return RmfsNameTableEnd + curLength;


}

/**
 * FUNCTION:      delNameTabByID()
 * TYPE:          public operation
 * OVERVIEW:      Delete one entity whose identifier is equal to input integer in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, jint identifier>
 *   returns:     < jint position>
 *                
 */


RMFS_BOOLEAN delNameTabByID(jint identifier)
{
  char *filename = NULL;
  int lastNameLen = 0;
  jint entityPos = 0;
  uchar nameID;

#ifdef TRACE_STORAGE
  printf("delNameTabById: identifier %d\n", identifier);
#endif
  entityPos = searchNameTabByID(&filename, identifier);

  if ((entityPos < 0) || (filename == NULL)) {
#ifdef TRACE_STORAGE
    printf("No match entity in Name Table\n");
#endif

    return RMFS_FALSE;
  }


  /*
     nameTab = (_RmfsNameTabHdrPtr) jvmMalloc (sizeof (_RmfsNameTabHdr) + fileNameLen);

     if (nameTab == NULL) {
     return RMFS_FALSE;
     }

     nameTab.identifier = 0;
     nameTab.nameLen = fileNameLen;
   */
  nameID = 0;
  WriteDataToStorage(&nameID, entityPos + sizeof(uchar),
		     sizeof(uchar));

  /*
     If the entity is the last Name Table entity, move the RmfsNameTabEnd Pointer 
   */
  if (entityPos == RmfsNameTableEnd) {
    lastNameLen = sizeof(_RmfsNameTabHdr) + strlen(filename) + 1;

    if(lastNameLen != (lastNameLen / ALIGNMENT_BYTE) * ALIGNMENT_BYTE) {
      lastNameLen = (lastNameLen / ALIGNMENT_BYTE + 1) * ALIGNMENT_BYTE;	
    }

    RmfsNameTableEnd += lastNameLen;

  }

  // jvmFree(filename);
  // jvmFree (nameTab);
  return RMFS_TRUE;


}


/**
 * FUNCTION:      delNameTabByString()
 * TYPE:          public operation
 * OVERVIEW:      Delete one entity whose identifier is equal to input integer in NameTable Space;
 * INTERFACE:
 *   parameters:  <char* filename, jint identifier>
 *   returns:     < jint position>
 *                
 */


RMFS_BOOLEAN delNameTabByString(const char *filename)
{
  uchar identifier = 0;
  jint  nameTabLen;
  jint entityPos = 0;
  //  jint fileNameLen;

#ifdef TRACE_STORAGE
  printf("delNameTabByString: filename %s\n", filename);
#endif
  entityPos = searchNameTabByString(filename, &identifier);

  if (entityPos < 0) {
#ifdef TRACE_STORAGE
    printf("No match entity in Name Table\n");
#endif

    return RMFS_FALSE;
  }

/*
    fileNameLen = strlen (filename) + 1;


    nameTab =
      (_RmfsNameTabHdrPtr) jvmMalloc (sizeof (_RmfsNameTabHdr) + fileNameLen);

    if (nameTab == NULL) {
      return RMFS_FALSE;
    }

    nameTab->identifier = 0;
    nameTab->nameLen = fileNameLen;
    memset ((void *) (nameTab + 1), 0x00, nameTab->nameLen);
*/

  identifier = 0;
  WriteDataToStorage(&identifier, entityPos + sizeof(uchar),
		     sizeof(uchar));

  //    jvmFree (nameTab);

  /*
     If the entity is the last Name Table entity, move the RmfsNameTabEnd Pointer 
   */
  if (entityPos == RmfsNameTableEnd) {
    nameTabLen =  sizeof(_RmfsNameTabHdr) + strlen(filename) + 1;
    if(nameTabLen != (nameTabLen/ALIGNMENT_BYTE) * ALIGNMENT_BYTE ) { 
      nameTabLen = (nameTabLen/ALIGNMENT_BYTE + 1) * ALIGNMENT_BYTE;
    }
	     
    RmfsNameTableEnd += nameTabLen;
  }

  return RMFS_TRUE;


}

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
jint rmfsTruncateNameTable()
{
  char *nameTabBuffer;
  _RmfsNameTabHdr nameTabHdr;
  jint tabLength = 0;
  jint curLength = 0;
  jint remainByte = 0;
  char *tempAddress = 0;

#ifdef TRACE_STORAGE
  printf("rmfsTruncateNameTable: \n");
#endif
  tabLength = RmfsMemoryEnd - RmfsNameTableEnd;

  if ((unsigned int)tabLength < sizeof(_RmfsNameTabHdr)) {
#ifdef TRACE_STORAGE
    printf("No entity in Name Table\n");
#endif

    return -1;
  }

  nameTabBuffer = (char *) RmfsNameTableEnd;
  /*
     nameTabBuffer = (char *) jvmMalloc (RmfsMemoryEnd - RmfsNameTableEnd);

     if (nameTabBuffer == NULL) {
#ifdef TRACE_STORAGE
     printf ("Memory Allocation Failed \n");
#endif
     return -1;
     }

     if (ReadDataFromStorage (nameTabBuffer, RmfsNameTableEnd, tabLength) < 0) {
     jvmFree (nameTabBuffer);
     return -1;
     }
   */
  while ((curLength + sizeof(_RmfsNameTabHdr)) <= (unsigned int)tabLength) {
    nameTabHdr.nameLen = (uchar) *(nameTabBuffer + curLength);

    curLength += sizeof(jint);

    nameTabHdr.identifier = (uchar) *(nameTabBuffer + curLength);

    curLength += sizeof(jint);

    if (nameTabHdr.identifier == 0x00) {
      /*
         This Name Table entity is empty, advance all subsequence data to fill this hole
         * In order to avoid employ another large RAM heap, just move the data segment by segment 
       */

      remainByte =
	  tabLength - curLength - (nameTabHdr.nameLen +
				   sizeof(_RmfsNameTabHdr));
      tempAddress = nameTabBuffer + curLength - sizeof(_RmfsNameTabHdr);
      while ((unsigned int)remainByte >= (nameTabHdr.nameLen + sizeof(_RmfsNameTabHdr))) {
	tempAddress =
	    tempAddress + sizeof(_RmfsNameTabHdr) + nameTabHdr.nameLen;

	memcpy((tempAddress - sizeof(_RmfsNameTabHdr) -
		nameTabHdr.nameLen), tempAddress,
	       (nameTabHdr.nameLen + sizeof(_RmfsNameTabHdr)));

	remainByte =
	    remainByte - (nameTabHdr.nameLen + sizeof(_RmfsNameTabHdr));

      }

      if (remainByte > 0) {
	memcpy(tempAddress,
	       (tempAddress + sizeof(_RmfsNameTabHdr) +
		nameTabHdr.nameLen), remainByte);

      }

      tabLength = tabLength - (nameTabHdr.nameLen + sizeof(_RmfsNameTabHdr));

      curLength = curLength - sizeof(_RmfsNameTabHdr);

      continue;
    }

    if ((curLength + nameTabHdr.nameLen) > tabLength) {
      break;
    }

    curLength += nameTabHdr.nameLen;
  }

  WriteDataToStorage(nameTabBuffer, RmfsMemoryEnd - tabLength, tabLength);

  RmfsNameTableEnd = RmfsMemoryEnd - tabLength;

  // jvmFree (nameTabBuffer);


  return 0;

}


/**
 * FUNCTION:      rmfsInitDataBlockHdrArray()
 * TYPE:          public operation
 * OVERVIEW:      Allocate space from the raw memory block for one file 
 * INTERFACE:
 *   parameters:  None
 *
 *   returns:     pointer to the newly allocated memory
 *                
 */
void rmfsInitDataBlockHdrArray()
{
  jint rmfsDataBlockPtr;
  _RmfsBlockHdr dataBlockHdr;
  short i = 0;

#ifdef TRACE_STORAGE
  printf("rmfsInitDataBlockHdrArray: \n");
#endif
  memset(rmfsDataBlockHdrArray, 0xFF,
	 sizeof(_RmfsDataBlockHdr) * MAX_DATABLOCK_NUMBER);

  rmfsDataBlockPtr = RmfsMemoryStart + sizeof(_RmfsHdr);

  while (((unsigned int)rmfsDataBlockPtr <= (RmfsDataBufferEnd - sizeof(_RmfsBlockHdr)))
	 && (i < MAX_DATABLOCK_NUMBER)) {

    if (ReadDataFromStorage
	(&dataBlockHdr, rmfsDataBlockPtr, sizeof(_RmfsBlockHdr)) < 0) {
#ifdef TRACE_STORAGE
      printf("Read Persistent Storage Failed \n");
#endif
    }

    rmfsDataBlockHdrArray[i].flag = dataBlockHdr.flag;
    rmfsDataBlockHdrArray[i].identifier = dataBlockHdr.nameID;
    rmfsDataBlockHdrArray[i].offset = rmfsDataBlockPtr;
    rmfsDataBlockHdrArray[i].size = dataBlockHdr.blockSize;
    rmfsDataBlockHdrArray[i].dataSize = dataBlockHdr.dataSize;
    rmfsDataBlockHdrArray[i].next = dataBlockHdr.next;
    i++;

    rmfsDataBlockPtr += dataBlockHdr.blockSize + sizeof(_RmfsBlockHdr);
  }

  curDataBlockNum = i;
}



/**
 * FUNCTION:      rmfsFileAlloc()
 * TYPE:          public operation
 * OVERVIEW:      Allocate space from the raw memory block for one file 
 * INTERFACE:
 *   parameters:  numBytesToAllocate       Number of byte to allocate
 *                filename   Filename where allocation occured
 *                dataBuffer The data buffer of the new file
 *                blockType  File Type (JAD/JAR/RMS )
 *   returns:     pointer to the newly allocated block
 *                
 */
jint
rmfsFileAlloc(jint numBytesToAllocate, const char *filename,
	      const void *dataBuffer, jboolean blockType)
{

  jint sizeDiff = -1;
  jint minSizeDiff = -1;
  short fitBlock = -1;
  jint rmfsMemoryPtr;
  _RmfsBlockHdr dataBlockHdr;
  _RmfsBlockHdr nextBlockHdr;
  jint i = 0;
  jint j = 0;
  jint minsizeDiff = -1;
  uchar fileID = 0;
  jint totalSpace = 0;
  RMFS_BOOLEAN firstBlock = RMFS_TRUE;
  jint prevBlockOffset = -1;
  jint firstBlockOffset = -1;
  jint prevIndex = 0;
  jint fileSize;	/* Byte number written to the file */
  jint newBytesToAllocate = numBytesToAllocate;

#ifdef TRACE_STORAGE
  printf("rmfsFileAlloc: numBytes: %d, filename: %s, blockType %d\n",
	 numBytesToAllocate, filename, blockType);
#endif
  /*
     When the dataBuffer is NULL, just allocate space for the file, doesn't write data into it 
   */
  if (dataBuffer == NULL) {
    fileSize = 0;
  } else {
    fileSize = numBytesToAllocate;
  }

  if((newBytesToAllocate/ALIGNMENT_BYTE) * ALIGNMENT_BYTE != numBytesToAllocate) { 
    newBytesToAllocate = (newBytesToAllocate/ALIGNMENT_BYTE + 1) * ALIGNMENT_BYTE; 
  }

  if ((searchNameTabByString(filename, &fileID) <= 0) || (fileID <= 0)) {
    fileID = rmfsNewNameTable(filename);

    if (fileID == 0) {

      return -1;
    }
  }

  rmfsMemoryPtr = RmfsMemoryStart + sizeof(_RmfsHdr);

  while ((unsigned int)rmfsMemoryPtr <= (RmfsNameTableEnd - sizeof(_RmfsBlockHdr))) {

    if (ReadDataFromStorage
	(&dataBlockHdr, rmfsMemoryPtr, sizeof(_RmfsBlockHdr)) < 0) {
#ifdef TRACE_STORAGE
      printf("Read Persistent Storage Failed \n");
#endif
    }

    /*
       Best Fit Algorithm 
     */
    while ((rmfsDataBlockHdrArray[i].flag != 0xFF)
	   && (i < MAX_DATABLOCK_NUMBER)) {
      /*
         allocating 
       */
      if ((rmfsDataBlockHdrArray[i].flag & 0x01) == 0) {
	/*
	   The block is free 
	 */
	if ((rmfsDataBlockHdrArray[i].size >= newBytesToAllocate) && 
		((rmfsDataBlockHdrArray[i].size != MAX_BLOCKSIZE_4_INSTALL_TEMP) || 
	        (newBytesToAllocate == MAX_BLOCKSIZE_4_INSTALL_TEMP)))	{
	  sizeDiff = rmfsDataBlockHdrArray[i].size - newBytesToAllocate;
	  if ((minSizeDiff == -1) || (sizeDiff < minsizeDiff)) {
	    minSizeDiff = sizeDiff;
	    fitBlock = i;
	  }
	}
      }

      i++;
    }

    curDataBlockNum = i;

    if ((minSizeDiff >= 0) && (fitBlock >= 0)) {

      rmfsDataBlockHdrArray[fitBlock].flag = 0x01 | blockType;
      rmfsDataBlockHdrArray[fitBlock].identifier = fileID;
      rmfsDataBlockHdrArray[fitBlock].next = 0;

      dataBlockHdr.flag = rmfsDataBlockHdrArray[fitBlock].flag;
      dataBlockHdr.dataSize = fileSize;
      dataBlockHdr.blockSize = rmfsDataBlockHdrArray[fitBlock].size;

      dataBlockHdr.nameID = fileID;

      rmfsDataBlockHdrArray[fitBlock].dataSize = fileSize;
      dataBlockHdr.next = 0;


      /*
         If the remain of the block is larger than the Block Head and the total block number doesn't exceed the Maximum,
         split the block
       */

      if (((unsigned int)minSizeDiff > sizeof(_RmfsBlockHdr))
	  && (i < (MAX_DATABLOCK_NUMBER - 1))) {
	nextBlockHdr.flag = 0x00;
	nextBlockHdr.blockSize = minSizeDiff - sizeof(_RmfsBlockHdr);

	rmfsDataBlockHdrArray[fitBlock].size =
	    rmfsDataBlockHdrArray[fitBlock].size - minSizeDiff;
	dataBlockHdr.blockSize = rmfsDataBlockHdrArray[fitBlock].size;
	nextBlockHdr.dataSize = 0;

	nextBlockHdr.nameID = 0;
	nextBlockHdr.next = 0;

	WriteDataToStorage(&nextBlockHdr,
			   (rmfsDataBlockHdrArray[fitBlock].offset +
			    dataBlockHdr.blockSize +
			    sizeof(_RmfsBlockHdr)), sizeof(_RmfsBlockHdr));

	/*
	   copy the block head info. behind the fitBlock slot in rmfsDataBlockHdrArray 
	 */

	while (i > (fitBlock + 1)) {
	  rmfsDataBlockHdrArray[i].flag = rmfsDataBlockHdrArray[i - 1].flag;
	  rmfsDataBlockHdrArray[i].identifier =
	      rmfsDataBlockHdrArray[i - 1].identifier;
	  rmfsDataBlockHdrArray[i].offset = rmfsDataBlockHdrArray[i - 1].offset;
	  rmfsDataBlockHdrArray[i].size = rmfsDataBlockHdrArray[i - 1].size;
	  rmfsDataBlockHdrArray[i].dataSize =
	      rmfsDataBlockHdrArray[i - 1].dataSize;

	  i--;
	}

	rmfsDataBlockHdrArray[fitBlock + 1].flag = nextBlockHdr.flag;
	rmfsDataBlockHdrArray[fitBlock + 1].identifier = 0;
	rmfsDataBlockHdrArray[fitBlock + 1].offset =
	    rmfsDataBlockHdrArray[fitBlock].offset +
	    dataBlockHdr.blockSize + sizeof(_RmfsBlockHdr);
	rmfsDataBlockHdrArray[fitBlock + 1].size = nextBlockHdr.blockSize;
	rmfsDataBlockHdrArray[fitBlock + 1].dataSize = 0;

	curDataBlockNum++;

#ifdef TRACE_STORAGE
	printf ("Split the Free Block:Index %d, Flag %d,  Remain Size: %d\n",
	     fitBlock + 1, nextBlockHdr.flag, nextBlockHdr.blockSize);
#endif

      }
      /* end of spliting */
      WriteDataToStorage(&dataBlockHdr,
			 rmfsDataBlockHdrArray[fitBlock].offset,
			 sizeof(_RmfsBlockHdr));

      WriteDataToStorage(dataBuffer,
			 rmfsDataBlockHdrArray[fitBlock].offset +
			 sizeof(_RmfsBlockHdr), dataBlockHdr.dataSize);

      return rmfsDataBlockHdrArray[fitBlock].offset;

    }

    /* end of best fit allocation */
    /*
       allocating one block for this file at the end of JAD/JAR/RMS space 
     */
    if (((RmfsNameTableEnd - sizeof(_RmfsBlockHdr) - newBytesToAllocate) >=
	 (unsigned int) RmfsDataBufferEnd)  
        && ((unsigned int)curDataBlockNum < MAX_DATABLOCK_NUMBER)) {
      /*
         If the last data block belongs to the same file, just enlarge the file 
       */
      if ((rmfsDataBlockHdrArray[curDataBlockNum - 1].identifier == fileID)
	  && (rmfsDataBlockHdrArray[curDataBlockNum - 1].next == 0)) {
	rmfsDataBlockHdrArray[curDataBlockNum - 1].size += newBytesToAllocate;
	rmfsDataBlockHdrArray[curDataBlockNum - 1].dataSize += fileSize;

	dataBlockHdr.flag = rmfsDataBlockHdrArray[curDataBlockNum - 1].flag;
	dataBlockHdr.blockSize =
	    rmfsDataBlockHdrArray[curDataBlockNum - 1].size;
	dataBlockHdr.dataSize =
	    rmfsDataBlockHdrArray[curDataBlockNum - 1].dataSize;
	dataBlockHdr.nameID =
	    rmfsDataBlockHdrArray[curDataBlockNum - 1].identifier;
	dataBlockHdr.next = rmfsDataBlockHdrArray[curDataBlockNum - 1].next;

	WriteDataToStorage(&dataBlockHdr,
			   rmfsDataBlockHdrArray[curDataBlockNum -
						 1].offset,
			   sizeof(_RmfsBlockHdr));

	if (fileSize > 0) {
	  WriteDataToStorage(dataBuffer, RmfsDataBufferEnd, fileSize);
	}

	RmfsDataBufferEnd += newBytesToAllocate;

	return RmfsDataBufferEnd - sizeof(_RmfsBlockHdr) -
	    dataBlockHdr.blockSize;

      }
      rmfsDataBlockHdrArray[curDataBlockNum].flag = 0x01 | blockType;
      rmfsDataBlockHdrArray[curDataBlockNum].identifier = fileID;
      rmfsDataBlockHdrArray[curDataBlockNum].size = newBytesToAllocate;
      rmfsDataBlockHdrArray[curDataBlockNum].offset = RmfsDataBufferEnd;
      rmfsDataBlockHdrArray[curDataBlockNum].dataSize = fileSize;
      rmfsDataBlockHdrArray[curDataBlockNum].next = 0;

      dataBlockHdr.flag = 0x01 | blockType;
      dataBlockHdr.blockSize = newBytesToAllocate;
      dataBlockHdr.dataSize = fileSize;
      dataBlockHdr.nameID = fileID;
      dataBlockHdr.next = 0;

      WriteDataToStorage(&dataBlockHdr, RmfsDataBufferEnd,
			 sizeof(_RmfsBlockHdr));

      if (dataBlockHdr.dataSize > 0) {
	WriteDataToStorage(dataBuffer,
			   RmfsDataBufferEnd + sizeof(_RmfsBlockHdr),
			   dataBlockHdr.dataSize);
      }

      RmfsDataBufferEnd += (sizeof(_RmfsBlockHdr) + dataBlockHdr.blockSize);

      curDataBlockNum++;

      return RmfsDataBufferEnd - sizeof(_RmfsBlockHdr) - dataBlockHdr.blockSize;


    }


    /* end of allocating new block for file at the end of the JAD/JAR/RMS space */
    /* Linked Blocks are allocated for the file */
    /* The free blocks are allocated to this file in order */
    /* Check the space of all holes (include free storage ) is sufficient for the new file */
    for (i = 0; i < curDataBlockNum; i++) {
      if ((rmfsDataBlockHdrArray[i].flag & 0x01) == 0) {
	totalSpace += rmfsDataBlockHdrArray[i].size;
      }
    }

    if ((unsigned int)(RmfsNameTableEnd - RmfsDataBufferEnd) > sizeof(_RmfsBlockHdr)) {
      totalSpace +=
	  RmfsNameTableEnd - RmfsDataBufferEnd - sizeof(_RmfsBlockHdr);
    }

    if (totalSpace < newBytesToAllocate) {
#ifdef TRACE_STORAGE
      printf("Free space is not sufficient for the new file or record \n");
#endif
      return -1;
    }

    totalSpace = 0;

    /*
       Allocate free blocks for the new file in order 
     */
    for (i = 0; i < curDataBlockNum; i++) {

      if ((rmfsDataBlockHdrArray[i].flag & 0x01) == 0) {
	if (!firstBlock) {
	  dataBlockHdr.next = rmfsDataBlockHdrArray[i].offset;
	  rmfsDataBlockHdrArray[prevIndex].next =
	      rmfsDataBlockHdrArray[i].offset;

	  /*
	     Non-frist block 
	   */
	  rmfsDataBlockHdrArray[i].flag = 0x80 | blockType;

	  WriteDataToStorage(&dataBlockHdr, prevBlockOffset,
			     sizeof(_RmfsBlockHdr));

	  if (dataBlockHdr.dataSize > 0) {
	    WriteDataToStorage(dataBuffer,
			       prevBlockOffset + sizeof(_RmfsBlockHdr),
			       dataBlockHdr.dataSize);
	  }

	  firstBlock = RMFS_FALSE;

	}

	prevIndex = i;

	rmfsDataBlockHdrArray[i].flag = 0x01 | blockType;
	rmfsDataBlockHdrArray[i].identifier = fileID;

	totalSpace += rmfsDataBlockHdrArray[i].size;

	if (totalSpace < newBytesToAllocate) {
	  dataBlockHdr.blockSize = rmfsDataBlockHdrArray[i].size;

	  if (fileSize > 0) {
	    dataBlockHdr.dataSize = rmfsDataBlockHdrArray[i].size;
	  } else {
	    dataBlockHdr.dataSize = 0;
	  }

	  dataBlockHdr.flag = 0x01 | blockType;
	  dataBlockHdr.nameID = fileID;

	  prevBlockOffset = rmfsDataBlockHdrArray[i].offset;

	  if (firstBlock) {
	    firstBlockOffset = prevBlockOffset;
	  }
	} else {		/* This is the last block of the file */

	  if (fileSize > 0) {
	    dataBlockHdr.dataSize =
		newBytesToAllocate - (totalSpace -
				      rmfsDataBlockHdrArray[i].size);
	  } else {
	    dataBlockHdr.dataSize = 0;
	  }


	  /*
	     if it is necessary, split the block 
	   */
	  if ((unsigned int)(rmfsDataBlockHdrArray[i].size - dataBlockHdr.dataSize) >
	      sizeof(_RmfsBlockHdr)
	      || ((unsigned int)curDataBlockNum < MAX_DATABLOCK_NUMBER)) {

	    nextBlockHdr.flag = 0x00;
	    nextBlockHdr.blockSize =
		rmfsDataBlockHdrArray[i].size - dataBlockHdr.dataSize -
		sizeof(_RmfsBlockHdr);

	    rmfsDataBlockHdrArray[i].dataSize = dataBlockHdr.dataSize;
	    dataBlockHdr.blockSize =
		newBytesToAllocate - (totalSpace -
				      rmfsDataBlockHdrArray[i].size);
	    rmfsDataBlockHdrArray[i].size = dataBlockHdr.blockSize;
	    dataBlockHdr.next = 0;
	    nextBlockHdr.dataSize = 0;

	    nextBlockHdr.nameID = 0;
	    nextBlockHdr.next = 0;

	    WriteDataToStorage(&nextBlockHdr,
			       (rmfsDataBlockHdrArray[i].offset +
				dataBlockHdr.blockSize +
				sizeof(_RmfsBlockHdr)), sizeof(_RmfsBlockHdr));

	    /*
	       copy the block head info. behind the fitBlock slot in rmfsDataBlockHdrArray 
	     */

	    for (j = curDataBlockNum; j > i + 1; j--) {
	      rmfsDataBlockHdrArray[j].flag = rmfsDataBlockHdrArray[j - 1].flag;
	      rmfsDataBlockHdrArray[j].identifier =
		  rmfsDataBlockHdrArray[j - 1].identifier;
	      rmfsDataBlockHdrArray[j].offset =
		  rmfsDataBlockHdrArray[j - 1].offset;
	      rmfsDataBlockHdrArray[j].size = rmfsDataBlockHdrArray[j - 1].size;
	      rmfsDataBlockHdrArray[j].dataSize =
		  rmfsDataBlockHdrArray[j - 1].dataSize;

	    }

	    rmfsDataBlockHdrArray[i + 1].flag = nextBlockHdr.flag;
	    rmfsDataBlockHdrArray[i + 1].identifier = nextBlockHdr.nameID;
	    rmfsDataBlockHdrArray[i + 1].offset =
		rmfsDataBlockHdrArray[i].offset + dataBlockHdr.dataSize +
		sizeof(_RmfsBlockHdr);
	    rmfsDataBlockHdrArray[i + 1].size = nextBlockHdr.blockSize;
	    rmfsDataBlockHdrArray[i + 1].dataSize = nextBlockHdr.dataSize;

	    curDataBlockNum++;

	  } else {
	    dataBlockHdr.blockSize = rmfsDataBlockHdrArray[i].size;
	    rmfsDataBlockHdrArray[i].dataSize = dataBlockHdr.dataSize;
	    dataBlockHdr.flag = 0x01 | blockType;
	    dataBlockHdr.nameID = fileID;
	    dataBlockHdr.next = 0;
	  }			/* End of split block */

	  WriteDataToStorage(&dataBlockHdr,
			     rmfsDataBlockHdrArray[i].offset,
			     sizeof(_RmfsBlockHdr));

	  if (dataBlockHdr.dataSize > 0) {
	    WriteDataToStorage(&dataBuffer + totalSpace -
			       rmfsDataBlockHdrArray[i].size,
			       rmfsDataBlockHdrArray[i].offset +
			       sizeof(_RmfsBlockHdr), dataBlockHdr.dataSize);
	  }

	  return firstBlockOffset;

	}			/* End of last block check */
      }
      /* End of free block check */
    }				/* End of For-Loop */

    /*
       If the space of all holes is insufficient for this new file, use the free space 
     */
    if ((totalSpace < newBytesToAllocate)
	&& (curDataBlockNum < MAX_DATABLOCK_NUMBER)) {
      /*
         Write the previous new allocated block  to storage 
       */
      dataBlockHdr.next = RmfsDataBufferEnd;

      WriteDataToStorage(&dataBlockHdr, prevBlockOffset, sizeof(_RmfsBlockHdr));

      WriteDataToStorage(dataBuffer,
			 prevBlockOffset + sizeof(_RmfsBlockHdr),
			 dataBlockHdr.dataSize);


      rmfsDataBlockHdrArray[curDataBlockNum].flag = 0x01 | blockType;
      rmfsDataBlockHdrArray[curDataBlockNum].identifier = fileID;
      rmfsDataBlockHdrArray[curDataBlockNum].size =
	  newBytesToAllocate - totalSpace;

      if (fileSize > 0) {
	rmfsDataBlockHdrArray[curDataBlockNum].dataSize =
	    rmfsDataBlockHdrArray[curDataBlockNum].size;
      } else {
	rmfsDataBlockHdrArray[curDataBlockNum].dataSize = 0;
      }

      rmfsDataBlockHdrArray[curDataBlockNum].offset = RmfsDataBufferEnd;

      dataBlockHdr.flag = 0x01 | blockType;
      dataBlockHdr.blockSize = newBytesToAllocate - totalSpace;

      if (fileSize > 0) {
	dataBlockHdr.dataSize = numBytesToAllocate - totalSpace;
      } else {
	dataBlockHdr.dataSize = 0;
      }

      dataBlockHdr.nameID = fileID;
      dataBlockHdr.next = 0;

      WriteDataToStorage(&dataBlockHdr, RmfsDataBufferEnd,
			 sizeof(_RmfsBlockHdr));

      if (dataBlockHdr.dataSize > 0) {
	WriteDataToStorage(dataBuffer,
			   RmfsDataBufferEnd + sizeof(_RmfsBlockHdr),
			   dataBlockHdr.dataSize);
      }

      RmfsDataBufferEnd += sizeof(_RmfsBlockHdr) + dataBlockHdr.blockSize;

      curDataBlockNum++;

      return firstBlockOffset;
    }
    break;
  }				/* End of While-Loop */

  return rmfsDataBlockHdrArray[i].offset;

}



/**
 * FUNCTION:      rmfsFreeDataBlock()
 * TYPE:          public operation
 * OVERVIEW:      Free one Data Block in JAD/JAR/RMS space.
 *                 
 * INTERFACE:
 *   parameters:  jint        the index of the data block (From 0 to curDataBlockNum -1 )
 *                
 *   returns:     pointer to newly cleared storage data block
 *                
 */
jint rmfsFreeDataBlock(jint index)
{
  _RmfsBlockHdr blockHdr;
  RMFS_BOOLEAN prevFree = RMFS_FALSE;
  RMFS_BOOLEAN nextFree = RMFS_FALSE;
  RMFS_BOOLEAN lastBlock = RMFS_FALSE;

  jint freeBlockPos = -1;
  jint i;

#ifdef TRACE_STORAGE
  printf("rmfsFreeDataBlock: index:  %d; size %d\n", index,
	 rmfsDataBlockHdrArray[index].size);
#endif

  if (index >= curDataBlockNum - 1) {
    lastBlock = RMFS_TRUE;
  }

  if (ReadDataFromStorage
      (&blockHdr, rmfsDataBlockHdrArray[index].offset,
       sizeof(_RmfsBlockHdr)) <= 0) {
#ifdef TRACE_STORAGE
    printf("Read data from persistent storage failed \n");
#endif
    return -1;
  }

  if (index > 0) {
    if ((rmfsDataBlockHdrArray[index - 1].flag & 0x01) == 0) {
      prevFree = RMFS_TRUE;
    }
  }

  if (index < (curDataBlockNum - 1)) {
    if ((rmfsDataBlockHdrArray[index + 1].flag & 0x01) == 0) {
      nextFree = RMFS_TRUE;
    }
  }

  blockHdr.flag = 0x00;
  blockHdr.next = 0;
  blockHdr.dataSize = 0;
  blockHdr.nameID = 0;

  rmfsDataBlockHdrArray[index].flag = 0x00;
  rmfsDataBlockHdrArray[index].identifier = 0;

  if (nextFree == RMFS_TRUE) {

    rmfsDataBlockHdrArray[index].size +=
	rmfsDataBlockHdrArray[index + 1].size + sizeof(_RmfsBlockHdr);
    for (i = index + 1; i < curDataBlockNum; i++) {
      rmfsDataBlockHdrArray[i].flag = rmfsDataBlockHdrArray[i + 1].flag;
      rmfsDataBlockHdrArray[i].offset = rmfsDataBlockHdrArray[i + 1].offset;
      rmfsDataBlockHdrArray[i].identifier =
	  rmfsDataBlockHdrArray[i + 1].identifier;
      rmfsDataBlockHdrArray[i].size = rmfsDataBlockHdrArray[i + 1].size;
      rmfsDataBlockHdrArray[i].dataSize = rmfsDataBlockHdrArray[i + 1].dataSize;
    }

    blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;

    memset(&rmfsDataBlockHdrArray[curDataBlockNum - 1], 0xFF,
	   sizeof(_RmfsDataBlockHdr));
    curDataBlockNum--;
  }

  if (prevFree == RMFS_TRUE) {
    rmfsDataBlockHdrArray[index - 1].identifier = 0;
    rmfsDataBlockHdrArray[index - 1].size +=
	rmfsDataBlockHdrArray[index].size + sizeof(_RmfsBlockHdr);


    blockHdr.blockSize = rmfsDataBlockHdrArray[index - 1].size;
    for (i = index; i < curDataBlockNum; i++) {
      rmfsDataBlockHdrArray[i].flag = rmfsDataBlockHdrArray[i + 1].flag;
      rmfsDataBlockHdrArray[i].offset = rmfsDataBlockHdrArray[i + 1].offset;
      rmfsDataBlockHdrArray[i].identifier =
	  rmfsDataBlockHdrArray[i + 1].identifier;
      rmfsDataBlockHdrArray[i].size = rmfsDataBlockHdrArray[i + 1].size;
      rmfsDataBlockHdrArray[i].dataSize = rmfsDataBlockHdrArray[i + 1].dataSize;
    }

    memset(&rmfsDataBlockHdrArray[curDataBlockNum - 1], 0xFF,
	   sizeof(_RmfsDataBlockHdr));
    curDataBlockNum--;

    WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index - 1].offset,
		       sizeof(_RmfsBlockHdr));

    freeBlockPos = rmfsDataBlockHdrArray[index - 1].size;

  } else {
    WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
		       sizeof(_RmfsBlockHdr));
    freeBlockPos = rmfsDataBlockHdrArray[index].size;
  }

  if (lastBlock == RMFS_TRUE) {

    if (curDataBlockNum < 2) {
      RmfsDataBufferEnd = RmfsMemoryStart + sizeof(_RmfsHdr);
      return 0;
    }

    if (rmfsDataBlockHdrArray[curDataBlockNum - 2].flag != 0xFF) {
      RmfsDataBufferEnd =
	  rmfsDataBlockHdrArray[curDataBlockNum - 2].offset +
	  rmfsDataBlockHdrArray[curDataBlockNum - 2].size +
	  sizeof(_RmfsBlockHdr);
    } else {
#ifdef TRACE_STORAGE
      printf("Exception happens in FreeDataBlock \n");
#endif
    }

    memset(&rmfsDataBlockHdrArray[index], 0xFF, sizeof(_RmfsDataBlockHdr));
    curDataBlockNum--;
  }

  return freeBlockPos;

}


/**
 * FUNCTION:      rmfsDelFile(const char* filename)
 * TYPE:          public operation
 * OVERVIEW:      Delete all blocks assigned for one file
 * INTERFACE:
 *   parameters:  const char* filename   Filename 
 *                
 *   return:      void             
 */
void rmfsDelFile(const char *filename)
{
  uchar filenameID = 0;
  jint i;

#ifdef TRACE_STORAGE
  printf("rmfsDelFile: filename:  %s\n", filename);
#endif
  if ((searchNameTabByString(filename, &filenameID) <= 0)
      || (filenameID <= 0)) {
    return;
  }

  for (i = curDataBlockNum - 1; i >= 0; i--) {
    if (rmfsDataBlockHdrArray[i].identifier == filenameID) {
      rmfsFreeDataBlock(i);
    }
  }

  return;

}


/**
 * FUNCTION:      rmfsFree(jint start, jint size )
 * TYPE:          public operation
 * OVERVIEW:      Free the whole RMFS storage, just rewrite the MAGIC and Head
 *                
 * INTERFACE:
 *   parameters:  size   Size of Raw Memory Storage to use
 *                start  start address of the Raw memory storage
 *   returns:     0 on succes; != 0 on failure
 *                
 */
/*jint rmfsFree(jint start, jint size) */

jint rmfsFree(jint start, jint size)
{
  _RmfsHdr rmfsHdr;


#ifdef TRACE_STORAGE
  printf("rmfsFree: start: %d; size: %d\n", start, size);
#endif
  if ((unsigned int)size < sizeof(_RmfsBlockHdr) + sizeof(_RmfsHdr)) {
#ifdef TRACE_STORAGE
    printf("Can't initialize. Not enough memory.\n");
#endif
    return -1;
  }

  if ((start < MIN_RAW_MEMORY_ADDR)
      || ((unsigned int)(start + size) > MAX_RAW_MEMORY_ADDR)) {
#ifdef TRACE_STORAGE
    printf("Start address of the memory block is invalid \n");
#endif
    return -1;
  }

  rmfsHdr.magic = 0;
  rmfsHdr.dataBufferEndPos = 0;
  RmfsDataBufferEnd = rmfsHdr.dataBufferEndPos;

  rmfsHdr.nameTableEndPos = 0;
  RmfsNameTableEnd = rmfsHdr.nameTableEndPos;

  rmfsHdr.maxFileID = 0;
  RmfsGlobalNameID = 0;

  WriteDataToStorage(&rmfsHdr, start, sizeof(_RmfsHdr));


  return 0;
}

/**
 * FUNCTION:      rmfsWriteDataToBlock()
 * TYPE:          public operation
 * OVERVIEW:      Wirte data buffer to one RMFS  Data Block
 * INTERFACE:
 *   parameters:  index:   DataBlock Index
 *                buffer   the data buffer will be written into RMFS Data Block.
 *                offset   start address in RMFS DataBlock
 *                size     the byte number 
 *   returns:     byte number of written buffer ; < 0 on failure
 *                
 */
jint rmfsReadBlock(jint index, void *buffer, jint size, jint offset)
{
  jint storePos;

#ifdef TRACE_STORAGE
  printf("rmfsReadBlock: index: %d; size: %d; offset %d\n", index, size,
	 offset);
#endif
  if ((offset + size) > rmfsDataBlockHdrArray[index].size) {
    return -1;
  }

  storePos =
      rmfsDataBlockHdrArray[index].offset + offset + sizeof(_RmfsBlockHdr);

  ReadDataFromStorage(buffer, storePos, size);

  return size;
}

/**
 * FUNCTION:      rmfsWriteDataToBlock()
 * TYPE:          public operation
 * OVERVIEW:      Wirte data buffer to one RMFS  Data Block
 * INTERFACE:
 *   parameters:  index:   DataBlock Index
 *                buffer   the data buffer will be written into RMFS Data Block.
 *                offset   start address in RMFS DataBlock
 *                size     the byte number 
 *   returns:     byte number of written buffer ; < 0 on failure
 *                
 */
jint rmfsWriteDataToBlock(jint index, const void *buffer, jint size, jint offset)
{
  jint storePos;
  _RmfsBlockHdr blockHdr;

#ifdef TRACE_STORAGE
  printf("rmfsWriteDataToBlock: index: %d; size: %d; offset %d\n", index,
	 size, offset);
#endif

  if ((offset + size) > rmfsDataBlockHdrArray[index].size) {
    return -1;
  }

  if ((offset + size) > rmfsDataBlockHdrArray[index].dataSize) {
    rmfsDataBlockHdrArray[index].dataSize = offset + size;
  }

  /*
     Write the new header to the storage 
   */
  blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
  blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
  blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
  blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
  blockHdr.next = rmfsDataBlockHdrArray[index].next;

  WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
		     sizeof(_RmfsBlockHdr));

  /*
     Write the contents to the storage 
   */
  storePos =
      rmfsDataBlockHdrArray[index].offset + offset + sizeof(_RmfsBlockHdr);

  WriteDataToStorage(buffer, storePos, size);

  return size;
}

/**
 * FUNCTION:      rmfsAllocLinkedBlock()
 * TYPE:          public operation
 * OVERVIEW:      Allocate new Data Block behind one allocated Data Block for same file
 * INTERFACE:
 *   parameters:  index:   DataBlock Index
 *                buffer   the data buffer will be written into the new allocated RMFS Data Block.
 *                size     the byte number 
 *   returns:     byte number of written buffer ; < 0 on failure
 *                
 */

jint rmfsAllocLinkedBlock(jint index, const void *buffer, jint size)
{
  jint storePos;
  char *filename = NULL;
  jboolean fileType;
  _RmfsBlockHdr blockHdr;
  jint freeSize = 0;
  jint i;

#ifdef TRACE_STORAGE
  printf("rmfsAllocLinkedBlock: index: %d; size: %d\n", index, size);
#endif
  fileType = rmfsDataBlockHdrArray[index].flag & 0x3E;

  if ((searchNameTabByID
       (&filename, rmfsDataBlockHdrArray[index].identifier)) < 0) {
    return -1;
  }

  if (filename == NULL) {
    return -1;
  }

  fileType |= 0x80;		/* It is not the first block */

  if ((index == curDataBlockNum - 1)) {
    if ((RmfsNameTableEnd - RmfsDataBufferEnd) >= size) {
      rmfsDataBlockHdrArray[index].size += size;
      rmfsDataBlockHdrArray[index].dataSize += size;

      /*
         Write the new header to the storage 
       */
      blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
      blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
      blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
      blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
      blockHdr.next = rmfsDataBlockHdrArray[index].next;
      WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
			 sizeof(_RmfsBlockHdr));

      /*
         Write data buffer to storage 
       */
      WriteDataToStorage(buffer, RmfsDataBufferEnd, size);

      /*
         Update RmfsDataBufferEnd 
       */
      RmfsDataBufferEnd += size;
    } else {
#ifdef TRACE_STORAGE
      printf("No enough space \n");
#endif
      return -1;
    }

    return size;

  }

  if ((rmfsDataBlockHdrArray[index + 1].flag & 0x01) == 0x00) {

    if (rmfsDataBlockHdrArray[index + 1].size > size) {
      rmfsDataBlockHdrArray[index].size += size;
      rmfsDataBlockHdrArray[index].dataSize += size;

      /*
         Write the new header to the storage 
       */
      blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
      blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
      blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
      blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
      blockHdr.next = rmfsDataBlockHdrArray[index].next;
      WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
			 sizeof(_RmfsBlockHdr));

      /*
         Write data buffer to storage 
       */
      WriteDataToStorage(buffer,
			 rmfsDataBlockHdrArray[index].offset +
			 sizeof(_RmfsBlockHdr)
			 + blockHdr.dataSize - size, size);

      /*
         Update next free block 
       */
      rmfsDataBlockHdrArray[index + 1].size -= size;

      /*
         Write the new header to the storage 
       */
      blockHdr.blockSize = rmfsDataBlockHdrArray[index + 1].size;
      blockHdr.dataSize = rmfsDataBlockHdrArray[index + 1].dataSize;
      blockHdr.flag = rmfsDataBlockHdrArray[index + 1].flag;
      blockHdr.nameID = rmfsDataBlockHdrArray[index + 1].identifier;
      blockHdr.next = rmfsDataBlockHdrArray[index + 1].next;

      rmfsDataBlockHdrArray[index + 1].offset =
	  rmfsDataBlockHdrArray[index].offset +
	  rmfsDataBlockHdrArray[index].size + sizeof(_RmfsBlockHdr);

      WriteDataToStorage(&blockHdr,
			 rmfsDataBlockHdrArray[index].offset +
			 rmfsDataBlockHdrArray[index].size +
			 sizeof(_RmfsBlockHdr), sizeof(_RmfsBlockHdr));

      return size;

    } else if ((rmfsDataBlockHdrArray[index + 1].size +
		sizeof(_RmfsBlockHdr)) >= (unsigned int)size) {

      rmfsDataBlockHdrArray[index].size +=
	  (rmfsDataBlockHdrArray[index + 1].size + sizeof(_RmfsBlockHdr));
      rmfsDataBlockHdrArray[index].dataSize += size;

      /*
         Write the new header to the storage 
       */
      blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
      blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
      blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
      blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
      blockHdr.next = rmfsDataBlockHdrArray[index].next;
      WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
			 sizeof(_RmfsBlockHdr));

      /*
         Write data buffer to storage 
       */
      WriteDataToStorage(buffer,
			 rmfsDataBlockHdrArray[index].offset +
			 sizeof(_RmfsBlockHdr)
			 + blockHdr.dataSize - size, size);


      /*
         Remove the Free Data Block record 
       */
      for (i = index + 1; i < curDataBlockNum; i++) {
	rmfsDataBlockHdrArray[i].flag = rmfsDataBlockHdrArray[i + 1].flag;
	rmfsDataBlockHdrArray[i].offset = rmfsDataBlockHdrArray[i + 1].offset;
	rmfsDataBlockHdrArray[i].identifier =
	    rmfsDataBlockHdrArray[i + 1].identifier;
	rmfsDataBlockHdrArray[i].size = rmfsDataBlockHdrArray[i + 1].size;
	rmfsDataBlockHdrArray[i].dataSize =
	    rmfsDataBlockHdrArray[i + 1].dataSize;
      }

      memset(&rmfsDataBlockHdrArray[curDataBlockNum - 1], 0xFF,
	     sizeof(_RmfsDataBlockHdr));

      curDataBlockNum--;
      return size;
    } else {

      freeSize = rmfsDataBlockHdrArray[index + 1].size + sizeof(_RmfsBlockHdr);

      rmfsDataBlockHdrArray[index].size += freeSize;
      rmfsDataBlockHdrArray[index].dataSize += freeSize;

      /*
         Write the new header to the storage 
       */
      blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
      blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
      blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
      blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
      blockHdr.next = rmfsDataBlockHdrArray[index].next;
      WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
			 sizeof(_RmfsBlockHdr));

      /*
         Write data buffer to storage 
       */
      WriteDataToStorage(buffer,
			 rmfsDataBlockHdrArray[index].offset +
			 sizeof(_RmfsBlockHdr) + blockHdr.dataSize -
			 freeSize, freeSize);

      /*
         Remove the Free Data Block record 
       */
      for (i = index + 1; i < curDataBlockNum; i++) {
	rmfsDataBlockHdrArray[i].flag = rmfsDataBlockHdrArray[i + 1].flag;
	rmfsDataBlockHdrArray[i].offset = rmfsDataBlockHdrArray[i + 1].offset;
	rmfsDataBlockHdrArray[i].identifier =
	    rmfsDataBlockHdrArray[i + 1].identifier;
	rmfsDataBlockHdrArray[i].size = rmfsDataBlockHdrArray[i + 1].size;
	rmfsDataBlockHdrArray[i].dataSize =
	    rmfsDataBlockHdrArray[i + 1].dataSize;
      }

      memset(&rmfsDataBlockHdrArray[curDataBlockNum - 1], 0xFF,
	     sizeof(_RmfsDataBlockHdr));

      curDataBlockNum--;
    }
  }

  storePos = rmfsFileAlloc(size - freeSize, filename, buffer, fileType);

  // jvmFree(filename);

  if (storePos < 0) {
#ifdef TRACE_STORAGE
    printf("No aviable space \n");
#endif
    return -1;
  }

  if (rmfsDataBlockHdrArray[index].offset != storePos) {

#ifdef TRACE_STORAGE
    printf("Really create a linked blocked \n");
#endif

    rmfsDataBlockHdrArray[index].next = storePos;

    /*
       Write the new header to the storage 
     */
    blockHdr.blockSize = rmfsDataBlockHdrArray[index].size;
    blockHdr.dataSize = rmfsDataBlockHdrArray[index].dataSize;
    blockHdr.flag = rmfsDataBlockHdrArray[index].flag;
    blockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
    blockHdr.next = rmfsDataBlockHdrArray[index].next;
    WriteDataToStorage(&blockHdr, rmfsDataBlockHdrArray[index].offset,
		       sizeof(_RmfsBlockHdr));
  }

  return size;
}

/**
 * FUNCTION:      rmfsSearchBlockByPos()
 * TYPE:          public operation
 * OVERVIEW:      Search the RMFS Data Block per the start address of the Data Block
 * INTERFACE:
 *   parameters:  index:   matched DataBlock Index
 *                address  the start address of the RMFS Data Block.
 *
 *   returns:     >=0 success ; < 0 on failure
 *                
 */
jint rmfsSearchBlockByPos(uchar *index, jint address)
{
  jint i;

#ifdef TRACE_STORAGE
  printf("rmfsSearchBlockByPos: index: %d; address: %d\n", *index, address);
#endif

  if (address <= 0) {
    return -1;
  }

  for (i = 0; i < MAX_DATABLOCK_NUMBER; i++) {
    if (rmfsDataBlockHdrArray[i].offset == address) {
      *index = i;
      return i;
    }
  }

  *index = 0;

  return -1;
}

/**
 * FUNCTION:      rmfsSplitBlock()
 * TYPE:          public operation
 * OVERVIEW:      Split one RMFS Data Block to two Data Blcoks, the second one is free
 * JINTERFACE:
 *   parameters:  index:   DataBlock Index
 *                address  the start address of the second RMFS Data Block.
 *
 *   returns:     >=0 success ; < 0 on failure
 *                
 */
jint rmfsSplitBlock(jint index, jint offset)
{
  _RmfsBlockHdr dataBlockHdr;
  _RmfsBlockHdr nextBlockHdr;
  jint j;
  jint orgBlockSize;
  jint newBlockSize = offset;

#ifdef TRACE_STORAGE
  printf("rmfsSplitBlock: index: %d; offset: %d\n", index, offset);
#endif

  if((newBlockSize/ALIGNMENT_BYTE) * ALIGNMENT_BYTE != offset) { 
    newBlockSize = (newBlockSize/ALIGNMENT_BYTE + 1) * ALIGNMENT_BYTE; 
  }

  dataBlockHdr.blockSize = newBlockSize;
  dataBlockHdr.dataSize = offset;
  dataBlockHdr.flag = rmfsDataBlockHdrArray[index].flag;
  dataBlockHdr.nameID = rmfsDataBlockHdrArray[index].identifier;
  dataBlockHdr.next = 0;

  orgBlockSize = rmfsDataBlockHdrArray[index].size;

  rmfsDataBlockHdrArray[index].size = newBlockSize;

  /*
     If the current block is the last block, just free the remain space 
   */
  if (index == (curDataBlockNum - 1)) {

    RmfsDataBufferEnd -= (orgBlockSize - newBlockSize);

    WriteDataToStorage(&dataBlockHdr, rmfsDataBlockHdrArray[index].offset,
		       sizeof(_RmfsBlockHdr));

    return 0;

  } else {

    if (curDataBlockNum >= MAX_DATABLOCK_NUMBER) {
      return -1;
    }

    nextBlockHdr.flag = 0x00;
    nextBlockHdr.blockSize = orgBlockSize - newBlockSize - sizeof(_RmfsBlockHdr);
    nextBlockHdr.dataSize = 0;
    nextBlockHdr.nameID = 0;
    nextBlockHdr.next = 0;

    WriteDataToStorage(&nextBlockHdr,
		       (rmfsDataBlockHdrArray[index].offset +
			dataBlockHdr.blockSize + sizeof(_RmfsBlockHdr)),
		       sizeof(_RmfsBlockHdr));

    /*
       move the block head info. behind the index slot in rmfsDataBlockHdrArray 
     */
    for (j = curDataBlockNum; j > index + 1; j--) {
      rmfsDataBlockHdrArray[j].flag = rmfsDataBlockHdrArray[j - 1].flag;
      rmfsDataBlockHdrArray[j].identifier =
	  rmfsDataBlockHdrArray[j - 1].identifier;
      rmfsDataBlockHdrArray[j].offset = rmfsDataBlockHdrArray[j - 1].offset;
      rmfsDataBlockHdrArray[j].size = rmfsDataBlockHdrArray[j - 1].size;
      rmfsDataBlockHdrArray[j].dataSize = rmfsDataBlockHdrArray[j - 1].dataSize;
      rmfsDataBlockHdrArray[j].next = rmfsDataBlockHdrArray[j - 1].next;

    }

    rmfsDataBlockHdrArray[index + 1].flag = nextBlockHdr.flag;
    rmfsDataBlockHdrArray[index + 1].identifier = nextBlockHdr.nameID;
    rmfsDataBlockHdrArray[index + 1].offset =
	rmfsDataBlockHdrArray[index].offset + dataBlockHdr.blockSize +
	sizeof(_RmfsBlockHdr);
    rmfsDataBlockHdrArray[index + 1].size = nextBlockHdr.blockSize;
    rmfsDataBlockHdrArray[index + 1].dataSize = 0;
    rmfsDataBlockHdrArray[index + 1].next = 0;

    curDataBlockNum++;
  }

  return 0;
}

#ifdef __cplusplus
}
#endif

