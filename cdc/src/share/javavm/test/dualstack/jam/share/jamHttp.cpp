/*
 * @(#)jamHttp.cpp	1.17 06/10/10 10:08:35
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

#include "jam.hpp"

#ifdef _WIN32_
#include <winsock.h>
#endif

#ifdef _UNIX_
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static void closesocket(int fd) {
    shutdown(fd, 2);
    close(fd);
}

#endif

#ifdef __SYMBIAN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

typedef size_t socklen_t;

static void closesocket(int fd) {
    shutdown(fd, 2);
    close(fd);
    printf("[closed socket %d]", fd);
}

#endif

#define DO_SEND(socket, buf, len, opt) { \
  int _retries = 5, _retval; \
  do { \
    _retval = send(socket, buf, len, opt); \
     if (_retval < 0) { \
       PlafStatusMsg("send failed: %d"); \
     }\
   } while (_retval < 0 && _retries-- > 0); \
  } 

#define RECV_TMO 2000

/*=========================================================================
 * Functions
 *=======================================================================*/

static int
parseURL(const char* url, char* host, int* port, char* path);

static char *
fetchURL(const char *host, int port, const char *path,
         int *httpCodeP, int *contentLengthP, int *retryDelayP,
	 int complain);

static char *
postURL(const char *host, int port, const char *path,
	int len, char* data,
	int *httpCodeP, int *contentLengthP, int *retryDelayP,
	int complain);

/*
 * Decompose a URL into host, port and part parts. Note that
 * the query string is not supported. Only "http://" URLs are supported.
 *
 * host and path must be of size MAX_URL.
 *
 * Returns TRUE if parse is successful, FALSE if it failed.
 */
static int
parseURL(const char* url, char* host, int* port, char* path)
{
    char* p;
    char* start;

    if (strncmp(url, "http://", 7) != 0) {
        PlafErrorMsg("bad protocol, must be http://\n");
        return FALSE;
    }

    /*
     * Get host name.
     */
    start = p = (char*)url + 7;
    while (*p && (isalnum((int)*p) || *p == '.' || *p == '-'|| *p == '_')) {
        p++;
    }

    if (p - start >= MAX_BUF) {
        PlafErrorMsg("bad host name: too long\n");
        return FALSE;
    }

    strnzcpy(host, start, p-start);

    /*
     * Get port number
     */
    if (*p == ':') {
        char num[10];

        p++;
        start = p;
        while (isdigit((int)*p)) {
            p++;
        }

        if ((start - p) > 5) {
            PlafErrorMsg("bad port number\n");
            return FALSE;
        }

        strnzcpy(num, start, p-start);
        *port = atoi(num);
        if (*port <= 0 || *port >= 65535) {
            PlafErrorMsg("bad port number %d\n", *port);
            return FALSE;
        }
    } else {
        *port = 80;
    }

    /*
     * Get path
     */
    if (*p == 0) {
        strcpy(path, "/");
    } else if (*p != '/') {
        PlafErrorMsg("bad path: must start with \"/\"\n");
    } else if (strlen(p) >= MAX_BUF) {
        PlafErrorMsg("bad path: too long\n");
    } else {
        strcpy(path, p);
    }
    return TRUE;
}

/*
 * Downloads the content of the given URL. The content is returned in
 * a malloc'ed buffer. NULL is returned in case of error.
 *
 * Implement HTTP policy for retransmissions, etc. ...
 */
char*
JamHttpGet(const char* url, int* contentLengthP, int complain)
{
    int port;
    char host[MAX_URL];
    char path[MAX_URL];
    if (!parseURL(url, host, &port, path)) {
        return FALSE;
    }

    for(;;) {
        int httpCode, retryDelay = 0;
        char *content = fetchURL(host, port, path,
                                 &httpCode, contentLengthP, &retryDelay,
				 complain);
        if (httpCode == 200) { 
            return content;
        }
        /* In all other cases, the content is NULL and should be ignored */
        if (httpCode == 503) { 
            if (retryDelay > 0) { 
                PlafSleep(retryDelay*1000);
            }
            continue;
        } 
        return NULL;
    }
}

