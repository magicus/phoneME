/*
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

#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "nams.h"
#include "javacall_file.h"
#include "javacall_logging.h"
/* #include "javacall_file.h" */
#include "javautil_jad_parser.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_ams_app_manager.h"
#include "javacall_ams_installer.h"

static char NAMS_DB_FILE[]=".\\nams.db";
static char NAMS_DB_HOME[]=".\\";

int InstalledSuite[MAX_SUITE];
int installed_suite_count=0;

javacall_result nams_db_init()
{
    int suiteID;
    javacall_handle dbHandle;
    long fileSize;
    char* dbBuf;
    char* line;
    int lineSize;
    char* startAddr;

    if (nams_db_open(JAVACALL_FILE_O_CREAT | JAVACALL_FILE_O_RDONLY, 
                     &dbHandle) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    if (nams_db_read_file(&dbBuf, &fileSize) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    startAddr = dbBuf;
    while (nams_db_read_line(&dbBuf, &line, &lineSize) != JAVACALL_END_OF_FILE)
    {
        if (nams_db_get_suiteid(line, &suiteID) != JAVACALL_OK)
        {
            javacall_free(line);
            continue;
        }
        InstalledSuite[suiteID] = 1;
        installed_suite_count ++;
        javacall_free(line);
    }

    javacall_free(startAddr);

    return JAVACALL_OK;
}

javacall_result nams_db_allocate_suiteid(javacall_suite_id* suiteID)
{
    int i;
    for (i = 1; i < MAX_SUITE; i ++)
    {
        if (!InstalledSuite[i])
        {
            break;
        }
    }
    if (i >= MAX_SUITE)
    {
        return JAVACALL_FAIL;
    }
    *suiteID = i;
    InstalledSuite[i] = 1;

    return JAVACALL_OK;
}

javacall_result nams_db_get_suiteid(char* textLine, javacall_suite_id* suiteID)
{
    int res;
    int index;
    char *strSuiteID;
    int id;

    // find the index of ':'
    res = javautil_string_index_of(textLine, ':', &index);
    if ((res != JAVACALL_OK) || (index <= 0)) 
    {
        return res;
    }

    // get sub string of suite id
    res = javautil_string_substring(textLine, 0, index, &strSuiteID);
    if (res != JAVACALL_OK || strSuiteID == NULL) 
    {
        return res;
    }
    res = javautil_string_trim(strSuiteID);
    if (res != JAVACALL_OK) 
    {
        return res;
    }
    id = atoi(strSuiteID);
    if (id <= 0)
    {
        return JAVACALL_FAIL;
    }
    *suiteID = id;
    javacall_free(strSuiteID);
    return JAVACALL_OK;
}

javacall_result nams_db_get_suitepath(char* textLine, char* outText)
{
    int res;
    int index;
    char *newString;

    // find the index of ':'
    res = javautil_string_index_of(textLine, ':', &index);
    if ((res != JAVACALL_OK) || (index <= 0)) 
    {
        return res;
    }

    // skip white space between jad property name and value
    while (*(textLine+index+1) == SPACE) 
    {
        index++;
    }

    // get sub string of suite path
    res = javautil_string_substring(textLine, index+1, strlen(textLine), &newString);
    if (res != JAVACALL_OK || newString == NULL) 
    {
        return res;
    }
    strcpy(outText, newString);

    javacall_free(newString);

    return JAVACALL_OK;
}

javacall_result nams_db_open(int flags, javacall_handle* handle)
{
    int fd;
    int creationMode=0;
    int oFlag=O_BINARY;

    /* compute open control flag */
    if ((flags & JAVACALL_FILE_O_WRONLY) == JAVACALL_FILE_O_WRONLY) {
        oFlag |= O_WRONLY;
    }
    if ((flags & JAVACALL_FILE_O_RDWR) == JAVACALL_FILE_O_RDWR) {
        oFlag |= O_RDWR;
    }
    if ((flags & JAVACALL_FILE_O_CREAT) == JAVACALL_FILE_O_CREAT) {
        oFlag |= O_CREAT;
        creationMode = _S_IREAD | _S_IWRITE;
    }
    if ((flags & JAVACALL_FILE_O_TRUNC) == JAVACALL_FILE_O_TRUNC) {
        oFlag |= O_TRUNC;
    }
    if ((flags & JAVACALL_FILE_O_APPEND) == JAVACALL_FILE_O_APPEND) {
        oFlag |= O_APPEND;
    }

    if ((fd = _open(NAMS_DB_FILE, oFlag, creationMode)) == -1)
    {
        javacall_print("[NAMS] Failed to open application db file");
        return JAVACALL_FAIL;
    }
    *handle = (void *)fd;
    return JAVACALL_OK;
}

javacall_result nams_db_close(javacall_handle handle)
{
    if (_close((int)handle) != 0)
    {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

javacall_result nams_db_get_app_list(contentList* appList, int* entryCount)
{
    char* dbBuf;
    char* startAddress;
    char* line;
    int lineSize;
    int fileSize;

    if (nams_db_read_file(&dbBuf, &fileSize) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    startAddress = dbBuf;
    (*entryCount) = 0;

    while (nams_db_read_line(&dbBuf, &line, &lineSize) != JAVACALL_END_OF_FILE)
    {
        nams_content_list_add(appList, line);
        (*entryCount) ++;
        javacall_free(line);
    }

    javacall_free(startAddress);

    return JAVACALL_OK;
}

javacall_result nams_db_read_file(char** destBuffer, long* size)
{
    javacall_handle dbHandle;
    long fileSize;
    char* fileBuffer;
    long bytesRead;

    if (nams_db_open(JAVACALL_FILE_O_RDONLY, &dbHandle) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }

    // get the open db file size
    fileSize = (long)javacall_file_sizeofopenfile(dbHandle);
    if (-1L == fileSize) 
    {
        nams_db_close(dbHandle);
        return JAVACALL_IO_ERROR;
    }

    // allocate a buffer to hold the db file contents
    fileBuffer = (char*)javacall_malloc(fileSize+1);
    if (fileBuffer == NULL) 
    {
        nams_db_close(dbHandle);
        return JAVACALL_OUT_OF_MEMORY;
    }
    memset(fileBuffer, 0, (fileSize+1));

    bytesRead = javacall_file_read(dbHandle, (unsigned char*)fileBuffer, fileSize);
    if ((bytesRead <= 0) || (bytesRead != fileSize)) 
    {
        javacall_free(fileBuffer);
        nams_db_close(dbHandle);
        return JAVACALL_IO_ERROR;
    }
    nams_db_close(dbHandle);

    *destBuffer = fileBuffer;
    *size = fileSize;
    return JAVACALL_OK;

}

javacall_result nams_db_read_line(char** dbBuf, char** textLine, int* lineSize)
{
    char* pBuf = *dbBuf;
    char* lineStart;
    int charCount=0;
    char* line;

    *textLine = NULL;
    *lineSize = -1;

    if (!(*pBuf)) 
    {
        return JAVACALL_END_OF_FILE;
    }

    /* skip commented out and blank lines */
    while ((*pBuf == POUND) || (javautil_is_new_line(pBuf))) {

        while (!javautil_is_new_line(pBuf)) {
            pBuf++; // skip commented out line
        }
        while (javautil_is_new_line(pBuf)) {
            pBuf++; // skip blank lines
        }
    }

    lineStart = pBuf;

    while (*pBuf) 
    {
        charCount++;
        pBuf++;

        /* reached the end of the line */
        if (javautil_is_new_line(pBuf)) 
        {
            /* if not end of file, point to the next jad file line */
            if (*(pBuf+1)) 
            {
                pBuf++;
                break;
            }
        }
    }

    *dbBuf = pBuf; // points to the next line

    line = (char*)javacall_malloc(charCount + 1);
    if (line == NULL) 
    {
        return JAVACALL_OUT_OF_MEMORY;
    }
    memset(line, 0, charCount + 1);
    memcpy(line, lineStart, charCount);

    *textLine = line;
    *lineSize = charCount;

    return JAVACALL_OK;
}

javacall_result nams_db_remove_line(char** dbBuf, int lineNum, long* outSize, int* outID)
{
    int lineCount=-1; // lineNum start from 0
    int res=0;
    char* textLine;
    int lineSize;
    char* removeStart;
    char* removeEnd;
    char* startAddr;

    startAddr = *dbBuf;
    while (lineCount < lineNum)
    {
        removeStart = *dbBuf;
        res = nams_db_read_line(dbBuf, &textLine, &lineSize);
        if (res == JAVACALL_END_OF_FILE)
        {
            return JAVACALL_FAIL; // did not find
        }
        if (++lineCount < lineNum)
        {
            javacall_free(textLine);
        }
    }
    nams_db_get_suiteid(textLine, outID);

    removeEnd = *dbBuf;
    while (*removeEnd == CR || *removeEnd == LF)
    {
        removeEnd++;
    }
    while (*removeStart == CR || *removeStart == LF)
    {
        removeStart++;
    }
    while (*removeEnd)
    {
        *(removeStart++) = *(removeEnd++);
    }
    *outSize = (long)(removeStart - startAddr);

    return JAVACALL_OK;
}

javacall_result nams_db_remove_suite_home(int removeID)
{
    char dbFiles[JAVACALL_MAX_FILE_NAME_LENGTH];
    char dbHome[JAVACALL_MAX_FILE_NAME_LENGTH];
    HANDLE hSearch;
    WIN32_FIND_DATA fd;
    char strID[10];
    char fileToBeDeleted[JAVACALL_MAX_FILE_NAME_LENGTH]="NotExistFile";

    strcpy(dbFiles, NAMS_DB_HOME);
    strcat(dbFiles, "suite-");
    _itoa(removeID, strID, 10);
    strcat(dbFiles, strID);
    strcpy(dbHome, dbFiles);
    strcat(dbFiles, "\\*");

    //TODO : Remove recursively
    hSearch = FindFirstFile(dbFiles, &fd);
    while ( FindNextFile(hSearch, &fd) )
    {
        DeleteFile(fileToBeDeleted);
        memset(fileToBeDeleted, 0, JAVACALL_MAX_FILE_NAME_LENGTH);
        if (javautil_string_equals(fd.cFileName, ".") ||
            javautil_string_equals(fd.cFileName, "..") )
        {
            continue;
        }
        strcpy(fileToBeDeleted, dbHome);
        strcat(fileToBeDeleted, "\\");
        strcat(fileToBeDeleted, fd.cFileName);
    }
    DeleteFile(fileToBeDeleted);
    FindClose(hSearch);

    RemoveDirectory(dbHome);

    return JAVACALL_OK;

}

javacall_result nams_db_get_suite_home(javacall_suite_id suiteID, 
                                                  javacall_utf16* outPath, int* pathLen)
{
    char dirName[JAVACALL_MAX_FILE_NAME_LENGTH*2];
    javacall_utf16* uDirName;
    char strID[10];

    GetCurrentDirectory(JAVACALL_MAX_FILE_NAME_LENGTH, dirName);

    strcat(dirName, "\\suite-");
    _itoa(suiteID, strID, 10);
    strcat(dirName, strID);

    // add back slash
    strcat(dirName, "\\");

    if (nams_string_to_utf16(dirName, strlen(dirName), &uDirName, strlen(dirName)) 
        != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }

    memcpy(outPath, uDirName, (strlen(dirName)+1)*sizeof(javacall_utf16));
    *pathLen = (strlen(dirName)+1)*sizeof(javacall_utf16);

    javacall_free(uDirName);

    return JAVACALL_OK;
}

javacall_result nams_db_get_root(javacall_utf16* outPath, int* pathLen)
{
    char dirName[JAVACALL_MAX_FILE_NAME_LENGTH*2];
    javacall_utf16* uDirName;

    GetCurrentDirectory(JAVACALL_MAX_FILE_NAME_LENGTH, dirName);

    // add back slash
    strcat(dirName, "\\");

    if (nams_string_to_utf16(dirName, strlen(dirName), &uDirName, strlen(dirName))
         != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }

    memcpy(outPath, uDirName, (strlen(dirName)+1)*sizeof(javacall_utf16));
    *pathLen = (strlen(dirName)+1)*sizeof(javacall_utf16);

    javacall_free(uDirName);

    return JAVACALL_OK;
}

javacall_result nams_db_install_app(char* textLine, javacall_utf16_string jarName)
{
    javacall_handle dbHandle;
    char newLine[2];
    int suiteID;
    char strSuiteID[10];
    char dbHome[JAVACALL_MAX_FILE_NAME_LENGTH];

    if (nams_db_open(JAVACALL_FILE_O_APPEND | JAVACALL_FILE_O_RDWR, 
                     &dbHandle) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    nams_db_allocate_suiteid(&suiteID);
    _itoa(suiteID, strSuiteID, 10);

    strcpy(dbHome, NAMS_DB_HOME);
    strcat(dbHome, "suite-");
    strcat(dbHome, strSuiteID);

    strcat(strSuiteID, " : ");
    javacall_file_write(dbHandle, strSuiteID, strlen(strSuiteID));

    javacall_file_write(dbHandle, textLine, strlen(textLine));

    newLine[0] = CR;
    newLine[1] = LF;
    javacall_file_write(dbHandle, newLine, 2);

    nams_db_close(dbHandle);

    if (CreateDirectory(dbHome, NULL) == 0)
    {
        javacall_print("[NAMS] Make DB home error!\n");
    }

    javanotify_ams_create_resource_cache(suiteID);

    installed_suite_count++;

    return JAVACALL_OK;
}

javacall_result nams_db_remove_app(int itemIndex)
{
    javacall_handle dbHandle;
    char* dbBuf;
    char* startAddr;
    long fileSize;
    long newBufSize;
    int removeID;

    if (nams_db_open(JAVACALL_FILE_O_RDONLY, &dbHandle) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }

    if (nams_db_read_file(&dbBuf, &fileSize) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    nams_db_close(dbHandle);

    startAddr = dbBuf;
    nams_db_remove_line(&dbBuf, itemIndex, &newBufSize, &removeID);

    if (nams_db_open(JAVACALL_FILE_O_TRUNC | JAVACALL_FILE_O_WRONLY, &dbHandle) != JAVACALL_OK)
    {
        return JAVACALL_FAIL;
    }
    if (newBufSize > 0)
    {
        // write back to db file
        javacall_file_write(dbHandle, startAddr, newBufSize);
    }
    nams_db_close(dbHandle);

    javacall_free(startAddr);

    if (nams_db_remove_suite_home(removeID) != JAVACALL_OK)
    {
        javacall_print("[NAMS] Remove DB home error!\n");
    }

    InstalledSuite[removeID] = 0;
    installed_suite_count--;

    return JAVACALL_OK;
}
