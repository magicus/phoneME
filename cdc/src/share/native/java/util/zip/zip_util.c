/*
 * @(#)zip_util.c	1.76 06/10/10
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

/*
 * Support for reading ZIP/JAR files.
 */

#include "jni.h"
#include "jlong.h"
#include "jvm.h"
#include "zip_util.h"
#include "zlib.h"

#include "javavm/include/ansi2cvm.h"
#include "javavm/include/porting/path.h"

#define MAXREFS 0xFFFF	/* max number of open zip file references */
#define MAXSIZE INT_MAX	/* max size of zip file or zip entry */

#define MCREATE()      JVM_RawMonitorCreate()
#define MLOCK(lock)    JVM_RawMonitorEnter(lock)
#define MUNLOCK(lock)  JVM_RawMonitorExit(lock)
#define MDESTROY(lock) JVM_RawMonitorDestroy(lock)

#include "jni_statics.h"

static char errbuf[256];

/*
 * Initialize zip file support. Return 0 if successful otherwise -1
 * if could not be initialized.
 */

jint InitializeZip()
{
    if (JNI_STATIC(zip_util, inited))
        return 0;
    JNI_STATIC(zip_util, zfiles_lock) = MCREATE();
    if (JNI_STATIC(zip_util, zfiles_lock) == 0) {
	return -1;
    }
    JNI_STATIC(zip_util, inited) = JNI_TRUE;

    return 0;
}

/*
 * Destroy zip file support. Return 0 if successful otherwise -1
 * if could not be destroyed.
 */
void
DestroyZip()
{
    if (JNI_STATIC(zip_util, zfiles_lock) != 0) {
	MDESTROY(JNI_STATIC(zip_util, zfiles_lock));
	JNI_STATIC(zip_util, zfiles_lock) = 0;
	JNI_STATIC(zip_util, inited) = JNI_FALSE;
    }
}

/*
 * Reads len bytes of data into buf. Returns 0 if all bytes could be read,
 * otherwise returns -1.
 */
static jint readFully(jint fd, void *buf, jint len)
{
    unsigned char *bp = (unsigned char *)buf;
    while (len > 0) {
	jint n = (jint)JVM_Read(fd, (char *)bp, len);
	if (n <= 0) {
	    return -1;
	}
	bp += n;
	len -= n;
    }
    return 0;
}

/*
 * Allocates a new zip file object for the specified file name.
 * Returns the zip file object or NULL if not enough memory.
 */
jzfile *allocZip(const char *name)
{
    jzfile *zip = (jzfile *)calloc(1, sizeof(jzfile));

    if (zip == 0) {
	return 0;
    }
    zip->name = strdup(name);
    if (zip->name == 0) {
	free(zip);
	return 0;
    }
    zip->lock = MCREATE();
    if (zip->lock == 0) {
	free(zip->name);
	free(zip);
	return 0;
    }
    return zip;
}

/*
 * Frees the specified zip file object.
 */
static void freeZip(jzfile *zip)
{
    int i;
    /* First free any cached jzentry */
    ZIP_FreeEntry(zip,0);
    if (zip->name != 0) {
	free(zip->name);
    }
    if (zip->lock != 0) {
	MDESTROY(zip->lock);
    }
    if (zip->comment != 0) {
	free(zip->comment);
    }
    if (zip->entries != 0) {
	free(zip->entries);
    }
    if (zip->table != 0) {
	free(zip->table);
    }
    if (zip->metanames != 0) {
	for (i = 0; i < zip->metacount; i++) {
	    if (zip->metanames[i]) {
	        free(zip->metanames[i]);
	    }
	}
	free(zip->metanames);
    }
    if (zip->comments != 0) {
	for (i = 0; i < zip->total; i++) {
	    if (zip->comments[i]) {
	        free(zip->comments[i]);
	    }
	}
	free(zip->comments);
    }
    free(zip);
}

/*
 * Searches for end of central directory (END) header. The contents of
 * the END header will be read and placed in endbuf. Returns the file
 * position of the END header, otherwise returns 0 if the END header
 * was not found or -1 if an error occurred.
 */
