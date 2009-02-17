/*
 * @(#)linuxMain.cpp	1.24 06/10/10 10:08:37
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
#include <time.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

static int childPid = 0;

extern char** environ;

static char * USAGE  =
 "usage: jam -repeat -url <url> -vm <vm> -bootclasspath <bootcp>\n"
 "               -tempfile <name> -vmargs <args> -timeout <timeout>\n"
 "\n"
 "[Required parameters]\n"
 "  -url <url>   Test server URL. E.g., http://localhost:8080/test/getNextApp\n"
 "  -vm <vm>     Pathname of Java VM. E.g., /opt/monty/build/monty\n"
 "\n"
 "[Optional parameters]\n"
 "  -repeat      If set, JAM keeps downloading app from server\n"
 "  -bootclasspath <bootcp>\n"
 "  -bp <bootcp>\n"
 "               System classpath, separated by \":\". Do not set this\n"
 "               parameter if the VM has romized classes\n"
 "  -tempfile <name>\n"
 "               The filename of the downloaded JAR file. Default\n"
 "               is /tmp/autotest.jar\n"
 "  -vmargs <args>\n"
 "  -J <args>\n"
 "               Extra arguments to pass to the VM. Separate each individual\n"
 "               word in <args> with a single space character. This means\n"
 "               none of the words in <args> may contain the space character\n"
 "               itself :-(. E.g., -vmargs \"-heapsize 256K\"\n"
 "   -timeout <timeout>\n"
 "   -T <timeout>\n"
 "               Timeout in milliseconds. VM will be killed if it will\n"
 "               not exit before timeout expires\n"
 "   -midp\n"
 "   -M\n"
 "               Start in MIDP mode, so tests considered as MIDlets\n"
 "   -type\n"
 "   -Y\n"
 "               Select type of VM to be used, use -Y help to see\n"
 "               list of supported VM switches\n"
 "   -standalone\n"
 "   -S\n"
 "               Run in standalone mode, without server assistance, \n"
 "               so you could measure startup time for standalone JVM\n" 
 "\n"
 "[Example]\n"
 "   jam -repeat -url http://localhost:8080/test/getNextApp \\\n"
 "       -vm ${JVMBuildSpace}/linux_i386/target/bin/cldc_hi\n"
;

int
PlafSaveJar(char * jarContent, int jarLength) {
    FILE *fp = NULL;

    /*
     * (1) Write the JAR file
     */
    if ((fp = fopen(g_tempfile, "wb+")) == NULL) {
        goto error;
    }
    
    if (fwrite(jarContent, 1, jarLength, fp) != (uint)jarLength) {        
        goto error;
    }
    
    fclose(fp);
    return 1;

error:
    if (fp != NULL) {
        fclose(fp);
    }
    return 0;
}

void sig_handler(int sig) {      
  if ((childPid > 0) && (sig == SIGALRM)) {
    printf("TIMEOUT: killing %d (elapsed %d msecs)\n", 
	   childPid, 
	   (int) (PlafCurrentTimeMillis() - testStartMillis)); 
    kill(childPid, SIGINT);
  }
}

int
PlafRunJar(char* vmPath, char * mainClass, long javaHeap) {
    const char *args[512];
    char  classpath[MAX_BUF];
    char  heapstr[MAX_BUF];
    int i, k, retval;
    const char* jheapStr = g_opt_info[g_vm_type].heapswitch;
    
    /*
     * (2) Exec the VM to run this JAR file
     */
    i = 0;   
    args[i++] = vmPath;

    if (javaHeap) {            
      if (g_opt_info[g_vm_type].flags & VM_OPT_HEAP_SEP) {	
	sprintf(heapstr, "%ld", javaHeap);
	args[i++] = jheapStr;
	args[i++] = heapstr;
      } else {
	sprintf(heapstr, "%s%ld", jheapStr, javaHeap);
	args[i++] = heapstr;
      }
    }

    if (g_opt_info[g_vm_type].flags & VM_OPT_CP_SEP) {
      args[i++] = g_opt_info[g_vm_type].classpath;
      if (g_bootclasspath != NULL) {
        sprintf(classpath, "%s"SEP"%s", g_bootclasspath, g_tempfile);
        args[i++] = classpath;
      } else {
        args[i++] = g_tempfile;
      }
    } else {
      sprintf(classpath, "%s%s", g_opt_info[g_vm_type].classpath, g_tempfile);
      args[i++] = classpath;
    }

    for (k=0; g_vmargs[k] != NULL; k++) {
      // if java max heap size already set we don't have to change it 
      if (jheapStr && javaHeap && strstr(g_vmargs[k], jheapStr)) 
	{ 
	  k++;
	  continue;
	}
      args[i++] = g_vmargs[k];	
    }

    args[i++] = mainClass;
   
    args[i++] = NULL;

    //for (i=0; args[i] != NULL; i++) { printf("args[%d]=%s\n", i, args[i]);}
   
    retval =  PlafExec(g_vm, args, g_timeout, TRACK_TIME | TRACK_MEMORY);

    return retval;
}

