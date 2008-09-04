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
 * RMFS storage management and file external functions.
 *
 * With these functions, we have the ability to manage RMFS
 * raw memory pool. Please refer to the RMFS design document for details
 *
 * @warning This code is not thread safe.
 * 
 */

#include <stdio.h>
#include <string.h>

#include <pcsl_file.h>
#include <rmfsAlloc.h>
#include <rmfsApi.h>


#ifdef __cplusplus
extern "C"
{
#endif

#define RMS_DB_HEADER_SIZE (40)
  /* #define TRACE_DEBUG 0 */

#define MAX_BLOCKSIZE_4_SPECFILE 0x800	/* 2K bytes */
#define MAX_BLOCKSIZE_4_INSTALL_INFO 0x400	/* 1K bytes */
#define MAX_BLOCKSIZE_4_MIDLET_PROR    0x1800	/* 6K bytes */
#define MAX_BLOCKSIZE_4_INSTALL_TEMP   0xF000	/* 60K bytes */

extern _RmfsDataBlockHdr rmfsDataBlockHdrArray[MAX_DATABLOCK_NUMBER];
extern jint RmfsNameTableEnd;	/* End Address of RMFS Name Table Space */

_rmfsFileDes fileTab[MAX_OPEN_FILE_NUM];

/**
 * FUNCTION:      initFileSystem()
 * TYPE:          public operation
 * OVERVIEW:      Load some files into the RMFS storage from Romized file. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  NONE 
 *
 *   returns:     >= 0; succes; < 0: failed 
 *                
 */
jint initFileSystem()
{
#if 0
  int i;
  jint fd;
  jint res = 0;
  jint filesize;
  jint filenamelength;
  char *filename;
  char *filebuffer;

  /*
     write all Romized file in tck_png.h into RMFS 
   */

  for (i = 0; i < NUM_ROMFILES; i++) {
    filename = (char *) ROMFiles[i].filename;
    filesize = ROMFiles[i].size;
    filebuffer = (char *) ROMFiles[i].contents;
    filenamelength = strlen((char *) filename);


    rmfsFileExist(filename, &res);
    if (res == 1) {
      /* The file is in place so continue */
      /* go back to for loop */
      continue;
    }

    fd = rmfsOpen(filename, PCSL_FILE_O_CREAT, 0);
    if (fd < 0) {
      /* can't create the file */
      /* do what? */
      return -3;
    }
    res = rmfsWrite(fd, (char *) filebuffer, filesize);
    if (res < 0) {
      /* can't write to the file */
      rmfsClose(fd);
      rmfsUnlink(filename);
      return -4;
    }

    res = rmfsClose(fd);
    if (res < 0) {
      return -5;
    }

    fd = 0;

  }				/* end of for */

#endif
  return 1;

}

/**
 * FUNCTION:      rmfsInitOpenFileTab()
 * TYPE:          public operation
 * OVERVIEW:      Initialize the Open File Table 
 * INTERFACE:
 *   parameters:  None
 *
 *   returns:     None
 *                
 */
void rmfsInitOpenFileTab()
{
  short i = 0;

#ifdef TRACE_DEBUG
  printf("rmfsInitOpenFileTab \n");
#endif

  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    fileTab[i].createMode = 0;
    fileTab[i].dataBlockIndex = 0;
    fileTab[i].fileType = 0;
    fileTab[i].flags = 0;
    fileTab[i].handle = -1;
    fileTab[i].nameID = 0;
    fileTab[i].offset = 0;
    fileTab[i].size = 0;
  }

}

/**
 * FUNCTION:      rmfsOpen()
 * TYPE:          public operation
 * OVERVIEW:      The open function creates and returns a new RMFS file identifier for the file named by filename. 
 *                Initially, the file position indicator for the file is at the beginning of the file. 
 *    
 * INTERFACE:
 *   parameters:  filename:  The name of the file
 *                flags:   controls how the file is to be opened. This is a bit mask; 
 *                         you create the value by the bitwise OR of the appropriate parameters 
 *                         (using the `|' operator in C)
 *                creationMode  is used only when a file is created.
 *
 *   returns:     None
 *                
 */
