/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#include <stdio.h>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <wchar.h>
#include <stdlib.h>
#include <errno.h>
#include "javacall_location.h"
#include "javacall_landmarkstore.h"

/** File separator */
#define FILESEP '\\'
#define MAX_FILE_NAME_LENGTH            450
#define MAX_FILE_NAME_LENGTH_WITH_PATH  512
#define MAX_ADDRESSINFO_FIELDS          17

/* size of landmarkStore and Category fields: */
#define SIZE_OF_NUMLANDMARKS    (long)sizeof(long)
#define SIZE_OF_NUMCATEGORIES   (long)sizeof(long)
#define SIZE_OF_LANDMARKID      (long)sizeof(long)
#define SIZE_OF_NAMELEN         (long)sizeof(long)
#define SIZE_OF_NAME(len)       ((len)*(long)sizeof(javacall_utf16))

/** A directory for saving LandmarkStores */
static const javacall_utf16 LANDMARK_STORE_DIR[] = 
    {'l','a','n','d','m','a','r','k','s','t','o','r','e','s',0};

static boolean landmarkStoreDirExist = FALSE;
/** Landmark Store file extension*/
static const javacall_utf16 FEXT_LANDMARKSTORE[] =
    {'.','l','m','s',0};
/** File extension of the file containing List of Categories */
static const javacall_utf16 FEXT_CATEGORY[] = 
    {'.','c','t','g',0};

/** Structure to support LandmarkStore List Next operation */
typedef struct _storeListType{
    javacall_handle handle;
    boolean first;
    int len;
    WIN32_FIND_DATAW dir_data;
    struct _storeListType *next;
}storeListType;

storeListType* storeListHead = NULL;

/** Structure to support Category List Next operation */
typedef struct _categoryListType{
    javacall_handle handle;
    int numCategories;
    javacall_utf16_string* categoryNames;
    int curCategory;
    struct _categoryListType *next;
}categoryListType;

categoryListType* categoryListHead = NULL;

/** Structure to support Landmark List Next operation */
typedef struct _landmarkListType{
    javacall_handle handle; /* descriptor */
    javacall_utf16_string storeName; /* LandmarkStore Name */
    javacall_utf16_string categoryName; /* Category Name or NULL */
    long numLandmarkIDs;
    long *landmarkIDs; /* ID of specified Landmarks if Category Name specified */
    long currentID;
    javacall_landmarkstore_landmark *curLandmark;
    struct _landmarkListType *next;
}landmarkListType;

landmarkListType* landmarkListHead = NULL;

/**
 * Internal function :
 * Opens store file (landmark or category store).
 */
javacall_result openStore(const javacall_utf16_string, const javacall_utf16_string, boolean, int*, long *);

/**
 * Adds a landmark to a landmark store.
 *
 * @param landmarkStoreName where the landmark will be added
 * @param landmark to add
 * @param categoryName where the landmark will belong to, NULL implies that the landmark does not belong to any category.
 * @param outLandmarkID returned id of added landmark
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid or the landmark has a longer name field than the implementation can support.
 */
javacall_result javacall_landmarkstore_landmark_add_to_landmarkstore(
        const javacall_utf16_string landmarkStoreName, 
        const javacall_landmarkstore_landmark* landmark,
        const javacall_utf16_string categoryName,
        javacall_handle* /*OUT*/outLandmarkID) {

    int fd;
    long fsize = 0;
    long pos;
    long numLandmarks;
    int i;
    javacall_landmarkstore_landmark *landmarkBuffer;
    long landmarkID = 0;
    javacall_result ret;

    /* Create name of LandmarkStore List file */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_LANDMARKSTORE, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* allocate and fill landmark data for Store*/
    landmarkBuffer = malloc(SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS));
    memset(landmarkBuffer, 0, SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS));
    memcpy(landmarkBuffer, landmark, SIZE_OF_LANDMARK_INFO(landmark->addressInfoFieldNumber));

    /* try to find gap (empty ID) in the store */
    ret = JAVACALL_OK;
    if (_read(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) != SIZE_OF_NUMLANDMARKS) {
        ret = JAVACALL_FAIL;
    } else {
        pos = SIZE_OF_NUMLANDMARKS;
        for (i=0; (i < numLandmarks) && (ret == JAVACALL_OK); i++) {
            if (_read(fd, &landmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
                ret = JAVACALL_FAIL;
                break;
            }
            if (landmarkID == 0) {
                /* free landmark ID */
                landmarkID = i+1;
                if (_lseek(fd, pos, SEEK_SET) != pos) {
                    ret = JAVACALL_FAIL;
                }
                break;
            }
            pos += SIZE_OF_LANDMARKID + SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS);
            if (_lseek(fd, pos, SEEK_SET) != pos) {
                ret = JAVACALL_FAIL;
                break;
            }
            landmarkID = 0;
        }
        if ((landmarkID == 0) && (ret == JAVACALL_OK)){
            numLandmarks++;
            landmarkID = numLandmarks;
            if (_lseek(fd, 0, SEEK_SET) != 0) {
                ret = JAVACALL_FAIL;
            }
            if ((ret == JAVACALL_OK) && 
                (_write(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) != SIZE_OF_NUMLANDMARKS)) {
                ret = JAVACALL_FAIL;
            }
            if ((ret == JAVACALL_OK) && (_lseek(fd, 0, SEEK_END) != fsize)) {
                ret = JAVACALL_FAIL;
            }
        }

        if ((ret == JAVACALL_OK) && 
            (_write(fd, &landmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID)) {
            ret = JAVACALL_FAIL;
        }
        if ((ret == JAVACALL_OK) && 
            (_write(fd, landmarkBuffer, SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS)) != 
                                        SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS))) {
            ret = JAVACALL_FAIL;
        }
    }
    _close(fd);

    free(landmarkBuffer);

    /* save landmarkID */
    *outLandmarkID = (javacall_handle)landmarkID;

    if ((ret == JAVACALL_OK) && (categoryName != NULL)) {
        ret = javacall_landmarkstore_landmark_add_to_category(landmarkStoreName, (javacall_handle)landmarkID, categoryName);
        if (ret == JAVACALL_INVALID_ARGUMENT) {
            javacall_landmarkstore_landmark_delete_from_landmarkstore(landmarkStoreName, (javacall_handle)landmarkID);
        }
    }
    
    return ret;
}

/**
 * Adds a landmark to a category.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID landmark id to add 
 * @param categoryName which the landmark will be added to
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid
 */