static jint findEND(jzfile *zip, void *endbuf)
{
    unsigned char buf[ENDHDR * 2];
    jint len, pos;
    jint fd = zip->fd;

    /* Get the length of the zip file */
    len = pos = jlong_to_jint(JVM_Lseek(fd, jlong_zero, SEEK_END));
    if (len == -1) {
	return -1;
    }
    /*
     * Search backwards ENDHDR bytes at a time from end of file stopping
     * when the END header has been found. We need to make sure that we
     * handle the case where the signature may straddle a record boundary.
     * Also, the END header must be located within the last 64k bytes of
     * the file since that is the maximum comment length.
     */
    memset(buf, 0, sizeof(buf));
    while (len - pos < 0xFFFF) {
	unsigned char *bp;
	/* Number of bytes to check in next block */
	int count = 0xFFFF - (len - pos);
	if (count > ENDHDR) {
	    count = ENDHDR;
	}
	/* Shift previous block */
	memcpy(buf + count, buf, count);
	/* Update position and read next block */
	pos -= count;
	if (jlong_to_jint(JVM_Lseek(fd, jint_to_jlong(pos), SEEK_SET)) == -1) {
	    return -1;
	}
	if (readFully(fd, buf, count) == -1) {
	    return -1;
	}
	/* Now scan the block for END header signature */
	for (bp = buf; bp < buf + count; bp++) {
	    if (GETSIG(bp) == ENDSIG) {
		/* Check for possible END header */
		jint endpos = pos + (jint)(bp - buf);
		jint clen = ENDCOM(bp);
		if (endpos + ENDHDR + clen == len) {
		    /* Found END header */
		    memcpy(endbuf, bp, ENDHDR);
		    if (jlong_to_jint(JVM_Lseek(fd,
						jint_to_jlong(endpos + ENDHDR),
						SEEK_SET))
			== -1) {
			return -1;
		    }
		    if (clen > 0) {
			zip->comment = (char *)malloc(clen + 1);
			if (zip->comment == 0) {
			    return -1;
			}
			if (readFully(zip->fd, zip->comment, clen) == -1) {
			    free(zip->comment);
			    zip->comment = 0;
			    return -1;
			}
			zip->comment[clen] = '\0';
		    }
		    return endpos;
		}
	    }
	}
    }
    return 0; /* END header not found */
}

/*
 * Returns a hash code value for the specified string.
 */
static unsigned int
hash(const char *s)
{
    int h = 0;
    while (*s != '\0') {
	h = 31*h + *s++;
    }
    return h;
}

/*
 * Returns true if the specified entry's name begins with the string
 * "META-INF/" irrespect of case.
 */
static int
isMetaName(char *name)
{
#define META_INF "META-INF/"
    char *s = META_INF, *t = name;
    while (*s != '\0') {
	if (*s++ != (char)toupper(*t++)) {
	    return 0;
	}
    }
    return 1;
}

int
ZIP_IsSigned(jzfile *zip)
{
    int i;
    char *dsa = ".DSA", *rsa = ".RSA", *sf = ".SF";
    char *p;
    if (zip->metanames == 0) {
        return 0;
    }
    for (i = 0; i < zip->metacount; i++) {
        if (zip->metanames[i] != 0) {
            p = zip->metanames[i];
            if (strstr(p, dsa) != NULL ||
                strstr(p, rsa) != NULL ||
                strstr(p, sf) != NULL) {
                return 1;
            }
        }
    }
    return 0;
}

int
ZIP_HasExtInfo(jzfile *zip)
{
    char *idx = "META-INF/INDEX.LIST";
    char *mf = "META-INF/MANIFEST.MF";
    char *clpath = "Class-Path";
    char *extlist = "Extension-List";
    char filename[21]; 
    jint fileSize;
    jint fileNameLen;
    char *buf = NULL;
    jzentry *idxze;
    jzentry *mfze; 

    idxze = ZIP_FindEntry(zip, idx, &fileSize, &fileNameLen);
    if (idxze != NULL) {
        return 1;
    }

    mfze = ZIP_FindEntry(zip, mf, &fileSize, &fileNameLen);
    if (mfze == NULL) {
        return 0;
    } else {
        buf = (char *)malloc(fileSize+1);
        if (buf == NULL) {
            return -1;
        }
        if (!ZIP_ReadEntry(zip, mfze, (unsigned char*)buf, filename)) {
            free(buf);
            return -1;
        } else {
           char *end = buf+fileSize;
           char *p = buf;
           *end = 0;
           while (p < end) {
               if ((*p == 'C' && strncmp(p, clpath, 10) == 0) ||
                   (*p == 'E' && strncmp(p, extlist, 14) == 0)) {
                   free(buf);
                   return 1;
               }
               p++;
           }
        }
        free(buf);
    }
    return 0;
}

