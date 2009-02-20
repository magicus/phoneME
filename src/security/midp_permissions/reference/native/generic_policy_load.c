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
#include <string.h>

typedef const char* string;
static string domainTBL[] = {
    "manufacturer",
    "operator",
    "identified_third_party",
    "unidentified_third_party,unsecured",
    "minimum,unsecured",
    "maximum,unsecured"
};
static const int domainTBLsize = sizeof(domainTBL) / sizeof(string);

static string groupTBL [] = {
    "net_access",
    "low_level_net_access",
    "call_control",
    "application_auto_invocation",
    "local_connectivity",
    "messaging",
    "restricted_messaging",
    "multimedia_recording",
    "read_user_data_access",
    "write_user_data_access",
    "location",
    "landmark",
    "payment",
    "authentication",
    "smart_card",
    "satsa"
};
static const int groupTBLsize = sizeof(groupTBL) / sizeof(string);

static string net_access_members[] = {
    "javax.microedition.io.Connector.http",
    "javax.microedition.io.Connector.https",
    "javax.microedition.io.Connector.obex.client.tcp",
    "javax.microedition.io.Connector.obex.server.tcp"
};

static string low_level_net_access_members[] = {
    "javax.microedition.io.Connector.datagram",
    "javax.microedition.io.Connector.datagramreceiver",
    "javax.microedition.io.Connector.socket",
    "javax.microedition.io.Connector.serversocket",
    "javax.microedition.io.Connector.ssl"
};

static string call_control_members[] = {
    "javax.microedition.io.Connector.sip",
    "javax.microedition.io.Connector.sips"
};
static string messaging_members[] = {
    "javax.wireless.messaging.sms.send",
    "javax.wireless.messaging.mms.send",
    "javax.microedition.io.Connector.sms",  
    "javax.wireless.messaging.sms.receive",  
    "javax.microedition.io.Connector.mms",
    "javax.wireless.messaging.mms.receive"
};
static string restricted_messaging_members[] = {
    "javax.wireless.messaging.cbs.receive",
    "javax.microedition.io.Connector.cbs"
};

static string application_auto_invocation_members[] = {
    "javax.microedition.io.PushRegistry",
    "javax.microedition.content.ContentHandler"
};
static string local_connectivity_members[] = {
    "javax.microedition.io.Connector.comm",
    "javax.microedition.io.Connector.obex.client",
    "javax.microedition.io.Connector.obex.server",
    "javax.microedition.io.Connector.bluetooth.client",
    "javax.microedition.io.Connector.bluetooth.server"
};
static string smart_card_members[] = {
    "javax.microedition.apdu.aid",
    "javax.microedition.jcrmi"
};

static string authentication_members[] = {
    "javax.microedition.securityservice.CMSMessageSignatureService"
};

static string multimedia_recording_members[] = {
    "javax.microedition.media.control.RecordControl",
    "javax.microedition.media.control.VideoControl.getSnapshot",
    "javax.microedition.amms.control.camera.enableShutterFeedback"
};

static string read_user_data_access_members[] = {
    "javax.microedition.pim.ContactList.read",
    "javax.microedition.pim.EventList.read",
    "javax.microedition.pim.ToDoList.read",
    "javax.microedition.io.Connector.file.read"
};

static string write_user_data_access_members[] = {
    "javax.microedition.pim.ContactList.write",
    "javax.microedition.pim.EventList.write",
    "javax.microedition.pim.ToDoList.write",
    "javax.microedition.io.Connector.file.write",
    "javax.microedition.amms.control.tuner.setPreset"
};

static string location_members[] = {
    "javax.microedition.location.Location",
    "javax.microedition.location.ProximityListener",
    "javax.microedition.location.Orientation"
};

static string landmark_members[] = {
    "javax.microedition.location.LandmarkStore.read",
    "javax.microedition.location.LandmarkStore.write",
    "javax.microedition.location.LandmarkStore.category",
    "javax.microedition.location.LandmarkStore.management"
};

static string payment_members[] = {
    "javax.microedition.payment.process"
};

static string satsa_members[] = {
    "javax.microedition.apdu.sat"
};

