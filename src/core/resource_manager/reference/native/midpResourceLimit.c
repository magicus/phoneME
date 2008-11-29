/*
 *
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

/**
 * @file midpResourceLimit.c
 * MIDP Resource limit
 *
 * MIDP native resource limit implementation.
 *
 * @note
 *
 * @header midpResourceLimit.h
 */

#include <midpServices.h>
#include <midpResourceLimit.h>
#include <midpMidletSuiteUtils.h>
#include <jvm.h>
#include <midp_logging.h>
#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <stdio.h>

#if 0 /* for local debug */
#define REPORT_INFO(a,b)  printf(b)
#define REPORT_INFO1(a,b,c)  printf(b,c)
#define REPORT_INFO2(a,b,c,d)  printf(b,c,d)
#define REPORT_INFO3(a,b,c,d,e)  printf(b,c,d,e)
#define REPORT_WARN3(a,b,c,d,e)  printf(b,c,d,e)
#endif

#if 0
#define MEASURE_HIGHWATER_MARK
#endif

/**
 * Macros for easy access to Resource Table values
 */
#define IS_AMS_ISOLATE         (getCurrentIsolateId()==midpGetAmsIsolateId())
#define GLOBAL_LIMIT(type)     gGlobalResourceTable[0][(type)]
#define SUITE_RESERVED(type)   gGlobalResourceTable[IS_AMS_ISOLATE?1:3][(type)]
#define SUITE_LIMIT(type)      gGlobalResourceTable[IS_AMS_ISOLATE?2:4][(type)]
#define NON_AMS_RESERVED(type) gGlobalResourceTable[3][(type)]

/**
 * Resource Limit Table (R/O) This table is statically initialised for
 * global and per suite limits for all resource types.
 */
const int gGlobalResourceTable[5][RSC_TYPE_COUNT] = {
  {
    TCP_CLI_GLOBAL_LIMIT,     /* RSC_TYPE_TCP_CLI*/
    TCP_SER_GLOBAL_LIMIT,     /* RSC_TYPE_TCP_SER*/
    UDP_GLOBAL_LIMIT,         /* RSC_TYPE_UDP*/
    FILE_GLOBAL_LIMIT,        /* RSC_TYPE_FILE*/
    AUDIO_CHA_GLOBAL_LIMIT,   /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_GLOBAL_LIMIT,   /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_GLOBAL_LIMIT  /* RSC_TYPE_IMAGE_IMMUT*/
  },
  {
    TCP_CLI_AMS_RESERVED,     /* RSC_TYPE_TCP_CLI*/
    TCP_SER_AMS_RESERVED,     /* RSC_TYPE_TCP_SER*/
    UDP_AMS_RESERVED,         /* RSC_TYPE_UDP*/
    FILE_AMS_RESERVED,        /* RSC_TYPE_FILE*/
    AUDIO_CHA_AMS_RESERVED,   /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_AMS_RESERVED,   /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_AMS_RESERVED  /* RSC_TYPE_IMAGE_IMMUT*/
  },
  {
    TCP_CLI_AMS_LIMIT,      /* RSC_TYPE_TCP_CLI*/
    TCP_SER_AMS_LIMIT,      /* RSC_TYPE_TCP_SER*/
    UDP_AMS_LIMIT,          /* RSC_TYPE_UDP*/
    FILE_AMS_LIMIT,         /* RSC_TYPE_FILE*/
    AUDIO_CHA_AMS_LIMIT,    /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_AMS_LIMIT,    /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_AMS_LIMIT   /* RSC_TYPE_IMAGE_IMMUT*/
  },
  {
    TCP_CLI_SUITE_RESERVED,     /* RSC_TYPE_TCP_CLI*/
    TCP_SER_SUITE_RESERVED,     /* RSC_TYPE_TCP_SER*/
    UDP_SUITE_RESERVED,         /* RSC_TYPE_UDP*/
    FILE_SUITE_RESERVED,        /* RSC_TYPE_FILE*/
    AUDIO_CHA_SUITE_RESERVED,   /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_SUITE_RESERVED,   /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_SUITE_RESERVED  /* RSC_TYPE_IMAGE_IMMUT*/
  },
  {
    TCP_CLI_SUITE_LIMIT,      /* RSC_TYPE_TCP_CLI*/
    TCP_SER_SUITE_LIMIT,      /* RSC_TYPE_TCP_SER*/
    UDP_SUITE_LIMIT,          /* RSC_TYPE_UDP*/
    FILE_SUITE_LIMIT,         /* RSC_TYPE_FILE*/
    AUDIO_CHA_SUITE_LIMIT,    /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_SUITE_LIMIT,    /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_SUITE_LIMIT   /*RSC_TYPE_IMAGE_IMMUT*/
  }
};