jint rmfsOpen(const char *filename, jint flags, jint creationMode)
{
  char *suffix;
  RMFS_BOOLEAN done = RMFS_FALSE;
  RMFS_BOOLEAN newFile = RMFS_FALSE;
    char fileType = 0;
  int i;
  jint fileIndex = -1;
  jint dataBlockAddr;
  _RmfsNameTabHdr nameTabHdr;
  jint nameTabAddr;

#ifdef TRACE_DEBUG
  printf("rmfsOpen: filename: %s; flags: %d; createionMode: %d \n",
	 filename, flags, creationMode);
#endif
  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == -1) {
      fileIndex = i;
      break;
    }
  }

  if (fileIndex >= MAX_OPEN_FILE_NUM) {
#ifdef TRACE_DEBUG
    printf("Open too much file \n");
#endif

    return -1;
  }

  fileTab[fileIndex].handle = fileIndex;

  nameTabAddr = searchNameTabByString(filename, &(fileTab[fileIndex].nameID));
  if ((nameTabAddr < 0) || (fileTab[fileIndex].nameID == 0)) {
    /* If file doesn't exist and the flags don't support PCSL_FILE_O_CREAT, return fail */
    if ((flags & PCSL_FILE_O_CREAT) != PCSL_FILE_O_CREAT) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
    fileTab[fileIndex].nameID = rmfsNewNameTable(filename);

    newFile = RMFS_TRUE;
  } else {

#ifdef TRACE_DEBUG
    printf("nameTabAddr: %d\n", nameTabAddr);
#endif
    ReadDataFromStorage(&nameTabHdr, nameTabAddr, sizeof(_RmfsNameTabHdr));

    fileTab[fileIndex].size = nameTabHdr.size;
  }


  if (fileTab[fileIndex].nameID == 0XFFFF) {
#ifdef TRACE_DEBUG
    printf("Failed to find the nameID \n");
#endif
    return -1;
  }

  if (!newFile) {
    for (i = 0; i < MAX_DATABLOCK_NUMBER; i++) {
      if ((rmfsDataBlockHdrArray[i].identifier == fileTab[fileIndex].nameID)
	  && ((rmfsDataBlockHdrArray[i].flag & 0x80) == 0x00)) {
	fileTab[fileIndex].dataBlockIndex = i;
	break;
      }
    }

    if (i == MAX_DATABLOCK_NUMBER) {
      fileTab[fileIndex].dataBlockIndex = 0xFFFF;
    }



    fileTab[fileIndex].createMode = creationMode;

    // fileTab[fileIndex].size = rmfsDataBlockHdrArray[i].dataSize;

    fileTab[fileIndex].fileType = rmfsDataBlockHdrArray[i].flag & 0x3E;	/* Bit 5, 4, 3, 2, 1 */

    if (i == MAX_DATABLOCK_NUMBER) {
      fileTab[fileIndex].fileType = RMFS_NORMAL_FILE;
    }

    fileTab[fileIndex].offset = 0;

    fileTab[fileIndex].flags = flags;

#ifdef TRACE_DEBUG
    printf("Open old file correctly \n");
#endif
    return fileIndex;
  }

  /* Decide the file type by the suffix of the file name  */

  suffix = (char*)strrchr(filename, '.');

  if (suffix == NULL) {
    done = RMFS_TRUE;
    fileType = RMFS_NORMAL_FILE;
  }

#ifdef TRACE_DEBUG
  printf("Suffix: %s, File Type: %d\n", suffix, fileType);
#endif

  if (!done) {
      if (strcmp(suffix, ".jad") == 0) {
        done = RMFS_TRUE;

        fileType = RMFS_JAD_FILE;
      }
  }


  if (!done) {
    if (strcmp(suffix, ".jar") == 0) {
      done = RMFS_TRUE;

      fileType = RMFS_JAR_FILE;
    }
  }

  if (!done) {
    if (strcmp(suffix, ".ss") == 0) {
      done = RMFS_TRUE;

      fileType = RMFS_MIDLET_SETTING;
    }
  }

  if (!done) {
    if (strcmp(suffix, ".ap") == 0) {
      done = RMFS_TRUE;

      fileType = RMFS_MIDLET_PROR;
    }
  }

  if (!done) {
    if (strcmp(suffix, ".ii") == 0) {
      done = RMFS_TRUE;

      fileType = RMFS_MIDLET_INSTALL;
    }
  }

  if (!done) {
    if ((strcmp(suffix, ".db") == 0) || (strcmp(suffix, ".dbx") == 0)) {
      done = RMFS_TRUE;

      fileType = RMFS_RMS_RECORD;
    }
  }

  if (!done) {
    if (strcmp(suffix, ".tmp") == 0) {
      done = RMFS_TRUE;

      fileType = RMFS_INSTALL_TEMP;
    }
  }

  if (!done) {
    suffix = (char*)strrchr(filename, '_');

    if ((suffix != NULL) && ((suffix - filename) > 6)) {
      suffix -= 6;

      if (strcmp(suffix, "delete_notify.dat") == 0) {
	done = RMFS_TRUE;

	fileType = RMFS_MIDLET_DEL_NOTIFY;
      }
    }
  }

  if (!done) {
    suffix = (char*)strrchr(filename, '_');

    if (suffix != NULL) {
      if (strcmp(suffix, "_suites.dat") == 0) {
	done = RMFS_TRUE;

	fileType = RMFS_MIDLET_SUITE_LIST;
      }
    }
  }

  if (!done) {
    fileType = RMFS_NORMAL_FILE;
  }