static struct _group_member {
    string *list;
    int    size;
} group_membersTBL[] = {
    {net_access_members, sizeof(net_access_members)/sizeof(string)},
    {low_level_net_access_members, 
                sizeof(low_level_net_access_members)/sizeof(string)},
    {call_control_members, 
                sizeof(call_control_members)/sizeof(string)},
    {application_auto_invocation_members, 
                sizeof(application_auto_invocation_members)/sizeof(string)},
    {local_connectivity_members, 
                sizeof(local_connectivity_members)/sizeof(string)},
    {messaging_members, 
                sizeof(messaging_members)/sizeof(string)},
    {restricted_messaging_members,
                sizeof(restricted_messaging_members)/sizeof(string)},
    {multimedia_recording_members, 
                sizeof(multimedia_recording_members)/sizeof(string)},
    {read_user_data_access_members, 
                sizeof(read_user_data_access_members)/sizeof(string)},
    {write_user_data_access_members, 
                sizeof(write_user_data_access_members)/sizeof(string)},
    {location_members, sizeof(location_members)/sizeof(string)},
    {landmark_members, sizeof(landmark_members)/sizeof(string)},
    {payment_members, sizeof(payment_members)/sizeof(string)},
    {authentication_members, 
                sizeof(authentication_members)/sizeof(string)},
    {smart_card_members, 
                sizeof(smart_card_members)/sizeof(string)},
    {satsa_members, sizeof(satsa_members)/sizeof(string)}
};

#define NEVER    0
#define ALLOW    1
#define BLANKET  4
#define SESSION  8
#define ONESHOT  16

typedef struct _values {
    int maxval;
    int defval;
} values;

static values identifiedTBL[] = {
    {BLANKET, SESSION},
    {BLANKET, SESSION},
    {BLANKET, ONESHOT},
    {BLANKET, ONESHOT},
    {BLANKET, SESSION},
    {BLANKET, ONESHOT},
    {BLANKET, ONESHOT},
    {BLANKET, SESSION},
    {BLANKET, ONESHOT},
    {BLANKET, ONESHOT},
    {BLANKET, SESSION},
    {BLANKET, SESSION},
    {ALLOW,   ALLOW},
    {BLANKET, SESSION},
    {BLANKET, SESSION},
    {NEVER,   NEVER},
};

static values unidentifiedTBL[] = {
    {SESSION, ONESHOT},
    {SESSION, ONESHOT},
    {ONESHOT, ONESHOT},
    {SESSION, ONESHOT},
    {BLANKET, ONESHOT},
    {ONESHOT, ONESHOT},
    {ONESHOT, ONESHOT},
    {SESSION, ONESHOT},
    {ONESHOT, ONESHOT},
    {ONESHOT, ONESHOT},
    {SESSION, ONESHOT},
    {SESSION, ONESHOT},
    {NEVER,   NEVER},
    {NEVER,   NEVER},
    {NEVER,   NEVER},
    {NEVER,   NEVER},
};


int permissions_load_domain_list(void** array) {
    *array = (void *)domainTBL;
    return domainTBLsize;
}
int permissions_load_group_list(void** array) {
    *array = (void *)groupTBL;
    return groupTBLsize;
}

int permissions_load_group_permissions(void** array, char* group_name) {
    int i1;
    int ret = 0;
    for (i1 = 0; i1 < groupTBLsize; i1++) {
        if (strcmp(groupTBL[i1], group_name) == 0) {
            *array = (void *)group_membersTBL[i1].list;
            ret = group_membersTBL[i1].size;
            break;
        }
    }
    return ret;
}

static int permissions_get1_value(values *tbl, char *group_name, int isMax) {
    int i1;
    for (i1 = 0; i1 < groupTBLsize; i1++) {
        if (strcmp(groupTBL[i1], group_name) == 0) {
            if (isMax) 
                return (int)tbl[i1].maxval;
            else
                return (int)tbl[i1].defval;
        }
    }
    return NEVER;
}
static int permissions_get_value(char* domain_name, char* group_name, int isMax) {
    int val = NEVER;
    if (strcmp("manufacturer", domain_name) == 0 ||
        strcmp("maximum", domain_name) == 0 ||
        strcmp("operator", domain_name) == 0){
        return ALLOW;
    } else if (strcmp("mimimum", domain_name) == 0) {
        return NEVER;
    } else if (strcmp("identified_third_party", domain_name) == 0) {
        val = permissions_get1_value(identifiedTBL, group_name, isMax);
    } else if (strcmp("unidentified_third_party", domain_name) == 0) {
        val = permissions_get1_value(unidentifiedTBL, group_name, isMax);
    }

    return val;
}

int permissions_get_default_value(char* domain_name, char* group_name) {
    return permissions_get_value(domain_name,group_name, 0);
}

int permissions_get_max_value(char* domain_name, char* group_name) {
    return permissions_get_value(domain_name,group_name, 1);
}

void permissions_dealloc(void* array) {
    array=NULL;
    return; //all arrays are static
}

void permissions_loading_finished() {
}