javacall_result javacall_landmarkstore_landmark_add_to_category(
        const javacall_utf16_string landmarkStoreName, 
        javacall_handle landmarkID,
        const javacall_utf16_string categoryName) {

    int wCategoryNameLen=0;
    char *buffer;
    int fd;
    long fsize, pos;
    long num_categories, categoryNameLen;
    long localLandmarkID = (long)landmarkID;
    long inListLandmarkID;
    long numLandmarks;
    long i,j;
    boolean found = FALSE;
    javacall_result ret;

    /* Calculate len of categoryName */
    wCategoryNameLen = wcslen(categoryName);

    /* Create name of LandmarkStore List file */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* read file into the buffer */
    if (_read(fd, buffer, fsize) != fsize) {
        free(buffer);
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* parse category file */
    ret = JAVACALL_OK;
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;
    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && (pos + SIZE_OF_NUMCATEGORIES)<fsize; i++) {
        /* read category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN;
        /* check if category Names equals */
        if ((wCategoryNameLen == categoryNameLen) && 
            ((pos + SIZE_OF_NAME(wCategoryNameLen))<fsize)) {
            if (memcmp(&buffer[pos], &categoryName[0], SIZE_OF_NAME(wCategoryNameLen)) == 0) {
                found = TRUE;
                /* we found it - add and save */
                pos += SIZE_OF_NAME(categoryNameLen);
                if (_lseek(fd, pos, SEEK_SET) != pos) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                numLandmarks = *(long *)&buffer[pos];
                /* verify if the landmark is not in the category */
                pos += SIZE_OF_NUMLANDMARKS;
                inListLandmarkID = -localLandmarkID;
                for (j=0; j<numLandmarks; j++) {
                    inListLandmarkID = *(long *)&buffer[pos];
                    if (inListLandmarkID == localLandmarkID) {
                        /* landmark already in the category */
                        break;
                    }
                    pos += SIZE_OF_LANDMARKID;
                }
                if (inListLandmarkID == localLandmarkID) {
                    /* landmark already in the category - exit with JAVACALL_OK*/
                    break;
                }
                numLandmarks++;
                /* update num landmarks */
                if (_write(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) 
                    != SIZE_OF_NUMLANDMARKS) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                /* add new landmarkID */
                if (_lseek(fd, pos, SEEK_SET) != pos) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                if (_write(fd, &localLandmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                /* add the rest of file */
                if (_write(fd, &buffer[pos], fsize - pos) != (fsize - pos)) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                break;
            }
        }
        pos += SIZE_OF_NAME(categoryNameLen);
        numLandmarks = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
    }
    
    if (ret == JAVACALL_FAIL) {
        /* file broken - need to restore */
        _lseek(fd, 0, SEEK_SET);
        _write(fd, buffer, fsize);
        _chsize(fd, fsize);
    }

    _close(fd);
    free(buffer);

    if ((ret == JAVACALL_OK) && (!found)) {
        return JAVACALL_INVALID_ARGUMENT;
    }
    return ret;
}

/**
 * Update existing landmark in the landmark store.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID landmark id to update 
 * @param landmark to update
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the landmarkID is invalid
 */
javacall_result javacall_landmarkstore_landmark_update(
        const javacall_utf16_string landmarkStoreName, 
        javacall_handle landmarkID,
        const javacall_landmarkstore_landmark* landmark) {

    int fd;
    long fsize = 0;
    long pos;
    long numLandmarks;
    long localLandmarkID = (long)landmarkID;
    javacall_result ret;

    /* Create name of LandmarkStore List file */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_LANDMARKSTORE, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* read & check landmarkID */
    if (_read(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) != SIZE_OF_NUMLANDMARKS) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    if (numLandmarks < localLandmarkID) {
        _close(fd);
        return JAVACALL_INVALID_ARGUMENT;
    }

    pos = SIZE_OF_NUMLANDMARKS + (localLandmarkID-1) * (SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS) + SIZE_OF_LANDMARKID);
    if (_lseek(fd, pos, SEEK_SET) != pos) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    if (_read(fd, &localLandmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    if (localLandmarkID == 0) {
        /* landmark is empty */
        _close(fd);
        return JAVACALL_INVALID_ARGUMENT;
    }

    if (_write(fd, landmark, SIZE_OF_LANDMARK_INFO(landmark->addressInfoFieldNumber)) != 
                        (long)SIZE_OF_LANDMARK_INFO(landmark->addressInfoFieldNumber)) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    _close(fd);
    return JAVACALL_OK;
}

/**
 * Deletes a landmark from a landmark store.
 *
 * This function removes the specified landmark from all categories and deletes the information from this landmark store.
 * If the specified landmark does not belong to this landmark store, then the request is silently ignored and this function call returns without error.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID        id of a landmark to delete
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 */
javacall_result javacall_landmarkstore_landmark_delete_from_landmarkstore(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle landmarkID) {

    char *buffer;
    long fsize = 0, newFsize;
    long pos, subpos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    int fd;
    int i,j;
    long localLandmarkID = (long)landmarkID, inListLandmarkID;
    javacall_result ret = JAVACALL_OK;

    /* Delete landmark ID from the categories */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* read file into the buffer */
    if (_read(fd, buffer, fsize) != fsize) {
        free(buffer);
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* parse it */
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;
    newFsize = fsize;
    localLandmarkID = (long)landmarkID;
    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && (pos+SIZE_OF_NAMELEN<fsize); i++) {
        /* skip category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN + SIZE_OF_NAME(categoryNameLen);
        if ((pos+SIZE_OF_NUMLANDMARKS) > newFsize) {
            /* out of buffer */
            ret = JAVACALL_FAIL;
            break;
        }
        numLandmarks = *(long *)&buffer[pos];
        subpos = pos + SIZE_OF_NUMLANDMARKS;
        if ((subpos+(numLandmarks*SIZE_OF_LANDMARKID)) > newFsize) {
            /* out of buffer */
            ret = JAVACALL_FAIL;
            break;
        }
        /* parse category - try to find landmarkID */
        for (j=0 ; j<numLandmarks; j++) {
            inListLandmarkID = *(long *)&buffer[subpos];
            if (inListLandmarkID == localLandmarkID) {
                /* found landmark ID - delete it */
                numLandmarks--;
                *(long *)&buffer[pos] = numLandmarks;
                newFsize -= SIZE_OF_LANDMARKID;
                memmove(&buffer[subpos], &buffer[subpos + SIZE_OF_LANDMARKID], newFsize - subpos);
                break;
            } 
            subpos += SIZE_OF_LANDMARKID;
        }
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks * SIZE_OF_LANDMARKID;
    }

    /* update category file and chsize */
    if ((newFsize != fsize) && (ret == JAVACALL_OK)) {
        ret = JAVACALL_FAIL;
        if (_lseek(fd, 0, SEEK_SET) == 0) {
            if (_write(fd, buffer, newFsize) == newFsize) {
                _chsize(fd, newFsize);
                ret = JAVACALL_OK;
            }
        }
    }

    free(buffer);
    _close(fd);

    /* if not delete landmark from categories failed - exit, 
     * else - delete landmark from store */
    if (ret != JAVACALL_OK) {
        return ret;
    }

    /* Create name of LandmarkStore List file */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_LANDMARKSTORE, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    _read(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS);
    if (numLandmarks < localLandmarkID) {
        /* landmarkID not found - silently exit */
        _close(fd);
        return JAVACALL_OK;
    }

    /* go to landmark and delete it */
    pos = SIZE_OF_NUMLANDMARKS + (localLandmarkID-1) * (SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS) + SIZE_OF_LANDMARKID);
    if (_lseek(fd, pos, SEEK_SET) != pos) {
        _close(fd);
        return JAVACALL_FAIL;
    }
    localLandmarkID = 0;
    if (_write(fd, &localLandmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* try to decrease size of Landmark Store 
     * we can do it if we delete the latest landmark in the store */
    localLandmarkID = (long)landmarkID;
    if (numLandmarks == localLandmarkID) {
        numLandmarks--;
        newFsize = fsize - (long)(SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS) + SIZE_OF_LANDMARKID);
        for (i = numLandmarks; i>0; i--) {
            pos = SIZE_OF_NUMLANDMARKS + (i-1) * (SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS) + SIZE_OF_LANDMARKID);
            /* go to the landmark */
            if (_lseek(fd, pos, SEEK_SET) != pos) {
                break;
            }
            /* read landmark id */
            if (_read(fd, &localLandmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
                break;
            }
            /* check if it is empty */
            if (localLandmarkID != 0) {
                break;
            }
            numLandmarks--;
            newFsize -= (long)(SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS) + SIZE_OF_LANDMARKID);
        }
        /* decrease total number of landmarks in the store */
        _lseek(fd, 0, SEEK_SET);
        if (_write(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) == SIZE_OF_NUMLANDMARKS) {
            /* if successfull - cut unused landmarks */
            _chsize(fd, newFsize);
        }
    }
    _close(fd);

    return JAVACALL_OK;
}
        
/**
 * Deletes a landmark from a category.
 *
 * If the specified landmark does not belong to the specified landmark store or category, then the request is silently ignored and this function call returns without error.
 *
 * @param landmarkStoreName where this landmark belongs
 * @param landmarkID id of a landmark to delete
 * @param categoryName from which the landmark to be removed
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 */
javacall_result javacall_landmarkstore_landmark_delete_from_category(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle landmarkID,
        const javacall_utf16_string categoryName) {

    int wCategoryNameLen=0;
    char *buffer;
    long fsize = 0;
    long pos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    int fd;
    int i,j;
    boolean found = FALSE;
    long inListLandmarkID, localLandmarkID = (long)landmarkID;
    javacall_result ret;

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }

    /* read file into the buffer */
    if (_read(fd, buffer, fsize) != fsize) {
        free(buffer);
        _close(fd);
        return JAVACALL_FAIL;
    }

    wCategoryNameLen = wcslen(categoryName);

    /* parse it */
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;
    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && (pos+SIZE_OF_NAMELEN<fsize); i++) {
        /* read category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN;
        if ((pos + SIZE_OF_NAME(categoryNameLen) + SIZE_OF_NUMLANDMARKS)>fsize ) {
            /* out of buffer */
            ret = JAVACALL_FAIL;
            break;
        }
        /* check if category Names equals */
        if (wCategoryNameLen == categoryNameLen) {
            if (memcmp(&buffer[pos], &categoryName[0], SIZE_OF_NAME(wCategoryNameLen)) == 0) {
                /* we found it - add and save */
                pos += SIZE_OF_NAME(categoryNameLen);
                if (_lseek(fd, pos, SEEK_SET) != pos) {
                    ret = JAVACALL_FAIL;
                    break;
                }
                numLandmarks = *(long *)&buffer[pos];
                pos += SIZE_OF_NUMLANDMARKS;
                if ((pos + numLandmarks*SIZE_OF_LANDMARKID)> fsize) {
                    /* out of buffer */
                    ret = JAVACALL_FAIL;
                    break;
                }
                for (j=0; j<numLandmarks; j++) {
                    inListLandmarkID = *(long *)&buffer[pos];
                    if (inListLandmarkID != localLandmarkID) {
                        pos += SIZE_OF_LANDMARKID;
                    } else {
                        if (_lseek(fd, pos, SEEK_SET) != pos) {
                            ret = JAVACALL_FAIL;
                            break;
                        };
                        pos += SIZE_OF_LANDMARKID;
                        /* actually remove landmark ID */
                        if (_write(fd, &buffer[pos], fsize - pos) == (fsize - pos)) {
                            pos -= ((j+1)*SIZE_OF_LANDMARKID + SIZE_OF_NUMLANDMARKS);
                            /* update numLandmarks */
                            if (_lseek(fd, pos, SEEK_SET) != pos) {
                                ret = JAVACALL_FAIL;
                            } else {
                                numLandmarks--;
                                if (_write(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) == SIZE_OF_NUMLANDMARKS) {
                                    _chsize(fd, fsize - SIZE_OF_LANDMARKID);
                                } else {
                                    ret = JAVACALL_FAIL;
                                }
                            }
                            /* check if error occured */
                            if (ret == JAVACALL_FAIL) {
                                /* update failed and file is broken - try to restore */
                                _lseek(fd, 0, SEEK_SET);
                                _write(fd, buffer, fsize);
                            }
                        } else {
                            /* nothing changed */
                            ret = JAVACALL_FAIL;
                            break;
                        }
                        break;
                    }
                }
                break;
            }
        }
        pos += SIZE_OF_NAME(categoryNameLen);
        numLandmarks = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
    }
    
    _close(fd);
    free(buffer);

    return ret;
}