char *
JamHttpPost(const char* url, 
	    int buflen, char* buf, 
	    int *contentLengthP, 
	    int complain)
{
  int port;
  char host[MAX_URL];
  char path[MAX_URL];
  if (!parseURL(url, host, &port, path)) {
      return FALSE;
  }
  
  for(;;) {
    int httpCode, retryDelay = 0;
    char *content = postURL(host, port, path,
			    buflen, buf,
			    &httpCode, contentLengthP, &retryDelay,
			    complain);
    if (httpCode == 200) { 
      return content;
    }
    /* In all other cases, the content is NULL and should be ignored */
    if (httpCode == 503) { 
      if (retryDelay > 0) { 
	PlafSleep(retryDelay*1000);
      }
      continue;
    } 
    return NULL;
  }
  return NULL;
}

static char *
fetchURL(const char *host, int port, const char *path,
         int *httpCodeP, int *contentLengthP, int *retryDelayP,
	 int complain)
{
    int sock = -1;
    char buffer[1024]; /* we reuse this for sending and receiving the data */
    unsigned int length, contentLength;
    char *p;
    char *content = NULL;
    int readBody;
    int retcode;
    
    /* Let's set the return args to default values */
    *httpCodeP = 404;           /* unable to make a connection */
    *contentLengthP = 0;
    *retryDelayP = 0;
    contentLength = 0;
    
    /* Open the socket and connect to the server */
    {
        struct hostent* hep;
        struct sockaddr_in sin;

        memset((void*)&sin, 0, sizeof (sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons((short)port);

        hep = gethostbyname(host);
        if (hep == NULL) {
            if (complain) 
	      PlafErrorMsg("Unable to resolve host name %s\n", host);
            return NULL;
        }
        memcpy(&sin.sin_addr, hep->h_addr, hep->h_length);
        sock = socket(PF_INET, SOCK_STREAM, 0);    
        printf("[opened socket %d]", sock);
        retcode = connect(sock, (struct sockaddr*)&sin, sizeof (sin));
        if (retcode < 0) {
	  if (complain) 
            PlafErrorMsg("Unable to connect to %s:%ld\n", 
			 host, (long)port);
	  goto error;
        }
    }

    //ret

    sprintf(buffer, "GET %s HTTP/1.0\r\n\r\n", path);

    DO_SEND(sock, buffer, strlen(buffer), 0);   

    /*
     * Read as much as we can into "buffer", upto the size of buffer.
     * Note that the size of the HTTP header need to be smaller than 1024
     * bytes, or else we'd be in trouble.
     */
    memset(buffer, 0, sizeof(buffer));
    for (length = 0; length < sizeof(buffer); ) {
        retcode = recv(sock, buffer + length, sizeof(buffer) - length, 0);
        if (retcode > 0) { 
            length += retcode;
        } else if (retcode == 0) { 
            /* EOF */
            break;
        } else { 
	  if (complain) 
            PlafErrorMsg("Error reading socket\n");
	  goto error;
        } 
    } 
    if (length <= 5) { 
        goto error;
    }
    
    /* We're assuming that the buffer is long enough to hold the complete
     * header.  If not, well something is wrong. 
     */
    p = strchr(buffer, ' ') + 1;        /* Skip past the first space */
    while (*p == ' ') p++;              /* Skip additional space */
    *httpCodeP = atoi(p);
    switch(*httpCodeP) {
        case 200:
            readBody = TRUE; break;
        case 503:
            readBody = FALSE; break;
        default:
	  if (complain) 
            PlafErrorMsg("%s not available", path);
	  goto error;
    }
    p = strchr(p, '\n') + 1;
    for (;;) { 
        if (*p == '\n') { 
            p++;
            break;
        } else if (*p == '\r' && p[1] == '\n') { 
            p += 2;
            break;
        } else if (   strncmp("Content-length:", p, 15) == 0 
                   || strncmp("Content-Length:", p, 15) == 0) {
            p += 15;
            while (*p == ' ') p++;        /* Skip additional space */
            contentLength = atoi(p);
        } else if (   strncmp("Retry-after:", p, 12) == 0
                   || strncmp("Retry-After:", p, 12) == 0) { 
            p += 12;
            while (*p == ' ') p++;        /* Skip additional space */
            *retryDelayP = atoi(p);
        }
        /* Skip to just past the '\r\n' or '\n' */
        p = strchr(p, '\n') + 1;
    }

    if (!readBody) { 
        closesocket(sock);
        return NULL;
    }

    content = (char*)jam_malloc(contentLength + 1);
    content[contentLength] = 0;    /* This saves us some grief on JAM */
    /* Set length to be the number of characters we're going to copy
     * into the content buffer */
    length = length - (p - buffer);
    memcpy(content, p, length);

    while (length < contentLength) { 
        int toread = contentLength - length;
#ifndef __SYMBIAN32__
        if (toread > 1024) {
            toread = 1024;
        }
#endif
        retcode = recv(sock, content + length, toread, 0);
        if (retcode < 0) {
	  if (complain) 
            PlafErrorMsg("Bad read on socket");
	  goto error;
        } else if (retcode == 0) { 
	  if (complain) 
            PlafErrorMsg("Unexpectedly reached EOF");
	  goto error;
        } else { 
            length += retcode;
            PlafDownloadProgress(contentLength, length);
        } 
    }
    
    closesocket(sock);
    *contentLengthP = contentLength;
    return content;

error:
    if (sock >= 0) { 
        closesocket(sock);
    }
    if (content != NULL) { 
        jam_free(content);
    }
    return NULL;
}

static char *
postURL(const char *host, int port, const char *path,
	int datalen, char* data,
	int *httpCodeP, int *contentLengthP, int *retryDelayP,
	int complain)
{
  int sock = -1;
  char buffer[1024]; /* we reuse this for sending and receiving the data */
  unsigned int length, contentLength;
  char *p;
  char *content = NULL;
  int readBody;
  int retcode;
  
  /* Let's set the return args to default values */
  *httpCodeP = 404;           /* unable to make a connection */
  *contentLengthP = 0;
  *retryDelayP = 0;
  contentLength = 0;
  
  /* Open the socket and connect to the server */
  {
    struct hostent* hep;
    struct sockaddr_in sin;
    
    memset((void*)&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    
    hep = gethostbyname(host);
    if (hep == NULL) {
      if (complain) 
	PlafErrorMsg("Unable to resolve host name %s\n", host);
      return NULL;
    }
    memcpy(&sin.sin_addr, hep->h_addr, hep->h_length);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    
    retcode = connect(sock, (struct sockaddr*)&sin, sizeof (sin));
    if (retcode < 0) {
      if (complain) 
	PlafErrorMsg("Unable to connect to %s:%ld\n", 
		     host, (long)port);
      goto error;
    }
  }
  
  sprintf(buffer, "POST %s HTTP/1.0\r\nContent-Length: %d\r\n\r\n", 
	  path, datalen);
  DO_SEND(sock, buffer, strlen(buffer), 0);
  DO_SEND(sock, data, datalen, 0);
  
  /*
   * Read as much as we can into "buffer", upto the size of buffer.
   * Note that the size of the HTTP header need to be smaller than 1024
   * bytes, or else we'd be in trouble.
   */
  memset(buffer, 0, sizeof(buffer));
  for (length = 0; length < sizeof(buffer); ) { 
    retcode = recv(sock, buffer + length, sizeof(buffer) - length, 0);
    if (retcode > 0) { 
      length += retcode;
    } else if (retcode == 0) { 
      /* EOF */
      break;
    } else { 
      if (complain) 
	PlafErrorMsg("Error reading socket\n");
      goto error;
    } 
  } 
  if (length <= 5) { 
    goto error;
  }
  
  /* We're assuming that the buffer is long enough to hold the complete
   * header.  If not, well something is wrong. 
     */
  p = strchr(buffer, ' ') + 1;        /* Skip past the first space */
  while (*p == ' ') p++;              /* Skip additional space */
  *httpCodeP = atoi(p);
  switch(*httpCodeP) {
  case 200:
    readBody = TRUE; break;
  case 503:
    readBody = FALSE; break;
  default:
    if (complain) 
      PlafErrorMsg("%s not available", path);
    goto error;
  }
  p = strchr(p, '\n') + 1;
  for (;;) { 
    if (*p == '\n') { 
      p++;
      break;
    } else if (*p == '\r' && p[1] == '\n') { 
      p += 2;
      break;
    } else if (   strncmp("Content-length:", p, 15) == 0 
		  || strncmp("Content-Length:", p, 15) == 0) {
      p += 15;
      while (*p == ' ') p++;        /* Skip additional space */
      contentLength = atoi(p);
    } else if (   strncmp("Retry-after:", p, 12) == 0
		  || strncmp("Retry-After:", p, 12) == 0) { 
      p += 12;
      while (*p == ' ') p++;        /* Skip additional space */
      *retryDelayP = atoi(p);
    }
    /* Skip to just past the '\r\n' or '\n' */
    p = strchr(p, '\n') + 1;
  }
  
  if (!readBody) { 
    closesocket(sock);
    return NULL;
  }
  
  content = (char*)jam_malloc(contentLength + 1);
  content[contentLength] = 0;    /* This saves us some grief on JAM */
  /* Set length to be the number of characters we're going to copy
   * into the content buffer */
  length = length - (p - buffer);
  memcpy(content, p, length);
  
  while (length < contentLength) { 
    int toread = contentLength - length;
    if (toread > 1024) {
      toread = 1024;
    }
    retcode = recv(sock, content + length, toread, 0);
    if (retcode < 0) {
      if (complain) 
	PlafErrorMsg("Bad read on socket");
	  goto error;
    } else if (retcode == 0) { 
	  if (complain) 
            PlafErrorMsg("Unexpectedly reached EOF");
	  goto error;
    } else { 
      length += retcode;
      PlafDownloadProgress(contentLength, length);
    } 
  }
  
  closesocket(sock);
  *contentLengthP = contentLength;
  return content;
  
 error:
  if (sock >= 0) { 
    closesocket(sock);
  }
  if (content != NULL) { 
    jam_free(content);
  }
  return NULL;
}

void
JamHttpInitialize(void) {
#ifdef _WIN32_
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(1,1), &wsaData) != 0) {
        PlafErrorMsg("Cannot initialize WSA\n");
    }
#endif
}

static int setNonBlock(int fd) {
#ifdef _UNIX_
  return fcntl(fd, F_SETFL, O_NONBLOCK); 
#endif
#ifdef _WIN32_
  unsigned long argp = 1;
  return ioctlsocket(fd, FIONBIO, &argp);
#endif
#ifdef __SYMBIAN32__
  return -1;
#endif
}
static struct sockaddr_in addr;

int JamListen(int port, int block) {   
   static int servfd = -1;

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = PF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons((short)port);

   if (servfd == -1) {
     servfd = socket(PF_INET, SOCK_STREAM, 0);
     int one = 1;
     setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR,
                 (char *) &one, sizeof(int));

     if (!block) {
         setNonBlock(servfd);
     }

     if (bind(servfd,
	      (struct sockaddr*)&addr,
	      sizeof(struct sockaddr_in)) < 0) {
       servfd = -1;
     }
   }
   
   if (servfd != -1) {
     listen(servfd, 5);
     return servfd;
   }

   return -1;
}