/**
 * Structure for isolate resource usage tracking
 */
typedef struct {
    int isolateId;
    int inUse;
    int resourceUsage[RSC_TYPE_COUNT];
#ifdef MEASURE_HIGHWATER_MARK
    int resourceMaxUsage[RSC_TYPE_COUNT];
#endif
} _IsolateResourceUsage;

/**
 * Resource available table (R/W)
 */
static int gResourcesAvailable[RSC_TYPE_COUNT] = {
    TCP_CLI_GLOBAL_LIMIT - TCP_CLI_AMS_RESERVED,         /* RSC_TYPE_TCP_CLI*/
    TCP_SER_GLOBAL_LIMIT - TCP_SER_AMS_RESERVED,         /* RSC_TYPE_TCP_SER*/
    UDP_GLOBAL_LIMIT - UDP_AMS_RESERVED,                 /* RSC_TYPE_UDP*/
    FILE_GLOBAL_LIMIT - FILE_AMS_RESERVED,               /*RSC_TYPE_FILE*/
    AUDIO_CHA_GLOBAL_LIMIT - AUDIO_CHA_AMS_RESERVED,     /* RSC_TYPE_AUDIO_CHA*/
    IMAGE_MUT_GLOBAL_LIMIT - IMAGE_MUT_AMS_RESERVED,     /* RSC_TYPE_IMAGE_MUT*/
    IMAGE_IMMUT_GLOBAL_LIMIT - IMAGE_IMMUT_AMS_RESERVED  /*RSC_TYPE_IMAGE_IMMUT*/
};

static int isInitialized = KNI_FALSE;
static _IsolateResourceUsage* gIsolateResourceUsage;
static int max_isolates = 0;


/**
 * Initialize the Resource limit structures.
 */
static void initResourceLimit(void) {
    int i, j;

#if ENABLE_CDC
    /* CDC does not have isolates. */
    max_isolates = 1;
#else
    max_isolates = getMaxIsolates();
#endif

    REPORT_INFO(LC_CORE, "initialize resource limit\n");

    gIsolateResourceUsage = (_IsolateResourceUsage*)
        midpMalloc(sizeof(_IsolateResourceUsage) * max_isolates);

    for (i = 0; i < max_isolates; i++) {
        gIsolateResourceUsage[i].isolateId = -1;
        gIsolateResourceUsage[i].inUse = 0;

        for (j = 0; j < RSC_TYPE_COUNT; j++) {
          gIsolateResourceUsage[i].resourceUsage[j] = 0;

#ifdef MEASURE_HIGHWATER_MARK
          gIsolateResourceUsage[i].resourceMaxUsage[j] = 0;
#endif
        }
    }

    /* initialize the entry for the AMS */
    gIsolateResourceUsage[0].isolateId = 1;
    gIsolateResourceUsage[0].inUse = 1;

    isInitialized = KNI_TRUE;
}

/**
 * Finalize the Resource limit structures.
 */
void midpFinalizeResourceLimit(void) {
    if (isInitialized) {
        if (gIsolateResourceUsage) {
            midpFree(gIsolateResourceUsage);
            gIsolateResourceUsage = NULL;
        }
        isInitialized = KNI_FALSE;
    }
}

/**
 * Find the _IsolateResourceUsage structure for the given isolateId.
 *
 * @param isolateId id of the isolate
 *
 * @return the _IsolateResourceUsage structure for the given isolateId if it
 *         exist, otherwise 0
 */