/**
 * Gets a handle for Landmark list.
 *
 * @param landmarkStoreName landmark store to get the landmark from
 * @param categoryName of the landmark to get, NULL implies a wildcard that matches all categories.
 * @param pHandle that can be used in javacall_landmarkstore_landmarklist_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_INVALID_ARGUMENT  if the category name is invalid 
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_landmarklist_open(
        const javacall_utf16_string landmarkStoreName, 
        const javacall_utf16_string categoryName, 
        javacall_handle* /*OUT*/pHandle) {

    int wLandmarkStoreNameLen = 0;
    int wCategoryNameLen=0;
    char *buffer = NULL;
    long fsize = 0;
    long pos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    landmarkListType *newList = NULL, *curEl;
    long expectedHandleID = 1;
    int fd;
    int i;
    javacall_result ret;

    /* Open Category file */
    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* Calculate len of categoryName */
    if (categoryName != NULL) {
        wCategoryNameLen = wcslen(categoryName);
        /* allocate buffer for category file contains */
        buffer = malloc(fsize);
        if (buffer == NULL) {
            ret = JAVACALL_FAIL;
        } else {
            /* read file into the buffer */
            _lseek(fd, 0, SEEK_SET);
            if (_read(fd, buffer, fsize) != fsize) {
                free(buffer);
                ret = JAVACALL_FAIL;
            };
        }
    }

    _close(fd);
    if (ret != JAVACALL_OK) {
        return ret;
    }

    /* Allocate and initialize new List infomation */
    newList = malloc(sizeof(landmarkListType));
    if (newList == NULL) {
        _close(fd);
        if (buffer != NULL) {
            free(buffer);
        }
        return JAVACALL_FAIL;
    }
    newList->handle = (javacall_handle)1;
    if (landmarkStoreName != NULL) {
        wLandmarkStoreNameLen = wcslen(landmarkStoreName);
        newList->storeName = malloc(SIZE_OF_NAME(wLandmarkStoreNameLen+1));
        if (newList->storeName != NULL) {
            memcpy(&newList->storeName[0], &landmarkStoreName[0], SIZE_OF_NAME(wLandmarkStoreNameLen));
            newList->storeName[wLandmarkStoreNameLen] = 0;
        } else {
            _close(fd);
            free(newList->storeName);
            free(newList);
            if (buffer != NULL) {
                free(buffer);
            }
            return JAVACALL_FAIL;
        }
    } else {
        newList->storeName = NULL;
    }
    if (categoryName != NULL) {
        newList->categoryName = malloc(SIZE_OF_NAME(wCategoryNameLen+1));
        memcpy(newList->categoryName, categoryName, SIZE_OF_NAME(wCategoryNameLen));
        newList->categoryName[wCategoryNameLen] = 0;
        newList->numLandmarkIDs = -1;
    } else {
        newList->categoryName = NULL;
        newList->numLandmarkIDs = 0;
        newList->landmarkIDs = NULL;
    }
    newList->currentID = 0;
    newList->curLandmark = malloc(SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS));
    newList->next = NULL;

    if (categoryName != NULL) {
        /* if category name is specified - read landmarkIDs of this category */
        /* parse category file */
        pos = 0;
        num_categories = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMCATEGORIES;
        /* File structure is :
            num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
        for (i=0; i<num_categories && (pos + SIZE_OF_NAMELEN)<fsize; i++) {
            /* read category Name */
            categoryNameLen = *(long *)&buffer[pos];
            pos += SIZE_OF_NAMELEN;
            if ((pos + SIZE_OF_NAME(categoryNameLen) + SIZE_OF_NUMLANDMARKS) > fsize) {
                /* out of buffer */
                ret = JAVACALL_FAIL;
                break;
            }
            if (memcmp(&buffer[pos], &categoryName[0], SIZE_OF_NAME(wCategoryNameLen)) == 0) {
                pos += SIZE_OF_NAME(categoryNameLen);
                newList->numLandmarkIDs = *(long *)&buffer[pos];
                pos += SIZE_OF_NUMLANDMARKS;
                newList->landmarkIDs = malloc(newList->numLandmarkIDs * SIZE_OF_LANDMARKID);
                if (newList->landmarkIDs != NULL) {
                    memcpy(newList->landmarkIDs, &buffer[pos], newList->numLandmarkIDs * SIZE_OF_LANDMARKID);
                } else {
                    ret = JAVACALL_FAIL;
                }
                break;
            }
            pos += SIZE_OF_NAME(categoryNameLen);
            numLandmarks = *(long *)&buffer[pos];
            pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
        }
        free(buffer);
        
        /* error during parsing */
        if ((newList->numLandmarkIDs == -1) || (newList->landmarkIDs == NULL) || ret != JAVACALL_OK) { 
            free(newList->storeName);
            free(newList->categoryName);
            if (newList->numLandmarkIDs == -1) {
                free(newList);
                return JAVACALL_INVALID_ARGUMENT;
            }
            free(newList);
            return JAVACALL_FAIL;
        }
    }
    /* add newList to landmarkList*/
    if (landmarkListHead == NULL) {
        landmarkListHead = newList;
    } else {
        if ((long)landmarkListHead->handle > expectedHandleID) {
            newList->next = landmarkListHead;
            landmarkListHead = newList;
        } else {
            curEl = landmarkListHead;
            expectedHandleID++;
            while (curEl->next != NULL) {
                if ((long)curEl->next->handle > expectedHandleID) {
                    newList->handle = (javacall_handle) expectedHandleID;
                    newList->next = curEl->next;
                    curEl->next = newList;
                    newList = NULL;
                    break;
                }
                expectedHandleID++;
                curEl = curEl->next;
            };
            if (newList != NULL) {
                newList->handle = (javacall_handle) expectedHandleID;
                curEl->next = newList;
            }
        }
    }
    
    *pHandle = newList->handle;

    return JAVACALL_OK;
}

