/*
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

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

#include <string.h>
#include <stdlib.h> /* strtol() */

#include <javacall_memory.h>
#include <javacall_logging.h>
#include <javacall_carddevice.h>
#include "local_defs.h"
#include "carddevice_simu.h"

/** Maximum length of a host name */
#define MAX_HOST_LENGTH 256

/** Minimum port number */
#define MIN_PORT_NUMBER 1L

/** Minimum port number */
#define MAX_PORT_NUMBER 65536L

/** Maximum length of a command */
#define MAX_COMMAND_LENGTH 128

static void *javacall_carddevice_strdup(const char *str) {
    int len;
    static char *const_str = "";
    char *p;

    if (str == NULL) {
        return NULL;
    }
    len = strlen(str);
    if (len == 0) {
        return (void *)const_str;
    }
    p = (char *)javacall_malloc(len + 1);
    if (p == NULL) {
        return NULL;
    }
    memcpy(p, str, len);
    p[len] = '\0';
    return (void *)p;
}

/**
 * The table of slots.
 */
static struct _SLOT_STRUCT {
    /** Card movement events. */
    JAVACALL_CARD_MOVEMENT events;
    /** Socket. */
    int s;
    /** Host name. */
    char *host_name;
    /** Port number. */
    int port;
    /** INET address of CREF port */
    struct sockaddr_in addr;
} slots[MAX_SLOT_COUNT];


/**
 * Number of slots.
 */
static int slot_count = 0;

/**
 * Current slot number.
 */
static int current_slot;

/** Configuration property name */
#define PROP_NUMBER 2
static char hostsandports[] = "com.sun.io.j2me.apdu.hostsandports";
static char satselectapdu[] = "com.sun.io.j2me.apdu.satselectapdu";
static char *properties[PROP_NUMBER] = { hostsandports,
                                         satselectapdu };

/** Configuration property name */
static char *saved_properties[PROP_NUMBER];

/**
 * APDU command for selecting SIM application
 */
static struct {
    /** Command name. */
    char *command;
    /** Length of the command. */
    int len;
} satselectcmd;

/**
 * Is the driver already initialized.
 */
static javacall_bool DriverInitialized = JAVACALL_FALSE;

#define CARDDEVICE_MUTEX_NAME       "CardDevice_MuteX"
#define CARDDEVICE_MUTEX_TIMEOUT    300

/* Is 0 slot SAT slot */
#define NOT_INITIALIZED -1
static int IsSatSlot = NOT_INITIALIZED;

/**
 * Mutex for device locking.
 */
static HANDLE locked = NULL;
// static pthread_mutex_t locked = PTHREAD_MUTEX_INITIALIZER;
/**
 * Verbose level of the driver.
 */
javacall_bool javacall_carddevice_verbose = JAVACALL_FALSE;

/* local functions */
static int isoOut(int slot, char *command, int length, char *response,
    int resp_len);
static int isoIn(int slot, char *command, int length, char *response,
    int resp_len);
static int transmissionError(int slot);
static int statusResponse(int slot, int code);
static int receiveTLP224Message(int slot, char *data, int buf_len);
static int sendTLP224Message(int slot, char *data, int len);
static int cmdXfer(int slot, char *tx_buffer, int tx_size, char *rx_buffer,
    int rx_size);
static int cmdPowerDown(int slot);
static int cmdReset(int slot, char *atr, int atr_size);
static char computeLRC(char *buf, int length);
static char make_hex(int val);

/**
 * Initializes the driver. This is not thread-safe function.
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_init() {
    int i;
    WSADATA wsaData;

    if (DriverInitialized) {
        return JAVACALL_OK;
    }

    if (locked == NULL) {
        locked = CreateMutex(
            NULL,                       // default security attributes
            FALSE,                      // initially not owned
            CARDDEVICE_MUTEX_NAME);     // named object
    }
    /* we lock the device because somebody can invoke this function from
     * native thread
     */
    if (!WaitForSingleObject(locked, CARDDEVICE_MUTEX_TIMEOUT) == WAIT_OBJECT_0) {
        javacall_carddevice_set_error("Device locking failed");
        return JAVACALL_FAIL;
    }
    /* if the thread was waiting for mutex */
    if (DriverInitialized) {
        ReleaseMutex(locked);
        return JAVACALL_OK;
    }

    if (slot_count == 0) {
        javacall_carddevice_set_error("No device slots found");
        goto err;
    }
    if (WSAStartup(MAKEWORD(2, 2), &wsaData ) != 0) {
        javacall_carddevice_set_error("Cannot startup winsock32");
        goto err;
    }
    for (i = 0; i < slot_count; i++) {
        struct hostent *he;

        memset((char*)&slots[i].addr, 0, sizeof slots[i].addr);
        /* IMPL_NOTE: gethostbyname() may return pointer to static data and can
         * not be used in a multi-threaded environment
         */
        he = gethostbyname(slots[i].host_name);
        if (he == NULL ||
                he->h_addr_list == NULL || he->h_addr_list == NULL) {
            javacall_carddevice_set_error("Invalid host: %s", slots[i].host_name);
            goto err;
        }
        /* we support only IPv4 */
        if (he->h_addrtype != AF_INET || he->h_length != 4) {
            javacall_carddevice_set_error("Invalid host type: %s", slots[i].host_name);
        err:
            ReleaseMutex(locked);
            return JAVACALL_FAIL;
        }

        memcpy((char*)&slots[i].addr.sin_addr.s_addr, he->h_addr_list[0], 4);
        slots[i].addr.sin_family = AF_INET;
        slots[i].addr.sin_port = htons((short)slots[i].port);
        slots[i].s = -1;
        slots[i].events = 0;
    }
    current_slot = -1;

    javacall_carddevice_clear_error();
    DriverInitialized = JAVACALL_TRUE;
    ReleaseMutex(locked);

    return JAVACALL_OK;
}