static _IsolateResourceUsage *findIsolateResourceUsageStruct(int isolateId) {
    int i;

    if (!isInitialized) {
        initResourceLimit();
    }

    /* the first entry is the ams */
    if (isolateId == midpGetAmsIsolateId()) {
        return &(gIsolateResourceUsage[0]);
    }

    for (i = 0; i < max_isolates; i++) {
        if (isolateId == gIsolateResourceUsage[i].isolateId) {
            return &(gIsolateResourceUsage[i]);
        }
    }

    REPORT_INFO1(LC_CORE, "RESOURCES [%d] isolateId not in resource table\n",
                 isolateId);

    return 0;
}

/**
 * Verify that the resource limit is not crossed.
 *
 * @param entry pointer to isolate's _IsolateResourceUsage structure
 * @param type Resource type
 * @param requestSize Requesting size
 *
 * @return 1 if resource limit is not crossed, otherwise 0
 */
static int checkResourceLimit(_IsolateResourceUsage *entry,
                              RscType type, int requestSize) {
    if (entry->resourceUsage[type] + requestSize <= SUITE_LIMIT(type)) {
        int fromGlobal = requestSize;

        if (entry->resourceUsage[type] < SUITE_RESERVED(type)) {
            /* part or all of the needed resource is already reserved */
            fromGlobal = (entry->resourceUsage[type] + requestSize) -
                SUITE_RESERVED(type);
            if (fromGlobal < 0) {
                fromGlobal = 0;
            }
        }

        if (gResourcesAvailable[type] - fromGlobal >= 0) {
            return 1;
        }
    }

    REPORT_INFO3(LC_CORE, "RESOURCES [%d] checkResourceLimit FAILED" \
                 "  used=%d  global=%d\n",
                 entry->isolateId, entry->resourceUsage[type],
                 gResourcesAvailable[type]);

    return 0;
}

/**
 * Verify that the resource limit is not crossed. IsolateID will be
 * fetched from getCurrentIsolateId() as defined in midpServices.h
 *
 * @param type Resource type
 * @param requestSize Requesting size
 *
 * @return 1 if resource limit is not crossed, otherwise 0
 */
int midpCheckResourceLimit(RscType type, int requestSize) {
    int isolateId = getCurrentIsolateId();
    _IsolateResourceUsage *entry = findIsolateResourceUsageStruct(isolateId);

    REPORT_INFO3(LC_CORE, "RESOURCES [%d] midpCheckResourceLimit(%d, %d)\n",
                 isolateId, type, requestSize);

    if (entry != 0 && entry->inUse) {
        return checkResourceLimit(entry, type, requestSize);
    }

    REPORT_INFO1(LC_CORE, "RESOURCES [%d] midpCheckResourceLimit FAILED\n",
                 isolateId);

    return 0; /* failed */
}

/*
 * Increment the resource consumption count. IsolateID will internally be
 * fetched from getCurrentIsolateId() as defined in midpServices.h
 *
 * @param type Resource type
 * mode the resource limit is always checked against the global limit.
 * @param delta requesting size
 *
 * @return 1 if count is successfully incremented, otherwise 0
 *
 */
