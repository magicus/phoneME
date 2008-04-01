/*
 *  
 *
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

#include <stdio.h>

#include <kni.h>
#include <sni.h>

#include <midpServices.h>
#include <midpResourceLimit.h>
#include <midp_logging.h>

#include <pcsl_network.h>
#include <gcf_export.h>

/**
 * Gets the name of the local device from the system. This method is
 * called when the <tt>microedition.hostname</tt> system property
 * is retrieved.
 *
 * @return the name of this device
 */
char*
getLocalHostName(void) {
    int status;
    static char hostname[MAX_HOST_LENGTH];

    status = pcsl_network_getLocalHostName(hostname);
    
    if (status == PCSL_NET_SUCCESS) {
        return hostname;
    }

    return NULL;
}

/**
 * When set, this flag indicates that the network is connected.
 * When not set the status of the network is down or unknown.
 * It is initialized at VM startup by NetworkConnectionBase_initializeInternal().
 * The network is considered as 'connected' if call(s) to
 * pcsl_network_init_start()/finish() were completed successfully.
 * Note that the networking subsystem on the device might already be conncted
 * due to other applications on the device.
 */
static int midp_network_connected = 0;

/* Sets midp network state as connected */
void setMidpNetworkConnected(void) {

    midp_network_connected = 1;
}

/* Sets midp network state as disconnected */
void unsetMidpNetworkConnected(void) {
    midp_network_connected = 0;

}

/* Returns the status of midp networking: connected/disconnected */
int isMidpNetworkConnected(void) {

    return midp_network_connected;
}


/**
 * Initialize the underlying network system.
 * <p>
 * Java declaration:
 * <pre>
 *     initializeNetwork(V)I
 * </pre>
 *
 * @return -1 for error or 0 if no error occured
 */
int initializeMidpNetwork(void) {
    int status = PCSL_NET_INVALID;
    int result = 0;
    MidpReentryData* info;


    /* if network already been initialized return */
    if(!isMidpNetworkConnected()) {
        info = (MidpReentryData*)SNI_GetReentryData(NULL);

        REPORT_INFO1(LC_PROTOCOL, "initializeNetwork0() >> %s Time\n",!info?"1st":"2nd");

        if(info == NULL) { /* First invocation */
            status = pcsl_network_init_start();
        } else { /* Second invocation */
            status = pcsl_network_init_finish();
        }

        if(status == PCSL_NET_IOERROR) {
            /* failure! - mark network as "not connected" */
            unsetMidpNetworkConnected();
            result = -1;
        } else if(status == PCSL_NET_SUCCESS) {
            /* mark network as "connected" to avoid multiple initializations */
            setMidpNetworkConnected();
            result = 0;
        } else if(status == PCSL_NET_WOULDBLOCK) {
            if(info == NULL) {
                info = (MidpReentryData*)
                (SNI_AllocateReentryData(sizeof (MidpReentryData)));
            }
            info->waitingFor = NETWORK_INIT_SIGNAL;
            /* network not ready, try again later */
            SNI_BlockThread();
            result = -1;
        }
    } else {
        /* network already been initialized */
        result = 0;
    }
    REPORT_INFO1(LC_PROTOCOL, "initializeNetwork0() << %d\n",result);
    return ((jint)result);
}

/**
 * Cancel close the network
 */
void cancelNetworkClose(void) {
    //if(NETWORK_LINGER_TIME > 0) {
        pcsl_network_cancel_finalize_request();
    //}
}

/**
 * Request close the network
 */
void requestNetworkClose(void) {
    //if(NETWORK_LINGER_TIME > 0) {
        if (1 == midpCheckSocketInUse()) {
            return;
        }
        pcsl_network_cancel_finalize_request();
        pcsl_network_init_finalize_request(1/*NETWORK_LINGER_TIME*/);
    //}
}