#ifdef TRACE_DEBUG
  printf("Suffix: %s, File Type: %d\n", suffix, fileType);
#endif
  fileTab[fileIndex].createMode = creationMode;

  fileTab[fileIndex].dataBlockIndex = 0;

  fileTab[fileIndex].fileType = fileType;

  fileTab[fileIndex].size = 0;

  fileTab[fileIndex].offset = 0;

  fileTab[fileIndex].flags = flags;

  /* if the file type is JAD file, Installed MIDlet suites  list file,    
   * MIDlet install information file;  MIDlet suites settings file;  
   * or MIDlet application properties file, allocate one 1K RMFS Data Block 
   * for the new created file
   */
  if ((fileType == RMFS_JAD_FILE) || (fileType == RMFS_MIDLET_SUITE_LIST)
      || (fileType == RMFS_MIDLET_DEL_NOTIFY)
      || (fileType == RMFS_MIDLET_SETTING)) {
    dataBlockAddr =
	rmfsFileAlloc(MAX_BLOCKSIZE_4_SPECFILE, filename, NULL, fileType);

    if ((rmfsSearchBlockByPos
	 (&fileTab[fileIndex].dataBlockIndex, dataBlockAddr)) < 0) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
  }

  if (fileType == RMFS_INSTALL_TEMP) {
    dataBlockAddr =
	rmfsFileAlloc(MAX_BLOCKSIZE_4_INSTALL_TEMP, filename, NULL, fileType);

    if ((rmfsSearchBlockByPos
	 (&fileTab[fileIndex].dataBlockIndex, dataBlockAddr)) < 0) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
  }

  if (fileType == RMFS_MIDLET_INSTALL) {
    dataBlockAddr =
	rmfsFileAlloc(MAX_BLOCKSIZE_4_INSTALL_INFO, filename, NULL, fileType);

    if ((rmfsSearchBlockByPos
	 (&fileTab[fileIndex].dataBlockIndex, dataBlockAddr)) < 0) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
  }


  if (fileType == RMFS_MIDLET_PROR) {
    dataBlockAddr =
	rmfsFileAlloc(MAX_BLOCKSIZE_4_MIDLET_PROR, filename, NULL, fileType);

    if ((rmfsSearchBlockByPos
	 (&fileTab[fileIndex].dataBlockIndex, dataBlockAddr)) < 0) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
  }

  if (fileType == RMFS_RMS_RECORD) {
    dataBlockAddr =
	rmfsFileAlloc(MAX_BLOCKSIZE_4_SPECFILE, filename, NULL, fileType);

    if ((rmfsSearchBlockByPos
	 (&fileTab[fileIndex].dataBlockIndex, dataBlockAddr)) < 0) {
      fileTab[fileIndex].handle = -1;
      return -1;
    }
  }
  return fileIndex;

}

/**
 * FUNCTION:      rmfsClose()
 * TYPE:          public operation
 * OVERVIEW:      closes the file with descriptor identifer . 
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *
 *   returns:     >=0 success; < 0 failure
 *                
 */
