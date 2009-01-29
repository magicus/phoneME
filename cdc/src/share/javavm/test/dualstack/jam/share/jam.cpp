/*
 * @(#)jam.cpp	1.32 06/10/10 10:08:34
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

#define JAR_FILE_URL_TAG        "JAR-File-URL"
#define JAR_FILE_SIZE_TAG       "JAR-File-Size"
#define MAIN_CLASS_TAG          "Main-Class"

int64         testStartMillis;
int64         javaStartMillis;
unsigned int  minMemSize;
unsigned int  avgMemSize;
unsigned int  maxMemSize;

int      g_midpMode = 0;
int      g_standaloneMode = 0;
int      volatile vmStatus = JAM_VM_STOPPED;
int      testNumber = 0;

char *   g_vm = NULL;
char *   g_url = NULL;
char *   g_bootclasspath = NULL;
char *   g_tempfile;
char *   g_vmargs[100] = { NULL };
int      g_repeat = 0;
int      g_retry = 0;
int      g_timeout = 0;
int      g_testNumber = 0;
int      g_vm_type = -1;
int      g_sticky = 0;

static const char* midp2Autotest[3] = {
  "-domain", "trusted", NULL
};

VMOptInfo g_opt_info[VM_TYPE_UNKNOWN+1] = {
  {VM_TYPE_KVM,     "kvm",   "-heapsize",     "-classpath", NULL,
   NULL, VM_OPT_HEAP_SEP | VM_OPT_CP_SEP },
  {VM_TYPE_MONTY,   "monty", "=HeapCapacity", "-cp",        "-StickyConsole", 
   NULL, VM_OPT_CP_SEP },
  {VM_TYPE_JEODE,   "jeode", "-mx",          "-classpath", NULL, 
   NULL, VM_OPT_CP_SEP},
  {VM_TYPE_J9,      "j9",    "-mx",          "-cp:",        NULL, 
   NULL, 0},
  {VM_TYPE_JDK1_3,  "jdk",   "-Xmx",          "-cp",        NULL, 
   NULL, VM_OPT_CP_SEP},
  {VM_TYPE_CDC,     "cdc",   "-Xms",          "-Dclass.path=", NULL, 
   NULL, 0},
  {VM_TYPE_MIDP,    "midp",   "-heapsize",    "-classpath",    NULL, 
   NULL, VM_OPT_HEAP_SEP | VM_OPT_CP_SEP },
  {VM_TYPE_MIDP2,    "midp2",   "-heapsize",    "-classpath",    NULL, 
   midp2Autotest, VM_OPT_HEAP_SEP | VM_OPT_CP_SEP },
  {VM_TYPE_UNKNOWN, "unknown","-Xmx",         "-classpath", NULL, 
   NULL, VM_OPT_CP_SEP}
};

static int RunMemTest(const char* url, 
		      char*       vm, 
		      int         timeout,
		      char*       jadContent);

static int
GetJarURL(char* jadContent, char* parentURL, char* jarURL) {
    char* p;
    //char* q;
    int len;
    //int parentLen;
    
    p = JamGetProp(jadContent, JAR_FILE_URL_TAG, &len);
    if (p == NULL) {
        goto invalid_manifest;
    }

    if (strncmp(p, "http:", 5) == 0) {
        if (len >= MAX_URL) {
            goto invalid_manifest;
        }
        strnzcpy(jarURL, p, len);
    } else {
#if 0
        q = strrchr(parentURL, '/');

        if (q == NULL) {
            goto invalid_manifest;
        }

        parentLen = (q - parentURL + 1);
        if ((parentLen + len) >= MAX_URL) {
            goto invalid_manifest;
        }

        strnzcpy(jarURL, parentURL, parentLen);
        strncat(jarURL, p, len);
#else
        // TODO: relative path for JAR_FILE_URL_TAG not supported
        return 0;
#endif
    }
    return 1;

invalid_manifest:
    return 0;
}

static char *
DownloadJar(char * jadContent, int *jarLenPtr) {
    static char jarURL[MAX_URL+1];

    if (GetJarURL(jadContent, NULL, jarURL) == 0) {
        return NULL;
    }
    //printf("JAR = %s\n", jarURL);
    return JamHttpGet(jarURL, jarLenPtr, 1);
}

static void 
ReportTimeStamp(const char* url, int64 ts) {
  static char report_url[200];
  int len, i=0, s=0;
  
  while (url[i] && s<3) {
    if (url[i] == '/') s++;
    report_url[i] = url[i];
    i++;
  }
  
  sprintf(report_url+i, "test/sendVMInitTime/%s",
	  GetCharsOfLong64(ts));
  char* buf = JamHttpGet(report_url, &len, 0);
  if (buf) {
      jam_free(buf);
  }
}

static void 
ReportMemoryUsage(const char*  url, 
		  unsigned int minMem,
		  unsigned int avgMem,
		  unsigned int maxMem) {
  static char report_url[300];
  int len, i=0, s=0;  

  while (url[i] && s<3) {
    if (url[i] == '/') s++;
    report_url[i] = url[i];
    i++;
  }

  sprintf(report_url+i, "test/sendMemInfo/%u_%u_%u",
	  minMem, avgMem, maxMem);
  char* buf = JamHttpGet(report_url, &len, 0);
  if (buf) {
      jam_free(buf);
  }
}

static int RunVmWithLimitedHeap(char*       vm, 
				char*       mainClass, 
				long        startHeap,
				int         decrement)
{
  long heap = startHeap;
  int status = 0;
    
  while (heap > 0 && (status == 0)) {
    // NOTE: for Monty
    heap = (heap/128)*128;
    PlafStatusMsg("Running with heap %dK", heap/1024);
    status = PlafRunJar(vm, mainClass, heap);
    if (status) {
      PlafStatusMsg("VM failed with heap %ld", heap);
      break;
    }
    heap -= decrement;
  }

  // not completely robust algorithm
  heap += decrement;
  heap = (heap/128)*128;

  return PlafRunJar(vm, mainClass, heap);
}

static char* JamGetNextTest(const char* url, int *len) {
  char report_url[300];
  int i=0, s=0;  

  while (url[i] && s<3) {
    if (url[i] == '/') s++;
    report_url[i] = url[i];
    i++;
  }

  sprintf(report_url+i, "test/getNextTest");
  return JamHttpGet(report_url, len, 0);
}

static char* JamPostTestResult(const char* url, 
			       int   reslen, char* result, 
			       int*  respLen) 
{
  char report_url[300];
  int i=0, s=0;  

  while (url[i] && s<3) {
    if (url[i] == '/') s++;
    report_url[i] = url[i];
    i++;
  }

  sprintf(report_url+i, "test/sendTestResult");
  return JamHttpPost(report_url, reslen, result, respLen, 1);
}

/*
 * Return non-zero if the URL was successfully downloaded and executed.
 */