/**
 * Closes the specified landmark list. 
 *
 * This handle will no longer be associated with this landmark list.
 *
 * @param handle that is returned by javacall_landmarkstore_landmarklist_open
 *
 */
void javacall_landmarkstore_landmarklist_close(
                  javacall_handle handle) {
    landmarkListType *delEl = NULL, *curEl;

    if (landmarkListHead != NULL) {
        if (landmarkListHead->handle == handle) {
            delEl = landmarkListHead;
            landmarkListHead = landmarkListHead->next;
        } else {
            curEl = landmarkListHead;
            while (curEl->next != NULL) {
                if (curEl->next->handle == handle) {
                    delEl = curEl->next;
                    curEl->next = delEl->next;
                    break;
                }
                curEl = curEl->next;
            }
        }
    }
    if (delEl != NULL) {
        free(delEl->storeName);
        if (delEl->categoryName != NULL) {
            free(delEl->categoryName);
        }
        if (delEl->landmarkIDs !=NULL) {
            free(delEl->landmarkIDs);
        }
        free(delEl->curLandmark);
        free(delEl);
    }

    (void)handle;
}

/**
 * Returns the next landmark from a landmark store.
 *
 * Assumes that the returned landmark memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pLandmarkID id of returned landmark.
 * @param pLandmark pointer to landmark on sucess, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_landmarklist_next(
        javacall_handle handle,
        int* /*OUT*/pLandmarkID,
        javacall_landmarkstore_landmark** /*OUT*/pLandmark) {

    long fsize = 0;
    long pos;
    long numLandmarks;
    int fd;
    int i;
    long localLandmarkID;
    long foundID;
    landmarkListType *curEl, *foundEl=NULL;
    boolean found = FALSE;
    javacall_result ret = JAVACALL_FAIL;

    *pLandmark = NULL;
    *pLandmarkID = 0;
    curEl = landmarkListHead;
    while(curEl != NULL) {
        if (curEl->handle == handle) {
            foundEl = curEl;
            break;
        }
    }

    if (foundEl != NULL) {
        if ((ret = openStore(foundEl->storeName, (javacall_utf16_string)FEXT_LANDMARKSTORE, FALSE, &fd, &fsize)) !=
            JAVACALL_OK) {
            return ret;
        }
        ret = JAVACALL_OK;
        if (foundEl->categoryName != NULL) {
            /* Look for category landmark IDs only */
            for (i=0; i<foundEl->numLandmarkIDs && !found; i++) {
                if (foundEl->landmarkIDs[i] > foundEl->currentID) {
                    /* found next - cImpl note: if it is not empty */
                    foundID = foundEl->landmarkIDs[i];
                    pos = SIZE_OF_NUMLANDMARKS + (foundID-1)*(SIZE_OF_LANDMARKID + SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS));
                    if (_lseek(fd, pos, SEEK_SET) != pos) {
                        /* landmark already deleted - try to find next */
                        continue;
                    }
                    if (_read(fd, &localLandmarkID, SIZE_OF_LANDMARKID) == SIZE_OF_LANDMARKID) {
                        /* check if landmark not deleted */
                        if (localLandmarkID != 0) {
                            /* read landmark */
                            if (_read(fd, foundEl->curLandmark, SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS)) == 
                                SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS)) {
                                foundEl->currentID = localLandmarkID;
                                found = TRUE;
                            }
                        }
                    }
                }
            }
        } else {
            if (_read(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS) == SIZE_OF_NUMLANDMARKS) {
                for (i=foundEl->currentID; i<numLandmarks && !found; i++ ) {
                    pos = SIZE_OF_NUMLANDMARKS + (i)*(SIZE_OF_LANDMARKID + SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS));
                    if (_lseek(fd, pos, SEEK_SET) != pos) {
                        ret = JAVACALL_FAIL;
                        break;
                    }
                    if (_read(fd, &localLandmarkID, SIZE_OF_LANDMARKID) != SIZE_OF_LANDMARKID) {
                        ret = JAVACALL_FAIL;
                        break;
                    }
                    if (localLandmarkID != 0) {
                        if (_read(fd, foundEl->curLandmark, SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS)) != 
                            SIZE_OF_LANDMARK_INFO(MAX_ADDRESSINFO_FIELDS)) {
                            ret = JAVACALL_FAIL;
                            break;
                        }
                        foundEl->currentID = localLandmarkID;
                        found = TRUE;
                    }
                }
            }
        }
        _close(fd);
        if (!found) {
            javacall_landmarkstore_landmarklist_close(handle);
        } else {
            *pLandmark = foundEl->curLandmark;
            *pLandmarkID = foundEl->currentID;
        }
        return ret;

    }
    return JAVACALL_FAIL;
}

