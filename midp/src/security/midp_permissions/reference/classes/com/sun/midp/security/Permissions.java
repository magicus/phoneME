/*
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
 */

package com.sun.midp.security;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

/**
 * This class is a standard list of permissions that
 * a suite can do and is used by all internal security
 * features. This class also builds a list of permission for each
 * security domain. This only class that would need to be updated in order to
 * add a new security domain.
 */
public final class Permissions {

    /** Binding name of the Manufacturer domain. (all permissions allowed) */
    public static final String MANUFACTURER_DOMAIN_BINDING = "manufacturer";

    /** Binding name of the Operator domain. */
    public static final String OPERATOR_DOMAIN_BINDING = "operator";

    /** Binding name of the Third party Identified domain. */
    public static final String IDENTIFIED_DOMAIN_BINDING = "identified";

    /** Binding name of the Third party Unidentified domain. */
    public static final String UNIDENTIFIED_DOMAIN_BINDING = "unidentified";

    /**
     * Binding name of the Minimum domain for testing.
     * (all permissions denied)
     */
    public static final String MINIMUM_DOMAIN_BINDING = "minimum";

    /**
     * Binding name of the Maximum domain for testing.
     * (all public permissions allowed)
     */
    public static final String MAXIMUM_DOMAIN_BINDING = "maximum";

    /**
     * The maximum levels are held in the first element of the permissions
     * array.
     */
    public static final int MAX_LEVELS = 0;
    /**
     * The current levels are held in the first element of the permissions
     * array.
     */
    public static final int CUR_LEVELS = 1;

    /** com.sun.midp permission ID. */
    public static final int MIDP = 0;
    /** com.sun.midp.midletsuite.ams permission ID. */
    public static final int AMS = 1;
    /** javax.microedition.io.Connector.http permission ID. */
    public static final int HTTP = 2;
    /** javax.microedition.io.Connector.socket permission ID. */
    public static final int TCP = 3;
    /** javax.microedition.io.Connector.https permission ID. */
    public static final int HTTPS = 4;
    /** javax.microedition.io.Connector.ssl permission ID. */
    public static final int SSL = 5;
    /** javax.microedition.io.Connector.serversocket permission ID. */
    public static final int TCP_SERVER = 6;
    /** javax.microedition.io.Connector.datagram permission ID. */
    public static final int UDP = 7;
    /** javax.microedition.io.Connector.datagramreceiver permission ID. */
    public static final int UDP_SERVER = 8;
    /** javax.microedition.io.Connector.comm permission ID. */
    public static final int COMM = 9;
    /** javax.microedition.io.PushRegistry permission ID. */
    public static final int PUSH = 10;
    /** javax.microedition.io.Connector.sms permission ID. */
    public static final int SMS_SERVER = 11;
    /** javax.microedition.io.Connector.cbs permission ID. */
    public static final int CBS_SERVER = 12;
    /** javax.wireless.messaging.sms.send permission ID. */
    public static final int SMS_SEND = 13;
    /** javax.wireless.messaging.sms.receive permission ID. */
    public static final int SMS_RECEIVE = 14;
    /** javax.wireless.messaging.scbs.receive permission ID. */
    public static final int CBS_RECEIVE = 15;
    /** javax.microedition.media.RecordControl permission ID. */
    public static final int MM_RECORD = 16;
    /** javax.microedition.media.VideoControl.getSnapshot permission ID. */
    public static final int MM_IMAGE_CAPTURING = 17;
    /** javax.microedition.io.Connector.mms permission ID. */
    public static final int MMS_SERVER = 18;
    /** javax.wireless.messaging.mms.send permission ID. */
    public static final int MMS_SEND = 19;
    /** javax.wireless.messaging.mms.receive permission ID. */
    public static final int MMS_RECEIVE = 20;
    /** javax.microedition.apdu.aid permission ID. */
    public static final int APDU_CONNECTION = 21;
    /** javax.microedition.jcrmi permission ID. */
    public static final int JCRMI_CONNECTION = 22;
    /**
     * javax.microedition.securityservice.CMSSignatureService
     * permission ID.
     */
    public static final int SIGN_SERVICE = 23;
    /** javax.microedition.apdu.sat permission ID. */
    public static final int APDU_CHANNEL0 = 24;