// Win32 haven't heard about POSIX
#ifdef _WIN32_
#define socklen_t int
#endif

int      JamAccept(int servfd, int block) {
  int len = sizeof(struct sockaddr_in);
  int fd  = accept(servfd,
		   (struct sockaddr*)&addr,
		   (socklen_t *)&len);
  if (fd < 0) return -1;
  if (!block) setNonBlock(fd);
  return fd;
}

#ifndef __SYMBIAN32__
int JamSelect(int fd, int timeout) {
  struct timeval tmo;
  fd_set rsocks;

  if (timeout >= 0) {
    tmo.tv_sec = (timeout / 1000);
    tmo.tv_usec = (timeout -  tmo.tv_sec*1000) * 1000;
  }

  FD_ZERO(&rsocks);
  FD_SET(fd, &rsocks);
    
  return select(fd + 1, &rsocks, NULL, NULL,
		timeout < 0 ? NULL : &tmo);
}
#endif

int JamRead(int fd, char* buf, int maxlen) {
  int rv, len = 0;

  do {
    rv = recv(fd, buf + len, maxlen - len, 0);
    if (rv > 0) { 
      len += rv;
    }
  } while (rv > 0 && len <= maxlen);
  
#ifdef _UNIX_
  if ((rv < 0 && errno != EAGAIN) || (rv == 0 && errno == EAGAIN)) {
    closesocket(fd);
    return -1;
  }
#endif
  return len;
}