/**
 * Finalizes the driver.
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_finalize() {
    int i;
    if (!DriverInitialized) {
        javacall_carddevice_set_error("Already closed");
        return JAVACALL_FAIL;
    }

    for (i = 0; i < slot_count; i++) {
        if (slots[i].s != -1) {
            cmdPowerDown(i);
            closesocket(slots[i].s);
            slots[i].s = -1;
        }
    }

    for (i = 0; i < PROP_NUMBER; i++) {
        javacall_free(saved_properties[i]);
    }

    javacall_free(satselectcmd.command);

    IsSatSlot = NOT_INITIALIZED;
    DriverInitialized = JAVACALL_FALSE;
    return JAVACALL_OK;
}

/**
 * Sets property value. If the property is used during the initialization
 * process then this method must be called before <code>javacall_carddevice_init()</code>
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when this property is not supported
 *         JAVACALL_OUT_OF_MEMORY if there is no enough memory
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_set_property(const char *prop_name,
                                                 const char *prop_value) {
    char *p, *s;
    int cnt;
    int prop;

    if (prop_name == NULL) {
        return JAVACALL_OK;
    }

    for (prop = 0; prop < PROP_NUMBER; prop++) {
        if (!strcmp((const char*)prop_name, properties[prop])) {
            break;
        }
    }

    if (prop == PROP_NUMBER) {
        javacall_carddevice_set_error("javacall_carddevice_set_property: invalid property name: %s",
            prop_name);
        return JAVACALL_NOT_IMPLEMENTED;
    }

    if (DriverInitialized) {
        if (prop_value == NULL && saved_properties[prop] == NULL) {
            return JAVACALL_OK;
        }
        if (prop_value != NULL && saved_properties[prop] != NULL &&
                    !strcmp((char*)prop_value, saved_properties[prop])) {
            return JAVACALL_OK;
        }
        javacall_carddevice_set_error("javacall_carddevice_set_property: driver already initialized");
        return JAVACALL_FAIL;
    }
    if (prop_value == NULL) {
        if (saved_properties[prop] != NULL) {
            javacall_free(saved_properties[prop]);
        }
        saved_properties[prop] = NULL;
        return JAVACALL_OK;
    }

    if (properties[prop] == hostsandports) {
        /* prop_value = "jcemulhost:9025,jcemulhost:9026" */
        for (p = (char*)prop_value, cnt = 0; p != NULL && *p != '\0'; p = s) {
            char host[MAX_HOST_LENGTH];
            char *p_end;
            int len;
            long port; /* long - because we use strtol() */

            if (*p == ',') {
                p++;
            }
            if ((s = strchr(p, ',')) == NULL) {
                s = p + strlen(p);
            }
            if (s == p) {
                javacall_carddevice_set_error("javacall_carddevice_set_property: empty host name");
                goto err;
            }
            if ((p_end = strchr(p, ':')) == NULL || p_end > s) {
                goto invalid_port;
            }
            len = (int)(p_end - p); /* length of hostname */
            if (len >= (int)sizeof host) {
                javacall_carddevice_set_error("javacall_carddevice_set_property: very long host name");
                goto err;
            }
            memcpy(host, p, len);
            host[len] = '\0';

            p = p_end + 1;
            if ((int)(s - p) < 1) {
                goto invalid_port;
            }
            if ((port = strtol(p, &p_end, 10)) <= MIN_PORT_NUMBER ||
                port > MAX_PORT_NUMBER || p_end != s) {
            invalid_port:
                javacall_carddevice_set_error("javacall_carddevice_set_property: invalid port number");
            err:
                return JAVACALL_FAIL;
            }
            slots[cnt].host_name = strdup(host);
            if (slots[cnt].host_name == NULL) {
                javacall_carddevice_set_error("javacall_carddevice_set_property: No memory");
                return JAVACALL_OUT_OF_MEMORY;
            }
            slots[cnt].port = (int)port;
            cnt++;
        }
        slot_count = cnt;
    }
    else {
        char command[MAX_COMMAND_LENGTH];
        for (p = (char*)prop_value, cnt = 0; ; cnt++, p++) {
            command[cnt] = 0;
            while (p != NULL && *p != '\0' && *p != '.') {
                if ((*p >= '0') && (*p <= '9')) {
                    command[cnt] = command[cnt]*16 + *p - '0';
                }
                else
                    if ((*p >= 'A') && (*p <= 'F')) {
                        command[cnt] = command[cnt]*16 + *p - 'A' + 10;
                    }
                    else
                        if ((*p >= 'a') && (*p <= 'f')) {
                            command[cnt] = command[cnt]*16 + *p - 'a' + 10;
                        }
                        else {
                            javacall_carddevice_set_error("javacall_carddevice_set_property: value of satselectapdu is incorrect");
                            return JAVACALL_FAIL;
                        }
                p++;
            }

            if (p == NULL || *p == '\0')
                break;
        }

        satselectcmd.command = malloc(++cnt);
        if (satselectcmd.command == NULL) {
            javacall_carddevice_set_error("javacall_carddevice_set_property: No memory");
            return JAVACALL_OUT_OF_MEMORY;
        }
        memcpy(satselectcmd.command, command, cnt);
        satselectcmd.len = cnt;
    }

    if (saved_properties[prop] != NULL) {
        javacall_free(saved_properties[prop]);
    }
    saved_properties[prop] = strdup((char*)prop_value);
    if (saved_properties[prop] == NULL) {
        javacall_carddevice_set_error("javacall_carddevice_set_property: No memory for property");
        return JAVACALL_OUT_OF_MEMORY;
    }

    return JAVACALL_OK;
}

