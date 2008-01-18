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

package com.sun.satsa.jcrmic.classfile;

import java.io.*;

import com.sun.satsa.jcrmic.classfile.attributes.JAttribute;
import com.sun.satsa.jcrmic.classfile.constants.JConstantPool;


/**
 * This class represents a Java Class.
 */
public class JClass {

    /**
     * Constant for interface flag.
     */
    public static final int ACC_INTERFACE = 0x0200;

    /**
     * Access flags.
     */
    private int access_flags;

    /**
     * Class name.
     */
    private String class_name;

    /**
     * Super class name.
     */
    private String super_class_name;

    /**
     * Array of declared methods.
     */
    private JMethod[] methods;

    /**
     * Names of implemented interfaces.
     */
    private String[] interface_names;

    /**
     * 'Remote' flag.
     */
    private boolean remote;

    /**
     * 'Verified' flag.
     */
    private boolean verified;

    /**
     * Super class.
     */
    private JClass superClass;

    /**
     * Implemented interfaces.
     */
    private JClass[] interfaces;

    /**
     * Class loader.
     */
    private Loader classes;

    /**
     * Constructor.
     * @param classes class loader
     */
    public JClass(Loader classes) {
        this.classes = classes;
    }

    /**
     *  Parse and resolve Java class.
     * @param dis input stream
     * @throws IOException if I/O error occurs
     */
    public void parse(DataInputStream dis) throws IOException {

        dis.readInt();   // magic

        dis.readUnsignedShort();    // minor_version
        dis.readUnsignedShort();    // major_version

        JConstantPool constant_pool =
                new JConstantPool(dis.readUnsignedShort());
        constant_pool.parse(dis);

        access_flags = dis.readUnsignedShort();
        int this_class_index = dis.readUnsignedShort();
        int super_class_index = dis.readUnsignedShort();

        int[] interface_indexes = new int[dis.readUnsignedShort()];

        for (int i = 0; i < interface_indexes.length; i++) {
            interface_indexes[i] = dis.readUnsignedShort();
        }

        int field_count = dis.readUnsignedShort();
        for (int i = 0; i < field_count; i++) {
            dis.readUnsignedShort();    // access_flags
            dis.readUnsignedShort();    // name_index
            dis.readUnsignedShort();    // descriptor_index
            int attrib_count = dis.readUnsignedShort();
            for (int k = 0; k < attrib_count; k++) {
                int index = dis.readUnsignedShort();
                JAttribute.create(constant_pool, index).parse(dis);
            }
        }

        methods = new JMethod[dis.readUnsignedShort()];
        for (int i = 0; i < methods.length; i++) {
            methods[i] = new JMethod(constant_pool);
            methods[i].parse(dis);
        }

        int attribute_count = dis.readUnsignedShort();
        for (int i = 0; i < attribute_count; i++) {
            int index = dis.readUnsignedShort();
            JAttribute.create(constant_pool, index).parse(dis);
        }

        // resolve

        class_name = constant_pool.getConstantClass(
                        this_class_index).getClassName();

        if (super_class_index != 0) {
            super_class_name = constant_pool.getConstantClass(
                                    super_class_index).getClassName();
        } else {
            super_class_name = null;
        }

        interface_names = new String[interface_indexes.length];
        for (int i = 0; i < interface_names.length; i++) {
            interface_names[i] = constant_pool.getConstantClass(
                                    interface_indexes[i]).getClassName();
        }
    }

    /**
     * Load all related classes.
     * @return true if successfull
     */
    public boolean update() {

        if (class_name.equals("java/rmi/Remote")) {
            setRemote();
        }

        String name = super_class_name;

        if (name != null) {

            if (! classes.loadClass(name))
                return false;

            superClass = classes.getClass(name);

            if (superClass.isRemote())
                setRemote();
        }

        interfaces = new JClass[interface_names.length];

        for (int i = 0; i < interface_names.length; i++) {

            if (!classes.loadClass(interface_names[i]))
                return false;

            interfaces[i] = classes.getClass(interface_names[i]);

            if (interfaces[i].isRemote())
                setRemote();
        }

        JMethod[] methods = getMethods();

        for (int i = 0; i < methods.length; i++) {

            String[] exceptions = methods[i].getExceptionsThrown();
            if (exceptions != null) {
                for (int j = 0; j < exceptions.length; j++) {
                    if (!classes.loadClass(exceptions[j])) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    /**
     * Returns super class.
     * @return super class
     */
    public JClass getSuperClass() {

        return superClass;
    }

    /**
     * Returns implemented interfaces.
     * @return implemented interfaces
     */
    public JClass[] getInterfaces() {

        return interfaces;
    }

    /**
     * Returns class name.
     * @return  class name
     */
    public String getClassName() {
        return class_name;
    }

    /**
     * Returns methods declared by class.
     * @return methods declared by class
     */
    public JMethod[] getMethods() {
        return methods;
    }

    /**
     * Check if the class is interface.
     * @return true if the class is interface
     */
    public boolean isInterface() {
        return (access_flags & ACC_INTERFACE) != 0;
    }

    /**
     * Set 'remote' flag.
     */
    public void setRemote() {
        remote = true;
    }

    /**
     * Returns 'remote' flag value.
     * @return 'remote' flag value
     */
    public boolean isRemote() {
        return remote;
    }

    /**
     * Set 'verified' flag.
     */
    public void setVerified() {
        verified = true;
    }

    /**
     * Returns 'verifed' flag value.
     * @return 'verifed' flag value
     */
    public boolean isVerified() {
        return verified;
    }

    /**
     * Verifies if the class is equal to or a sublass of specified class.
     * @param c2 class
     * @return true if the class is equal to or a sublass of specified class
     */
    public boolean isSubclass(JClass c2) {

        String name = c2.getClassName();

        JClass c1 = this;

        while (true) {

            if (c1.getClassName().equals(name)) {
                return true;
            }

            c1 = c1.getSuperClass();

            if (c1 == null) {
                return false;
            }
        }
    }
}
