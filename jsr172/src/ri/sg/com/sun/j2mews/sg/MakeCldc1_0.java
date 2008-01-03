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

package com.sun.j2mews.sg;

import java.io.*;
import java.util.*;
import java.text.*;

import com.sun.xml.rpc.processor.*;
import com.sun.xml.rpc.processor.model.*;
import com.sun.xml.rpc.processor.config.Configuration;
import com.sun.xml.rpc.processor.model.Service;
import com.sun.xml.rpc.processor.ProcessorOptions;
import com.sun.xml.rpc.processor.model.Model;
import com.sun.xml.rpc.processor.model.AbstractType;
import com.sun.xml.rpc.processor.model.java.*;
import com.sun.xml.rpc.util.localization.*;

/**
 * MakeCldc1_0 will take the internal data structures and coerce them
 * to be CLDC 1.0 compliant (float and double become String).
 *
 */
public class MakeCldc1_0 implements ProcessorAction {
    protected com.sun.xml.rpc.processor.util.ProcessorEnvironment env;
    private Map originalTypes;

    protected LocalizableMessageFactory messageFactory;

    public MakeCldc1_0(Map originalTypes) {
        messageFactory = new LocalizableMessageFactory("com.sun.j2mews.sg.stubgenerator");
        this.originalTypes = originalTypes;
    }

    public void setEnvironment(com.sun.xml.rpc.processor.util.ProcessorEnvironment e) {
        env = e;
    }

    public void perform(Model model, Configuration config, Properties options) {
        //env = config.getEnvironment();
        for (Iterator it = model.getServices(); it.hasNext(); ) {
            Service service = (Service) it.next();
            mungeTo1_0(service);
        }
    }

    public void mungeTo1_0(Service service) {
        for (Iterator it = service.getPorts(); it.hasNext(); ) {
            Port port = (Port) it.next();
            mungeTo1_0(port);
        }
    }

    protected void mungeTo1_0(Port port) {
        for (Iterator operations = port.getOperations(); operations.hasNext(); ) {
            Operation operation = (Operation) operations.next();
            mungeTo1_0(operation, port);
        }
    }

    protected void mungeTo1_0(Operation operation, Port port) {
        JavaMethod method = operation.getJavaMethod();
        //System.out.println("mungeTo1_0: method.getName="+method.getName());
        JavaType returnType = method.getReturnType();
        mungeTo1_0(returnType, port);
        for (Iterator parameters = method.getParameters(); parameters.hasNext(); ) {
            JavaParameter parameter = (JavaParameter) parameters.next();
            JavaType parameterType = parameter.getType();
            mungeTo1_0(parameterType, port);
        }
        for (Iterator faults = operation.getFaults(); faults.hasNext(); ) {
            Fault fault = (Fault) faults.next();
            Block faultBlock = fault.getBlock();
            JavaException jexcep = fault.getJavaException();
            mungeTo1_0(jexcep, port);
        }
    }

    protected void mungeTo1_0(JavaType type, Port port) {
        //System.out.println("mungeTo1_0: type="+type);
        if (type == null)
            return;
        if (type instanceof JavaSimpleType) {
            String typeName = javaTypeToString(type, port).intern();
            //System.out.println("typeName="+typeName);
            if (typeName == "float" || typeName == "Float" ||
                typeName == "java.lang.Float" || typeName == "double" ||
                typeName == "Double" || typeName == "java.lang.Double") {
                //System.out.println("Found odd type: "+typeName+" name="+type.getName());
                originalTypes.put(type, typeName);
                type.doSetName("java.lang.String");
            }
        } else if (type instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) type;
            for (Iterator members = jst.getMembers(); members.hasNext(); ) {
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                JavaType memberType = jsm.getType();
                mungeTo1_0(memberType, port);
            }
        } else if (type instanceof JavaArrayType) {
            JavaArrayType jat = (JavaArrayType) type;
            JavaType elementType = jat.getElementType();
            mungeTo1_0(elementType, port);
            if (originalTypes.containsKey(elementType)) {
                String typeName = javaTypeToString(type, port);
                //System.out.println("Changing array type, was: "+typeName);
                originalTypes.put(type, typeName);
                jat.doSetName(elementType.getName()+"[]");
            }
        } else if (type instanceof JavaEnumerationType) {
            JavaEnumerationType jet = (JavaEnumerationType) type;
            JavaType baseType = jet.getBaseType();
            mungeTo1_0(baseType, port);
        } else {
            onWarning(getMessage("stubgenerator.unknownType", "mungeTo1_0", type.toString()));
        }
    }

    protected String javaTypeToString(JavaType type, Port port) {
        if (type == null)
            return null;
        String result;
        if (type.isHolder())
            result = env.getNames().holderClassName(port, type);
        else
            result = env.getNames().typeClassName(type);
        return result;
    }

    protected void onWarning(Localizable msg) {
        //System.err.println("Warning: "+msg);
        env.warn(msg);
    }

    protected Localizable getMessage(String key, String arg) {
        return messageFactory.getMessage(key, new Object[] {arg});
    }

    protected Localizable getMessage(String key, String arg1, String arg2) {
        return messageFactory.getMessage(key, new Object[] {arg1, arg2});
    }
}