/**
 * Selects specified slot (if possible).
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_select_slot(javacall_int32 slot_index) {
    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }
    javacall_carddevice_clear_error();
    if (slot_index >= 0  && slot_index < slot_count) {
        current_slot = slot_index;
        return JAVACALL_OK;
    }
    javacall_carddevice_set_error("Invalid slot number");
    return JAVACALL_FAIL;
}

/**
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_get_slot_count(javacall_int32 *slot_cnt) {
    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }
    *slot_cnt = slot_count;
    return JAVACALL_OK;
}

/**
 * Sends 'RESET' ('POWER UP') command to device and gets ATR
 * into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: length of received ATR.
 * @return JAVACALL_OK if all done successfuly, JAVACALL_FAIL otherwise
 */
static javacall_result javacall_carddevice_reset(char *atr, javacall_int32 *atr_size) {
    int bytes;
    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }

    if (current_slot == -1) {
        javacall_carddevice_set_error("Slot not selected");
        return JAVACALL_FAIL;
    }
    javacall_carddevice_clear_error();
    slots[current_slot].events = 0;
    if ((bytes = cmdReset(current_slot, atr, *atr_size)) < 0) {
        javacall_carddevice_set_error("Reset failed");
        return JAVACALL_FAIL;
    }
    *atr_size = bytes;
    slots[current_slot].events = 0;

    if (current_slot == 0) {
        IsSatSlot = NOT_INITIALIZED;
    }

    return JAVACALL_OK;

}

/**
 * Performs platform lock of the device. This is intended to make
 * sure that no other native application
 * uses the same device during a transaction.
 * @return JAVACALL_OK if all done successfuly,
           JAVACALL_WOULD_BLOCK if the device is locked by the other
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_lock() {
    int result;

    if ((result = WaitForSingleObject(locked, 0)) == WAIT_OBJECT_0) {
        return JAVACALL_OK;
    }
    return JAVACALL_WOULD_BLOCK; //IMPL_NOTE: check for error
}

/**
 * Unlocks the device.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_unlock() {

    ReleaseMutex(locked);
    javanotify_carddevice_event(JAVACALL_CARDDEVICE_UNLOCK, NULL);
    return JAVACALL_OK; //IMPL_NOTE: check for error
}

/**
 * Retrieves current slot's card movement events from driver.
 * Events is retrieved as bit mask. It can include
 * all movements from last reading, but can contain only the last.
 * Enum JAVACALL_CARD_MOVEMENT should be used to specify type of movement.
 * Clears the slot event state.
 * @param mask Movements retrived.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_card_movement_events(JAVACALL_CARD_MOVEMENT *events) {

    if (current_slot == -1) {
        javacall_carddevice_set_error("Slot not selected");
        return JAVACALL_FAIL;
    }
    *events = slots[current_slot].events;
    slots[current_slot].events = 0;
    return JAVACALL_OK;
}

/**
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JAVACALL_OK if all done successfuly, JAVACALL_FAIL otherwise
 */
static javacall_result javacall_carddevice_xfer_data(char *tx_buffer, javacall_int32 tx_size,
                                                     char *rx_buffer, javacall_int32 *rx_size) {
    int ret_value;

    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }

    javacall_carddevice_clear_error();

    if (javacall_carddevice_verbose) {
        int i;
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "Try to send: ");
        for (i = 0; i < tx_size; i++) {
	     javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "%02X ", (tx_buffer[i] & 0xFF));
        }
	 javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "\n");
    }

    ret_value = cmdXfer(current_slot, tx_buffer, tx_size, rx_buffer, *rx_size);

    if (javacall_carddevice_verbose) {
        int i;
        if (ret_value < 0) {
   	     javautil_debug_print (JAVACALL_LOG_ERROR, "carddevice_simu", "Received: error\n");
        } else {
   	     javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "Received: %s", (ret_value == 0 ? "none" : ""));
            for (i = 0; i < ret_value; i++) {
     	         javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "%02X ", (rx_buffer[i] & 0xFF));
            }
   	     javautil_debug_print (JAVACALL_LOG_INFORMATION, "carddevice_simu", "\n");
        }
    }
    if (ret_value < 0) {
        return JAVACALL_FAIL;
    }
    *rx_size = ret_value;
    return JAVACALL_OK;
}