static int
addMetaName(jzfile *zip, char *name)
{
    int i;
    if (zip->metanames == 0) {
	zip->metanames = (char **)calloc(2, sizeof(char *));
	if (zip->metanames == NULL) {
	    return 0;
	}
	zip->metacount = 2;
    }
    for (i = 0; i < zip->metacount; i++) {
	if (zip->metanames[i] == 0) {
	    zip->metanames[i] = strdup(name);
	    if (zip->metanames[i] == NULL) {
		return 0;
	    }
	    return 1;
	}
    }
    /* If necessary, grow the metanames array */
    {
	int new_count = 2 * zip->metacount;
	char **tmp = (char **)calloc(new_count, sizeof(char *));
	if (tmp == NULL) {
	    return 0;
	}
	for (i = 0; i < zip->metacount; i++) {
	    tmp[i] = zip->metanames[i];
	}
	zip->metacount = new_count;
	tmp[i] = strdup(name);
	free(zip->metanames);
	zip->metanames = tmp;
	if (tmp[i] == NULL) {
	    return 0;
	} else {
	    return 1;
	}
    }
}

static void
addEntryComment(jzfile *zip, int index, char *comment)
{
    if (zip->comments == NULL) {
	zip->comments = (char **)calloc(zip->total, sizeof(char *));
    }
    if (zip->comments != NULL) {
	zip->comments[index] = comment;
    }
}

/*
 * Reads zip file central directory. Returns the file position of first
 * CEN header, otherwise returns 0 if central directory not found or -1
 * if an error occurred. If zip->msg != NULL then the error was a zip
 * format error and zip->msg has the error text.
 */
static jint readCEN(jzfile *zip)
{
    jint endpos, locpos, cenpos, cenoff, cenlen;
    jint total, count, tablelen, i, tmplen;
    unsigned char endbuf[ENDHDR], *cenbuf, *cp;
    jzcell *entries;
    unsigned short *table;
    char namebuf[ZIP_TYPNAMELEN + 1];
    char* name = namebuf;
    int namelen = ZIP_TYPNAMELEN + 1;


    /* Clear previous zip error */
    zip->msg = 0;
    /* Get position of END header */
    endpos = findEND(zip, endbuf);
    if (endpos == 0) {
	return 0;  /* END header not found */
    }
    if (endpos == -1) {
	return -1; /* system error */
    }
    /* Get position and length of central directory */
    cenlen = ENDSIZ(endbuf);
    if (cenlen < 0 || cenlen > endpos) {
	zip->msg = "invalid END header (bad central directory size)";
	return -1;
    }
    cenpos = endpos - cenlen;
    /*
     * Get position of first local file (LOC) header, taking into
     * account that there maybe a stub prefixed to the zip file.
     */ 
    cenoff = ENDOFF(endbuf);
    if (cenoff < 0 || cenoff > cenpos) {
	zip->msg = "invalid END header (bad central directory offset)";
	return -1;
    }
    locpos = cenpos - cenoff;
    /* Get total number of central directory entries */
    total = zip->total = ENDTOT(endbuf);
    if (total < 0 || total * CENHDR > cenlen) {
	zip->msg = "invalid END header (bad entry count)";
	return -1;
    }
    if (total > ZIP_MAXENTRIES) {
	zip->msg = "too many entries in ZIP file";
	return -1;
    }
    /* Seek to first CEN header */
    if (jlong_to_jint(JVM_Lseek(zip->fd, jint_to_jlong(cenpos), SEEK_SET))
	== -1) {
	return -1;
    }

    /* Allocate temporary buffer for central directory bytes */
    cenbuf = (unsigned char *)malloc(cenlen);
    if (cenbuf == 0) {
	return -1;
    }
    /* Read central directory */
    if (readFully(zip->fd, cenbuf, cenlen) == -1) {
	free(cenbuf);
	return -1;
    }
    /* Allocate array for item descriptors */
    entries = zip->entries = (jzcell *)calloc(total, sizeof(jzcell));
    if (entries == 0) {
	free(cenbuf);
	return -1;
    }
    /* Allocate hash table */
    tmplen = total/2;
    tablelen = zip->tablelen = (tmplen > 0 ? tmplen : 1);
    table = zip->table = (unsigned short *)calloc(tablelen, sizeof(unsigned short));
    if (table == 0) {
	free(cenbuf);
	free(entries);
	zip->entries = 0;
	return -1;
    }
    for (i = 0; i < tablelen; i++) {
	table[i] = ZIP_ENDCHAIN;
    }

    /* Now read the zip file entries */
    for (count = 0, cp = cenbuf; count < total; count++) {
	jzcell *zc = &entries[count];
	int method, nlen, clen, elen, hsh;

	/* Check CEN header looks OK */
	if ((cp - cenbuf) + CENHDR > cenlen) {
	    zip->msg = "invalid CEN header (bad header size)";
	    break;
	}
	/* Verify CEN header signature */
	if (GETSIG(cp) != CENSIG) {
	    zip->msg = "invalid CEN header (bad signature)";
	    break;
	}
	/* Check if entry is encrypted */
	if ((CENVER(cp) & 1) == 1) {
	    zip->msg = "invalid CEN header (encrypted entry)";
	    break;
	}
	method = CENHOW(cp);
	if (method != STORED && method != DEFLATED) {
	    zip->msg = "invalid CEN header (bad compression method)";
	    break;
	}

	/* Get header field lengths */
	nlen         = CENNAM(cp);
	elen         = CENEXT(cp);
	clen         = CENCOM(cp);
	if ((cp - cenbuf) + CENHDR + nlen + clen + elen > cenlen) {
	    zip->msg = "invalid CEN header (bad header size)";
	    break;
	}

	zc->size     = CENLEN(cp);
	zc->csize    = CENSIZ(cp);
        zc->crc      = CENCRC(cp);
	/* Set compressed size to zero if entry uncompressed */
	if (method == STORED) {
	    zc->csize = 0;
	}

	/*
         * Copy the name into a temporary location so we can null
         * terminate it (sigh) as various functions expect this.
         */
        if (namelen < nlen + 1) { /* grow temp buffer */
            do  
                namelen = namelen * 2;
            while (namelen < nlen + 1);
            if (name != namebuf) /* free malloc()ated buffer */
                free(name);
            name = (char *)malloc(namelen);
	    if (name == 0) {
		free(cenbuf);
		free(entries);
		zip->entries = 0;
		return -1;
	    }
        } 
	memcpy(name, cp+CENHDR, nlen);
        name[nlen] = 0;

	/*
         * Record the LOC offset and the name hash in our hash cell.
         */
	zc->pos = CENOFF(cp) + locpos;
	zc->nelen = nlen + elen;
	zc->hash = hash(name);
     	zc->cenpos = cenpos + (cp - cenbuf);
	zc->elen = elen;
	/*
	 * if the entry is metdata add it to our metadata names
         */
	if (isMetaName(name)) {
	    if (!addMetaName(zip, name)) {
		goto error;
	    }
	}

	/*
         * If there is a comment add it to our comments array.
         */
	if (clen > 0) {
	    char *comment = (char *)malloc(clen+1);
	    if (comment == NULL) {
		goto error;
	    }
	    memcpy(comment, cp+CENHDR+nlen+elen, clen);
            comment[clen] = 0;
	    addEntryComment(zip, count, comment);
	    if (zip->comments == NULL) {
		free(comment);
		goto error;
	    }
 	}

	/*
         * Finally we can add the entry to the hash table
         */
	hsh = zc->hash % tablelen;
	zc->next = table[hsh];
	table[hsh] = count;

	cp += (CENHDR + nlen + elen + clen);
    }
    /* Free up temporary buffers */
error:
    free(cenbuf);
    if (name != namebuf)
        free(name);

    /* Check for error */
    if (count != total) {
	printf("count = %d, total = %d\n", count, total); /* DBG */
	/* Central directory was invalid, so free up entries and return */
	free(entries);
	zip->entries = 0;
	free(table);
	zip->table = 0;
	return -1;
    }
    return cenpos;
}