jint rmfsClose(jint identifier)
{
    char fileType;
  int i;
  signed short index = -1;
  jint nameTabAddr = 0;
  _RmfsNameTabHdrPtr nameTabHdr;
  char *filename = NULL;

#ifdef TRACE_DEBUG
  printf("rmfsClose: identifier: %d \n", identifier);
#endif

  for (i = 0; i <= MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      index = i;
      break;
    }

  }

  if (index < 0) {
    return -1;
  }

  fileType = fileTab[index].fileType;

  /* Update the file size in File Name Table */
  if (((nameTabAddr = searchNameTabByID(&filename, fileTab[index].nameID)) > 0)
      && (filename != NULL)) {
    nameTabHdr = (_RmfsNameTabHdrPtr) nameTabAddr;
    nameTabHdr->size = fileTab[index].size;

    WriteDataToStorage(nameTabHdr, nameTabAddr, sizeof(_RmfsNameTabHdr));


  } else {

    return -1;

  }

  /* if the file type is JAD file, Installed MIDlet suites  list file,    
   * MIDlet install information file;  MIDlet suites settings file;  
   * or MIDlet application properties file, and the file size is less than 
   * the size of the allocated block
   */
  if (((fileType == RMFS_JAD_FILE)
       || (fileType == RMFS_MIDLET_SUITE_LIST)
       || (fileType == RMFS_MIDLET_DEL_NOTIFY)
       || (fileType == RMFS_MIDLET_SETTING))
      && (rmfsDataBlockHdrArray[fileTab[index].dataBlockIndex].size >
	  fileTab[index].size)) {

    rmfsTruncate(identifier, fileTab[index].size);

  }

  fileTab[index].createMode = 0;
  fileTab[index].dataBlockIndex = 0;
  fileTab[index].fileType = 0;
  fileTab[index].flags = 0;
  fileTab[index].handle = -1;
  fileTab[index].nameID = 0;
  fileTab[index].offset = 0;
  fileTab[index].size = 0;
  return 0;
}

/**
 * FUNCTION:      rmfsUnlink()
 * TYPE:          public operation
 * OVERVIEW:      deletes the file named filename from the persistent storage. 
 *    
 * INTERFACE:
 *   parameters:  filename:  file name
 *
 *   returns:     >=0 success; < 0 failure
 *                
 */

jint rmfsUnlink(const char *filename)
{
  uchar nameID = 0;

#ifdef TRACE_DEBUG
  printf("rmfsUnlink: filename: %s \n", filename);
#endif
  if ((searchNameTabByString(filename, &nameID) < 0) || (nameID == 0)) {
    return -1;

  }

  /* Remove the Datablock structures belongs to the file */
  rmfsDelFile(filename);

  /* Remove the right record in NameTab space */
  delNameTabByID(nameID);

  return 0;
}

/**
 * FUNCTION:      rmfsWrite()
 * TYPE:          public operation
 * OVERVIEW:      writes up to size bytes from buffer to the file with descriptor identifier.  
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *                buffer:      data buffer will be written to RMFS storage
 *                size:        the number of bytes intend to be written
 *
 *   returns:     >=0 is the number of bytes actually written; success; < 0 failure
 *                
 */