    /** javax.microedition.content.ContentHandler permission ID. */
    public static final int CHAPI_REGISTER = 25;
    
    /** javax.microedition.pim.ContactList.read ID. */
    public static final int PIM_CONTACT_READ = 26;
    /** javax.microedition.pim.ContactList.write ID. */
    public static final int PIM_CONTACT_WRITE = 27;
    /** javax.microedition.pim.EventList.read ID. */
    public static final int PIM_EVENT_READ = 28;
    /** javax.microedition.pim.EventList.write ID. */
    public static final int PIM_EVENT_WRITE = 29; 
    /** javax.microedition.pim.ToDoList.read ID. */
    public static final int PIM_TODO_READ = 30;
    /** javax.microedition.pim.ToDoList.write ID. */
    public static final int PIM_TODO_WRITE = 31;
    /** javax.microedition.io.Connector.file.read ID. */
    public static final int FILE_CONNECTION_READ = 32;
    /** javax.microedition.io.Connector.file.write ID. */
    public static final int FILE_CONNECTION_WRITE = 33;

    /** javax.microedition.io.Connector.obex.client ID. */
    public static final int OBEX_CLIENT = 34;
    /** javax.microedition.io.Connector.obex.server ID. */
    public static final int OBEX_SERVER = 35;
    /** javax.microedition.io.Connector.obex.client.tcp ID. */
    public static final int TCP_OBEX_CLIENT = 36;
    /** javax.microedition.io.Connector.obex.server.tcp ID. */
    public static final int TCP_OBEX_SERVER = 37;

    /** javax.microedition.io.Connector.bluetooth.client ID. */
    public static final int BLUETOOTH_CLIENT = 38;
    /** javax.microedition.io.Connector.bluetooth.server ID. */
    public static final int BLUETOOTH_SERVER = 39;

    /** javax.microedition.location.Location ID. */
    public static final int LOCATION = 40;
    /** javax.microedition.location.Orientation ID. */
    public static final int ORIENTATION = 41;
    /** javax.microedition.location.ProximityListener ID. */
    public static final int LOCATION_PROXIMITY = 42;

    /** javax.microedition.location.LandmarkStore.read ID. */
    public static final int LANDMARK_READ = 43;
    /** javax.microedition.location.LandmarkStore.write ID. */
    public static final int LANDMARK_WRITE = 44;
    /** javax.microedition.location.LandmarkStore.category ID. */
    public static final int LANDMARK_CATEGORY = 45;
    /** javax.microedition.location.LandmarkStore.management ID. */
    public static final int LANDMARK_MANAGE = 46;
    /** javax.microedition.io.Connector.sip permission ID. */
    public static final int SIP = 47;
    /** javax.microedition.io.Connector.sips permission ID. */
    public static final int SIPS = 48;
    /** javax.microedition.payment.process permission ID. */
    public static final int PAYMENT = 49;

    /** javax.microedition.amms.control.camera.enableShutterFeedback perm. ID */
    public static final int AMMS_CAMERA_SHUTTERFEEDBACK = 50;    
    /** javax.microedition.amms.control.tuner.setPreset permission ID. */
    public static final int AMMS_TUNER_SETPRESET = 51;

    /** Number of permissions. */
    public static final int NUMBER_OF_PERMISSIONS = 52;

