/*
 * @(#)hprof_md.c	1.26 05/12/06
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


#include "javavm/include/jni_md.h"
#include "jni.h"
#include "jvmti_hprof.h"

#ifdef WINCE
#define FILETAB_SIZE 32
static FILE* filetab[FILETAB_SIZE];
static BOOL filetab_initted = FALSE;

static void init_filetab()
{
    int i;
    for (i = 0; i < FILETAB_SIZE; i++) {
        filetab[i] = NULL;
    }
}

static FILE* GETFILE(int fd)
{
    if (fd < 0 || fd >= FILETAB_SIZE)
        return NULL;
    return filetab[fd];
}

static int file_to_fd(FILE *fp)
{
    int i;
    if (!filetab_initted) {
        init_filetab();
        filetab_initted = TRUE;
    }
    for (i = 0; i < FILETAB_SIZE; i++) {
        if (filetab[i] == NULL) {
            filetab[i] = fp;
            return i;
        }
    }
    return -1;
}

#define GETPID()        GetCurrentProcessId()


void
abort(void)
{
    TerminateProcess(GetCurrentProcess(), -1);
}

#else
#define GETFILE(x) x
#define GETPID()        getpid()
#endif

int 
md_getpid(void)
{
    static int pid = -1;

    if ( pid >= 0 ) {
	return pid;
    }
    pid = GETPID();
    return pid;
}

void 
md_sleep(unsigned seconds)
{
    Sleep((DWORD)seconds*1000);
}

void
md_init(void)
{
}

int
md_connect(char *hostname, unsigned short port)
{
    struct hostent *hentry;
    struct sockaddr_in s;
    int fd;

    /* create a socket */
    fd = (int)socket(AF_INET, SOCK_STREAM, 0);

    /* find remote host's addr from name */
    if ((hentry = gethostbyname(hostname)) == NULL) {
        return -1;
    }
    (void)memset((char *)&s, 0, sizeof(s));
    /* set remote host's addr; its already in network byte order */
    (void)memcpy(&s.sin_addr.s_addr, *(hentry->h_addr_list),
           (int)sizeof(s.sin_addr.s_addr));
    /* set remote host's port */
    s.sin_port = htons(port);
    s.sin_family = AF_INET;

    /* now try connecting */
    if (-1 == connect(fd, (struct sockaddr*)&s, sizeof(s))) {
        return 0;
    }
    return fd;
}

int
md_recv(int f, char *buf, int len, int option)
{
    return recv(f, buf, len, option);
}

int
md_shutdown(int filedes, int option)
{
    return shutdown(filedes, option);
}

int 
md_open(const char *filename)
{
#ifdef WINCE
    FILE *fp = fopen(filename, "rb");
    if (fp != NULL) {
	return file_to_fd(fp);
    } else {
	return -1;
    }
#else
    return open(filename, O_RDONLY);
#endif
}

int 
md_open_binary(const char *filename)
{
    return md_open(filename);
}

int 
md_creat(const char *filename)
{
#ifdef WINCE
    FILE *fp = fopen(filename, "wb");
    if (fp != NULL) {
	return file_to_fd(fp);
    } else {
	return -1;
    }
#else
    return open(filename, O_CREAT | O_WRONLY | O_TRUNC, 
			     _S_IREAD | _S_IWRITE);
#endif
}

int 
md_creat_binary(const char *filename)
{
    return md_open(filename);
}

jlong
md_seek(int filedes, jlong pos)
{
#ifdef WINCE
    long lpos = (unsigned int)pos;
    if (lpos == -1) {
	return fseek(GETFILE(filedes), 0, SEEK_END);
    } else {
	return fseek(GETFILE(filedes), lpos, SEEK_SET);
    }
#else
    jlong new_pos;

    if ( pos == (jlong)-1 ) {
	new_pos = _lseeki64(filedes, 0L, SEEK_END);
    } else {
	new_pos = _lseeki64(filedes, pos, SEEK_SET);
    }
    return new_pos;
#endif
}

void
md_close(int filedes)
{
#ifdef WINCE
    fclose(GETFILE(filedes));
    filetab[filedes] = NULL;
#else
    (void)close(filedes);
#endif
}

int 
md_send(int s, const char *msg, int len, int flags)
{
    return send(s, msg, len, flags);
}