jint rmfsWrite(jint identifier, void *buffer, jint size)
{
  int i;
  jint filePos = 0;
  jint fileIndex = -1;
  jint blockIndex = -1;
  jint byteNum = 0;
  jint blockOffset = 0;
  jint prevBlockIndex = -1;
  char *filename = NULL;
  jint oldDataSize = 0;
  jint remainSize = 0;
  jint writeSize = 0;

#ifdef TRACE_DEBUG
  printf("rmfsWrite: identifer: %d, size: %d \n", identifier, size);
#endif
  /* Search the File Open Table to find the matched record */

  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      fileIndex = i;
      break;
    }
  }

  if (i >= MAX_OPEN_FILE_NUM) {
    return -1;
  }

  if ((searchNameTabByID(&filename, fileTab[fileIndex].nameID)) < 0) {
    return -1;
  }


  remainSize = size;

  while(remainSize > 0) {
	  blockIndex = -1;
	  buffer = (void*)((int)buffer + writeSize);
	  writeSize = remainSize;
  for (i = prevBlockIndex + 1; i < MAX_DATABLOCK_NUMBER; i++) {
    if (rmfsDataBlockHdrArray[i].identifier == fileTab[fileIndex].nameID) {

      filePos += rmfsDataBlockHdrArray[i].dataSize;

      if ((filePos > fileTab[fileIndex].offset) ||
	  ((rmfsDataBlockHdrArray[i].next == 0) &&
	   (rmfsDataBlockHdrArray[i].size >
	    rmfsDataBlockHdrArray[i].dataSize))) {
	if ((filePos >= fileTab[fileIndex].offset + remainSize) ||
	    (rmfsDataBlockHdrArray[i].size >=
	     fileTab[fileIndex].offset + remainSize)) {
	  /* The write operation happens in this block */
	  blockIndex = i;
	  blockOffset =  fileTab[fileIndex].offset - (filePos -
					   rmfsDataBlockHdrArray[i].dataSize);
	  writeSize = remainSize;
          remainSize = 0;
		  prevBlockIndex = i;
	  break;
	} else {
	  /* The write operation happens in this block */
	  blockIndex = i;
	  blockOffset =  fileTab[fileIndex].offset - (filePos -
					   rmfsDataBlockHdrArray[i].dataSize);
	  writeSize = filePos - fileTab[fileIndex].offset + 
                      rmfsDataBlockHdrArray[i].size - rmfsDataBlockHdrArray[i].dataSize;

          if(writeSize > remainSize ) {
            writeSize = remainSize;
          }  
	  remainSize = remainSize - writeSize;
	  prevBlockIndex = i;
	  break;
	}
      }
      writeSize = remainSize;
      remainSize = 0;
      prevBlockIndex = i;
    }
  }

#ifdef TRACE_DEBUG
  tty->print_cr("offset: %d; filePos: %d, blockIndex: %d, prevBlock: %d \n",
	 fileTab[fileIndex].offset, filePos, blockIndex, prevBlockIndex);
#endif

  if ((blockIndex == -1) && (prevBlockIndex == -1)) {

#if 0
    /* Allocate the first data block for this file */
    if (fileTab[fileIndex].fileType == RMFS_RMS_RECORD) {
      /* If the File Type is the RMS RecordStorage,  the block size 
       * shouldn't be less than the size of RMS header 
       */
      if (size < RMS_DB_HEADER_SIZE) {
	rmfsFileAlloc(RMS_DB_HEADER_SIZE, filename, buffer,
		      fileTab[fileIndex].fileType);
	byteNum = rmfsWriteDataToBlock(blockIndex, buffer, writeSize, 0);

      } else {

	if (rmfsFileAlloc
	    (writeSize, filename, buffer, fileTab[fileIndex].fileType) >= 0) {
	  byteNum = size;
	}
      }
    } else
#endif
    {
      if (rmfsFileAlloc
	  (writeSize, filename, buffer, fileTab[fileIndex].fileType) >= 0) {
	byteNum = writeSize;
      }

    }

    fileTab[fileIndex].size = byteNum;
	remainSize = remainSize - writeSize;
  } else if ((blockIndex == -1) && (prevBlockIndex >= 0)) {
    /* Allocated another Data Block for this file */
    byteNum = rmfsAllocLinkedBlock(prevBlockIndex, buffer, writeSize);
    fileTab[fileIndex].size += byteNum;
	remainSize = remainSize - writeSize;
  } else {

    oldDataSize = rmfsDataBlockHdrArray[blockIndex].dataSize;

    byteNum = rmfsWriteDataToBlock(blockIndex, buffer, writeSize, blockOffset);

    fileTab[fileIndex].size +=
	(rmfsDataBlockHdrArray[blockIndex].dataSize - oldDataSize);

  }

  fileTab[fileIndex].offset += byteNum;
  }

  return byteNum;

}

/**
 * FUNCTION:      rmfsRead()
 * TYPE:          public operation
 * OVERVIEW:      reads up to size bytes from the file with descriptor identifier , 
 *                storing the results in the buffer.   
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *                buffer:      data buffer is used to storage the data read from RMFS storage
 *                size:        the number of bytes intend to be read
 *
 *   returns:     >=0 is the number of bytes actually read; success; < 0 failure
 */               
