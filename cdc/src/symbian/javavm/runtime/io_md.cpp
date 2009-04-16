/*
 * @(#)io_md.cpp	1.14 06/10/10
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
 *
 */

extern "C" {
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"
}
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <assert.h>
#include <utf.h>
#include <f32file.h>

static int init_done = 0;
static RFs fserv;
static int SYMBIANfileServerInit();

class RMyFile {
public:
    RFile &getRFile() { return f; }
    void setOpened(int o) { opened = o; }
    int isOpened() { return opened; }
    void setAppend(int a) { append = a; }
    int isAppend() { return append; }
    TInt getPos() { return pos; }
    void setPos(TInt p) { pos = p; }
    void lock() { cs.Wait(); }
    void unlock() { cs.Signal(); }
    TInt Create() { return cs.CreateLocal(); }
    void Close() {
	setOpened(0);
	f.Close();
	cs.Close();
    }
private:
    RFile f;
    RCriticalSection cs;
    int opened;
    int append;
    TInt pos;
};

static int fds;
static RMyFile *fdTable;
static RCriticalSection cs;

static TInt
openCommon(RMyFile &f, const char *name, CVMInt32 openMode, CVMInt32 filePerm);
static TInt
SYMBIANseek(RMyFile &f, TInt pos, CVMInt32 whence);

CVMInt32
CVMioOpen(const char *name, CVMInt32 openMode, CVMInt32 filePerm)
{
    if (!init_done) {
	if (!SYMBIANfileServerInit()) {
	    return -1;
	}
    }
    cs.Wait();
    int fd = -1;
    int i;
    for (i = 3; i < fds; ++i) {
	if (!fdTable[i].isOpened()) {
	    fdTable[i].setOpened(1);
	    fd = i;
	    break;
	}
    }
    cs.Signal();
    if (fd == -1) {
	return -1;
    }
    TInt err = openCommon(fdTable[fd], name, openMode, filePerm);
    if (err == KErrNone) {
	return fd;
    } else {
	return -1;
    }
}

CVMInt32
CVMioClose(CVMInt32 fd)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    if (fd > 2) {
	cs.Wait();
	RMyFile &f = fdTable[fd];
	if (!f.isOpened()) {
	    cs.Signal();
	    return -1;
	}
	f.Close();
	cs.Signal();
	return 0;
    } else {
	return -1;
    }
}

CVMInt32
CVMioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    if (fd <= 2) {
	return read(fd, buf, nBytes);
    }
    RMyFile &f = fdTable[fd];
    if (!f.isOpened()) {
	return -1;
    }
    if (f.getPos() != -1) {
	return -1;
    }
    f.lock();
    TPtr8 des((TUint8 *)buf, nBytes);
    TInt err = f.getRFile().Read(des);
    f.unlock();
    if (err != KErrNone) {
	return -1;
    }
    return des.Length();
}

CVMInt32
CVMioWrite(CVMInt32 fd, const void *buf, CVMUint32 nBytes)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    CVMInt32 ret = -1;
    RMyFile &f = fdTable[fd];
    int locked = 0;
    TPtrC8 des((const TUint8 *)buf, nBytes);
    TInt err;
    TInt pos;
    if (!f.isOpened()) {
	goto err;
    }
    f.lock();
    locked = 1;
    if (f.isAppend()) {
	SYMBIANseek(f, 0, SEEK_END);
    }
    pos = f.getPos();
    if (pos != -1) {
	TInt sz;
	err = f.getRFile().Size(sz);
	assert(err == KErrNone);
	if (pos > sz) {
	    err = f.getRFile().SetSize(pos);
	    if (err != KErrNone) {
		goto err;
	    }
	}
	err = f.getRFile().Seek(ESeekStart, pos);
	if (err != KErrNone) {
	    goto err;
	}
	TInt npos = 0;
	err = f.getRFile().Seek(ESeekCurrent, npos);
	if (err != KErrNone) {
	    goto err;
	}
	if (npos != pos) {
	    goto err;
	}
    }
    err = f.getRFile().Write(des);
    if (err != KErrNone) {
	goto err;
    }
    f.setPos(-1);
    ret = nBytes;

err:
    if (fd <= 2) {
	int r = write(fd, buf, nBytes);
	if (locked) {
	    f.unlock();
	}
	return r;
    }
    f.unlock();
    return ret;
}