int
JamRunURL(char* vm, const char* url, int timeout) {
    char *jadContent, *jarContent;
    int jadLen, jarLen;
    char *p;
    static char mainClass[MAX_BUF];
    int mainClassLen;
    int len;
    int result;

    PlafStatusMsg("Getting JAD %d ...", testNumber);
    PlafDownloadProgress(0, 0);
    if ((jadContent = JamHttpGet(url, &jadLen, 1)) == NULL) {
      PlafStatusMsg("Can't download JAD");
      return 0;
    }
    PlafDownloadProgress(100, 99);

    //printf("jad=%s\n", jadContent);

    /*
     * (1) Determine the main class
     */
    p = JamGetProp(jadContent, MAIN_CLASS_TAG, &mainClassLen);

    if (p == NULL) {
      PlafStatusMsg("no tag \"%s\" in JAD", MAIN_CLASS_TAG);
      return 0;
    }
    if (mainClassLen >= MAX_BUF) {
      PlafStatusMsg("too long main class name");
      return 0;
    }
    strnzcpy(mainClass, p, mainClassLen);

    /*
     * (2) Download the JAR and run it
     */
    PlafStatusMsg("Getting JAR %d ...", testNumber);
    if ((jarContent = DownloadJar(jadContent, &jarLen)) == NULL) {      
      PlafStatusMsg("Can't download JAR");
      return 0;
    }
    if (PlafSaveJar(jarContent, jarLen) == 0) {
      PlafStatusMsg("Can't save JAR");
      return 0;
    } else {
      jam_free(jarContent);
    }
        
    if ((p = JamGetProp(jadContent, "VMMemTest", &len))) {
      
      result = RunMemTest(url, vm, timeout, jadContent);

    } else {

      result = PlafRunJar(vm, mainClass, 0);

    }

#ifndef __SYMBIAN32__ 
    if (JamGetProp(jadContent, "VMInitTest", &len)) {
        ReportTimeStamp(url, testStartMillis);
    }
    ReportMemoryUsage(url, minMemSize, avgMemSize, maxMemSize);
#endif
    
    if (jadContent) {
        jam_free(jadContent);
    }
    return (!result);
}