/**
 * Gets a handle to get Category list in specified landmark store.
 *
 * @param landmarkStoreName landmark store to get the categories from
 * @param pHandle that can be used in javacall_landmarkstore_categorylist_next
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 */
javacall_result javacall_landmarkstore_categorylist_open(
        const javacall_utf16_string landmarkStoreName,
        javacall_handle* /*OUT*/pHandle){

    char *buffer;
    long fsize = 0;
    long pos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    categoryListType *newList = NULL, *curEl;
    long expectedHandleID = 1;
    int fd;
    int i,j;
    javacall_result ret;

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }
    /* read file into the buffer */
    if (_read(fd, buffer, fsize) != fsize) {
        free(buffer);
        _close(fd);
        return JAVACALL_FAIL;
    }

    _close(fd);

    /* parse it */
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;

    /* Allocate and initialize new List infomation */
    newList = malloc(sizeof(categoryListType));
    if (newList == NULL) {
        free(buffer);
        return JAVACALL_FAIL;
    }
    newList->handle = (javacall_handle)1;
    newList->numCategories = num_categories;
    newList->categoryNames = malloc(sizeof(javacall_utf16_string)*num_categories);
    newList->curCategory = 0;
    newList->next = NULL;

    if (newList->categoryNames == NULL) {
        free(buffer);
        free(newList);
        return JAVACALL_FAIL;
    };

    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && (pos+SIZE_OF_NAMELEN)<fsize; i++) {
        /* read category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN;
        if ((pos+SIZE_OF_NAME(categoryNameLen)+SIZE_OF_NUMLANDMARKS) > fsize) {
            ret = JAVACALL_FAIL;
            break;
        }
        /* add category Name to newList*/
        newList->categoryNames[i] = malloc(SIZE_OF_NAME(JAVACALL_LANDMARKSTORE_MAX_CATEGORY_NAME));
        if (newList->categoryNames[i] == NULL) {
            ret = JAVACALL_FAIL;
            break;
        } else {
            memset(&newList->categoryNames[i][0], 0, SIZE_OF_NAME(JAVACALL_LANDMARKSTORE_MAX_CATEGORY_NAME));
            memcpy(&newList->categoryNames[i][0], &buffer[pos], SIZE_OF_NAME(categoryNameLen));
        }
        pos += SIZE_OF_NAME(categoryNameLen);
        numLandmarks = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
    }
    
    free(buffer);

    if (ret != JAVACALL_OK) {
        for (j=0; j<i; j++) {
            free(newList->categoryNames[j]);
        }
        free(newList->categoryNames);
        free(newList);
        return ret;
    }

    /* add newList to categoryList*/
    if (categoryListHead == NULL) {
        categoryListHead = newList;
    } else {
        if ((long)categoryListHead->handle > expectedHandleID) {
            newList->next = categoryListHead;
            categoryListHead = newList;
        } else {
            curEl = categoryListHead;
            expectedHandleID++;
            while (curEl->next != NULL) {
                if ((long)curEl->next->handle > expectedHandleID) {
                    newList->handle = (javacall_handle) expectedHandleID;
                    newList->next = curEl->next;
                    curEl->next = newList;
                    newList = NULL;
                    break;
                }
                expectedHandleID++;
                curEl = curEl->next;
            };
            if (newList != NULL) {
                newList->handle = (javacall_handle) expectedHandleID;
                curEl->next = newList;
            }
        }
    }
    
    *pHandle = (javacall_handle)newList->handle;


    return JAVACALL_OK;
}