int midpIncResourceCount(RscType type, int delta) {
    int isolateId = getCurrentIsolateId();
    _IsolateResourceUsage *entry = findIsolateResourceUsageStruct(isolateId);

    REPORT_INFO3(LC_CORE, "RESOURCES [%d] midpIncResourceCount(%d, %d)\n",
                 isolateId, type, delta);

    if (entry != 0 && entry->inUse &&
        entry->resourceUsage[type] + delta <= SUITE_LIMIT(type)) {

        int fromGlobal = delta;

        if (entry->resourceUsage[type] < SUITE_RESERVED(type)) {
            /* part or all of the needed resource is already reserved */
            fromGlobal = (entry->resourceUsage[type] + delta) - SUITE_RESERVED(type);
            if (fromGlobal < 0) {
                fromGlobal = 0;
            }
        }

        if (gResourcesAvailable[type] - fromGlobal >= 0) {
            gResourcesAvailable[type] -= fromGlobal;
            entry->resourceUsage[type] += delta;


#ifdef MEASURE_HIGHWATER_MARK
            if (entry->resourceUsage[type] > entry->resourceMaxUsage[type]) {
                entry->resourceMaxUsage[type] = entry->resourceUsage[type];
            }
#endif
            REPORT_INFO3(LC_CORE, "    [%d]  used=%d  global=%d\n",
                         isolateId, entry->resourceUsage[type],
                         gResourcesAvailable[type]);

            return 1; /* succeeded */
        }
    }
    if (entry != 0) {
        REPORT_INFO3(LC_CORE, "RESOURCES [%d] midpIncResourceCount FAILED" \
                 "  used=%d  global=%d\n",
                 isolateId, entry->resourceUsage[type],
                 gResourcesAvailable[type]);
    } else {
        REPORT_ERROR2(LC_CORE, "RESOURCES [%d] midpDecResourceCount FAILED" \
                 "  used=unknown  global=%d\n",
                 isolateId,
                 gResourcesAvailable[type]);
    }
    return 0; /* failed */
}

/*
 * Decrement the resource consumption count.  IsolateID will internally
 * be fetched from getCurrentIsolateId() as defined in midpServices.h
 *
 * @param type Resource type
 * mode the resource limit is always checked against the global limit.
 * @param delta requesting size
 *
 * @return 1 if count is successfully decremented, otherwise 0
 *
 */
int midpDecResourceCount(RscType type, int delta) {
    int isolateId = getCurrentIsolateId();
    _IsolateResourceUsage *entry = findIsolateResourceUsageStruct(isolateId);

    REPORT_INFO3(LC_CORE, "RESOURCES [%d] midpDecResourceCount(%d, %d)\n",
                 isolateId, type, delta);

    if (entry != 0) {

        int toGlobal = 0;

        if (entry->resourceUsage[type] > SUITE_RESERVED(type)) {
            toGlobal = delta;

            if (entry->resourceUsage[type] - delta < SUITE_RESERVED(type)) {
                /* only return the resources that came from the global pool */
                toGlobal = entry->resourceUsage[type] - SUITE_RESERVED(type);
            }
        }

        if (entry->inUse) {
            gResourcesAvailable[type] += toGlobal;
        } else {
            gResourcesAvailable[type] += delta;
        }
        entry->resourceUsage[type] -= delta;

        REPORT_INFO3(LC_CORE, "    [%d]  used=%d  global=%d\n",
                     isolateId, entry->resourceUsage[type],
                     gResourcesAvailable[type]);

        return 1; /* succeeded */
    }

    REPORT_ERROR2(LC_CORE, "RESOURCES [%d] midpDecResourceCount FAILED" \
                 "  used=unknown  global=%d\n",
                 isolateId,
                 gResourcesAvailable[type]);

    return 0; /* failed */
}

/**
 * check if the reserved resources for a new isolate are available
 *
 * @return true if the resources are available, otherwise return false
 */
int midpCheckReservedResources() {
    int i = 0;
    int status = KNI_TRUE;

    REPORT_INFO1(LC_CORE, "RESOURCES [%d] midpCheckReservedResources()\n",
                 getCurrentIsolateId());

    if (!isInitialized) {
        initResourceLimit();
    }

    /* check if the reserved resources are available for each resource type */
    for (i = 0; i < RSC_TYPE_COUNT; i++) {
        if (NON_AMS_RESERVED(i) > gResourcesAvailable[i]) {
            status = KNI_FALSE;
            break;
        }
    }

    return status;
}

/**
 * allocate the reserved resources for the isolate
 *
 * @return true if the resources are available, otherwise return false
 */