int 
md_read(int filedes, void *buf, int nbyte)
{
#ifdef WINCE
    return fread(buf, 1, nbyte, GETFILE(filedes));
#else
    return read(filedes, buf, nbyte);
#endif
}

int 
md_write(int filedes, const void *buf, int nbyte)
{
#ifdef WINCE
    int ret;
    ret = fwrite(buf, 1, nbyte, GETFILE(filedes));
    fflush(GETFILE(filedes));
    return ret;
#else
    return write(filedes, buf, nbyte);
#endif
}

#define FT2INT64(ft) \
        ((jlong)(ft).dwHighDateTime << 32 | (jlong)(ft).dwLowDateTime)

jlong 
md_get_microsecs(void)
{
#ifndef WINCE
    static CVMInt64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;

    if (fileTime_1_1_70 == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);
    }

    GetSystemTime(&st0);
    SystemTimeToFileTime(&st0, &ft0);

    return (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
#else 
    CVMInt64 fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;
    static CVMInt64 originTick = 0;
    CVMInt64 ttt;

    if (originTick == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
         * 1/1/70.
         */

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
        fileTime_1_1_70 = FT2INT64(ft0);

        GetSystemTime(&st0);
        SystemTimeToFileTime(&st0, &ft0);
        originTick = (FT2INT64(ft0) - fileTime_1_1_70) / 10000;
        originTick -= GetTickCount(); 
    }

    ttt = GetTickCount() + originTick;
    return ttt;
#endif
}

#define FT2JLONG(ft) \
	((jlong)(ft).dwHighDateTime << 32 | (jlong)(ft).dwLowDateTime)

jlong 
md_get_timemillis(void)
{
    static jlong fileTime_1_1_70 = 0;
    SYSTEMTIME st0;
    FILETIME   ft0;

    if (fileTime_1_1_70 == 0) {
        /* Initialize fileTime_1_1_70 -- the Win32 file time of midnight
	 * 1/1/70.
         */ 

        memset(&st0, 0, sizeof(st0));
        st0.wYear  = 1970;
        st0.wMonth = 1;
        st0.wDay   = 1;
        SystemTimeToFileTime(&st0, &ft0);
	fileTime_1_1_70 = FT2JLONG(ft0);
    } 

    GetSystemTime(&st0);
    SystemTimeToFileTime(&st0, &ft0);

    return (FT2JLONG(ft0) - fileTime_1_1_70) / 10000;
}

jlong 
md_get_thread_cpu_timemillis(void)
{
    return md_get_timemillis();
}

HINSTANCE hJavaInst;
static int nError = 0;

BOOL WINAPI
DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    WSADATA wsaData;
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            hJavaInst = hinst;
            nError = WSAStartup(MAKEWORD(2,0), &wsaData);
            break;
        case DLL_PROCESS_DETACH:
            WSACleanup();
            hJavaInst = NULL;
        default:
            break;
    }
    return TRUE;
}

void 
md_get_prelude_path(char *path, int path_len, char *filename)
{

    _TCHAR libdir[FILENAME_MAX+1];
    _TCHAR *lastSlash;
    char * mb_libdir[FILENAME_MAX+1];
    size_t *len;

    GetModuleFileName(hJavaInst, libdir, FILENAME_MAX);

    /* This is actually in the bin directory, so move above bin for lib */
    lastSlash = _tcsrchr(libdir, '\\');
    if ( lastSlash != NULL ) {
	*lastSlash = '\0';
    }
    lastSlash = _tcsrchr(libdir, '\\');
    if ( lastSlash != NULL ) {
	*lastSlash = '\0';
    }
    WideCharToMultiByte(CP_ACP, 0, libdir, FILENAME_MAX+1, (LPSTR)mb_libdir,
			FILENAME_MAX, NULL, NULL);
    (void)md_snprintf(path, path_len, "%s\\lib\\%s", mb_libdir, filename);
}

int     
md_vsnprintf(char *s, int n, const char *format, va_list ap)
{
    return _vsnprintf(s, n, format, ap);
}

int     
md_snprintf(char *s, int n, const char *format, ...)
{
    int ret;
    va_list ap;

    va_start(ap, format);
    ret = md_vsnprintf(s, n, format, ap);
    va_end(ap);
    return ret;
}