/*
 * Opens a zip file with the specified mode. Returns the jzfile object 
 * or NULL if an error occurred. If a zip error occurred then *msg will 
 * be set to the error message text if msg != 0. Otherwise, *msg will be
 * set to NULL.
 */
jzfile *
ZIP_Open_Generic(const char *name, char **pmsg, int mode, jlong lastModified)
{
    char buf[CVM_PATH_MAXLEN];
    jzfile *zip;

    if (InitializeZip()) {
        return NULL;
    }

    /* Clear zip error message */
    if (pmsg != 0) {
	*pmsg = 0;
    }

    if (strlen(name) >= CVM_PATH_MAXLEN) {
        if (pmsg) {
            *pmsg = "zip file name too long";
        }
        return (jzfile *) 0;
    }
    strcpy(buf, name);
    JVM_NativePath(buf);
    name = buf;

    MLOCK(JNI_STATIC(zip_util, zfiles_lock));
    for (zip = (jzfile *) JNI_STATIC(zip_util, zfiles); zip != 0; zip = zip->next) {
        if (strcmp(name, zip->name) == 0
            && (zip->lastModified == lastModified || zip->lastModified == 0)
            && zip->refs < MAXREFS) {
	    zip->refs++;
	    break;
	}
    }
    MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
    if (zip == 0) {
	jlong len;
	/* If not found then allocate a new zip object */
	zip = allocZip(name);
	if (zip == 0) {
	    return 0;
	}
	zip->refs = 1;
        zip->lastModified = lastModified;
	zip->mode = mode;
	zip->fd = JVM_Open(name, mode, 0);
	if (zip->fd == -1) {
            if (pmsg != 0) {
                char* buf = errbuf;
                if (JVM_GetLastErrorString(buf, 256) > 0) {
                    *pmsg = buf;
                }
            }
	    freeZip(zip);
	    return 0;
	}
	len = JVM_Lseek(zip->fd, jlong_zero, SEEK_END);
	if (CVMlongEq(len, jint_to_jlong(-1))) {
            if (pmsg != 0) {
                char* buf = errbuf;
                if (JVM_GetLastErrorString(buf, 256) > 0) {
                    *pmsg = buf;
                }
            }
            JVM_Close(zip->fd);
	    freeZip(zip);
	    return 0;
	}
	if (CVMlongGt(len, jint_to_jlong(MAXSIZE))) {
	    if (pmsg != 0) {
		*pmsg = "zip file too large";
	    }
            JVM_Close(zip->fd);
	    freeZip(zip);
	    return 0;
	}
	if (readCEN(zip) <= 0) {
	    /* An error occurred while trying to read the zip file */
	    if (pmsg != 0) {
		/* Set the zip error message */
		*pmsg = zip->msg;
	    }
	    JVM_Close(zip->fd);
	    freeZip(zip);
	    return 0;
	}
	MLOCK(JNI_STATIC(zip_util, zfiles_lock));
	zip->next = (jzfile *) JNI_STATIC(zip_util, zfiles);
	JNI_STATIC(zip_util, zfiles) = zip;
	MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
    }
    return zip;
}