jint rmfsRead(jint identifier, void *buffer, jint size)
{
  int i;
  jint filePos = 0;
  jint fileIndex = -1;
  jint blockIndex = -1;
  jint readSize = 0;
  jint blockOffset = 0;
  jint nextOffset = 0;
  void *tempPos;

#ifdef TRACE_DEBUG
  printf("rmfsRead: identifer: %d, size: %d \n", identifier, size);
#endif
  /* Search the File Open Table to find the matched record */

  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      fileIndex = i;
      break;
    }
  }

  if (i >= MAX_OPEN_FILE_NUM) {
    return -1;
  }


  /* If the file reach the end, stop and return 0 */
  if (fileTab[fileIndex].offset >= fileTab[fileIndex].size) {
    return 0;
  }

  for (i = 0; i < MAX_DATABLOCK_NUMBER; i++) {
    if (rmfsDataBlockHdrArray[i].identifier == fileTab[fileIndex].nameID) {

      filePos += rmfsDataBlockHdrArray[i].dataSize;

      if (filePos > fileTab[fileIndex].offset) {
	blockOffset =
	    fileTab[fileIndex].offset - (filePos -
					 rmfsDataBlockHdrArray[i].dataSize);

	readSize = (filePos - fileTab[fileIndex].offset);

	if (readSize >= size) {
	  rmfsReadBlock(i, buffer, size, blockOffset);
	} else {
	  rmfsReadBlock(i, buffer, readSize, blockOffset);
	  tempPos = (void *) ((jint) buffer + readSize);
	}

	blockIndex = i;

	while (readSize < size) {
	  nextOffset = rmfsDataBlockHdrArray[blockIndex].next;

	  if ((rmfsSearchBlockByPos
	       ((uchar*) &blockIndex, nextOffset)) < 0) {

	    fileTab[fileIndex].offset += readSize;
	    return readSize;
	  }

	  readSize += rmfsDataBlockHdrArray[blockIndex].dataSize;

	  if (readSize >= size) {
	    rmfsReadBlock(blockIndex, buffer,
			  size - (readSize -
				  rmfsDataBlockHdrArray
				  [blockIndex].dataSize), 0);
	    break;
	  } else {
	    rmfsReadBlock(blockIndex, buffer,
			  rmfsDataBlockHdrArray[blockIndex].dataSize, 0);
	    tempPos = (void *) ((jint) buffer + readSize);
	  }
	}

	fileTab[fileIndex].offset += size;
	return size;
      }

    }
  }

  return 0;
}

/**
 * FUNCTION:      rmfsLseek()
 * TYPE:          public operation
 * OVERVIEW:      change the file position of the file with descriptor identifier 
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *                offset:      new position
 *                whence:     specifies how the offset should be interpreted, 
 *                            and can be one of the symbolic constants PCSL_FILE_SEEK_SET, 
 *                            PCSL_FILE_SEEK_CUR, or PCSL_FILE_SEEK_END. 
 *                            PCSL_FILE_SEEK_SET 
 *                            Specifies that whence is a count of characters from the beginning of the file. 
 *                            PCSL_FILE_SEEK_CUR 
 *                            Specifies that whence is a count of characters from the current file position.
 *                            This count may be positive or negative. 
 *                            PCSL_FILE_SEEK_END 
 *                            Specifies that whence is a count of characters from the end of the file. 
 *                            A negative count specifies a position within the current extent of the file; 
 *                            a positive count specifies a position past the current end. If you set the position
 *                            past the current end, and actually write data, you will extend the file with zeros 
 *                            up to that position
 *
 *   returns:     >=0  success; < 0 failure
 *                
 */
jint rmfsLseek(jint identifier, jint offset, jint whence)
{
  int i;
  int fileIndex = 0;
  int absoluteOff = 0;

#ifdef TRACE_DEBUG
  printf("rmfsLseek: identifer: %d, whence: %d \n", identifier, whence);
#endif
  /* check the input parameter */
  if ((offset < 0) && (whence == PCSL_FILE_SEEK_SET)) {
    return -1;
  }


  /* Search the File Open Table to find the matched record */

  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      fileIndex = i;
      break;
    }
  }

  if (i >= MAX_OPEN_FILE_NUM) {
    return -1;
  }

  if (whence == PCSL_FILE_SEEK_SET) {
    absoluteOff = offset;
  } else if (whence == PCSL_FILE_SEEK_CUR) {
    absoluteOff = offset + fileTab[i].offset;
  } else if (whence == PCSL_FILE_SEEK_END) {
    absoluteOff = offset + fileTab[i].size;
  }

  fileTab[fileIndex].offset = absoluteOff;

  return absoluteOff;

}

/**
 * FUNCTION:      rmfsFileSize()
 * TYPE:          public operation
 * OVERVIEW:      quiry the size of the file. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *
 *
 *   returns:     >=0 file size; success; < 0 failure
 *                
 */
jint rmfsFileSize(jint identifier)
{
    int i;
    int fileIndex = 0;

  /* Search the File Open Table to find the matched record */

#ifdef TRACE_DEBUG
  printf("rmfsFileSize: identifer: %d \n", identifier);
#endif
  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      fileIndex = i;
      break;
    }
  }

  if (i >= MAX_OPEN_FILE_NUM) {
    return -1;
  }

  return fileTab[fileIndex].size;
}

/**
 * FUNCTION:      rmfsTruncate()
 * TYPE:          public operation
 * OVERVIEW:      truncate the size of an open file in storage. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *                size:        new file size 
 *
 *
 *   returns:     >=0 file size; success; < 0 failure
 *                
 */