    /** Never allowed an permission. */
    public static final byte NEVER = 0;
    /** Allow an permission with out asking the user. */
    public static final byte ALLOW = 1;
    /** Allow permission until the the user changes it in the settings form. */
    public static final byte BLANKET_GRANTED = 2;
    /** Allow a permission after asking the user once. */
    public static final byte BLANKET = 4;
    /** Allow an permission after asking the user once a session. */
    public static final byte SESSION = 8;
    /** Allow an permission after asking the user every use. */
    public static final byte ONE_SHOT = 16;
    /** Denied by the user, until next session. */
    public static final byte DENY_SESSION = 32;
    /** Ask the user to Deny by default. */
    public static final byte DENY = 64;
    /** Deny by the user, until the user changes it in the settings form. */
    public static final byte USER_DENIED = -128;

    /** Third Party Never permission group. */
    static final PermissionGroup NEVER_GROUP =
        new PermissionGroup(0, 0, 0, 0, 0, 0, NEVER, NEVER, NEVER, NEVER);

    /** Third Party Allowed permission group. */
    static final PermissionGroup ALLOWED_GROUP =
        new PermissionGroup(0, 0, 0, 0, 0, 0, ALLOW, ALLOW, ALLOW, ALLOW);

    /** Idenitified Third Party Allowed permission group. */
    static final PermissionGroup ID_ALLOWED_GROUP =
        new PermissionGroup(0, 0, 0, 0, 0, 0, ALLOW, ALLOW, NEVER, NEVER);

