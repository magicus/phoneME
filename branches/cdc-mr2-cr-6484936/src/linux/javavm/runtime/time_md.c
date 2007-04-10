/*
 * @(#)time_md.c	1.8 06/10/10
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
 *
 */

#include "javavm/include/porting/time.h"
#include <dlfcn.h>
#include "javavm/include/assert.h"

#ifdef CVM_JVMTI
static int (*_clock_gettime)(clockid_t, struct timespec *);

void clock_init() {
  // we do dlopen's in this particular order due to bug in linux
  // dynamical loader (see 6348968) leading to crash on exit
  void* handle = dlopen("librt.so.1", RTLD_LAZY);
  if (handle == NULL) {
    handle = dlopen("librt.so", RTLD_LAZY);
  }

  if (handle) {
    int (*clock_getres_func)(clockid_t, struct timespec*) = 
           (int(*)(clockid_t, struct timespec*))dlsym(handle, "clock_getres");
    int (*clock_gettime_func)(clockid_t, struct timespec*) = 
           (int(*)(clockid_t, struct timespec*))dlsym(handle, "clock_gettime");
    if (clock_getres_func && clock_gettime_func) {
      // See if monotonic clock is supported by the kernel. Note that some
      // early implementations simply return kernel jiffies (updated every
      // 1/100 or 1/1000 second). It would be bad to use such a low res clock
      // for nano time (though the monotonic property is still nice to have).
      // It's fixed in newer kernels, however clock_getres() still returns
      // 1/HZ. We check if clock_getres() works, but will ignore its reported
      // resolution for now. Hopefully as people move to new kernels, this
      // won't be a problem.
      struct timespec res;
      struct timespec tp;
      if (clock_getres_func (CLOCK_PROCESS_CPUTIME_ID, &res) == 0 &&
          clock_gettime_func(CLOCK_PROCESS_CPUTIME_ID, &tp)  == 0) {
        // yes, monotonic clock is supported
        _clock_gettime = clock_gettime_func;
      } else {
        // close librt if there is no monotonic clock
        dlclose(handle);
      }
    }
  }
}

CVMInt64
CVMtimeNanosecs(void)
{
  if (_clock_gettime != NULL) {
    struct timespec tp;
    int status = _clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
    CVMassert(status == 0);
    return (CVMInt64)(((CVMInt64)tp.tv_sec) * (1000 * 1000 * 1000) +
		      (CVMInt64)tp.tv_nsec);
  } else {
    struct timeval t;
    gettimeofday(&t, 0);
    return (CVMInt64)(((CVMInt64)t.tv_sec) * 1000000 + (CVMInt64)(t.tv_usec));
  }
}
#endif

CVMInt64
CVMtimeMillis(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return (CVMInt64)(((CVMInt64)t.tv_sec) * 1000 + (CVMInt64)(t.tv_usec/1000));
}