jint rmfsTruncate(jint identifier, jint size)
{
  int i;
  int fileIndex = 0;
  short nameID;
  jint fileSize = 0;
  RMFS_BOOLEAN freeSucceedBlock = RMFS_FALSE;

  /* Search the File Open Table to find the matched record */

#ifdef TRACE_DEBUG
  printf("rmfsTruncate: identifer: %d size: %d\n", identifier, size);
#endif
  for (i = 0; i < MAX_OPEN_FILE_NUM; i++) {
    if (fileTab[i].handle == identifier) {
      fileIndex = i;
      break;
    }
  }

  if (i >= MAX_OPEN_FILE_NUM) {
    return -1;
  }

  nameID = fileTab[fileIndex].nameID;

  for (i = 0; i < MAX_DATABLOCK_NUMBER; i++) {
    if (rmfsDataBlockHdrArray[i].identifier == nameID) {

      if (freeSucceedBlock == RMFS_TRUE) {
	rmfsFreeDataBlock(i);
	continue;
      }

      fileSize += rmfsDataBlockHdrArray[i].dataSize;

      if (((unsigned int)(fileSize - size) > sizeof(_RmfsBlockHdr)) ||
	  ((rmfsDataBlockHdrArray[i].size -
	    (unsigned int)rmfsDataBlockHdrArray[i].dataSize) >= sizeof(_RmfsBlockHdr))) {
	rmfsSplitBlock(i,
		       size - (fileSize - rmfsDataBlockHdrArray[i].dataSize));
      }

      if (fileSize >= size) {
	freeSucceedBlock = RMFS_TRUE;
      }
    }
  }
  return 0;
}

/**
 * FUNCTION:      rmfsFileExist()
 * TYPE:          public operation
 * OVERVIEW:      check if the file exist in RMFS storage. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  filename:  file nmae
 *                status:    indicator if the file exists. 
 *
 *
 *   returns:    none
 *                
 */
void rmfsFileExist(const char *filename, jint *status)
{
  uchar nameID = 0;

#ifdef TRACE_DEBUG
  printf("rmfsFileExist: filename: %s \n", filename);
#endif
  if (((searchNameTabByString(filename, &nameID)) < 0) || (nameID == 0)) {

    *status = 0;
    return;
  } else if (nameID > 0) {
    *status = 1;
    return;
  } else {
    *status = -1;
    return;
  }

  return;

}

/**
 * FUNCTION:      rmfsFileExist()
 * TYPE:          public operation
 * OVERVIEW:      check if the DIR exist in RMFS storage. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  filename:  file nmae
 *                status:    indicator if the file exists. 
 *
 *
 *   returns:    none
 *                
 */
void rmfsDirExist(char *dirname, jint *status)
{
  uchar nameID = 0;

#ifdef TRACE_DEBUG
  printf("rmfsDirExist: dirname: %s \n", dirname);
#endif
  if (((searchNameTabByString(dirname, &nameID)) < 0) || (nameID == 0)) {

    *status = 0;
    return;
  } else if (nameID > 0) {
    *status = 1;
    return;
  } else {
    *status = -1;
    return;
  }

  return;

}

/**
 * FUNCTION:      rmfsOpendir()
 * TYPE:          public operation
 * OVERVIEW:      opens and returns a directory stream for reading the directory whose file name is dirname. 
 *                Note: This function isn't ready in this phase. A workaround is used to meet AMS requirements.  
 *    
 * INTERFACE:
 *   parameters:  dirname:  DIR name
 *
 *   returns:     DIR  DIR stream.
 *                
 */
DIR* rmfsOpendir(const char *dirname)
{
#ifdef TRACE_DEBUG
    printf("rmfsOpendir: dirname: %s \n", dirname);
#endif
  return (DIR*) dirname;
}


/**
 * FUNCTION:      rmfsClosedir()
 * TYPE:          public operation
 * OVERVIEW:      closes the directory stream dirstream. 
 *                Note: This function isn't ready in this phase. A workaround is used to meet AMS requirements.
 *                  
 *    
 * INTERFACE:
 *   parameters:  dirname:  DIR name
 
 *
 *   returns:     >=0  success; < 0 failure
 *                
 */
jint rmfsClosedir(char *dirname)
{
#ifdef TRACE_DEBUG
  printf("rmfsClosedir: dirname: %s \n", dirname);
#endif
  return 0;
}

/**
 * FUNCTION:      rmfsRename()
 * TYPE:          public operation
 * OVERVIEW:      updates the filename from old filename to new file name. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  oldFilename:  old file name
 *                newFilename:  new file name 
 *
 *
 *   returns:     >=0 file size; success; < 0 failure
 *                
 */