CVMInt64
CVMioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    RMyFile &f = fdTable[fd];
    if (!f.isOpened()) {
	return -1;
    }
    TInt pos = offset;
    // Check for integer overflow
    if (pos != offset) {
	return -1;
    }
    f.lock();
    TInt r = SYMBIANseek(f, pos, whence);
    f.unlock();
    return r;
}

static TInt
SYMBIANseek(RMyFile &f, TInt pos, CVMInt32 whence)
{
    TInt ret = -1;
    TInt err;
    TInt cur = f.getPos();
    TInt end = 0;
    TSeek mode = (TSeek)0;
    TInt npos = 0;
    if (cur == -1) {
	cur = 0;
	err = f.getRFile().Seek(ESeekCurrent, cur);
	if (err != KErrNone) {
	    goto done;
	}
    }
    err = f.getRFile().Seek(ESeekEnd, end);
    if (err != KErrNone) {
	goto done;
    }
    switch (whence) {
    case SEEK_CUR:
	pos += cur;
	break;
    case SEEK_END:
	pos += end;
	break;
    case SEEK_SET:
	break;
    default:
	goto done;
    }
    err = f.getRFile().Seek(ESeekStart, pos);
    if (err != KErrNone) {
	goto done;
    }
    err = f.getRFile().Seek(ESeekCurrent, npos);
    if (err != KErrNone) {
	goto done;
    }
    assert(npos == pos || pos > end);
    if (pos > end) {
	assert(npos == end);
	f.setPos(pos);
    } else {
	f.setPos(-1);
    }
    ret = pos;
done:
    return ret;
}

CVMInt32
CVMioSetLength(CVMInt32 fd, CVMInt64 length)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    TInt sz = length;
    // Check for integer overflow
    if (sz != length) {
	return -1;
    }
    RMyFile &f = fdTable[fd];
    if (!f.isOpened()) {
	return -1;
    }
    f.lock();
    TInt err = f.getRFile().SetSize(sz);
    if (err == KErrNone) {
	TInt sz;
	TInt err2 = f.getRFile().Size(sz);
	TInt pos = 0;
	err = f.getRFile().Seek(ESeekEnd, pos);
	assert(err2 == KErrNone);
	assert(err == KErrNone);
	assert(pos == sz);
#ifdef CVM_DEBUG
fprintf(stderr, "fd %d length --> %d\n", fd, sz);
#endif
	pos = f.getPos();
	if (pos != -1) {
	    if (pos <= sz) {
		f.setPos(-1);
	    }
	}
	f.unlock();
	return 0;
    } else {
	f.unlock();
	return -1;
    }
}

CVMInt32
CVMioSync(CVMInt32 fd)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    RMyFile &f = fdTable[fd];
    if (!f.isOpened()) {
	return -1;
    }
    TInt err = f.getRFile().Flush();
    if (err == KErrNone) {
	return 0;
    } else {
	return -1;
    }
}

CVMInt32
CVMioAvailable(CVMInt32 fd, CVMInt64 *bytes)
{
    if (fd < 0 || fd >= fds) {
	return 0;
    }
    CVMInt64 cur, end;
    struct stat buf;

    if (fd < 3 && fstat(fd, &buf) >= 0) {
	int mode = buf.st_mode;
        if (S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
	    int n;
            if (ioctl(fd, E32IONREAD, &n) >= 0) {
		/* %comment d003 */
		*bytes = CVMint2Long(n);
                return 1;
            }
        }
    }
    {
	CVMInt32 cur32, end32;
	if ((cur32 = CVMioSeek(fd, 0L, SEEK_CUR)) == -1) {
	    return 0;
	} else if ((end32 = CVMioSeek(fd, 0L, SEEK_END)) == -1) {
	    return 0;
	} else if (CVMioSeek(fd, cur32, SEEK_SET) == -1) {
	    return 0;
	}
	/* %comment d003 */
	cur = CVMint2Long(cur32);
	end = CVMint2Long(end32);
    }
    *bytes = CVMlongSub(end, cur);
    return 1;
}

CVMInt32
CVMioFileSizeFD(CVMInt32 fd, CVMInt64 *size)
{
    if (fd < 0 || fd >= fds) {
	return -1;
    }
    TInt sz;
    RMyFile &f = fdTable[fd];
    if (!f.isOpened()) {
	return -1;
    }
    TInt err = f.getRFile().Size(sz);
    if (err == KErrNone) {
	*size = sz;
	return 0;
    } else {
	return -1;
    }
}

