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

package com.sun.midp.odd;

import javax.microedition.rms.RecordStore;
import javax.microedition.rms.RecordStoreException;
import java.io.*;

/**
 * Data class encapsulating the user settings.
 * The SettingsScreen is the view.
 *
 * @author Roy Ben Hayun
 */
public class AgentSettings {

    //
    // Members
    //

    /** RMS holding ODT Agent settings */
    private static final String ODT_SETTINGS_RMS = "oddsettings";

    /**
     * Indicates if installation should be silent, with no prompts
     */
    boolean silentInstallation = true;

    /**
     * Indicates if pin authentication is required, when connection received
     */
    boolean pinRequired = true;
    
    /**
     * Indicates if only signed suites are allowed to be installed
     */
    boolean signedOnly = true;

    //
    // Life cycle
    //
    
    /**
     * C'tor
     */
    AgentSettings() {
        try {
            load();
        } catch (Exception e) {
            // probably the RMS is corrupted - delete it and use default settings
            try {
                RecordStore.deleteRecordStore(ODT_SETTINGS_RMS);
            } catch (Exception ex) {
                // ignore it
            }

            e.printStackTrace();
        }
    }
    
    //
    // Operations
    //
    
    
    /**
     * Load settings from persistant storage 
     */
    void load() throws RecordStoreException, IOException {
        ByteArrayInputStream bas;
        DataInputStream dis;
        byte[] data;
        RecordStore settings = null;
        boolean tmp;

        try {
            settings = RecordStore.openRecordStore(ODT_SETTINGS_RMS, false);

            for (int i = 1; i <= 3; i++) {
                data = settings.getRecord(i);

                if (data != null) {
                    bas = new ByteArrayInputStream(data);
                    dis = new DataInputStream(bas);
                    tmp = dis.readBoolean();
                } else {
                    tmp = true; // apply defaults
                }

                switch (i) {
                    case 1:
                        silentInstallation = tmp;
                        break;
                    case 2:
                        pinRequired = tmp;
                        break;
                    case 3:
                        signedOnly = tmp;
                        break;
                }
            }
        } finally {
            if (settings != null) {
                try {
                    settings.closeRecordStore();
                } catch (RecordStoreException e) {
                    // ignore
                }
            }
        }
    }
    
    /**
     * Save settings to persistant storage
     */
    void save() throws RecordStoreException, IOException {
        RecordStore settings;

        settings = RecordStore.openRecordStore(ODT_SETTINGS_RMS, true);

        try {
            if (settings.getNumRecords() == 0) {
                // add space for 3 records
                settings.addRecord(null, 0, 0);
                settings.addRecord(null, 0, 0);
                settings.addRecord(null, 0, 0);
            }

            ByteArrayOutputStream bas = new ByteArrayOutputStream();
            DataOutputStream dos = new DataOutputStream(bas);
            boolean tmp = true;
            byte[] data;

            for (int i = 1; i <= 3; i++) {
                switch (i) {
                    case 1:
                        tmp = silentInstallation;
                        break;
                    case 2:
                        tmp = pinRequired;
                        break;
                    case 3:
                        tmp = signedOnly;
                        break;
                }

                dos.writeBoolean(tmp);
                data = bas.toByteArray();
                settings.setRecord(i, data, 0, data.length);
                bas.reset();
            }

            settings.closeRecordStore();
            dos.close();
        } finally {
            settings.closeRecordStore();
        }
    }
    
}