#ifdef CVM_MTASK
static jboolean 
zipInitialized()
{
    return JNI_STATIC(zip_util, inited);
}

/*
 * Close fd's of zip files we have handled
 */
void JNICALL
ZIP_Closefds()
{
    jzfile* zip;

    if (!zipInitialized()) {
	return;
    }

    MLOCK(JNI_STATIC(zip_util, zfiles_lock));
    for (zip = (jzfile *) JNI_STATIC(zip_util, zfiles); zip != 0; zip = zip->next) {
#if 0
	jio_fprintf(stderr, "closing zip name=%s, fd=%d\n",
		    zip->name, zip->fd);
#endif
	JVM_Close(zip->fd);
	/* Make sure this does not get used accidentally */
	zip->fd = -1;
    }
    MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
}

/*
 * Re-open fd's of zip files we have handled
 */
jboolean JNICALL
ZIP_Reopenfds()
{
    jzfile* zip;

    if (!zipInitialized()) {
	return JNI_TRUE;
    }

    MLOCK(JNI_STATIC(zip_util, zfiles_lock));
    for (zip = (jzfile *) JNI_STATIC(zip_util, zfiles); zip != 0; zip = zip->next) {
#if 0
	jio_fprintf(stderr, "reopening zip name=%s, mode=%d\n",
		    zip->name, zip->mode);
#endif
	zip->fd = JVM_Open(zip->name, zip->mode, 0);
	if (zip->fd == -1) {
	    perror("JVM_Open()");
	    MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
	    return JNI_FALSE;
	}
    }
    MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
    return JNI_TRUE;
}
#endif

/*
 * Opens a zip file for reading. Returns the jzfile object or NULL
 * if an error occurred. If a zip error occurred then *msg will be
 * set to the error message text if msg != 0. Otherwise, *msg will be
 * set to NULL.
 */
jzfile * JNICALL
ZIP_Open(const char *name, char **pmsg)
{
    return ZIP_Open_Generic(name, pmsg, O_RDONLY, 0);
}

/*
 * Closes the specified zip file object.
 */
void ZIP_Close(jzfile *zip)
{
    MLOCK(JNI_STATIC(zip_util, zfiles_lock));
    if (--zip->refs > 0) {
	/* Still more references so just return */
	MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
	return;
    }
    /* No other references so close the file and remove from list */
    if ((jzfile *) JNI_STATIC(zip_util, zfiles) == zip) {
	JNI_STATIC(zip_util, zfiles) = ((jzfile *) JNI_STATIC(zip_util, zfiles))->next;
    } else {
	jzfile *zp;
	for (zp = (jzfile *) JNI_STATIC(zip_util, zfiles); zp->next != 0; zp = zp->next) {
	    if (zp->next == zip) {
		zp->next = zip->next;
		break;
	    }
	}
    }
    MUNLOCK(JNI_STATIC(zip_util, zfiles_lock));
    JVM_Close(zip->fd);
    freeZip(zip);
    return;
}