    /** Net Access permission group. */
    static final PermissionGroup NET_ACCESS_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_NET_SETTINGS,
        ResourceConstants.AMS_MGR_NET_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_NET_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_NET_ACCESS_DIALOG_TITLE,
        ResourceConstants.PERMISSION_NET_ACCESS_QUE, 0,
        BLANKET, SESSION, SESSION, ONE_SHOT);

    /** Read Message permission group. */
    static final PermissionGroup READ_MESSAGE_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_MSG_SETTINGS,
        ResourceConstants.AMS_MGR_MSG_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_MSG_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_RECEIVE_MESSAGE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_RECEIVE_MESSAGE_QUE, 0,
        BLANKET, ONE_SHOT, ONE_SHOT, ONE_SHOT);

    /**
     * Send Message permission group. Send was broken out because send
     * is treated as one shot even though it is in the messaging group.
     */
    static final PermissionGroup SEND_MESSAGE_GROUP = new PermissionGroup(
        0, 0, 0, 
        ResourceConstants.PERMISSION_SEND_MESSAGE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_SEND_MESSAGE_QUE, 0,
        ONE_SHOT, ONE_SHOT, ONE_SHOT, ONE_SHOT);

    /** Application Auto Invocation permission group. */
    static final PermissionGroup AUTO_INVOCATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS,
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_AUTO_START_DIALOG_TITLE,
        ResourceConstants.PERMISSION_AUTO_START_QUE, 0,
        BLANKET, ONE_SHOT, SESSION, ONE_SHOT);

    /** Local Connectivity permission group. */
    static final PermissionGroup LOCAL_CONN_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS,
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LOCAL_CONN_DIALOG_TITLE,
        ResourceConstants.PERMISSION_LOCAL_CONN_QUE, 0,
        BLANKET, SESSION, BLANKET, ONE_SHOT);

    /** Multimedia Recording permission group. */
    static final PermissionGroup MULTIMEDIA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_REC_SETTINGS,
        ResourceConstants.AMS_MGR_REC_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_REC_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_MULTIMEDIA_DIALOG_TITLE,
        ResourceConstants.PERMISSION_MULTIMEDIA_QUE, 0,
        BLANKET, SESSION, SESSION, ONE_SHOT);

    /** Read User Data permission group. */
    static final PermissionGroup READ_USER_DATA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS,
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_READ_USER_DATA_TITLE,
        ResourceConstants.PERMISSION_READ_USER_DATA_QUE, 0,
        BLANKET, ONE_SHOT, ONE_SHOT, ONE_SHOT);

    /** Write User Data permission group. */
    static final PermissionGroup WRITE_USER_DATA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS,
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_TITLE,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_QUE,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_ONESHOT_QUE,
        BLANKET, ONE_SHOT, ONE_SHOT, ONE_SHOT);

    /** Location permission group. */
    static final PermissionGroup LOCATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LOC_SETTINGS,
        ResourceConstants.AMS_MGR_LOC_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LOC_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LOCATION_TITLE,
        ResourceConstants.PERMISSION_LOCATION_QUE, 0,
        BLANKET, SESSION, SESSION, ONE_SHOT);

    /** Landmark store permission group. */
    static final PermissionGroup LANDMARK_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS,
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LANDMARK_TITLE, 0,
        ResourceConstants.PERMISSION_LANDMARK_QUE,
        BLANKET, SESSION, SESSION, ONE_SHOT);

    /** Authentication (identification) permission group. */
    static final PermissionGroup AUTHENTICATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS,
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_SIGNATURE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_SIGNATURE_QUE, 0,
        ONE_SHOT, ONE_SHOT, NEVER, NEVER);

    /** Call Control (restricted network connection) permission group. */
    static final PermissionGroup CALL_CONTROL_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS,
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_CALL_CONTROL_TITLE,
        ResourceConstants.PERMISSION_CALL_CONTROL_QUE, 0,
        BLANKET, ONE_SHOT, ONE_SHOT, ONE_SHOT);

    /** Permission specifications. */
    static final PermissionSpec[] permissionSpecs = {
        new PermissionSpec("com.sun.midp", NEVER_GROUP),
        new PermissionSpec("com.sun.midp.midletsuite.ams", NEVER_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.http",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.socket",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.https",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.ssl",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.serversocket",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.datagram",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.datagramreceiver",
            NET_ACCESS_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.comm",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.PushRegistry",
            AUTO_INVOCATION_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.sms",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.cbs",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.wireless.messaging.sms.send",
            SEND_MESSAGE_GROUP),
        new PermissionSpec("javax.wireless.messaging.sms.receive",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.wireless.messaging.cbs.receive",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.microedition.media.control.RecordControl",
            MULTIMEDIA_GROUP),
        new PermissionSpec(
            "javax.microedition.media.control.VideoControl.getSnapshot",
            MULTIMEDIA_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.mms",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.wireless.messaging.mms.send",
            SEND_MESSAGE_GROUP),
        new PermissionSpec("javax.wireless.messaging.mms.receive",
            READ_MESSAGE_GROUP),
        new PermissionSpec("javax.microedition.apdu.aid",
            NEVER_GROUP),
        new PermissionSpec("javax.microedition.jcrmi",
            NEVER_GROUP),
        new PermissionSpec(
            "javax.microedition.securityservice.CMSMessageSignatureService",
            AUTHENTICATION_GROUP),
        new PermissionSpec("javax.microedition.apdu.sat",
            NEVER_GROUP),
        new PermissionSpec("javax.microedition.content.ContentHandler",
            AUTO_INVOCATION_GROUP),
        new PermissionSpec("javax.microedition.pim.ContactList.read",
            READ_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.pim.ContactList.write",
            WRITE_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.pim.EventList.read",
            READ_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.pim.EventList.write",
            WRITE_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.pim.ToDoList.read",
            READ_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.pim.ToDoList.write",
            WRITE_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.file.read",
            READ_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.file.write",
            WRITE_USER_DATA_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.obex.client",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.obex.server",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.obex.client.tcp",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.obex.server.tcp",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.bluetooth.client",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.bluetooth.server",
            LOCAL_CONN_GROUP),
        new PermissionSpec("javax.microedition.location.Location",
            LOCATION_GROUP),
        new PermissionSpec("javax.microedition.location.Orientation",
            ALLOWED_GROUP),
        new PermissionSpec("javax.microedition.location.ProximityListener",
            LOCATION_GROUP),
        new PermissionSpec("javax.microedition.location.LandmarkStore.read",
            LANDMARK_GROUP),
        new PermissionSpec("javax.microedition.location.LandmarkStore.write",
            LANDMARK_GROUP),
        new PermissionSpec(
            "javax.microedition.location.LandmarkStore.category",
            NEVER_GROUP),
        new PermissionSpec(
            "javax.microedition.location.LandmarkStore.management",
            NEVER_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.sip",
            CALL_CONTROL_GROUP),
        new PermissionSpec("javax.microedition.io.Connector.sips",
            CALL_CONTROL_GROUP),
        new PermissionSpec("javax.microedition.payment.process",
            ID_ALLOWED_GROUP),
        new PermissionSpec(
            "javax.microedition.amms.control.camera.enableShutterFeedback",
            MULTIMEDIA_GROUP),
        new PermissionSpec(
            "javax.microedition.amms.control.tuner.setPreset",
            WRITE_USER_DATA_GROUP)
    };

    /**
     * Get the name of a permission.
     *
     * @param permission permission number
     *
     * @return permission name
     *
     * @exception SecurityException if the permission is invalid
     */
    public static String getName(int permission) {
        if (permission < 0 || permission >= permissionSpecs.length) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }

        return permissionSpecs[permission].name;
    }

    /**
     * Get the dialog title for a permission.
     *
     * @param permission permission number
     *
     * @return Resource constant for the permission dialog title
     * @exception SecurityException if the permission is invalid
     */
    public static int getTitle(int permission) {
        if (permission < 0 || permission >= permissionSpecs.length) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }

        return permissionSpecs[permission].group.getRuntimeDialogTitle();
    }

    /**
     * Get the question for a permission.
     *
     * @param permission permission number
     *
     * @return Resource constant for the permission question
     *
     * @exception SecurityException if the permission is invalid
     */
    public static int getQuestion(int permission) {
        if (permission < 0 || permission >= permissionSpecs.length) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }

        return permissionSpecs[permission].group.getRuntimeQuestion();
    }

    /**
     * Get the oneshot question for a permission.
     *
     * @param permission permission number
     *
     * @return Resource constant for the permission question
     *
     * @exception SecurityException if the permission is invalid
     */
    public static int getOneshotQuestion(int permission) {
        if (permission < 0 || permission >= permissionSpecs.length) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }

        return permissionSpecs[permission].group.getRuntimeOneshotQuestion();
    }

    /**
     * Determine if a domain is a trusted domain.
     *
     * @param domain Binding name of a domain
     *
     * @return true if a domain is trusted, false if not
     */
    public static boolean isTrusted(String domain) {
        if (MANUFACTURER_DOMAIN_BINDING.equals(domain)) {
            return true;
        }

        if (OPERATOR_DOMAIN_BINDING.equals(domain)) {
            return true;
        }

        if (MAXIMUM_DOMAIN_BINDING.equals(domain)) {
            return true;
        }

        if (IDENTIFIED_DOMAIN_BINDING.equals(domain)) {
            return true;
        }

        return false;
    }

    /**
     * Create a list of permission groups a domain is permitted to perform.
     *
     * @param name binding name of domain
     *
     * @return 2 arrays, the first containing the maximum level for each
     *     permission, the second containing the default or starting level
     *     for each permission supported
     */
    public static byte[][] forDomain(String name) {
        byte[] maximums = new byte[NUMBER_OF_PERMISSIONS];
        byte[] defaults = new byte[NUMBER_OF_PERMISSIONS];
        byte[][] permissions = {maximums, defaults};

        if (MANUFACTURER_DOMAIN_BINDING.equals(name)) {
            // All permissions allowed
            for (int i = 0; i < maximums.length; i++) {
                maximums[i] = ALLOW;
                defaults[i] = ALLOW;
            }

            return permissions;
        }

        if (OPERATOR_DOMAIN_BINDING.equals(name) ||
                MAXIMUM_DOMAIN_BINDING.equals(name)) {
            for (int i = 0; i < maximums.length; i++) {
                maximums[i] = ALLOW;
                defaults[i] = ALLOW;
            }

            // Only public permissions allowed, never internal
            maximums[MIDP] = NEVER;
            defaults[MIDP] = NEVER;
            maximums[AMS] = NEVER;
            defaults[AMS] = NEVER;

            return permissions;
        }

        if (IDENTIFIED_DOMAIN_BINDING.equals(name)) {
            for (int i = 2; i < maximums.length; i++) {
                maximums[i] =
                    permissionSpecs[i].group.getIdentifiedMaxiumLevel();
                defaults[i] =
                    permissionSpecs[i].group.getIdentifiedDefaultLevel();
            }

            return permissions;
        }

        if (UNIDENTIFIED_DOMAIN_BINDING.equals(name)) {
            for (int i = 2; i < maximums.length; i++) {
                maximums[i] =
                    permissionSpecs[i].group.getUnidentifiedMaxiumLevel();
                defaults[i] =
                    permissionSpecs[i].group.getUnidentifiedDefaultLevel();
            }

            return permissions;
        }

        // the default domain is minimum, all permissions denied
        return permissions;
    }

    /**
     * Create an empty list of permission groups.
     *
     * @return array containing the empty permission groups
     */
    public static byte[] getEmptySet() {
        byte[] permissions = new byte[NUMBER_OF_PERMISSIONS];

        // Assume perms array is non-null
        for (int i = 0; i < permissions.length; i++) {
            // This is default permission
            permissions[i] = Permissions.NEVER;
        }

        return permissions;
    }

    /**
     * Get a list of all permission groups for the settings dialog.
     *
     * @return array of permission groups
     */
    public static PermissionGroup[] getSettingGroups() {
        PermissionGroup[] groups = new PermissionGroup[12];

        groups[0] = NET_ACCESS_GROUP;
        groups[1] = READ_MESSAGE_GROUP;
        groups[2] = SEND_MESSAGE_GROUP;
        groups[3] = AUTO_INVOCATION_GROUP;
        groups[4] = LOCAL_CONN_GROUP;
        groups[5] = MULTIMEDIA_GROUP;
        groups[6] = READ_USER_DATA_GROUP;
        groups[7] = WRITE_USER_DATA_GROUP;
        groups[8] = LOCATION_GROUP;
        groups[9] = LANDMARK_GROUP;
        groups[10] = AUTHENTICATION_GROUP;
        groups[11] = CALL_CONTROL_GROUP;

        return groups;
    }

    /**
     * Find the max level of all the permissions in the same group.
     *
     * This is a policy dependent function for permission grouping.
     *
     * @param levels array of permission levels
     * @param group desired permission group
     *
     * @return permission level
     */
    public static byte getPermissionGroupLevel(byte[] levels,
            PermissionGroup group) {
        byte maxLevel = NEVER;

        for (int i = 0; i < permissionSpecs.length; i++) {
            if (permissionSpecs[i].group == group && levels[i] != NEVER) {
                /*
                 * Except for NEVER the lower the int value the higher
                 * the permission level.
                 */
                if (levels[i] < maxLevel || maxLevel == NEVER) {
                    maxLevel = levels[i];
                }                    
            }
        }

        return maxLevel;
    }

    /**
     * Set the level of all the permissions in the same group as this
     * permission to the given level.
     * <p>
     * This is a policy dependent function for permission grouping.</p>
     *
     * The following combinations of permissions are mutually exclusive:
     * <ul>
     * <li> Any of Net Access, Messaging or Local Connectivity set to Blanket
     *      in combination with any of Multimedia recording or Read User Data
     *      Access set to Blanket</li>
     * <li> Application Auto Invocation (or push interrupt level) set to
     *      Blanket and Net Access set to Blanket</li>
     * </ul>
     *
     * @param current current permission levels
     * @param pushInterruptLevel Push interrupt level
     * @param group desired permission group
     * @param level permission level
     *
     * @exception SecurityException if the change would produce a mutually
     *                              exclusive combination
     */
    public static void setPermissionGroup(byte[] current,
            byte pushInterruptLevel, PermissionGroup group, byte level)
            throws SecurityException {

        checkForMutuallyExclusiveCombination(current, pushInterruptLevel,
                                             group, level);

        for (int i = 0; i < permissionSpecs.length; i++) {
            if (permissionSpecs[i].group == group) {
                setPermission(current, i, level);
            }
        }
    }

    /**
     * Grant or deny of a permission and all of the other permissions in
     * it group.
     * <p>
     * This is a policy dependent function for permission grouping.</p>
     *
     * This method must only be used when not changing the interaction level
     * (blanket, session, one shot).
     *
     * @param current current permission levels
     * @param permission permission ID from the group
     * @param level permission level
     * @exception SecurityException if the change would produce a mutually
     *                              exclusive combination
     */
    public static void setPermissionGroup(byte[] current, int permission,
            byte level) throws SecurityException {

        if (permission < 0 || permission >= permissionSpecs.length) {
            return;
        }

        PermissionGroup group = permissionSpecs[permission].group;

        for (int i = 0; i < permissionSpecs.length; i++) {
            if (permissionSpecs[i].group == group) {
                setPermission(current, i, level);
            }
        }
    }


    /**
     * Check to see if a given push interrupt level would produce a mutually
     * exclusive combination for the current security policy. If so, throw
     * an exception.
     * <p>
     * This is a policy dependent function for permission grouping.</p>
     *
     * The mutually combination is the push interrupt level set to Blanket and
     * Net Access set to Blanket.
     *
     * @param current current permission levels
     * @param pushInterruptLevel Push interrupt level
     *
     * @exception SecurityException if the change would produce a mutually
     *                              exclusive combination
     */
    public static void checkPushInterruptLevel(byte[] current,
            byte pushInterruptLevel) throws SecurityException {

        byte level;

        if (pushInterruptLevel != BLANKET_GRANTED) {
            return;
        }

        level = getPermissionGroupLevel(current, NET_ACCESS_GROUP);
        if (level == BLANKET_GRANTED || level == BLANKET) {
            throw new SecurityException(createMutuallyExclusiveErrorMessage(
                ResourceConstants.AMS_MGR_INTRUPT,
                NET_ACCESS_GROUP.getName()));
        }
    }

    /**
     * Set the level the permission if the permission is not set to NEVER
     * or ALLOW.
     *
     * @param current current permission levels
     * @param permission permission ID for permission to set
     * @param level permission level
     */
    private static void setPermission(byte[] current, int permission,
                                      byte level) {
        if (current[permission] != NEVER || current[permission] != ALLOW) {
            current[permission] = level;
        }
    }

    /**
     * Check to see if a given level for a group would produce a mutually
     * exclusive combination for the current security policy. If so, throw
     * an exception.
     * <p>
     * This is a policy dependent function for permission grouping.</p>
     *
     * The following combinations of permissions are mutually exclusive:
     * <ul>
     * <li> Any of Net Access, Messaging or Local Connectivity set to Blanket
     *      in combination with any of Multimedia recording or Read User Data
     *      Access set to Blanket</li>
     * <li> Application Auto Invocation set to Blanket and Net Access set to
     *      Blanket</li>
     * </ul>
     *
     * @param current current permission levels
     * @param pushInterruptLevel Push interrupt level
     * @param group desired permission group
     * @param newLevel permission level
     *
     * @exception SecurityException if the change would produce a mutually
     *                              exclusive combination
     */
    private static void checkForMutuallyExclusiveCombination(byte[] current,
            byte pushInterruptLevel, PermissionGroup group, byte newLevel)
            throws SecurityException {

        byte level;

        if (newLevel != BLANKET_GRANTED) {
            return;
        }

        if (group == NET_ACCESS_GROUP) {
            if (pushInterruptLevel == BLANKET_GRANTED ||
                   pushInterruptLevel == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(
                        NET_ACCESS_GROUP.getName(),
                        ResourceConstants.AMS_MGR_INTRUPT));
            }
            
            level = getPermissionGroupLevel(current, AUTO_INVOCATION_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(NET_ACCESS_GROUP,
                        AUTO_INVOCATION_GROUP));
            }
            
            level = getPermissionGroupLevel(current, READ_USER_DATA_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(NET_ACCESS_GROUP,
                        READ_USER_DATA_GROUP));
            }
            
            level = getPermissionGroupLevel(current, MULTIMEDIA_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(NET_ACCESS_GROUP,
                        MULTIMEDIA_GROUP));
            }
            
            return;
        }
                    
        if (group == LOCAL_CONN_GROUP) {
            level = getPermissionGroupLevel(current, READ_USER_DATA_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(LOCAL_CONN_GROUP,
                        READ_USER_DATA_GROUP));
            }
            
            
            level = getPermissionGroupLevel(current, MULTIMEDIA_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(LOCAL_CONN_GROUP,
                        MULTIMEDIA_GROUP));
            }

            return;
        }

        if (group == AUTO_INVOCATION_GROUP) {
            level = getPermissionGroupLevel(current, NET_ACCESS_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(AUTO_INVOCATION_GROUP,
                        NET_ACCESS_GROUP));
            }
        }            

        if (group == READ_USER_DATA_GROUP) {
            level = getPermissionGroupLevel(current, NET_ACCESS_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(READ_USER_DATA_GROUP,
                        NET_ACCESS_GROUP));
            }

            level = getPermissionGroupLevel(current, LOCAL_CONN_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(READ_USER_DATA_GROUP,
                        LOCAL_CONN_GROUP));
            }
        }

        if (group == MULTIMEDIA_GROUP) {
            level = getPermissionGroupLevel(current, NET_ACCESS_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(MULTIMEDIA_GROUP,
                        NET_ACCESS_GROUP));
            }

            level = getPermissionGroupLevel(current, LOCAL_CONN_GROUP);
            if (level == BLANKET_GRANTED || level == BLANKET) {
                throw new SecurityException(
                    createMutuallyExclusiveErrorMessage(MULTIMEDIA_GROUP,
                        LOCAL_CONN_GROUP));
            }
        }
    }

    /**
     * Create a mutally exclusive permission setting error message.
     *
     * @param groupToSet Group that is to be set
     * @param blanketGroup The a mutually exclusive group that was set to
     *                     blanket
     *
     * @return Translated error message with both group names in it
     */
    private static String createMutuallyExclusiveErrorMessage(
            PermissionGroup groupToSet, PermissionGroup blanketGroup) {
        return createMutuallyExclusiveErrorMessage(groupToSet.getName(),
            blanketGroup.getName());
    }                                                    

    /**
     * Create a mutally exclusive permission setting error message.
     *
     * @param nameId ID of the first group in the message
     * @param otherNameId ID of the name of other group
     *
     * @return Translated error message with both group names in it
     */
    private static String createMutuallyExclusiveErrorMessage(
            int nameId, int otherNameId) {
        String[] values = {Resource.getString(nameId),
                           Resource.getString(otherNameId)};
        
        return Resource.getString(
            ResourceConstants.PERMISSION_MUTUALLY_EXCLUSIVE_ERROR_MESSAGE,
                values);
    }                                                    
}

/** Specifies a permission name and its group. */
class PermissionSpec {
    /** Name of permission. */
    String name;

    /** Group of permission. */
    PermissionGroup group;

    /**
     * Construct a permission specification.
     *
     * @param theName Name of permission
     * @param theGroup Group of permission
     */
    PermissionSpec(String theName, PermissionGroup theGroup) {
        name = theName;
        group = theGroup;
    }
}
