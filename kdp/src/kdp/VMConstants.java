/*
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

package kdp;

public interface VMConstants {

    final static byte VIRTUALMACHINE_CMDSET = 1;
    final static byte SENDVERSION_CMD = 1;
    final static byte CLASSESBYSIG_CMD = 2;
    final static byte ALL_CLASSES_CMD = 3;
    final static byte ALL_THREADS_CMD = 4;
    final static byte TOPLEVELTHREADGROUP_CMD = 5;
    final static byte DISPOSE_CMD = 6;
    final static byte IDSIZES_CMD = 7;
    final static byte RESUME_CMD = 9;
    final static byte EXIT_CMD = 10;
    final static byte CAPABILITIES_CMD = 12;
    final static byte CLASSPATHS_CMD = 13;
    final static byte DISPOSE_OBJECTS_CMD = 14;

    final static byte REFERENCE_TYPE_CMDSET = 2;
    final static byte SIGNATURE_CMD = 1;
    final static byte CLASSLOADER_CMD = 2;
    final static byte MODIFIERS_CMD = 3;
    final static byte FIELDS_CMD = 4;
    final static byte METHODS_CMD = 5;
    final static byte SOURCEFILE_CMD = 7;
    final static byte STATUS_CMD = 9;
    final static byte INTERFACES_CMD = 10;

    final static byte METHOD_CMDSET = 6;
    final static byte METHOD_LINETABLE_CMD = 01;
    final static byte METHOD_VARIABLETABLE_CMD = 2;
    final static byte METHOD_BYTECODES_CMD = 3;

    final static byte THREADREFERENCE_CMDSET = 11;
    final static byte THREADGROUP_CMD = 5;
    final static byte FRAMES_CMD = 6;

    final static byte THREADGROUPREFERENCE_CMDSET = 12;
    final static byte THREADGROUP_NAME_CMD = 1;
    final static byte THREADGROUP_PARENT_CMD = 2;
    final static byte THREADGROUP_CHILDREN_CMD = 3;

    final static byte EVENT_REQUEST_CMDSET = 15;
    final static byte EVENT_SET_CMD = 1;
    final static byte EVENT_CLEAR_CMD = 2;

    final static byte STACKFRAME_CMDSET = 16;
    final static byte STACKFRAME_GETVALUES_CMD = 1;
    final static byte STACKFRAME_THISOBJECT_CMD = 3;

    final static byte CLASSOBJECTREFERENCE_CMDSET = 17;
    final static byte REFLECTEDTYPE_CMD = 1;

    final static byte EVENT_CMDSET = 64;
    final static byte COMPOSITE_CMD = 100;

    final static byte KVM_CMDSET = (byte)0x80;
    final static byte KVM_HANDSHAKE_CMD = 1;
    final static byte KVM_GET_STEPPINGINFO_CMD = 2;
    final static byte KVM_STEPPING_EVENT_CMD = 3;
    final static byte KVM_GET_VAR_TABLE_CMD = 4;
    final static byte KVM_GET_LINE_TABLE_CMD = 5;

    final static short NOTFOUND_ERROR = 41;
    final static short INVALID_METHODID = 23;
    final static short INVALID_OBJECT = 20;
    final static short ILLEGAL_ARGUMENT = 103;

    final static byte JDWP_TypeTag_ARRAY = 3;
    final static byte JDWP_EventKind_CLASS_PREPARE = 8;

    final static int CLASS_PREPARED = 2;
    final static int CLASS_INITIALIZED = 4;

    final static byte TYPE_TAG_CLASS = 1;
    final static byte TYPE_TAG_ARRAY = 3;

    final static int ONLY_THREADGROUP_ID = 0xffffffe0;
    final static String KVM_THREADGROUP_NAME = "KVM_System";

    final static String VMcmds[][] = 
    { { "" },   // make the list 1-based
      { "Virtual Machine", 
        "Version", "ClassBySignature", "AllClasses",
        "AllThreads", "TopLevelThreadGroups",
        "Dispose", "IDSizes", "Suspend", "Resume",
        "Exit", "CreateString", "Capabilities",
        "ClassPaths", "DisposeObjects",
        "HoldEvents", "ReleaseEvents" },
      { "ReferenceType", 
        "Signature", "ClassLoader", "Modifiers",
        "Fields", "Methods", "GetValues", 
        "SourceFile", "NestedTypes", "Status",
        "Interfaces", "ClassObject" },
      { "ClassType",
        "Superclass", "SetValues", "InvokeMethod",
        "NewInstance" },
      { "ArrayType",
        "NewInstance" },
      { "InterfaceType",
        "" },
      { "Method",
        "LineTable", "VariableTable", "Bytecodes" },
      { "UNUSED", "UNUSED" },
      { "Field",
        "" },
      { "ObjectReference",
        "ReferenceType", "GetValues", "SetValues",
        "UNUSED", "MonitorInfo", "InvokeMethod", 
        "DisableCollection", "EnableCollection",
        "IsCollected" },
      { "StringReference",
        "Value" },
      { "ThreadReference",
        "Name", "Suspend", "Resume", "Status",
        "ThreadGroup", "Frames", "FrameCount",
        "OwnedMonitors", "CurrentCountendedMonitor",
        "Stop", "Interrupt", "SuspendCount" },
      { "ThreadGroupReference", 
        "Name", "Parent", "Children" },
      { "ArrayReference",
        "Length", "GetValues", "SetValues" },
      { "ClassLoaderReference",
        "VisibleClasses" },
      { "EventRequest", 
        "Set", "Clear", "ClearAllBreakpoints" },
      { "StackFrame",
        "GetValues", "SetValues", "ThisObject" },
      { "ClassObjectReference", 
        "ReflectedType" },
    };
    
    final static String DBGcmds[][] = 
    { { "Event",
        "Composite" },
      
    };

    final static String VENcmds[][] = 
    { { "Vender-Specific",
        "UNKNOWN" }
    };
}