/**
 * Closes the specified category list. 
 * This handle will no longer be associated with this category list.
 *
 * @param handle that is returned by javacall_landmarkstore_categorylist_open
 *
 */
void javacall_landmarkstore_categorylist_close(
          javacall_handle handle) {
    int i;
    categoryListType *delEl = NULL, *curEl;

    if (categoryListHead != NULL) {
        if (categoryListHead->handle == handle) {
            delEl = categoryListHead;
            categoryListHead = categoryListHead->next;
        } else {
            curEl = categoryListHead;
            while (curEl->next != NULL) {
                if (curEl->next->handle == handle) {
                    delEl = curEl->next;
                    curEl->next = delEl->next;
                    break;
                }
                curEl = curEl->next;
            }
        }
    }
    if (delEl != NULL) {
        for (i=0; i<delEl->numCategories; i++) {
            free(delEl->categoryNames[i]);
        }
        free(delEl->categoryNames);
        free(delEl);
    }
}


/**
 * Returns the next category name in specified landmark store.
 *
 * Assumes that the returned landmark memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pCategoryName pointer to UNICODE string for the next category name on success, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 *
 */
javacall_result javacall_landmarkstore_categorylist_next(
        javacall_handle handle,
        javacall_utf16_string* /*OUT*/pCategoryName) {

    categoryListType *curEl, *foundEl=NULL;

    curEl = categoryListHead;
    while(curEl != NULL) {
        if (curEl->handle == handle) {
            foundEl = curEl;
            break;
        }
    }

    if (foundEl != NULL) {
        if (foundEl->curCategory < foundEl->numCategories) {
            *pCategoryName = foundEl->categoryNames[foundEl->curCategory++];
        } else {
            *pCategoryName = NULL;
            javacall_landmarkstore_categorylist_close(handle);
        }
    }
    return JAVACALL_OK;
}

/**
 * Creates a landmark store.
 *
 * If the implementation supports several storage media, this function may e.g. prompt the end user to make the choice.
 *
 * @param landmarkStoreName name of a landmark store.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if the name is too long or if a landmark store with specified name already exists.
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_create(
                        const javacall_utf16_string landmarkStoreName) {
    /* Create name of LandmarkStore List file */
    int fd;
    long fsize;
    javacall_result ret;

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, TRUE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return ret;
    }
    _close(fd);

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_LANDMARKSTORE, TRUE, &fd, &fsize)) !=
        JAVACALL_OK) {
        _close(fd);
        javacall_landmarkstore_delete(landmarkStoreName);
        return ret;
    }
    _close(fd);

    return JAVACALL_OK;
}

/**
 * Deletes a landmark store.
 *
 * All the landmarks and categories defined in named landmark store are removed.
 * If a landmark store with the specified name does not exist, this function returns silently without any error.
 *
 * @param landmarkStoreName name of a landmark store.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if the name is too long 
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_delete(
                        const javacall_utf16_string landmarkStoreName) {
    /* Create name of LandmarkStore List file */
    wchar_t wOsPath[MAX_FILE_NAME_LENGTH_WITH_PATH];
    int wLandmarkStoreNameLen=0;
    int fileNameLen=0;

    if (landmarkStoreName == NULL) {
        return JAVACALL_FAIL;
    }
    /* Calculate len of landmarkStoreName */
    wLandmarkStoreNameLen = wcslen(landmarkStoreName);
    if (wLandmarkStoreNameLen == 0) {
        return JAVACALL_FAIL;
    }
    if (wLandmarkStoreNameLen > MAX_FILE_NAME_LENGTH) {
        return JAVACALL_INVALID_ARGUMENT;
    }

    /* create path to LandmarkStore */
    fileNameLen = wcslen(LANDMARK_STORE_DIR);
    memcpy(wOsPath, LANDMARK_STORE_DIR, SIZE_OF_NAME(fileNameLen));
    wOsPath[fileNameLen++] = FILESEP;
    memcpy(&wOsPath[fileNameLen], landmarkStoreName, SIZE_OF_NAME(wLandmarkStoreNameLen));
    fileNameLen += wLandmarkStoreNameLen;
    memcpy(&wOsPath[fileNameLen], (javacall_utf16_string)FEXT_LANDMARKSTORE, SIZE_OF_NAME(wcslen(FEXT_LANDMARKSTORE)));
    fileNameLen += wcslen(FEXT_LANDMARKSTORE);
    wOsPath[fileNameLen] = 0;
    
    /* remove LandmarkStore */
    _wremove(wOsPath);

    /* Create category file name for LandmarkStore */
    fileNameLen -= wcslen(FEXT_LANDMARKSTORE);
    memcpy(&wOsPath[fileNameLen], FEXT_CATEGORY, SIZE_OF_NAME(wcslen(FEXT_CATEGORY)));
    fileNameLen += wcslen(FEXT_CATEGORY);
    wOsPath[fileNameLen] = 0;

    /* remove Category */
    _wremove(wOsPath);

    return JAVACALL_OK;
}

/**
 * Adds a category to a landmark store.
 *
 * This function must support category name that have length up to and 
 * including 32 chracters.
 *
 * @param landmarkStoreName the name of the landmark store.
 * @param categoryName category name - UNICODE string
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 * @retval JAVACALL_INVALID_ARGUMENT   if a category name already exists
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_category_add(
        const javacall_utf16_string landmarkStoreName,
        const javacall_utf16_string categoryName) {

    int wCategoryNameLen=0;
    char *buffer;
    long fsize = 0;
    long pos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    int fd;
    int i;
    javacall_result ret;

    /* Calculate len of categoryName */
    wCategoryNameLen = wcslen(categoryName);

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        return JAVACALL_FAIL;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }
    /* read file into the buffer */
    if(_read(fd, buffer, fsize) != fsize) {
        _close(fd);
        free(buffer);
        return JAVACALL_FAIL;
    };

    /* parse it */
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;
    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && (pos + SIZE_OF_NAMELEN)<fsize; i++) {
        /* read category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN;
        if ((pos + SIZE_OF_NAME(categoryNameLen) + SIZE_OF_NUMLANDMARKS) > fsize) {
            /* out of buffer */
            _close(fd);
            free(buffer);
            return JAVACALL_FAIL;
        }
        /* check if category Name exists */
        if (wCategoryNameLen == categoryNameLen) {
            if (memcmp(&buffer[pos], &categoryName[0], SIZE_OF_NAME(wCategoryNameLen)) == 0) {
                _close(fd);
                free(buffer);
                return JAVACALL_INVALID_ARGUMENT;
            }

        }
        pos += SIZE_OF_NAME(categoryNameLen);
        numLandmarks = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
    }

    free(buffer);

    ret = JAVACALL_OK;
    /* category Name not found - add new */
    if (_lseek(fd, 0, SEEK_END) == fsize) {
        if (_write(fd, &wCategoryNameLen, SIZE_OF_NAMELEN) != SIZE_OF_NAMELEN) {
            ret = JAVACALL_FAIL;
        } else {
            if (_write(fd, categoryName, SIZE_OF_NAME(wCategoryNameLen)) != SIZE_OF_NAME(wCategoryNameLen)) {
                ret = JAVACALL_FAIL;
            } else {
                numLandmarks = 0;
                if (_write(fd, &numLandmarks, SIZE_OF_NUMLANDMARKS)!= SIZE_OF_NUMLANDMARKS) {
                    ret = JAVACALL_FAIL;
                };
            }
        }
    } else {
        ret = JAVACALL_FAIL;
    }
    
    if (ret != JAVACALL_OK) {
        /* error - remove partly added category */
        _chsize(fsize, fsize);
    } else {
        /* increase total number of categories */
        num_categories++;
        _lseek(fd, 0, SEEK_SET);
        if (_write(fd, &num_categories, SIZE_OF_NUMCATEGORIES) != SIZE_OF_NUMCATEGORIES) {
            ret = JAVACALL_FAIL;
        }
    }

    _close(fd);

    return ret;
}