int
JamRunMIDPURL(char* vm, const char* url, int timeout) {
  const char* args[200];  
  int i, retcode;

  /* step 1 - install test suite */
  PlafStatusMsg("Installing from %s", url);
  i = 0;
#ifndef _WIN32_WCE
  args[i++] = vm;
#endif  
  args[i++] = "-force";
  args[i++] = "-install";
  args[i++] = url;
  if (g_opt_info[g_vm_type].midpAutotest) {
    for (int k = 0; g_opt_info[g_vm_type].midpAutotest[k] !=0; k++)
      args[i++] = g_opt_info[g_vm_type].midpAutotest[k];
  }
  args[i++] = NULL;
  retcode = PlafExec(vm, args, timeout, 0);

  if (retcode) {
    PlafErrorMsg("Couldn't install MIDlet");
    return 0;
  }
  /* step 2 - run test suite */
  PlafStatusMsg("Starting MIDP...");
  i = 0;
#ifndef _WIN32_WCE
  args[i++] = vm;
#endif
  args[i++] = "-run";
  args[i++] = "1";
  args[i++] = "TestSuite";
  args[i++] = NULL;
  retcode = PlafExec(vm, args, timeout, TRACK_MEMORY | TRACK_TIME);
  
  /* store run statistics */
  int64        timestamp = testStartMillis;
  unsigned int minMem    = minMemSize;
  unsigned int avgMem    = avgMemSize;
  unsigned int maxMem    = maxMemSize;

  /* step 3 - remove test suite */
  PlafStatusMsg("Removing suite...");
  i = 0;
#ifndef _WIN32_WCE
  args[i++] = vm;
#endif
  args[i++] = "-remove";
  args[i++] = "1";
  args[i++] = "TestSuite";
  args[i++] = NULL;
  PlafExec(vm, args, timeout, 0);
  
  /* for we don't read value of JAD we don't know if this run requires 
     to be timestamped, so just send it out */
  ReportTimeStamp(url, timestamp);
  ReportMemoryUsage(url, minMem, avgMem, maxMem);

  //return (retcode != 0) ? 0 : 1 ;
  return 1;
}

int
JamRunStandaloneURL(char* vm, const char* url, int timeout) {
  const char* args[20];  
  int i, retcode;

  if (memcmp(url, "class://", 8)) {
    PlafErrorMsg("standalone URL should be like class://com.sun.mep.ManyClasses");
    return 1;
  }
  url += 8;
  PlafStatusMsg("Running class %s", url);
  i = 0;
#ifndef _WIN32_WCE
  args[i++] = vm;
#endif
  if (g_midpMode) {
    args[i++] = "-run";
    args[i++] = url;
    args[i++] = NULL;
    retcode = PlafExec(vm, args, 0, TRACK_MEMORY | TRACK_TIME);
  } else {
    args[i++] = url;
    args[i++] = NULL;
    retcode = PlafRunJar(vm, (char*)url, 0);    
  }  
  if (retcode) {
      PlafStatusMsg("sorry, VM execution error");
      return 0;
  }
 
#ifndef __SYMBIAN32__ 
  if (javaStartMillis) {
        long diff = long(javaStartMillis - testStartMillis);
	PlafStatusMsg("startup time: %ld msecs", diff);
  }
#endif
  return 0;
}

void JamUpdateMemInfo(int reset) {
  static int tick = 0; 
  
  if (reset) minMemSize = maxMemSize = avgMemSize = tick = 0;
  MemInfo mi = PlafGetMemInfo();
  if (mi.used > maxMemSize) maxMemSize = mi.used;
  if (mi.used < minMemSize || minMemSize == 0) minMemSize = mi.used;
  avgMemSize = (unsigned int)((double)avgMemSize*tick/(tick+1) 
			      + mi.used/(tick+1));
  tick++;
}

#ifndef __SYMBIAN32__
static int dataLen;

static int ReadData(int fd) {
#define BUF_LEN 2048
  static char buf[BUF_LEN+1];
  int  len;
  
  len = JamRead(fd, buf+dataLen, BUF_LEN-dataLen);
  if (len > 0) 
    {   
      int l = 0;      
      dataLen += len;
      buf[dataLen] = 0;
      //PlafStatusMsg("%d read %d bytes(dl=%d): %s", fd, len, dataLen, buf);
      char* p = JamGetProp(buf, "startup", &l);
      if (p && l > 0) {
	javaStartMillis = ParseInt64(p, l);	
      }
    }
  if (len < 0) return -1;

  return len;
}