char *
CVMioNativePath(char *path)
{
    return path;
}

CVMInt32
CVMioFileType(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0) {
	mode_t mode = st.st_mode & S_IFMT;
	if (mode == S_IFREG) return CVM_IO_FILETYPE_REGULAR;
	if (mode == S_IFDIR) return CVM_IO_FILETYPE_DIRECTORY;
	return CVM_IO_FILETYPE_OTHER;
    }
    return -1;
}

CVMInt32
CVMioGetLastErrorString(char *buf, CVMInt32 len)
{
    return -1;
}

char *
CVMioReturnLastErrorString()
{
    return NULL;
}

static int
SYMBIANfileServerInit()
{
    if (!init_done) {
	cs.Wait();
	if (!init_done) {
	    TInt err = fserv.Connect();
	    if (err != KErrNone) {
		fprintf(stderr, "RFs.Connect failed with %d\n",
		    err);
		cs.Signal();
		return 0;
	    }
#ifdef EKA2
	    if (fserv.ShareAuto() != KErrNone) {
#else
	    if (fserv.Share(RSessionBase::EAutoAttach) != KErrNone) {
#endif
		cs.Signal();
		return 0;
	    }
	    init_done = 1;
	}
	cs.Signal();
    }
    return 1;
}

int
SYMBIANioInit()
{
#ifdef __MARM_ARMV5__
    char *outfile = "E:\\out.txt";
    char *errfile = "E:\\err.txt";
#else
    char *outfile = "J:\\out.txt";
    char *errfile = "J:\\err.txt";
#endif
    if (!SYMBIANfileServerInit()) {
	return 0;
    }
    fds = 256;
    fdTable = (RMyFile *)malloc(fds * sizeof fdTable[0]);
    if (fdTable == NULL) {
	return 0;
    }
    if (cs.CreateLocal() != KErrNone) {
	free(fdTable);
	fdTable = NULL;
	return 0;
    }
    memset(fdTable, 0, fds * sizeof fdTable[0]);
    int i;
    for (i = 0; i < fds; ++i) {
	(void) new (&fdTable[i]) RMyFile();
    }

    TInt err = openCommon(fdTable[1], outfile,
	O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (err != KErrNone) {
	return 0;
    }
    err = openCommon(fdTable[2], errfile,
	O_WRONLY|O_CREAT|O_TRUNC, 0600);
    if (err != KErrNone) {
	fdTable[1].Close();
	return 0;
    }
    fdTable[1].setOpened(1);
    fdTable[2].setOpened(1);
    return 1;
}

void
SYMBIANioDestroy()
{
    cs.Close();
}

static TInt
openCommon(RMyFile &f, const char *name, CVMInt32 openMode,
    CVMInt32 /* filePerm */)
{
    TInt err = f.Create();
    if (err != KErrNone) {
	return 0;
    }
    TUint mode = EFileShareAny;
    switch (openMode & (O_RDONLY|O_RDWR|O_WRONLY)) {
    case O_RDWR:
    case O_WRONLY:
	mode |= EFileWrite;
	break;
    case O_RDONLY:
	mode |= EFileRead;
	break;
    default:
	assert(0);
	;
    }

    TPtrC8 path8((const TUint8 *)name, strlen(name));
#ifdef _UNICODE
    TBuf16<KMaxFileName> path16;
    CnvUtfConverter::ConvertToUnicodeFromUtf8(path16, path8);
#define path path16
#else
#define path path8
#endif
    RFile &rf = f.getRFile();
    // Convert multiple slashes into single slashes.
    // Should be done in an upper layer.
    _LIT(sslash, "\\\\");
    assert(((const TDesC &)sslash).Length() == 2);
    TInt pos;
    while ((pos = path.Find(sslash)) != KErrNotFound) {
	path.Delete(pos, 1);
    }
    if ((openMode & (O_CREAT|O_TRUNC)) == (O_CREAT|O_TRUNC)) {
	err = rf.Replace(fserv, path, mode);
    } else {
	err = rf.Open(fserv, path, mode);
	if (err != KErrNone && (openMode & O_CREAT) != 0) {
	    err = rf.Create(fserv, path, mode);
	}
    }
    if (err != KErrNone) {
	f.setOpened(0);
    } else {
	if ((openMode & O_TRUNC) != 0) {
	    rf.SetSize(0);
	}
	f.setPos(-1);
	f.setAppend((openMode & O_APPEND) != 0);
    }
    return err;
}