/*
 * Read a LOC corresponding to a given hash cell and
 * create a corrresponding jzentry entry descriptor
 * The ZIP lock should be held here.
 */
static jzentry *
readLOC(jzfile *zip, jzcell *zc)
{
    unsigned char *locbuf;
    jint nlen, elen;
    jzentry *ze = 0;

    /* Seek to beginning of LOC header */
    if (jlong_to_jint(JVM_Lseek(zip->fd, jint_to_jlong(zc->pos), SEEK_SET))
	== -1) {
	zip->msg = "seek failed";
	return NULL;
    }

    /* Allocate buffer for LOC header only */
    locbuf = (unsigned char *)malloc(LOCHDR);
    if (locbuf == 0) {
        zip->msg = "out of memory";
        return NULL;
    }

    /* Try to read in the LOC header */
    if (readFully(zip->fd, locbuf, LOCHDR) == -1) {
	zip->msg = "couldn't read LOC header";
	goto FREE_AND_RETURN_NULL;
    }

    /* Verify signature */
    if (GETSIG(locbuf) != LOCSIG) {
	zip->msg = "invalid LOC header (bad signature)";
        goto FREE_AND_RETURN_NULL;
    }

    /* nlen is name length in LOC */
    nlen = LOCNAM(locbuf);
    if (nlen < 0) {
        zip->msg = "invalid LOC header (bad name length)";
        goto FREE_AND_RETURN_NULL;
    }

    /* elen is extra data length in LOC */
    elen = LOCEXT(locbuf);
    if (elen < 0) {
        zip->msg = "invalid LOC header (bad extra data length)";
        goto FREE_AND_RETURN_NULL;
    }

    /* Allocate the entry to return */
    ze = (jzentry *)calloc(1, sizeof(jzentry));
    if (ze == NULL) {
	goto out_of_memory;
    }
    ze->name = (char *)malloc(nlen + 1);
    if (ze->name == NULL) {
	goto out_of_memory;
    }

    /* Read in the entry name and zero terminate it */
    if (readFully(zip->fd, ze->name, nlen) == -1) {
	zip->msg = "couldn't read name";
        goto FREE_AND_RETURN_NULL;
    }
    ze->name[nlen] = 0;

    /* If extra in CEN, use it instead of extra in LOC */
    if (zc->elen > 0) {
        jint off = CENHDR + zc->nelen - zc->elen + zc->cenpos;
        elen = zc->elen;
	ze->extra = (jbyte *)malloc(elen+2);
        if (ze->extra == NULL) {
            goto out_of_memory;
        }
	ze->extra[0] = (unsigned char)elen;
	ze->extra[1] = (unsigned char)(elen >> 8);

	/* Seek to begin of CEN header extra field */
        if (jlong_to_jint(JVM_Lseek(zip->fd, jint_to_jlong(off), SEEK_SET)) == -1) {
	    zip->msg = "seek failed";
            goto FREE_AND_RETURN_NULL;
	}
	/* Try to read in the CEN Extra */
	if (readFully(zip->fd, &ze->extra[2], elen) == -1) {
	    zip->msg = "couldn't read CEN extra";
            goto FREE_AND_RETURN_NULL;
	}
    }

    else if (LOCEXT(locbuf) != 0) {
        /* Allocate space for extra data plus two bytes for length */
	ze->extra = (jbyte *)malloc(elen + 2);
	if (ze->extra == NULL) {
	    goto out_of_memory;
	}
	/* Store the extra data size in the first two bytes */
	ze->extra[0] = (unsigned char)elen;
	ze->extra[1] = (unsigned char)(elen >> 8);

       	/* Try to read in the extra data */
	if (readFully(zip->fd, &ze->extra[2], elen) == -1) {
	    zip->msg = "couldn't read extra";
            goto FREE_AND_RETURN_NULL;
	}
    }

    /*
     * Process any comment (this should be very rare)
     */
    if (zip->comments) {
        /* 
         * Avoid defining a variable to hold the pointer difference.
         */
	ze->comment = zip->comments[zc - zip->entries];
    }
    /*
     * We'd like to initialize the sizes from the LOC, but unfortunately
     * some ZIPs, including the jar command, don't put them there.
     * So we have to store them in the szcell.
     */
    ze->size = zc->size;
    ze->csize = zc->csize;
    ze->crc = zc->crc;

    /* Fill in the rest of the entry fields from the LOC */
    ze->time = LOCTIM(locbuf);
    ze->pos = zc->pos + LOCHDR + LOCNAM(locbuf) + LOCEXT(locbuf);
    free(locbuf);
    return ze;

out_of_memory:
    zip->msg = "out of memory";

FREE_AND_RETURN_NULL:
    if (ze != NULL) {
	if (ze->extra != NULL)
	    free(ze->extra);
	if (ze->name != NULL)
	    free(ze->name);
	free(ze);
    }
    if (locbuf != NULL)
        free(locbuf);
    return NULL;
}