int
main(int argc, char *argv[]) {
    int retval;

    g_tempfile = "/tmp/autotest.jar";
    // Parse command-parameters. ParseCmdArgs() will exit the app if
    // the parameters contain any error.
    ParseCmdArgs(argc, argv);
    
    // Initialize winsock
    JamHttpInitialize();

    if (g_standaloneMode) {
      JamRunStandaloneURL(g_vm, g_url, g_timeout);
      return 0;
    }    

    // Download and run apps. If repeat!=0, we run until the app
    // server stops serving apps.    
    do {
        testNumber ++;
        if (g_midpMode) 
	  retval = JamRunMIDPURL(g_vm, g_url, g_timeout);
	else
	  retval = JamRunURL(g_vm, g_url, g_timeout);
    } while (g_repeat && retval != 0);

    return 0;
}

void
PlafErrorMsg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    printf("\n");
}

void
PlafStatusMsg(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    printf("\n");
}

void
PlafDownloadProgress(int total, int downloaded) {
    /* Do nothing */
}

int64
PlafCurrentTimeMillis() {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  return (int64)tv.tv_sec * 1000 + tv.tv_usec/1000;

}

MemInfo 
PlafGetMemInfo() {
  static long page = 0;
  static long total = 0;
  MemInfo mi;

  if (!page || !total) {
    page = sysconf(_SC_PAGE_SIZE);
    total = sysconf(_SC_PHYS_PAGES); 
  }

  //  invalid on machines with > 4Gb memory
  mi.total  = total*page;
  mi.used   = (total - sysconf(_SC_AVPHYS_PAGES))*page;

  return mi;
}

int 
PlafExec(const char*   prog,
	 const char**  argv, 
	 int           timeout,
	 int           flags)
{
  int retcode = 0, k;

  if (flags & TRACK_TIME) 
    testStartMillis = PlafCurrentTimeMillis();    
  
  vmStatus = JAM_VM_STARTED;

  if (flags & TRACK_MEMORY) { 
    // start memory tracking thread
    pthread_t      tracker;
    
    PlafUpdateMemInfo(1);

    if (pthread_create(&tracker, NULL, &JamThreadFunction, NULL))
      {
	PlafErrorMsg("Couldn't start tracking thread");
      }
    else 
      {
	// we don't care about tracker thread much later
	pthread_detach(tracker);
      }
  }

  if ((childPid=fork()) == 0) {
    // pass our environment to child, for stuff like DISPLAY
    execve(prog, (char**)argv, (char *const *)environ);
    exit(178); // should never got here
  }

  if (timeout > 0) {
    struct sigaction sa;
    memset(&sa.sa_mask, 0, sizeof(sigset_t));
    sa.sa_handler = sig_handler;
    sigaction(SIGALRM, &sa, NULL);
    
    struct  itimerval  itv;
    itv.it_interval.tv_sec  = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec  = timeout / 1000;
    itv.it_value.tv_usec = (timeout - itv.it_value.tv_sec*1000)*1000;
    if (setitimer(ITIMER_REAL, &itv, NULL)) 
      perror("setitimer");
  }
  
  do {
    
    if (childPid <= 0) break; 
    
    if (waitpid(childPid, &k, 0) < 0 && errno == EINTR) 
      continue;
    
    if (WIFSIGNALED(k)) {
      PlafStatusMsg("%s killed by signal: %d", prog, WTERMSIG(k));
      retcode = 127;      
      break;
    }
    
    if (WIFEXITED(k)) {
      retcode = WEXITSTATUS(k);
      PlafStatusMsg("%s finished, exit code = %d", prog, retcode);
      break;
    }

  } while (1);
  
  childPid = 0;
  vmStatus = JAM_VM_STOPPED;

  // to let memory tracking thread to wake up and finish 
  if (flags & TRACK_MEMORY) {
    PlafSleep(MEM_TRACK_SLICE*2);
    PlafUpdateMemInfo(0);
  }

  return retcode;
}

void PlafUpdateMemInfo(int reset) {
  char buf[200];
  
  buf[0] = '\0';
  JamUpdateMemInfo(reset);
  if (vmStatus == JAM_VM_STOPPED) {
    sprintf(buf, "%d/%d/%d of %d", 
	    minMemSize, 
	    avgMemSize, 
	    maxMemSize,
	    PlafGetMemInfo().total);
    //fprintf(stderr, "%s\n", buf);
  }
}

int  PlafSleep(long ms) {
  timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms - ts.tv_sec*1000) * 1000000;
  return nanosleep(&ts, NULL);
}

char* PlafGetUsage() {
  return USAGE;
}