/**
 * Removes a category from a landmark store.
 *
 * The category will be removed from all landmarks that are in that category. However, this function will not remove any of the landmarks. Only the associated category information from the landmarks are removed.
 * If a category with the supplied name does not exist in the specified landmark store, this function returns silently without error.
 *
 * @param landmarkStoreName the name of the landmark store.
 * @param categoryName category name - UNICODE string
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        the other error
 */
javacall_result /*OPTIONAL*/ javacall_landmarkstore_category_delete(
        const javacall_utf16_string landmarkStoreName,
        const javacall_utf16_string categoryName) {
    /* Create name of LandmarkStore List file */
    int wCategoryNameLen=0;
    char *buffer;
    long fsize = 0;
    long pos;
    long num_categories, categoryNameLen;
    long numLandmarks;
    long startCtg = 0, endCtg = 0;
    int fd;
    int i;
    javacall_result ret = JAVACALL_OK;

    /* Calculate len of categoryName */
    wCategoryNameLen = wcslen(categoryName);

    if ((ret = openStore(landmarkStoreName, (javacall_utf16_string)FEXT_CATEGORY, FALSE, &fd, &fsize)) !=
        JAVACALL_OK) {
        if (ret == JAVACALL_INVALID_ARGUMENT) {
            /* return silently */
            return JAVACALL_OK;
        }
        return ret;
    }

    /* allocate buffer for file contains */
    buffer = malloc(fsize);
    if (buffer == NULL) {
        _close(fd);
        return JAVACALL_FAIL;
    }
    /* read file into the buffer */
    if(_read(fd, buffer, fsize) != fsize) {
        _close(fd);
        free(buffer);
        return JAVACALL_FAIL;
    };

    /* parse it */
    pos = 0;
    num_categories = *(long *)&buffer[pos];
    pos += SIZE_OF_NUMCATEGORIES;
    /* File structure is :
        num_categories | {categoryNameLen + categoryName + numLandmarks + {LandmarkIDs}} */
    for (i=0; i<num_categories && startCtg==0 && (pos + SIZE_OF_NAMELEN)<fsize; i++) {
        /* read category Name */
        categoryNameLen = *(long *)&buffer[pos];
        pos += SIZE_OF_NAMELEN;
        if ((pos + SIZE_OF_NAME(categoryNameLen) + SIZE_OF_NUMLANDMARKS) > fsize) {
            /* out of buffer */
            _close(fd);
            free(buffer);
            return JAVACALL_FAIL;
        }
        /* check if category Names equals */
        if (wCategoryNameLen == categoryNameLen) {
            if (memcmp(&buffer[pos], &categoryName[0], SIZE_OF_NAME(wCategoryNameLen)) == 0) {
                startCtg = pos - SIZE_OF_NAMELEN;
            }

        }
        pos += SIZE_OF_NAME(categoryNameLen);
        numLandmarks = *(long *)&buffer[pos];
        pos += SIZE_OF_NUMLANDMARKS + numLandmarks*SIZE_OF_LANDMARKID;
        endCtg = pos;
    }

    if (startCtg != 0) {
        ret = JAVACALL_FAIL;
        /* Category found */
        _lseek(fd, 0, SEEK_SET);
        /* Update num categories */
        num_categories--;
        if (_write(fd, &num_categories, SIZE_OF_NUMCATEGORIES)== SIZE_OF_NUMCATEGORIES) {
            if (_lseek(fd, startCtg, SEEK_SET) == startCtg) {
                if (_write(fd, &buffer[endCtg], fsize - endCtg) == (fsize - endCtg)) {
                    _chsize(fd, fsize - (endCtg-startCtg));
                    ret = JAVACALL_OK;
                }
            };
        }
        if (ret != JAVACALL_OK) {
            /* error during removing - reload buffer */
            _lseek(fd, 0, SEEK_SET);
            _write(fd, buffer, fsize);
        }
    }
    free(buffer);

    _close(fd);

    return ret;
}

/**
 * Gets a handle for LandmarkStore list.
 *
 * @param pHandle that can be used in javacall_landmarkstore_list_next
 * @param pDefLandmarkStore pointer to UNICODE string for the default LandmarkStore name.
 *      default LandmarkStore name can not be NULL
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_list_open(
        javacall_handle* /*OUT*/pHandle) {

    struct _stat stat;
    wchar_t wOsPath[MAX_FILE_NAME_LENGTH]; // max file name
    int fileNameLen=wcslen(LANDMARK_STORE_DIR);
    storeListType *newList;

    if (!landmarkStoreDirExist) {
        if (_wstat(LANDMARK_STORE_DIR, &stat) != 0) {
            /* directory not exist */
            _wmkdir(LANDMARK_STORE_DIR);
        }
        landmarkStoreDirExist = TRUE;
    }

    /* Allocate and initialize new List infomation */
    newList = malloc(sizeof(storeListType));
    if (newList == NULL) {
        return JAVACALL_FAIL;
    }

    memset(wOsPath, 0, SIZE_OF_NAME(MAX_FILE_NAME_LENGTH));
    memcpy(wOsPath, LANDMARK_STORE_DIR, SIZE_OF_NAME(fileNameLen));

    wOsPath[fileNameLen++] = FILESEP;
    wOsPath[fileNameLen++] = '*';
    memcpy(&wOsPath[fileNameLen], (javacall_utf16_string)FEXT_LANDMARKSTORE, SIZE_OF_NAME(wcslen(FEXT_LANDMARKSTORE)));

    *pHandle = FindFirstFileW(wOsPath, &(newList->dir_data));

    if (*pHandle != INVALID_HANDLE_VALUE) {
        newList->first = TRUE;
        /* add newList to storeList*/
        newList->handle = *pHandle;
        newList->next = storeListHead;
        storeListHead = newList;
    } else {
        free(newList);
    }
    
    return JAVACALL_OK;
}