/* Internal functions */

/**
 * Sends 'POWER UP' command to device and gets ATR into specified buffer. At any
 * reset we try to reconnect to the CREF. Any error causes a movement event.
 * @param slot Slot number.
 * @param atr Buffer to store ATR.
 * @param atr_size size of provided buffer
 * @return Length of ATR or -1 in case of error
 */
static int cmdReset(int slot, char *atr, javacall_int32 atr_size) {
    struct sockaddr_in cli_addr;
    int s;
    char data[7];
    char msg[MAX_MESSAGE_LEN];
    javacall_int32 atr_len;
    int len;

    if (slots[slot].s != -1) {
        closesocket(slots[slot].s);
        slots[slot].s = -1;
    }

    memset((char *)&cli_addr, 0, sizeof cli_addr);
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = 0;
    cli_addr.sin_port = 0;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        javacall_carddevice_set_error("javacall_carddevice_reset: can't open stream socket");
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    if (bind(s, (struct sockaddr*)&cli_addr, sizeof cli_addr) < 0) {
        javacall_carddevice_set_error("javacall_carddevice_reset: can't bind stream socket");
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    if (connect(s, (struct sockaddr*)&slots[slot].addr, sizeof slots[slot].addr) < 0) {
        javacall_carddevice_set_error("javacall_carddevice_reset: can't connect to server");
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    slots[slot].s = s;
    data[0] = TLP224Message_ACK;
    data[1] = 4;
    data[2] = TLP224Message_POWER_UP;
    data[3] = 0;
    data[4] = 0;
    data[5] = 0;
    data[6] = computeLRC(data, 6);
    if (sendTLP224Message(slot, data, 7) < 0) {
        return -1;
    }

    len = receiveTLP224Message(slot, msg, sizeof msg);
    if (len < 0) {
        return -1;
    }
    if (msg[2] == TLP224Message_STATUS_CARD_REMOVED) {
        // the card has been just inserted, try again
        if (sendTLP224Message(slot, data, 7) < 0) {
            return -1;
        }
        len = receiveTLP224Message(slot, msg, sizeof msg);
        if (len < 0) {
            return -1;
        }
    }
    if (len < 5) {
        javacall_carddevice_set_error("cmdReset: answer too short");
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    if (msg[2] != 0) {
        javacall_carddevice_set_error("cmdReset: TLP224Error: %d", (msg[2] & 0xff));
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    if (msg[1] < 5 || (msg[1] == 1 && msg[2] == 0)) {
        javacall_carddevice_set_error("CREF is not ready");
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    // card on-line
    atr_len = msg[5];
    if (atr_len < 0 || atr_len > atr_size) {
        javacall_carddevice_set_error("cmdReset: Invalid length of ATR (%d)", atr_len);
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    memcpy(atr, msg + 6, atr_len);
    return atr_len;
}

/**
 * Sends 'POWER DOWN' command to device and gets ATR into specified buffer. At any
 * reset we try to reconnect to the CREF. Any error causes a movement event.
 * @param slot Slot number.
 * @return 0 if ok or -1 in case of error
 */
static int cmdPowerDown(int slot) {
        char data[4];

        data[0] = TLP224Message_ACK;
        data[1] = 1;
        data[2] = TLP224Message_POWER_DOWN;
        data[3] = computeLRC(data, 3);
        if (sendTLP224Message(slot, data, 4) < 0) {
            return -1;
        }
        if (receiveTLP224Message(slot, NULL, 0) < 0) {
            return -1;
        }
        return 0;
}

/**
 * Transfers APDU data to the device and receives response from the device.
 * @param slot Slot number.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size size of <tt>rx_buffer</tt>
 * @return Number of received bytes or -1 in case of error
 */
static int cmdXfer(int slot, char *tx_buffer, javacall_int32 tx_size,
                   char *rx_buffer, javacall_int32 rx_size) {
    int received;

    if (tx_size < 4) {
        goto err;
    }

    if (tx_size == 4) { // case 1 -> ISO_IN
        received = isoIn(slot, tx_buffer, tx_size, rx_buffer, rx_size);
        if (received < 0) {
            return -1;
        }
    } else
    if (tx_size == 5) { // case 2 -> ISO_OUT
        received = isoOut(slot, tx_buffer, tx_size, rx_buffer, rx_size);
        if (received < 0) {
            return -1;
        }
    } else
    if (tx_size == (tx_buffer[4] & 0xFF) + 5) { // case 3 -> ISO_IN
        received = isoIn(slot, tx_buffer, tx_size, rx_buffer, rx_size);
        if (received < 0) {
            return -1;
        }
    } else
    if (tx_size > (tx_buffer[4] & 0xFF) + 5) { // case 4 -> ISO_IN
        tx_size = (tx_buffer[4] & 0xFF) + 5;
        received = isoIn(slot, tx_buffer, tx_size, rx_buffer, rx_size);
        if (received < 0) {
            return -1;
        }
    } else {
    err:
        javacall_carddevice_set_error("cmdXfer: invalid APDU length (%d)", tx_size);
        return -1;
    }
    return received;
}

/**
 * Computes the TLP224 LRC of provided buffer.
 * The TLP224 LRC is the exclusive-or of all the bytes in the message.
 * @param buf Buffer.
 * @param length The number of bytes to compute the LRC over.
 * @return The computed LRC.
 */
static char computeLRC(char *buf, int length) {
    int lrc = 0;
    int i;

    for (i = 0; i < length; i++)
        lrc ^= buf[i];
    return (char) lrc;
}

/**
 * Makes hexadecimal digit from number.
 * @param val The number
 * @return Digit in ASCII
 */
static char make_hex(int val) {
    if (val < 10) {
        return val + 0x30;
    }
    return val + 0x37;
}

/**
 * Formats a TLP224Message into it's ASCII representation and
 * sends the message to the CREF.
 * @param slot Slot number
 * @param data TLP224 encoded message to be sent
 * @param len The length of message
 * @return 0 if message is sent successfuly or -1 in case of error
 */
static int sendTLP224Message(int slot, char *data, int len) {
    int i;
    static const char eot = TLP224Message_EOT;

    for (i = 0; i < len; i++) {
        char nibble = make_hex(data[i] >> 4 & 0xf);
        if (send(slots[slot].s, &nibble, 1, 0) == SOCKET_ERROR) {
            goto err;
        }
        nibble = make_hex(data[i] & 0xf);
        if (send(slots[slot].s, &nibble, 1, 0) == SOCKET_ERROR) {
        err:
            javacall_carddevice_set_error(
                "sendTLP224Message: cannot send: Winssock Error: %d", WSAGetLastError());
            slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
            return -1;
        }
    }
    if (send(slots[slot].s, &eot, 1, 0) == SOCKET_ERROR) {
        javacall_carddevice_set_error(
            "sendTLP224Message: cannot send EOT: Winssock Error: %d", WSAGetLastError());
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    return 0;
}

/**
 * Receives a TLP224 formatted message from the CREF.
 * This method reads bytes from the input socket until an EOT (0x03)
 * character is received. The resulting message is decoded and stored
 * in the provided buffer. In the event of a transmission error, this
 * method will send attempt error recovery by sending a TLP224 NACK
 * message to the sender. Up to 5 retries will be performed.
 * @param slot Slot number
 * @param data Buffer for TLP224 encoded message
 * @param buf_len The length of the buffer
 * @return Length of received message or -1 in case of error
 */
static int receiveTLP224Message(int slot, char *data, int buf_len) {

    int tries = 0;

    for (;;) {
        int got;
        javacall_bool messageTooLong = JAVACALL_FALSE;
        javacall_bool xmitError = JAVACALL_FALSE;

        char hiNibble, loNibble;
        // Only retry link level errors 5 times before giving up.
        if (tries++ > 5) {
            javacall_carddevice_set_error("receiveTLP224Message: TLP224Error: 1");
            slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
            return -1;
        }

        // loop reading characters until EOT is received.
        got = 0;
        for (;;) {
            int result = recv(slots[slot].s, &hiNibble, 1, 0);

            if (result == 0) {
                javacall_carddevice_set_error("receiveTLP224Message: EOF occured");
                slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
                return -1;
            }
            if (result == SOCKET_ERROR) {
                javacall_carddevice_set_error(
                    "receiveTLP224Message: cannot read: Winssock Error: %d", WSAGetLastError());
                slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
                return -1;
            }
            if (hiNibble == TLP224Message_EOT) {
                break;
            }

            result = recv(slots[slot].s, &loNibble, 1, 0);
            if (result == 0) {
                javacall_carddevice_set_error("receiveTLP224Message: EOF occured");
                slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
                return -1;
            }
            if (result == SOCKET_ERROR) {
                javacall_carddevice_set_error(
                    "receiveTLP224Message: cannot read: Winssock Error: %d", WSAGetLastError());
                slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
                return -1;
            }
            if (hiNibble == TLP224Message_EOT) {
                break;
            }
            xmitError |= (hiNibble < '0' || hiNibble > 'F' ||
                          (hiNibble > '9' && hiNibble < 'A')) ||
                         (loNibble < '0' || loNibble > 'F' ||
                          (loNibble > '9' && loNibble < 'A'));

            if (xmitError) {
                continue;
            }

            hiNibble -= hiNibble > '9' ? 0x37 : 0x30;
            loNibble -= loNibble > '9' ? 0x37 : 0x30;

            if (got >= buf_len) {
                messageTooLong = JAVACALL_TRUE;
            } else {
                if (data != NULL) {
                    data[got] = (char) ((hiNibble << 4) | loNibble);
                }
                got++;
            }
        }

        if (xmitError || got < 3) {
            if (transmissionError(slot) < 0) {
                return -1;
            }
            continue;
        }

        if (messageTooLong) {
            if (statusResponse(slot,
                    TLP224Message_STATUS_MESSAGE_TOO_LONG) < 0) {
                return -1;
            }
            continue;
        }

        if (data == NULL) {
            continue;
        }
        if (data[got - 1] != computeLRC(data, got - 1) ||
            data[1] != (char) (got - 3)) {
            // the message must contain a valid LRC, the second byte
            // of the message is the command length. The total message
            // length includes the ACK/NACK, the length and the LRC
            if (transmissionError(slot) < 0) {
                return -1;
            }
            continue;
        }

        // The first byte of the message must be either an ACK or a NACK
        if (data[0] != TLP224Message_ACK &&
            data[0] != TLP224Message_NACK) {
            if (statusResponse(slot, TLP224Message_STATUS_PROTOCOL_ERROR) < 0) {
                return -1;
            }
            continue;
        }

        return got;
    }
}

/**
 * Sends a one byte TLP224 status response.
 * @param slot The slot number
 * @param code The status response
 * @return 0 if response is sent successfuly or -1 in case of error
 */
static int statusResponse(int slot, int code) {
    char data[4];

    data[0] = TLP224Message_ACK;
    data[1] = 1;
    data[2] = (char) code;
    data[3] = computeLRC(data, 3);
    return sendTLP224Message(slot, data, 4);
}

/**
 * Sends a TLP224 Transmission Error response.
 * @param slot The slot number
 * @return 0 if response is sent successfuly or -1 in case of error
 */
static int transmissionError(int slot) {
    char data[3];

    data[0] = TLP224Message_NACK;
    data[1] = 0;
    data[2] = computeLRC(data, 2);
    return sendTLP224Message(slot, data, 3);
}

/**
 * Formats and sends an ISO_IN command to the CAD.
 * @param slot Slot number.
 * @param command Buffer with APDU command to be sent.
 * @param length Size of APDU.
 * @param response Buffer to store the response.
 * @param resp_len size of <tt>response</tt>
 * @return Number of received bytes (always 2) or -1 in case of error
 */
static int isoIn(int slot, char *command, javacall_int32 length, char *response,
                 javacall_int32 resp_len) {
    char status;
    int received;
    char data[MAX_MESSAGE_LEN];

    if (length + 4 > (javacall_int32)sizeof data) {
        javacall_carddevice_set_error("isoIn: APDU command too long (%d)", length);
        return -1;
    }
    if (resp_len < 2) {
        javacall_carddevice_set_error("isoIn: response buffer too small (%d)", resp_len);
        return -1;
    }
    data[0] = TLP224Message_ACK;
    // data[1] - the length will be counted later
    data[2] = TLP224Message_ISO_INPUT;
    memcpy(data + 3, command, 4);
    if (length == 4) {
        data[7] = 0;
        length++;
    } else {
        data[7] = command[4];
    }
    memcpy(data + 8, command + 5, length - 5);
    data[1] = (char)(length + 1);
    data[length + 3] = computeLRC(data, length + 3);
    if (sendTLP224Message(slot, data, length + 4) < 0) {
        return -1;
    }
    if ((received = receiveTLP224Message(slot, data, sizeof data)) < 0) {
        return -1;
    }
    status = data[2];
    if (status != TLP224Message_STATUS_SUCCESS &&
        status != TLP224Message_STATUS_CARD_ERROR &&
        status != TLP224Message_STATUS_INTERRUPTED_EXCHANGE) {

        javacall_carddevice_set_error("isoIn: TLP224Error %d", status);
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }

    if (response != NULL) {
        response[0] = data[3]; // SW1
        response[1] = data[4]; // SW2
    }

    return 2; // only SW1 and SW2
}

/**
 * Formats and sends an ISO_IN command to the CAD. The APDU command
 * must be of cases 1 or 2.
 * @param slot Slot number.
 * @param command Buffer with APDU command to be sent.
 * @param length Size of APDU.
 * @param response Buffer to store the response.
 * @param resp_len size of <tt>response</tt>
 * @return Number of received bytes or -1 in case of error
 */
static int isoOut(int slot, char *command, javacall_int32 length, char *response,
                  javacall_int32 resp_len) {
    char status;
    char data[MAX_MESSAGE_LEN];
    int received;

    if (length < 4 || length > 5) {
        javacall_carddevice_set_error("isoOut: invalid command length (%d)", length);
        return -1;
    }
    data[0] = TLP224Message_ACK;
    data[1] = 6;
    data[2] = TLP224Message_ISO_OUTPUT;
    memcpy(data + 3, command, length);
    if (length == 4) {
        data[7] = 0;
    }
    data[8] = computeLRC(data, 8);
    if (sendTLP224Message(slot, data, 9) < 0) {
        return -1;
    }
    received = receiveTLP224Message(slot, data, sizeof data);
    if (received < 0) {
        return -1;
    }
    status = data[2];
    if (status != TLP224Message_STATUS_SUCCESS &&
        status != TLP224Message_STATUS_CARD_ERROR &&
        status != TLP224Message_STATUS_INTERRUPTED_EXCHANGE) {

        javacall_carddevice_set_error("isoOut: TLP224Error %d", status);
        slots[slot].events |= JAVACALL_EVENT_CARD_CHANGED;
        return -1;
    }
    // response extra bytes: 3 bytes header + 1 byte LRC
    if (response != NULL && resp_len < received - 4) {
        javacall_carddevice_set_error("isoOut: response buffer too small (%d, required %d)",
            resp_len, received - 4);
        return -1;
    }
    if (response != NULL && received > 4) {
        memcpy(response, data + 3, received - 4);
    }
    return received - 4;
}


static DWORD WINAPI resetThreadProc(LPVOID lpParam) {
    struct RESET_PARAMS *par = (struct RESET_PARAMS*) lpParam;
    par->status = javacall_carddevice_reset(par->atr, &par->atr_size);
    javanotify_carddevice_event(JAVACALL_CARDDEVICE_RESET, (void*)lpParam);
    return 0; // Terminate the thread
}

static DWORD WINAPI xferThreadProc(LPVOID lpParam) {
    struct XFER_PARAMS *par = (struct XFER_PARAMS*)lpParam;

    par->status = javacall_carddevice_xfer_data(par->tx_buffer,
                                                par->tx_length,
                                                par->rx_buffer,
                                                &par->rx_length);
    javanotify_carddevice_event(JAVACALL_CARDDEVICE_XFER, (void*)lpParam);
    return 0; // Terminate the thread
}

/**
 * Sends 'RESET' command to device and gets ATR into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context the context saved during asynchronous operation.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call
 *         the javacall_carddevice_reset_finish function to complete
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_reset_start(char *atr,
                                                javacall_int32 *atr_size,
                                                void **context) {
    struct RESET_PARAMS *par;
    DWORD dwJavaThreadId;   // unused
    HANDLE hJavaThread;     // unused
    int rxlen = *atr_size;

    (void)atr;
    par = javacall_malloc(sizeof *par);
    if (par == NULL) {
        return JAVACALL_FAIL;
    }
    par->atr = javacall_malloc(rxlen);
    if (par->atr == NULL) {
        javacall_free(par);
        return JAVACALL_FAIL;
    }
    par->atr_size = rxlen;
    par->status = JAVACALL_WOULD_BLOCK;
    *context = (void*)par;
    hJavaThread = CreateThread(
                      NULL,              // default security attributes
                      0,                 // use default stack size
                      resetThreadProc,   // thread function
                      (LPVOID)par,       // argument to thread function
                      0,                 // use default creation flags
                      &dwJavaThreadId);  // returns the thread identifier
    return JAVACALL_WOULD_BLOCK;
}

/**
 * Finished 'RESET' command on device and gets ATR into specified buffer.
 * Must be called after CARD_READER_DATA_SIGNAL with SIGNAL_RESET parameter is
 * received.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context the context saved during asynchronous operation.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call
 *         this function again to complete the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_reset_finish(char *atr,
                                                 javacall_int32 *atr_size,
                                                 void *context) {
    struct RESET_PARAMS *par = (struct RESET_PARAMS*) context;
    int rxlen = *atr_size;
    javacall_result status = par->status;

    if (status == JAVACALL_WOULD_BLOCK) {
        return JAVACALL_WOULD_BLOCK;
    }
    if (status != JAVACALL_OK) {
        return status;
    }
    if (rxlen < par->atr_size) {
        javacall_free(par->atr);
        javacall_free(par);
        return JAVACALL_FAIL;
    }
    memcpy(atr, par->atr, par->atr_size);
    *atr_size = par->atr_size;
    javacall_free(par->atr);
    javacall_free(par);
    return JAVACALL_OK;
}

/**
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call
 *         the javacall_carddevice_xfer_data_finish function to complete
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_xfer_data_start(char *tx_buffer,
                                                    javacall_int32 tx_size,
                                                    char *rx_buffer,
                                                    javacall_int32 *rx_size,
                                                    void **context) {
    struct XFER_PARAMS *par;
    DWORD dwJavaThreadId;   // unused
    HANDLE hJavaThread;     // unused
    int rxlen = *rx_size;
    (void)rx_buffer;

    par = javacall_malloc(sizeof *par);
    if (par == NULL) {
        return JAVACALL_FAIL;
    }
    par->rx_buffer = javacall_malloc(rxlen);
    if (par->rx_buffer == NULL) {
        javacall_free(par);
        return JAVACALL_FAIL;
    }
    par->rx_length = rxlen;
    par->tx_buffer = javacall_malloc(tx_size);
    if (par->tx_buffer == NULL) {
        javacall_free(par->rx_buffer);
        javacall_free(par);
        return JAVACALL_FAIL;
    }
    par->tx_length = tx_size;
    memcpy(par->tx_buffer, tx_buffer, tx_size);
    par->status = JAVACALL_WOULD_BLOCK;
    *context = (void*)par;
    hJavaThread = CreateThread(
                      NULL,              // default security attributes
                      0,                 // use default stack size
                      xferThreadProc,    // thread function
                      (LPVOID)par,       // argument to thread function
                      0,                 // use default creation flags
                      &dwJavaThreadId);  // returns the thread identifier
    return JAVACALL_WOULD_BLOCK;
}

/**
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_WOULD_BLOCK caller must call
 *         this function again to complete the operation
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_xfer_data_finish(char *tx_buffer,
                                                     javacall_int32 tx_size,
                                                     char *rx_buffer,
                                                     javacall_int32 *rx_size,
                                                     void *context) {
    struct XFER_PARAMS *par = (struct XFER_PARAMS*) context;
    int rxlen = *rx_size;
    javacall_result status = par->status;

    (void)tx_buffer;
    (void)tx_size;
    if (status == JAVACALL_WOULD_BLOCK) {
        return JAVACALL_WOULD_BLOCK;
    }
    if (status != JAVACALL_OK) {
        return status;
    }
    if (rxlen < par->rx_length) {
        javacall_free(par->rx_buffer);
        javacall_free(par);
        return JAVACALL_FAIL;
    }
    memcpy(rx_buffer, par->rx_buffer, par->rx_length);
    *rx_size = par->rx_length;
    javacall_free(par->rx_buffer);
    javacall_free(par->tx_buffer);
    javacall_free(par);
    return JAVACALL_OK;
}

/**
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>JAVACALL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>JAVACALL_FALSE</code> otherwise
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call
 *         the javacall_carddevice_is_sat_finish function to complete
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_is_sat_start(javacall_int32 slot,
                                                 javacall_bool *result,
                                                 void **context) {
    javacall_int32 rx_length;
    unsigned char rx_buffer[2];
    javacall_result status;

    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }

    if (slot == 0) {
        if (IsSatSlot != NOT_INITIALIZED) {
            *result = IsSatSlot;
		    return JAVACALL_OK;
        }
    }
    else {
        *result = JAVACALL_FALSE;
		return JAVACALL_OK;
    }

	rx_length = sizeof rx_buffer;
    status = javacall_carddevice_xfer_data_start(satselectcmd.command, satselectcmd.len, (char *)rx_buffer,
                                                 &rx_length, context);
	if (status != JAVACALL_OK) {
        *result = JAVACALL_FALSE;
		return status;
	}

    if (rx_buffer[0] == 0x90  &&  rx_buffer[1] == 0x00)
        *result = JAVACALL_TRUE;
    else
        *result = JAVACALL_FALSE;

    IsSatSlot = *result;

    return JAVACALL_OK;
}

/**
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>JAVACALL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>JAVACALL_FALSE</code> otherwise
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_WOULD_BLOCK caller must call
 *         this function again to complete the operation
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_is_sat_finish(javacall_int32 slot,
                                                 javacall_bool *result,
                                                 void *context) {
    javacall_int32 rx_length;
    unsigned char rx_buffer[2];
    javacall_result status;

    if (!DriverInitialized) {
        javacall_carddevice_set_error("Driver is not initialized");
        return JAVACALL_FAIL;
    }

	rx_length = sizeof rx_buffer;
    status = javacall_carddevice_xfer_data_finish(satselectcmd.command, satselectcmd.len, (char *)rx_buffer,
                                                 &rx_length, context);
	if (status != JAVACALL_OK) {
        *result = JAVACALL_FALSE;
		return status;
	}

    if (rx_buffer[0] == 0x90  &&  rx_buffer[1] == 0x00)
        *result = JAVACALL_TRUE;
    else
        *result = JAVACALL_FALSE;

    IsSatSlot = *result;

    return JAVACALL_OK;
}

/**
 * Clears error state.
 */
void javacall_carddevice_clear_error() { // empty
}

/**
 * Sets error state and stores message (like printf).
 * @param fmt printf-like format string
 */
void javacall_carddevice_set_error(const char *fmt, ...) {
	va_list ap;
    int len = 0;

	va_start(ap, fmt);
    len += sprintf(print_buffer + len, "CARDDEVICE ERROR: ");
    len += _vsnprintf(print_buffer + len, PRINT_BUFFER_SIZE - len, fmt, ap);
    len += sprintf(print_buffer + len, "\n");
	javautil_debug_print (JAVACALL_LOG_ERROR, "carddevice_simu", print_buffer);
	va_end(ap);

}

/**
 * Retrieves error message into the provided buffer and clears state.
 * @param buf Buffer to store message
 * @param buf_size Size of the buffer in bytes
 * @return JAVACALL_TRUE if error messages were returned, JAVACALL_FALSE otherwise
 */
javacall_bool javacall_carddevice_get_error(char *buf, javacall_int32 buf_size) {
    return JAVACALL_FALSE;
}