void
md_system_error(char *buf, int len)
{
    long errval;
    
    errval = GetLastError();
    buf[0] = '\0';
    if (errval != 0) {
        int n;
#ifdef WINCE
        n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			  FORMAT_MESSAGE_IGNORE_INSERTS,
			  NULL, errval,
			  0, (LPWSTR)buf, len, NULL);
#else
        n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
			   FORMAT_MESSAGE_IGNORE_INSERTS,
			   NULL, errval,
			   0, buf, len, NULL);
#endif
        if (n > 3) {
            /* Drop final '.', CR, LF */
            if (buf[n - 1] == '\n') n--;
            if (buf[n - 1] == '\r') n--;
            if (buf[n - 1] == '.') n--;
            buf[n] = '\0';
        }
    }
}

unsigned
md_htons(unsigned short s)
{
    return htons(s);
}

unsigned
md_htonl(unsigned l)
{
    return htonl(l);
}

unsigned        
md_ntohs(unsigned short s)
{
    return ntohs(s);
}

unsigned 
md_ntohl(unsigned l)
{
    return ntohl(l);
}

static int
get_last_error_string(char *buf, int len)
{
    long errval;

    errval = GetLastError();
    if (errval != 0) {
	/* DOS error */
	int n;
#ifdef WINCE
	n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
			  FORMAT_MESSAGE_IGNORE_INSERTS,
			  NULL, errval,
			  0, (LPWSTR)buf, len, NULL);
#else
	n = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
			   FORMAT_MESSAGE_IGNORE_INSERTS,
			   NULL, errval,
			   0, buf, len, NULL);
#endif
	if (n > 3) {
	    /* Drop final '.', CR, LF */
	    if (buf[n - 1] == '\n') n--;
	    if (buf[n - 1] == '\r') n--;
	    if (buf[n - 1] == '.') n--;
	    buf[n] = '\0';
	}
	return n;
    }

    if (errno != 0) {
	/* C runtime error that has no corresponding DOS error code */
	const char *s;
	int         n;
	
	s = strerror(errno);
	n = (int)strlen(s);
	if (n >= len) {
	    n = len - 1;
	}
	(void)strncpy(buf, s, n);
	buf[n] = '\0';
	return n;
    }

    return 0;
}

/* Build a machine dependent library name out of a path and file name.  */
void
md_build_library_name(char *holder, int holderlen, char *pname, char *fname)
{
    int   pnamelen;
    char  c;
    char *suffix;

#ifdef DEBUG   
    suffix = "_g";
#else
    suffix = "";
#endif 

    pnamelen = pname ? (int)strlen(pname) : 0;
    c = (pnamelen > 0) ? pname[pnamelen-1] : 0;

    /* Quietly truncates on buffer overflow. Should be an error. */
    if (pnamelen + strlen(fname) + 10 > (unsigned int)holderlen) {
        *holder = '\0';
        return;
    }

    if (pnamelen == 0) {
        sprintf(holder, "%s%s%s", JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    } else if (c == ':' || c == '\\') {
        sprintf(holder, "%s%s%s%s%s", pname, JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    } else {
        sprintf(holder, "%s\\%s%s%s%s", pname, JNI_LIB_PREFIX, fname, suffix,
		JNI_LIB_SUFFIX);
    }
}

void *
md_load_library(const char * name, char *err_buf, int err_buflen)
{
    HANDLE result;
#ifdef UNICODE
    WCHAR *wc;
    char *pathName = (char *)name;

    wc = createWCHAR(pathName);
    result = LoadLibrary(wc);
#else
    char *wc = (char *)name;
    result = LoadLibraryA(wc);
#endif

    if (result == NULL) {
	/* Error message is pretty lame, try to make a better guess. */
	long errcode;
	
	errcode = GetLastError();
	if (errcode == ERROR_MOD_NOT_FOUND) {
	    strncpy(err_buf, "Can't find dependent libraries", err_buflen-2);
	    err_buf[err_buflen-1] = '\0';
	} else {
	    get_last_error_string(err_buf, err_buflen);
	}
    }
#ifdef UNICODE
    free(wc);
#endif
    return result;
}

void 
md_unload_library(void *handle)
{
    FreeLibrary(handle);
}

void * 
md_find_library_entry(void *handle, const char *name)
{
#ifdef WINCE
    return GetProcAddressA(handle, (LPCSTR)name);
#else
    return GetProcAddress(handle, name);
#endif
}

#ifdef WINCE
void    signal(int sig, void *func)
{
}

int
remove(char *old_name) {
    return !DeleteFile((LPCTSTR)old_name);
}
#endif