void*    JamThreadFunction(void* args) {
  int servfd = -1, fd = -1;

  if (g_standaloneMode) {
    dataLen = 0;
    // start local server to receive data from VM
    servfd = JamListen(55000, 0);
    if (servfd < 0) {
      PlafErrorMsg("cannot listen on socket, check if port 55000 isn't taken");
    } 
  }

  while (vmStatus == JAM_VM_STARTED) {   
    PlafUpdateMemInfo(0);
    if (g_standaloneMode) {      
      if (fd >= 0) {
	if ((JamSelect(fd, 0)>0) && ReadData(fd) < 0) fd = -1;
      } else {
	if (JamSelect(servfd, 0)>0 && (fd == -1)) {
	  fd = JamAccept(servfd, 0);      
	} else {
	  //PlafErrorMsg("already connected"); 
	}
      }
    }
    PlafSleep(MEM_TRACK_SLICE);
  }
  return NULL;
}

#endif

static char* EncodeResults(char* str, 
			   int inlen, 
			   int *outlen) 
{
#define ADD_WORD1(t, l) \
  *(p++) = t; \
  *(p++) = 1; \
  sprintf(lenstr, "%d", l); \
  len = strlen(lenstr); \
  *(p++) = (char)(len & 0xff); \
  memcpy(p, lenstr, len); \
  p += len;  \
  types++;

  char* result = (char*)jam_malloc (inlen + 200);
  char* p = result+2;
  int   types = 0, len;
  char  lenstr[20];

  ADD_WORD1(5, 1);  
  ADD_WORD1(4, inlen);

  result[0] = (char) (p-result-1);
  result[1] = (char) (types & 0xff);

  *(p++) = ' ';
  memcpy(p, str, inlen);
  *outlen = inlen +  result[0] + 2;
  return result;
}

static int RunMemTest(const char * url, 
		      char* vm, 
		      int     timeout,
		      char* jadContent) 
{
  int len;
  char memTest[200];

  char* p = JamGetProp(jadContent, "VMMemTest", &len);
  
  memcpy(memTest, p, len);
  memTest[len] = '\0';      
  long startHeap = 1000000L;

  if ((p = JamGetProp(jadContent, "VMStartHeap", &len))) 
    {
      char startHeapStr[100];
      memcpy(startHeapStr, p, len);
      startHeapStr[len] = '\0'; 
      startHeap = atol(startHeapStr);  
    }
  
  //printf("memTest = %s startHeap=%ld\n", memTest, startHeap);
  int httpLen = 0;
  char* httpBuf = JamGetNextTest(url, &httpLen);
  
  
  if (!httpBuf) {
    PlafErrorMsg("getNextTest failed");
    return 0; 
  }
  jam_free (httpBuf);
  
  int rv =  RunVmWithLimitedHeap(vm, memTest, startHeap, startHeap/20);

  if (rv) return rv;
  
  char result[1000];
  int encLen = 0;
  sprintf(result, "\
score.value=%u,\
warmupScore.value=0,\
scale.value=1,\
warmupScale.value=0,\
totalMemory.value=10000,\
usedMemory.value=10000,\
type.value=SB", 
	  maxMemSize-minMemSize);

  char* resultBuf = EncodeResults(result, strlen(result), &encLen);
  if (!resultBuf || !encLen) return 0;
  
  httpBuf = JamPostTestResult(url, encLen, resultBuf, &httpLen);
  
  if (resultBuf) jam_free (resultBuf);
			    
  if (!httpBuf) {
    PlafErrorMsg("sendTestResult failed");
    return 0;
  }
  if (httpBuf) jam_free (httpBuf);

  httpBuf = JamGetNextTest(url, &httpLen);
  if (httpBuf) jam_free (httpBuf);
  
  return 0;
}

#ifdef  JAM_TRACE_MEM
void *my_malloc(int size) {
  void* r = malloc(size);
  printf("MALLOC: %d %p\n", size, r);
  return r;
}

void my_free(void* ptr) {
  printf("FREE: %p\n", ptr);
  free(ptr);
}

void *my_realloc(void *ptr, size_t size){
  void* r=NULL;
  
  r = realloc(ptr, size);
  printf("REALLOC: %d %p->%p\n", size, ptr, r);
  return r;
}
#endif

