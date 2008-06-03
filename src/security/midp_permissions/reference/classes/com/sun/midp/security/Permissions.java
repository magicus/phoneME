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

import java.util.Hashtable;
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
    /**
     * Permission denied by the user until the user changes it in the
     * settings form.
     */
    public static final byte BLANKET_DENIED = -128;

    /** Third Party Never permission group. */
    static final PermissionGroup NEVER_GROUP =
        new PermissionGroup(null, null, null, null, null, null);

    /** Permission to group map table. */
    static PermissionSpec[]  permissionSpecs = null;

    /** Number of permissions. */
    public static int NUMBER_OF_PERMISSIONS;

    /* list of domains */
    private static DomainPolicy []    domainsAll=null;

    /* The domain name for unsigned midlets */
    private static String unsignedDomain=null;

    /* Permissions index lookup table */
    private static Hashtable          permissionsHash;

    /* list of groups */
    private static PermissionGroup [] groupsAll=null;
    
    // a hack to be solved
    private static PermissionGroup NET_ACCESS_GROUP;
    private static PermissionGroup SEND_MESSAGE_GROUP;
    private static PermissionGroup READ_MESSAGE_GROUP;
    private static PermissionGroup AUTO_INVOCATION_GROUP;
    private static PermissionGroup READ_USER_DATA_GROUP;
    private static PermissionGroup MULTIMEDIA_GROUP;
    private static PermissionGroup LOCAL_CONN_GROUP;
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
     * @return Resource constant for the permission question
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
     * @return Resource constant for the permission question
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
        int index;
        try {
            index = ((Integer)permissionsHash.get(name)).intValue();
            return index;
        } catch (Exception e){
            throw new SecurityException(SecurityToken.STD_EX_MSG);
        }
    }

    /**
     * Determine if a domain is a trusted domain.
     *
     * @param domain Binding name of a domain
     *
     * @return true if a domain is trusted, false if not
     */
    public static boolean isTrusted(String domain) {
        if (domainsAll == null)
            init();
        for (int i1 = 0; i1 < domainsAll.length;i1++)
            if (domainsAll[i1].getName().equals(domain))
                return domainsAll[i1].isTrusted();

        return false;
    }

    /**
     * Returns domain for unsigned midlets.
     *
     * @return domain name
     */
    public static String getUnsignedDomain() {
        return unsignedDomain;
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
        if (domainsAll == null)
            init();
                
        byte[] maximums = new byte[NUMBER_OF_PERMISSIONS];
        byte[] defaults = new byte[NUMBER_OF_PERMISSIONS];
        byte[][] permissions = {maximums, defaults};

        if (MANUFACTURER_DOMAIN_BINDING.equals(name)) {
            for (int i1 = 0; i1 < NUMBER_OF_PERMISSIONS; i1 ++) {
                maximums[i1] = ALLOW;
                defaults[i1] = ALLOW;
            }
            return permissions;
        }

        for (int i1 = 0; i1 < domainsAll.length; i1++)
            if (domainsAll[i1].getName().equals(name)) {
                domainsAll[i1].getPermissionlevels(defaults, CUR_LEVELS);
                domainsAll[i1].getPermissionlevels(maximums, MAX_LEVELS);
            }
        
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
        return groupsAll;

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
     * @param nameId ID of the first group in the message
     * @param otherNameId ID of the name of other group
     *
     * @return Translated error message with both group names in it
     */
    private static String createMutuallyExclusiveErrorMessage(
            String nameId, String otherNameId) {
        String[] values = {nameId, otherNameId};
        return Resource.getString(
            ResourceConstants.PERMISSION_MUTUALLY_EXCLUSIVE_ERROR_MESSAGE,
                values);
    }

    private static void init() {
        try {
            // initialization process
            // step 1: permissions list and hashtable
            int i1, i2;
            String [] list;
            String [] permList = PermissionsStrings.PERMISSION_STRINGS;
            permissionSpecs = new PermissionSpec[permList.length+2];
            permissionSpecs[0] = new PermissionSpec(MIDP_PERMISSION_NAME, NEVER_GROUP);
            permissionSpecs[1] = new PermissionSpec(AMS_PERMISSION_NAME, NEVER_GROUP);
            NUMBER_OF_PERMISSIONS = permissionSpecs.length;
            permissionsHash = new Hashtable(NUMBER_OF_PERMISSIONS);
            permissionsHash.put(MIDP_PERMISSION_NAME, new Integer(0));
            permissionsHash.put(AMS_PERMISSION_NAME, new Integer(1));
            for (i1 = 2; i1 < NUMBER_OF_PERMISSIONS; i1++) {
                permissionsHash.put(permList[i1-2], new Integer(i1));
                permissionSpecs[i1] = new PermissionSpec(permList[i1-2], NEVER_GROUP);
            }

            //step 2: groups list
            list =  loadGroupList();
            groupsAll = new PermissionGroup[list.length];
            String [] messages = new String[6];
            for (i1 = 0; i1 < groupsAll.length; i1++) {
                String [] tmp = getGroupMessages(list[i1]);
                if (tmp != null) {
                    for (i2 = 0; i2 < tmp.length; i2++)
                        messages[i2] = replaceCRLF(tmp[i2]); 
                } else {
                    messages[0] = list[i1];
                    i2 = 1;
                }
                for (; i2 < messages.length; i2++)
                    messages[i2] = "n/a";
                    
                groupsAll[i1] = new PermissionGroup(list[i1], 
                                        messages[0], messages[1],
                                        messages[2], messages[3],
                                        messages[4], messages[5]);
            }

            //step 3: group's permissions members
            String [] members;
            for (i1 = 0; i1 < groupsAll.length; i1++) {
                members = loadGroupPermissions(groupsAll[i1].getNativeName());
                for (i2 = 0; i2 < members.length; i2++) {
                    Integer c = (Integer)permissionsHash.get(members[i2]);
                    permissionSpecs[c.intValue()] = new PermissionSpec(members[i2], groupsAll[i1]);
                }
            }
                        
            // step 4: Domains list
            list = loadDomainList();
            DomainPolicy [] domains = new DomainPolicy[list.length+1]; // internal 'manufacturer' domain always exist
            domains[0] = new DomainPolicy(MANUFACTURER_DOMAIN_BINDING, true);
            int domainsCounter = 1;
            for (i1 = 0; i1 < list.length; i1++) {
                String item = list[i1];

                if (MANUFACTURER_DOMAIN_BINDING.startsWith(item))
                    continue;

                boolean isTrusted = true;
                String name;
                int pos = item.indexOf(',');
                if (pos > 0) {
                    name = item.substring(0,pos);
                    if (item.charAt(pos+1) == 'u')
                        isTrusted = false;
                } else
                    name = item;

                domains[domainsCounter++] = new DomainPolicy(name, isTrusted);
                if (name.startsWith("untrusted") || name.startsWith("unidentified"))
                    unsignedDomain = name;
            }
            domainsAll = new DomainPolicy[domainsCounter];
            System.arraycopy(domains, 0, domainsAll, 0, domainsCounter);
            
            // fill hack groups
            Hashtable hack = new Hashtable(groupsAll.length);
            for (i1 = 0; i1 < groupsAll.length; i1++)
                hack.put(groupsAll[i1].getNativeName(), groupsAll[i1]);
            NET_ACCESS_GROUP = (PermissionGroup)hack.get("net_access");
            SEND_MESSAGE_GROUP = (PermissionGroup)hack.get("messaging");
            READ_MESSAGE_GROUP = (PermissionGroup)hack.get("messaging");
            AUTO_INVOCATION_GROUP = (PermissionGroup)hack.get("application_auto_invocation");
            READ_USER_DATA_GROUP = (PermissionGroup)hack.get("read_user_data_access");
            MULTIMEDIA_GROUP = (PermissionGroup)hack.get("multimedia_recording");
            LOCAL_CONN_GROUP = (PermissionGroup)hack.get("local_connectivity");
            
        } catch (Throwable e) {
            System.out.println("init() error: " + e.toString());
        }
    }

    private static String replaceCRLF(String value) {
        int posCRLF, pos = 0;
        String result = "";

        while ((posCRLF = value.indexOf("\\n", pos)) != -1) {
            result += value.substring(pos,  posCRLF) + "\n";
            pos = posCRLF + 2;
        }

        return result + value.substring(pos);
    }

    private static native String [] loadDomainList();
    private static native String [] loadGroupList();
    static native String [] loadGroupPermissions(String group);
    static native String [] getGroupMessages(String group);
    static native byte      getMaxValue(String domain, String group);
    static native byte      getDefaultValue(String domain, String group);
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
