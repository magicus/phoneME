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
 
package com.sun.cardreader;

import java.io.IOException;
import com.sun.j2me.main.Configuration;

/**
 * This is an internal class to hold a device configuration records.
 */
class DeviceRecord {
    /** Class name. */
    public String className;
    /** Reference to device object. */
    public CardDevice device;
}

/**
 * This class represents card slot factory.
 */
public class SlotFactory {
    /**
     * Property which contains number of devices.
     */
    private static final String DEVICE_NUMBER_PROP = 
        "com.sun.cardreader.deviceNumber";
    /**
     * Prefix of properties which contain class names.
     */
    private static final String DEVICE_CLASS_PROP = 
        "com.sun.cardreader.deviceClass";

    /**
     * Reads devices and slots configuration.
     *
     * @throws CardDeviceException When the configuration is broken
     * @throws IOException When the device creatiom failed
     */
    public static void init() throws IOException, CardDeviceException {
        synchronized (syncObject) {
            if (initialized) {
                return;
            }
        
            /* 
             * Get device number from system property.
             * It is ok to have zero devices in configuration, we'll just 
             * do nothing in this case. 
             */
            int deviceNum = Configuration.getIntProperty(DEVICE_NUMBER_PROP, 0);
            if (deviceNum < 0) {
                throw new CardDeviceException("Number of devices must be > 0");
            }

            deviceRecords = new DeviceRecord[deviceNum];
            
            /* 
             * Read device classes and create objects
             * Also we count total slot number here 
             */
            CardDevice device = null;
            int slotCount = 0;

            for (int i = 0; i < deviceNum; i++) {
                deviceRecords[i] = new DeviceRecord();

                String pName = DEVICE_CLASS_PROP + Integer.toString(i);
                deviceRecords[i].className = Configuration.getProperty(pName);
                if (deviceRecords[i].className == null) {
                    throw new CardDeviceException("Class not found the device "
                                                  + Integer.toString(i));
                }
                
                /* Create card device */
                try {
                    Class cl = Class.forName(deviceRecords[i].className);
                    device = (CardDevice)(cl.newInstance());
                    deviceRecords[i].device = device;
                }
                catch (ClassNotFoundException e) {
                    throw new CardDeviceException("Class not found: " +
                                                  deviceRecords[i].className);
                }
                catch (InstantiationException ie) {  
                    throw new IOException(ie.getMessage());
                }
                catch (IllegalAccessException iae) {  
                    throw new IOException(iae.getMessage());
                }
                device.init();

                device.setStartSlotNumber(slotCount);
                slotCount += device.getSlotCount();
            }

            slots = new CardSlot[slotCount];
            initialized = true;
        }
    }
    
    /**
     * Creates specified card slot for the specified device.
     *
     * @param slot Global slot number
     * @return CardSlot object or null if slot number is out of bounds
     * @throws IOException If CardSlot creation failed
     * @throws CardDeviceException If something wrong with the configuration
     */
    public static CardSlot getCardSlot(int slot)
	throws IOException, CardDeviceException {

        synchronized (syncObject) {
            if (!initialized || 
                (slot < 0) || 
                (slot >= slots.length)) {
                return null;
            }

            if (slots[slot] != null) {
                return slots[slot];
            } else {
                /* Find device for the slot */
                CardDevice device = null;
                for (int i = 0; i < deviceRecords.length; i++) {
                    if (deviceRecords[i].device.checkSlotNumber(slot)) {
                        device = deviceRecords[i].device;
                        break;
                    }
                }

                if (device == null) {
                    /* Should not be here, paranoic check */
                    throw new CardDeviceException();
                }
                    
                int localSlot = slot - device.getStartSlotNumber();
                slots[slot] = new CardSlot(device, localSlot);

                return slots[slot];
            }
        }
    }
    
    /**
     * Get total number of card slots.
     *
     * @return Total number of card slots even if they are not created yet
     */
    public static int getCardSlotCount() {        
        synchronized (syncObject) {
            return slots.length;
        }
    }

    /**
     * Get total number of configured devices.
     *
     * @return Total number of configured devices
     */
    public static int getCardDeviceCount() {
        synchronized (syncObject) {
            return deviceRecords.length;
        }
    }

    /**
     * Checks if this slot is SAT slot.
     *
     * @param slot Slot number.
     * @return <code>true</code> if the slot is dedicated for SAT,
     *         <code>false</code> otherwise
     * @exception IOException If an error occured.
     * @exception IllegalArgumentException if illegal slot number provided
     */
    public static boolean isSatSlot(int slot) throws IOException {
        synchronized (syncObject) {
            if (!initialized || 
                (slot < 0) || 
                (slot >= slots.length)) {
                throw new IllegalArgumentException("Slot does not exist");
            }

            try {
                return getCardSlot(slot).isSAT();              
            }
            catch (CardDeviceException e) {
                /* Should not be here, paranoic check */
                throw new IllegalArgumentException("Slot's device does not exist");
            }
        }
    }

    /**
     * Device table, holds class names and object references.
     */
    private static DeviceRecord deviceRecords[] = new DeviceRecord[0];

    /**
     * All slots created by the factory.
     */
    private static CardSlot slots[] = new CardSlot[0];

    /**
     * Initialization flag.
     */
    private static boolean initialized = false;
    
    /**
     * Service object, used in synchronized().
     */
    private static Object syncObject = new Object();
}