jint rmfsRename(const char *oldFilename, const char *newFilename)
{
  uchar nameID = 0;
  uchar newFileID;
  jint fileSize;
  jint nameTabAddr;
  _RmfsNameTabHdr nameRecord;

#ifdef TRACE_DEBUG
  printf("rmfsRename: oldFilename: %s newFilename: %s\n", oldFilename,
	 newFilename);
#endif

  /* check for null and empty strings */
  if(newFilename == NULL) {
      return -1;
  };

  if(strlen(newFilename) == 0) {
      return -1;
  };

  if (((nameTabAddr = searchNameTabByString(oldFilename, &nameID)) < 0)
      || (nameID == 0)) {

    return -1;
  } else {

    ReadDataFromStorage(&nameRecord, nameTabAddr, sizeof(_RmfsNameTabHdr));

    if((newFileID = rmfsNewNameTable(newFilename)) == 0) {
        return -1;
    };

    if (((nameTabAddr = searchNameTabByString(newFilename, &nameID)) < 0)
	|| (nameID == 0)) {

      return -1;
    } else {

      nameID = nameRecord.identifier;
      fileSize = nameRecord.size;

      ReadDataFromStorage(&nameRecord, nameTabAddr, sizeof(_RmfsNameTabHdr));

      nameRecord.identifier = nameID;

      nameRecord.size = fileSize;

      WriteDataToStorage(&nameRecord, nameTabAddr, sizeof(_RmfsNameTabHdr));

      delNameTabByString(oldFilename);

    }

  }

  return 0;

}

/**
 * FUNCTION:      rmfsFileEof()
 * TYPE:          public operation
 * OVERVIEW:      Check if the file reaches the end. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  identifier:  file descriptor
 *
 *   returns:     >=0 file size; success; < 0 failure
 *                
 */
jint rmfsFileEof(jint handle)
{
  if (fileTab[handle].size == fileTab[handle].offset) {
    return 1;
  } else {
    return 0;
  }
}

/**
 * FUNCTION:      rmfsFileStartWith()
 * TYPE:          public operation
 * OVERVIEW:      search the file whose filename starts with input string. 
 *                  
 *    
 * INTERFACE:
 *   parameters:  filename: input string 
 *
 *
 *   returns:     == NULL no matched file; otherwise: Full filename of the matched item. 
 *                
 */
char *rmfsFileStartWith(const char *filename)
{

  uchar nameID;
  char *fullFilename = NULL;
  jint nameAddress;

#ifdef TRACE_DEBUG
  printf("rmfsFileStartWith: filename: %s \n", filename);
#endif

  nameAddress = searchNameTabStartWith(filename, &nameID);

  if ((nameAddress > 0) && (nameID > 0)) {

    fullFilename = (char *) (nameAddress + sizeof(_RmfsNameTabHdr));

#ifdef TRACE_DEBUG
    printf("Return Full Filename %s \n", fullFilename);
#endif
    return fullFilename;
  } else {
    return NULL;
  }

}
/**
 * FUNCTION:      rmfsGetUsedSpace()
 * TYPE:          public operation
 * OVERVIEW:      Get the used space size in RMFS
 * INTERFACE:
 *   None   
 *
 *   returns:     the byte numer was used
 *                
 */
int rmfsGetUsedSpace()
{

  return getUsedSpace(); 


}


/**
 * FUNCTION:      rmfsGetFreeSpace()
 * TYPE:          public operation
 * OVERVIEW:      Get the free space size in RMFS
 * INTERFACE:
 *   None   
 *
 *   returns:     the byte numer was used
 *                
 */
int rmfsGetFreeSpace()
{

  return getFreeSpace();


}


/**
 * FUNCTION:      rmfsMkdir()
 * TYPE:          public operation
 * OVERVIEW:      create a Dir in the rmfs File System
 * INTERFACE:
 *   None   
 *
 *   returns:    >=0 Success, -1 failure
 *                
 */
int rmfsMkdir()
{

  return 0;


}

/**
 * FUNCTION:      rmfsRmdir()
 * TYPE:          public operation
 * OVERVIEW:      delete a existed Dir in the rmfs File System
 * INTERFACE:
 *   None   
 *
 *   returns:    >=0 Success, -1 failure
 *                
 */
int rmfsRmdir()
{

  return 0;


}

#ifdef __cplusplus
}
#endif