/**
 * Closes the specified landmarkstore list. 
 *
 * This handle will no longer be associated with this landmarkstore list.
 *
 * @param handle that is returned by javacall_landmarkstore_list_open
 *
 */
void javacall_landmarkstore_list_close(
                  javacall_handle handle) {
    storeListType *delEl = NULL, *curEl;

    if (handle != INVALID_HANDLE_VALUE) {
        if (storeListHead != NULL) {
            if (storeListHead->handle == handle) {
                delEl = storeListHead;
                storeListHead = storeListHead->next;
            } else {
                curEl = storeListHead;
                while (curEl->next != NULL) {
                    if (curEl->next->handle == handle) {
                        delEl = curEl->next;
                        curEl->next = delEl->next;
                        break;
                    }
                    curEl = curEl->next;
                }
            }
        }
        if (delEl != NULL) {
            free(delEl);
        }
    }
}

/**
 * Returns the next landmark store name
 *
 * Assumes that the returned landmarkstore memory block is valid until the next this function call
 *
 * @param handle of landmark store
 * @param pLandmarkStore pointer to UNICODE string for the next landmark store name on success, NULL otherwise
 *      returned param is a pointer to platform specific memory block.
 *      platform MUST BE responsible for allocating and freeing it.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on other error
 */
javacall_result javacall_landmarkstore_list_next(
        javacall_handle handle,
        javacall_utf16_string* /*OUT*/pLandmarkStore) {

    storeListType *curEl, *foundEl=NULL;

    if (handle != INVALID_HANDLE_VALUE) {

        curEl = storeListHead;
        while(curEl != NULL) {
            if (curEl->handle == handle) {
                foundEl = curEl;
                break;
            }
        }

        if (foundEl != NULL) {
            if (foundEl->first || (foundEl->len > 0)) {
                if (!foundEl->first) {
                    /* add extension to store name and try to find next */
                    foundEl->dir_data.cFileName[foundEl->len] = FEXT_LANDMARKSTORE[0];
                    if (FindNextFileW(handle, &(foundEl->dir_data)) == 0) {
                        /* no more stores - exit */
                        foundEl->len = 0;
                    } else {
                        /* remove extension */
                        foundEl->len = wcslen((wchar_t *)(foundEl->dir_data.cFileName)) - wcslen(FEXT_LANDMARKSTORE);
                        foundEl->dir_data.cFileName[foundEl->len] = 0;
                    }
                } else {
                    /* remove extension */
                    foundEl->len = wcslen((wchar_t *)(foundEl->dir_data.cFileName)) - wcslen(FEXT_LANDMARKSTORE);
                    foundEl->dir_data.cFileName[foundEl->len] = 0;
                    foundEl->first = FALSE;
                }
            }

            if (foundEl->len == 0) {
                *pLandmarkStore = NULL;
            } else {
                *pLandmarkStore = (javacall_utf16_string)foundEl->dir_data.cFileName;
            }
        } else {
            return JAVACALL_FAIL;
        }

    } else {
        *pLandmarkStore = NULL;
    }
    return JAVACALL_OK;
}

/**
 * Internal function :
 * Opens store file (landmark or category store).
 * Returns file description of the opened store file and file size in bytes
 *
 * @param name of the destination file
 * @param ext - extention of the destination file
 * @param create - TRUE if create a new file
 * @param fd - file descriptor of the opened file
 * @param fsize - size of the opened file
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        on error
 * @retval JAVACALL_INVALID_ARGUMENT  if the store name is invalid or the landmark has a longer name field than the implementation can support.
 */

javacall_result openStore(const javacall_utf16_string name, const javacall_utf16_string ext, boolean create, int* fd, long *fsize) {

    wchar_t  wOsPath[MAX_FILE_NAME_LENGTH];
    int len, fullLen;
    int flag = O_RDWR | O_BINARY;
    int mode = S_IREAD | S_IWRITE;
    long zero = 0;
    struct _stat stat;

    if (!landmarkStoreDirExist) {
        if (_wstat(LANDMARK_STORE_DIR, &stat) != 0) {
            /* directory not exist */
            _wmkdir(LANDMARK_STORE_DIR);
        }
        landmarkStoreDirExist = TRUE;
    }

    fullLen = wcslen(LANDMARK_STORE_DIR);
    memcpy(wOsPath, LANDMARK_STORE_DIR, SIZE_OF_NAME(fullLen));
    wOsPath[fullLen++] = FILESEP;
    if (name != NULL) {
        len = wcslen(name);
        if (len > MAX_FILE_NAME_LENGTH) {
            return JAVACALL_INVALID_ARGUMENT;
        }
        if (len == 0) {
            return JAVACALL_FAIL;
        }
        memcpy(&wOsPath[fullLen], name, SIZE_OF_NAME(len));
        fullLen += len;
        memcpy(&wOsPath[fullLen], ext, SIZE_OF_NAME(wcslen(ext)));
        fullLen += wcslen(ext);
    } else {
        len = wcslen(ext)-1;
        create = TRUE;
        memcpy(&wOsPath[fullLen], &ext[1], SIZE_OF_NAME(len));
        fullLen += len;
    }
    wOsPath[fullLen] = 0;

    if (create) {
        flag |= O_CREAT;
    }

    *fd = _wopen(wOsPath, flag, mode);
    if (*fd < 0) {
        if (!create) {
            if (_wstat(wOsPath, &stat) != 0) {
                return JAVACALL_INVALID_ARGUMENT;
            }
        }
        return JAVACALL_FAIL;
    }

    /* calculate size of the file */
    *fsize = _lseek(*fd, 0, SEEK_END);
    if (!create) {
        /* check if it is not empty */
        if (*fsize == 0) {
            _close(*fd);
            return JAVACALL_FAIL;
        }
    } else {
        if (*fsize > 0) {
            if (name != NULL) {
                _close(*fd);
                return JAVACALL_INVALID_ARGUMENT;
            }
        } else {
            if (_write(*fd, &zero, sizeof(zero)) != sizeof(zero)) {
                _close(*fd);
               return JAVACALL_FAIL;
            }
            *fsize = sizeof(zero);
        }
    }
   _lseek(*fd, 0, SEEK_SET);
    return JAVACALL_OK;
}