/*
 * Free the given jzentry.
 * In fact we maintain a one-entry cache of the most recently used
 * jzentry for each zip.  This optimizes a common access pattern.
 */

void
ZIP_FreeEntry(jzfile *jz, jzentry *ze)
{
    jzentry *last;
    ZIP_Lock(jz);
    last = jz->cache;
    jz->cache = ze;
    if (last != NULL) {
        /* Free the previously cached jzentry */
        if (last->extra) {
	    free(last->extra);
        }
        if (last->name) {
	    free(last->name);
        }
        free(last);
    }
    ZIP_Unlock(jz);
}

/*
 * Returns the zip entry corresponding to the specified name, or
 * NULL if not found.
 */
jzentry * ZIP_GetEntry(jzfile *zip, const char *name)
{
    unsigned int hsh = hash(name);
    int idx = zip->table[hsh % zip->tablelen];
    jzentry *ze;

    ZIP_Lock(zip);

    /* Check the cached entry first */
    ze = zip->cache;
    if (ze && strcmp(ze->name,name) == 0) {
	/* Cache hit!  Remove and return the cached entry. */
	zip->cache = 0;
        ZIP_Unlock(zip);
	return ze;
    }
    ze = 0;

    /*
     * Search down the target hash chain for a cell who's
     * 32 bit hash matches the hashed name.
     */
    while (idx != ZIP_ENDCHAIN) {
	jzcell *zc = &zip->entries[idx];

	if (zc->hash == hsh) {
	    /*
	     * OK, we've found a ZIP entry whose 32 bit hashcode
	     * matches the name we're looking for.  Try to read its
	     * entry information from the LOC.
	     * If the LOC name matches the name we're looking,
	     * we're done.  
	     * If the names don't (which should be very rare) we
             * keep searching.
	     */
	    ze = readLOC(zip, zc);
	    if (ze && strcmp(ze->name, name)==0) {
		break;
	    }
	    if (ze != 0) {
		/* We need to relese the lock across the free call */
	        ZIP_Unlock(zip);
		ZIP_FreeEntry(zip, ze);
	        ZIP_Lock(zip);
	    }
	    ze = 0;
	}
	idx = zc->next;
    }
    ZIP_Unlock(zip);
    return ze;
}

/*
 * Returns the n'th (starting at zero) zip file entry, or NULL if the
 * specified index was out of range.
 */
jzentry * JNICALL
ZIP_GetNextEntry(jzfile *zip, jint n)
{
    jzentry *result;
    if (n < 0 || n >= zip->total) {
	return 0;
    }
    ZIP_Lock(zip);
    result = readLOC(zip, &zip->entries[n]);
    ZIP_Unlock(zip);
    return result;
}

/*
 * Locks the specified zip file for reading.
 */
void ZIP_Lock(jzfile *zip)
{
    MLOCK(zip->lock);
}

/*
 * Unlocks the specified zip file.
 */
void ZIP_Unlock(jzfile *zip)
{
    MUNLOCK(zip->lock);
}


/*
 * Reads bytes from the specified zip entry. Assumes that the zip
 * file had been previously locked with ZIP_Lock(). Returns the
 * number of bytes read, or -1 if an error occurred. If err->msg != 0
 * then a zip error occurred and err->msg contains the error text.
 */
jint ZIP_Read(jzfile *zip, jzentry *entry, jint pos, void *buf, jint len)
{
    jint n, avail, size;
    /* Clear previous zip error */
    zip->msg = 0;
    /* Check specified position */
    size = entry->csize != 0 ? entry->csize : entry->size;
    if (pos < 0 || pos > size - 1) {
	zip->msg = "ZIP_Read: specified offset out of range";
	return -1;
    }
    /* Check specified length */
    if (len <= 0) {
	return 0;
    }
    avail = size - pos;
    if (len > avail) {
	len = avail;
    }

    /* Seek to beginning of entry data and read bytes */
    n = jlong_to_jint(JVM_Lseek(zip->fd, jint_to_jlong(entry->pos + pos),
				SEEK_SET));
    if (n != -1) {
	n = (jint)JVM_Read(zip->fd, buf, len);
    }

    return n;
}

/*
 * Converts DOS (ZIP) time to UNIX time.
 */