#define DEF_NUM_OF_LINES 6
static struct _messages {
    string list[DEF_NUM_OF_LINES];
} messagesTBL[] = {
    {{"Airtime",
    "How often should %1 ask for permission to use airtime? Using airtime may result in charges.",
    "Don't use airtime and don't ask",
    "Is it OK to Use Airtime?",
    "%1 wants to send and receive data using the network. This will use airtime and may result in charges.\n\nIs it OK to use airtime?",
    }},
    {{"Network",
    "How often should %1 ask for permission to use network? Using network may result in charges.",
    "Don't use network and don't ask",
    "Is it OK to Use Network?",
    "%1 wants to send and receive data using the network. This will use airtime and may result in charges.\n\nIs it OK to use network?"
    }},
    {{"Restricted Network Connections",
    "How often should %1 ask for permission to open a restricted network connection?",
    "Don't open any restricted connections and don't ask",
    "Is it OK to open a restricted network connection?",
    "%1 wants to open a restricted network connection.\n\nIs it OK to open a restricted network connection?"
    }},
    {{"Auto-Start Registration",
    "How often should %1 ask for permission to register itself to automatically start?",
    "Don't register and don't ask",
    "Is it OK to automatically start the application?",
    "%1 wants to register itself to be automatically started.\n\nIs it OK to register to be automatically started?"
    }},
    {{"Computer Connection",
    "How often should %1 ask for permission to connect to a computer? This may require a data cable that came with your phone.",
    "Don't connect and don't ask",
    "Is it OK to Connect?",
    "%1 wants to connect to a computer. This may require a data cable.\n\nIs it OK to make a connection?"
    }},
    {{"Messaging",
    "How often should %1 ask for permission before sending or receiving text messages?",
    "Don't send or receive messages and don't ask",
    "Is it OK to Send Messages?",
    "%1 wants to send text message(s). This could result in charges.%3 message(s) will be sent to %2.\n\nIs it OK to send messages?"
    }},
    {{"Secured Messaging",
    "How often should %1 ask for permission before sending or receiving secured text messages?",
    "Don't send or receive secured messages and don't ask",
    "Is it OK to Send secured Messages?",
    "%1 wants to send text secured message(s). This could result in charges.%3 message(s) will be sent to %2.\n\nIs it OK to send messages?"
    }},
    {{"Recording",
    "How often should %1 ask for permission to record audio and images? This will use space on your phone.",
    "Don't record and don't ask",
    "Is it OK to Record?",
    "%1 wants to record an image or audio clip.\n\nIs it OK to record?"
    }},
    {{"Read Personal Data",
    "How often should %1 ask for permission to read your personal data (contacts, appointments, etc)?",
    "Don't read my data and don't ask",
    "Is it OK to read your personal data?",
    "%1 wants to read your personal data (contacts, appointments, etc)\n\nIs it OK to read your personal data?"
    }},
    {{"Update Personal Data",
    "How often should %1 ask for permission to update your personal data (contacts, appointments, etc)?",
    "Don't update my data and don't ask",
    "Is it OK to update your personal data?",
    "%1 wants to update your personal data (contacts, appointments, etc)\n\nIs it OK to update your personal data?",
    "%1 wants to update %2\n\nIs it OK to update this data?"
    }},
    {{"Obtain Current Location",
    "How often should %1 ask for permission to obtain your location?",
    "Don't give my location and don't ask",
    "Is it OK to obtain your location?",
    "Application %1 wants to obtain your the location.\n\nIs it OK to obtain your location?"
    }},
    {{"Access Landmark Database",
    "How often should %1 ask for permission to access your landmark database?",
    "Don't access my landmark database and don't ask",
    "Is it OK to access your landmark database?",
    "Application %1 wants to access your landmark database.\n\nIs it OK to access your landmark database?"
    }},
    {{"payment"}},
    {{"Personal Indentification",
    "How often should %1 ask for permission to use your smart card to identify you?",
    "Don't sign and don't ask",
    "Is it OK to obtain your personal signature?",
    "%1 wants to obtain your personal digital signature.\n\nIs it OK to obtain your personal signature?\nContent to be signed:\n\n%3"
    }},
    {{"Smart Card Communication",
    "How often should %1 ask for permission to access your smart card?",
    "Don't access my smart card and don't ask",
    "Is it OK to access your smart card?",
    "Application %1 wants to access your smart card.\n\nIs it OK to access your smart card?"
    }},
    {{"satsa"}}

};

int permissions_load_group_messages(void** array, char* group_name) {
    int i1, i2;
    int ret = 0;
    for (i1 = 0; i1 < groupTBLsize; i1++) {
        if (strcmp(groupTBL[i1], group_name) == 0) {
            *array = (void *)messagesTBL[i1].list;
            for (i2 = DEF_NUM_OF_LINES-1; i2 >= 0; i2--) {
                if (messagesTBL[i1].list[i2] != NULL)
                    break;
            }
            ret = i2+1;
            break;
        }
    }
    return ret;
}