int midpAllocateReservedResources() {
    int isolateId = getCurrentIsolateId();
    int i = 0, idx;
    int status = KNI_TRUE;

    REPORT_INFO1(LC_CORE, "RESOURCES [%d] midpAllocateReservedResources()\n",
                 isolateId);

    if (!isInitialized) {
        initResourceLimit();
    }

    /* do not allocate the ams again */
    if (isolateId == midpGetAmsIsolateId()) {
        REPORT_INFO(LC_CORE, "attempt to allocate AMS entry\n");
        return KNI_TRUE;
    }

    /* find a free entry in the resource usage list */
    for (idx = 0; idx < max_isolates; idx++) {
        if (!gIsolateResourceUsage[idx].inUse) {
            break;
        }
    }

    if (idx < max_isolates) {
        /* check if the reserved resources are available
           for each resource type */
        for (i = 0; i < RSC_TYPE_COUNT; i++) {
            if (SUITE_RESERVED(i) > gResourcesAvailable[i]) {
                status = KNI_FALSE;
                break;
            }
        }

        /* if there is enough resources available, reserve them */
        if (i == RSC_TYPE_COUNT) {
            for (i = 0; i < RSC_TYPE_COUNT; i++) {
                gResourcesAvailable[i] -= SUITE_RESERVED(i);

                if (gIsolateResourceUsage[idx].resourceUsage[i] > 0) {
                    REPORT_WARN3(LC_CORE, "previous Isolate(%d) did not free" \
                                 " all resource type %d: %d left\n",
                                 gIsolateResourceUsage[idx].isolateId, i,
                                 gIsolateResourceUsage[idx].resourceUsage[i]);
                }

                gIsolateResourceUsage[idx].resourceUsage[i] = 0;

#ifdef MEASURE_HIGHWATER_MARK
                gIsolateResourceUsage[idx].resourceMaxUsage[i] = 0;
#endif
            }

            gIsolateResourceUsage[idx].isolateId = isolateId;
            gIsolateResourceUsage[idx].inUse = 1;

        }
    } else {
        status = KNI_FALSE;
    }

    return status;
}

/**
 * free the reserved resources for the isolate
 *
 */
void midpFreeReservedResources() {
    int isolateId = getCurrentIsolateId();
    int idx, i;

    REPORT_INFO1(LC_CORE, "RESOURCES [%d] midpFreeReservedResources()\n",
                 isolateId);

    if (!isInitialized) {
        initResourceLimit();
        if (!isInitialized) {
            return;
        }
    }

    /* do not free the AMS entry */
    if (isolateId == midpGetAmsIsolateId()) {
        REPORT_INFO(LC_CORE, "attempt to free AMS entry\n");

#ifdef MEASURE_HIGHWATER_MARK
        {
            int x = 0;

            REPORT_INFO(LC_CORE, "High Water Mark for AMS\n");
            for (x = 0; x < RSC_TYPE_COUNT; x++) {
                REPORT_INFO2(LC_CORE, "[%d]  %d\n", x,
                             gIsolateResourceUsage[0].resourceMaxUsage[x]);
            }
        }
#endif

        return;
    }

    /* find the entry for the isolate in the resource usage list */
    for (idx = 0; idx < max_isolates; idx++) {
        if (gIsolateResourceUsage[idx].isolateId == isolateId) {
            REPORT_INFO2(LC_CORE, "RESOURCES [%d] found index %d\n",
                         isolateId, idx);

            /* mark this entry as free */
            gIsolateResourceUsage[idx].inUse = 0;

#ifdef MEASURE_HIGHWATER_MARK
            {
                int x = 0;

                REPORT_INFO1(LC_CORE, "High Water Mark for isolate %d\n",
                             gIsolateResourceUsage[idx].isolateId);
                for (x = 0; x < RSC_TYPE_COUNT; x++) {
                    REPORT_INFO2(LC_CORE, "[%d]  %d\n", x,
                                 gIsolateResourceUsage[idx].resourceMaxUsage[x]);
                }
            }
#endif

            /* return unused reserved resources */
            for (i = 0; i < RSC_TYPE_COUNT; i++) {
                int unusedResources = SUITE_RESERVED(i) -
                    gIsolateResourceUsage[idx].resourceUsage[i];

                if (unusedResources > 0) {
                    gResourcesAvailable[i] += unusedResources;
                }
            }

            break;
        }
    }
}