jint ZIP_DosToUnixTime(jint dtime)
{
    struct tm tm;

    tm.tm_sec  = (dtime << 1) & 0x3E;
    tm.tm_min  = (dtime >> 5) & 0x3F;
    tm.tm_hour = (dtime >> 11) & 0x1F;
    tm.tm_mday = (dtime >> 16) & 0x1F;
    tm.tm_mon  = ((dtime >> 21) & 0x0F) - 1;
    tm.tm_year = ((dtime >> 25) & 0x7F) + 1980;

    /*
     * This is going to break in 2038 ;-)
     */
    return (jint)mktime(&tm);
}

/* The maximum size of a stack-allocated buffer.
 */
#define BUF_SIZE 4096

/*
 * This function is used by the runtime system to load compressed entries
 * from ZIP/JAR files specified in the class path. It is defined here
 * so that it can be dynamically loaded by the runtime if the zip library
 * is found.
 */
jboolean
InflateFully(jzfile *zip, jzentry *entry, void *buf, char **msg)
{
    z_stream strm;
    char tmp[BUF_SIZE];
    jint pos = 0, count = entry->csize;

    *msg = NULL; /* Reset error message */

    if (count == 0) {
	*msg = "inflateFully: entry not compressed";
	return JNI_FALSE;
    }

    memset(&strm, 0, sizeof(z_stream));
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
	if (strm.msg != NULL) {
	    *msg = strm.msg;
	} else {
	    *msg = "inflateFully: inflateInit2 error";
	}
        return JNI_FALSE;
    }

    strm.next_out = (Bytef *)buf;
    strm.avail_out = entry->size;

    while (count > 0) {
	jint n = count > (jint)sizeof(tmp) ? (jint)sizeof(tmp) : count;
	ZIP_Lock(zip);
	n = ZIP_Read(zip, entry, pos, tmp, n);
	ZIP_Unlock(zip);
	if (n == 0) {
	    *msg = "inflateFully: Unexpected end of file";
	    return JNI_FALSE;
	}
	if (n < 0) {
	    *msg = "inflateFully: ZIP_Read error";
	    return JNI_FALSE;
	}
	pos += n;
	count -= n;
	strm.next_in = (Bytef *)tmp;
	strm.avail_in = n;
	do {
	    switch (inflate(&strm, Z_PARTIAL_FLUSH)) {
	    case Z_OK:
		break;
	    case Z_STREAM_END:
		if (count != 0 || strm.total_out != entry->size) {
		    inflateEnd(&strm);
		    *msg = "inflateFully: Unexpected end of stream";
		    return JNI_FALSE;
		}
		break;
	    case Z_MEM_ERROR:
		inflateEnd(&strm);
		*msg = "inflateFully: Out of memory";
		return JNI_FALSE;
	    default:
		break;
	    }
	} while (strm.avail_in > 0);
    }
    inflateEnd(&strm);

    return JNI_TRUE;
}

jzentry * JNICALL
ZIP_FindEntry(jzfile *zip, const char *name, jint *sizeP, jint *nameLenP)
{
    jzentry *entry = ZIP_GetEntry(zip, name);
    if (entry) {
        *sizeP = entry->size;
	*nameLenP = (jint)strlen(entry->name);
    }
    return entry;
}

/*
 * Reads a zip file entry into the specified byte array
 * When the method completes, it release the jzentry.
 * Note: this is called from the separately delivered VM (hotspot/classic)
 * so we have to be careful to maintain the expected behaviour.
 */
jboolean JNICALL
ZIP_ReadEntry(jzfile *zip, jzentry *entry, unsigned char *buf, char *entryname)
{
    char *msg;

    strcpy(entryname, entry->name);
    if (entry->csize == 0) {
	/* Entry is stored */
	jint pos = 0, count = entry->size;
	while (count > 0) {
	    jint n;
	    ZIP_Lock(zip);
	    n = ZIP_Read(zip, entry, pos, buf, count);
	    msg = zip->msg;
	    ZIP_Unlock(zip);
	    if (n == -1) {
		jio_fprintf(stderr, "%s: %s\n", zip->name,
			    msg != 0 ? msg : strerror(errno));
		return JNI_FALSE;
	    }
	    buf += n;
	    pos += n;
	    count -= n;
	}
    } else {
	/* Entry is compressed */
	if (!InflateFully(zip, entry, buf, &msg)) {
	    if ((msg == NULL) || (*msg == 0)) {
		msg = zip->msg;
	    }
	    jio_fprintf(stderr, "%s: %s\n", zip->name,
			msg != 0 ? msg : strerror(errno));
	    return JNI_FALSE;
	}
    }

    ZIP_FreeEntry(zip, entry);

    return JNI_TRUE;
}
