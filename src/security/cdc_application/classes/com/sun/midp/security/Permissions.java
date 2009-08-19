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

    /** Name of the MIDP permission. */
    public static final String MIDP_PERMISSION_NAME = "com.sun.midp";

    /** Name of the AMS permission. */
    public static final String AMS_PERMISSION_NAME = "com.sun.midp.ams";

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
    /** com.sun.midp.ams permission ID. */
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
    /** javax.microedition.io.Connector.sensor permission ID. */
    public static final int SENSOR = 52;
    /** javax.microedition.io.Connector.PrivateSensor permission ID. */
    public static final int PRIVATE_SENSOR = 53;
    /** javax.microedition.io.Connector.ProtectedSensor permission ID. */
    public static final int PROTECTED_SENSOR = 54;

    /** Never allow the permission. */
    public static final byte NEVER = 0;
    /** Allow an permission with out asking the user. */
    public static final byte ALLOW = 1;
    /**
     * Permission granted by the user until the the user changes it in the
     * settings form.
     */
    public static final byte BLANKET_GRANTED = 2;
    /**
     * Allow a permission to be granted or denied by the user
     * until changed in the settings form.
     */
    public static final byte BLANKET = 4;
    /** Allow a permission to be granted only for the current session. */
    public static final byte SESSION = 8;
    /** Allow a permission to be granted only for one use. */
    public static final byte ONESHOT = 16;

    /** User granted permission for this session. */
    public static final byte SESSION_GRANTED = 32;
    /** User denied permission for this session. */
    public static final byte SESSION_DENIED = 64;
    /** Session level interaction has not occured. */
    public static final byte SESSION_UNKNOWN = -64;

    /**
     * Permission denied by the user until the user changes it in the
     * settings form.
     */
    public static final byte BLANKET_DENIED = -128;

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
        BLANKET, SESSION, SESSION, ONESHOT);

    /** Read Message permission group. */
    static final PermissionGroup READ_MESSAGE_GROUP = new PermissionGroup(
        0, 0, 0,
        ResourceConstants.PERMISSION_RECEIVE_MESSAGE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_RECEIVE_MESSAGE_QUE, 0,
        BLANKET, BLANKET, BLANKET, BLANKET);

    /**
     * Send Message permission group. Send was broken out because send
     * is treated as one shot even though it is in the messaging group.
     */
    static final PermissionGroup SEND_MESSAGE_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_MSG_SETTINGS,
        ResourceConstants.AMS_MGR_MSG_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_MSG_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_SEND_MESSAGE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_SEND_MESSAGE_QUE, 0,
        ONESHOT, ONESHOT, ONESHOT, ONESHOT);

    /** Application Auto Invocation permission group. */
    static final PermissionGroup AUTO_INVOCATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS,
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_AUTO_START_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_AUTO_START_DIALOG_TITLE,
        ResourceConstants.PERMISSION_AUTO_START_QUE, 0,
        BLANKET, ONESHOT, SESSION, ONESHOT);

    /** Local Connectivity permission group. */
    static final PermissionGroup LOCAL_CONN_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS,
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LOCAL_CONN_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LOCAL_CONN_DIALOG_TITLE,
        ResourceConstants.PERMISSION_LOCAL_CONN_QUE, 0,
        BLANKET, SESSION, BLANKET, ONESHOT);

    /** Multimedia Recording permission group. */
    static final PermissionGroup MULTIMEDIA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_REC_SETTINGS,
        ResourceConstants.AMS_MGR_REC_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_REC_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_MULTIMEDIA_DIALOG_TITLE,
        ResourceConstants.PERMISSION_MULTIMEDIA_QUE, 0,
        BLANKET, SESSION, SESSION, ONESHOT);

    /** Read User Data permission group. */
    static final PermissionGroup READ_USER_DATA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS,
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_READ_USER_DATA_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_READ_USER_DATA_TITLE,
        ResourceConstants.PERMISSION_READ_USER_DATA_QUE, 0,
        BLANKET, ONESHOT, ONESHOT, ONESHOT);

    /** Write User Data permission group. */
    static final PermissionGroup WRITE_USER_DATA_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS,
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_WRITE_USER_DATA_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_TITLE,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_QUE,
        ResourceConstants.PERMISSION_WRITE_USER_DATA_ONESHOT_QUE,
        BLANKET, ONESHOT, ONESHOT, ONESHOT);

    /** Location permission group. */
    static final PermissionGroup LOCATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LOC_SETTINGS,
        ResourceConstants.AMS_MGR_LOC_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LOC_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LOCATION_TITLE,
        ResourceConstants.PERMISSION_LOCATION_QUE, 0,
        BLANKET, SESSION, SESSION, ONESHOT);

    /** Landmark store permission group. */
    static final PermissionGroup LANDMARK_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS,
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_LANDMARK_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_LANDMARK_TITLE,
        ResourceConstants.PERMISSION_LANDMARK_QUE, 0,
        BLANKET, SESSION, SESSION, ONESHOT);

    /** Smart card permission group. */
    static final PermissionGroup SMART_CARD_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_SMART_CARD_SETTINGS,
        ResourceConstants.AMS_MGR_SMART_CARD_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_SMART_CARD_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_SMART_CARD_TITLE,
        ResourceConstants.PERMISSION_SMART_CARD_QUE, 0,
        BLANKET, SESSION, NEVER, NEVER);

    /** Authentication (identification) permission group. */
    static final PermissionGroup AUTHENTICATION_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS,
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_AUTHENTICATION_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_SIGNATURE_DIALOG_TITLE,
        ResourceConstants.PERMISSION_SIGNATURE_QUE, 0,
        BLANKET, SESSION, NEVER, NEVER);

    /** Call Control (restricted network connection) permission group. */
    static final PermissionGroup CALL_CONTROL_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS,
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_CALL_CONTROL_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_CALL_CONTROL_TITLE,
        ResourceConstants.PERMISSION_CALL_CONTROL_QUE, 0,
        BLANKET, ONESHOT, ONESHOT, ONESHOT);

    /** Sensor permission group. */
    static final PermissionGroup SENSOR_GROUP = new PermissionGroup(
        ResourceConstants.AMS_MGR_SENSOR_SETTINGS,
        ResourceConstants.AMS_MGR_SENSOR_SETTINGS_QUE,
        ResourceConstants.AMS_MGR_SENSOR_SETTINGS_QUE_DONT,
        ResourceConstants.PERMISSION_SENSOR_TITLE,
        ResourceConstants.PERMISSION_SENSOR_QUE, 0,
        BLANKET, ONESHOT, ONESHOT, ONESHOT);

    /** Permission specifications. */
    static final PermissionSpec[] permissionSpecs =
        PermissionsTable.permissionSpecs;

    /** Number of permissions. */
    public static final int NUMBER_OF_PERMISSIONS =
        PermissionsTable.NUMBER_OF_PERMISSIONS;

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
     * @return permission dialog title
     * @exception SecurityException if the permission is invalid
     */
    public static String getTitle(int permission) {
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
     * @return permission question
     *
     * @exception SecurityException if the permission is invalid
     */
    public static String getQuestion(int permission) {
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
     * @return permission question
     *
     * @exception SecurityException if the permission is invalid
     */
    public static String getOneshotQuestion(int permission) {
        if (permission < 0 || permission >= permissionSpecs.length) {
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }

        return permissionSpecs[permission].group.getRuntimeOneshotQuestion();
    }

    /**
     * Get the ID of a permission.
     *
     * @param name permission name
     *
     * @return permission ID
     *
     * @exception SecurityException if the permission is invalid
     */
    public static int getId(String name) {
        for (int i = 0; i < permissionSpecs.length; i++) {
            if (permissionSpecs[i].name.equals(name)) {
                return i;
            }
        }

        throw new SecurityException(SecurityToken.STD_EX_MSG);
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
     * Returns domain for unsigned midlets.
     *
     * @return domain name
     */
    public static String getUnsignedDomain() {
        return UNIDENTIFIED_DOMAIN_BINDING;
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
    static PermissionGroup[] getSettingGroups() {
        PermissionGroup[] groups = new PermissionGroup[13];

        groups[0] = NET_ACCESS_GROUP;
        groups[1] = SEND_MESSAGE_GROUP;
        groups[2] = AUTO_INVOCATION_GROUP;
        groups[3] = LOCAL_CONN_GROUP;
        groups[4] = MULTIMEDIA_GROUP;
        groups[5] = READ_USER_DATA_GROUP;
        groups[6] = WRITE_USER_DATA_GROUP;
        groups[7] = LOCATION_GROUP;
        groups[8] = LANDMARK_GROUP;
        groups[9] = SMART_CARD_GROUP;
        groups[10] = AUTHENTICATION_GROUP;
        groups[11] = CALL_CONTROL_GROUP;
        groups[12] = SENSOR_GROUP;

        return groups;
    }

    /**
     * Get a list of all permission groups for the settings dialog.
     *
     * @param levels current permission levels
     *
     * @return array of permission groups
     */
    public static PermissionGroup[] getSettingGroups(byte[] levels) {
        PermissionGroup[] groups = getSettingGroups();

        /*
         * The setting dialog only presents one message group since the spec
         * doesn't separate send and receive message groups. In most cases,
         * it's the send group. The exception from this rule if there are no
         * send permissions present in MIDlet suite properties. If so, the read
         * message group is displayed in the dialog.
         */
        if (getPermissionGroupLevel(levels, SEND_MESSAGE_GROUP) != NEVER) {
            groups[1] = SEND_MESSAGE_GROUP;
        } else {
            groups[1] = READ_MESSAGE_GROUP;
        }

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

        /*
         * For some reason specs do not want separate send and
         * receive message groups, but want the questions and interrupt
         * level to be different for send, so internally we have 2 groups
         * that must be kept in synch. The setting dialog only presents
         * the send message group, see the getSettingGroups method.
         */
        if (group == SEND_MESSAGE_GROUP) {
            /*
             * Since the send group have a max level of oneshot, this method
             * will only code get used by the settings dialog, when a user
             * changes the send group from blanket denied to oneshot.
             */
            if (level != BLANKET_DENIED) {
                /*
                 * If send is set to to any thing but blanket denied
                 * then receive is set to blanket.
                 */
                level = BLANKET_GRANTED;
            }

            for (int i = 0; i < permissionSpecs.length; i++) {
                if (permissionSpecs[i].group == READ_MESSAGE_GROUP) {
                    setPermission(current, i, level);
                }
            }

            return;
        }

        if (group == READ_MESSAGE_GROUP && level == BLANKET_DENIED) {
            /*
             * This code will only be used when the user says no during
             * a message read runtime permission prompt.
             */

            for (int i = 0; i < permissionSpecs.length; i++) {
                if (permissionSpecs[i].group == SEND_MESSAGE_GROUP) {
                    setPermission(current, i, BLANKET_DENIED);
                }
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

        setPermissionGroup(current, NEVER, group, level);
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
                Resource.getString(ResourceConstants.AMS_MGR_INTRUPT),
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
        if (current[permission] != NEVER && current[permission] != ALLOW) {
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
                      Resource.getString(ResourceConstants.AMS_MGR_INTRUPT)));
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
     * @param name name of the first group in the message
     * @param otherNameId name of the name of other group
     *
     * @return Translated error message with both group names in it
     */
    private static String createMutuallyExclusiveErrorMessage(
            String name, String otherName) {
        String[] values = {name, otherName};

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
