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

import javax.xml.namespace.QName;
import com.sun.xml.rpc.processor.model.Model;
import com.sun.xml.rpc.processor.model.AbstractType;
import com.sun.xml.rpc.processor.model.java.*;
import com.sun.xml.rpc.encoding.InternalEncodingConstants;
import com.sun.xml.rpc.wsdl.document.soap.SOAPConstants;
import com.sun.xml.rpc.processor.model.soap.*;
import com.sun.xml.rpc.processor.model.*;
import com.sun.xml.rpc.processor.model.literal.*;
import com.sun.xml.rpc.processor.generator.*;
import com.sun.xml.rpc.processor.*;
import com.sun.xml.rpc.processor.config.Configuration;
import com.sun.xml.rpc.processor.model.Service;
import com.sun.xml.rpc.processor.ProcessorOptions;
import com.sun.xml.rpc.processor.util.GeneratedFileInfo;

import org.netbeans.modules.schema2beansdev.gen.*;

/**
 * Generate the Remote Interface.
 *
 */
public class RemoteInterfaceGenerator extends AbstractGenerator implements ProcessorAction {
    public RemoteInterfaceGenerator() {
    }

    protected String getFullClassName() {
        JavaInterface intf = port.getJavaInterface();
        return env.getNames().customJavaTypeClassName(intf);
    }
    
    protected File getSourceFile() {
        return env.getNames().sourceFileForClass(fullClassName, fullClassName, sourceDir, env);
    }

    protected String getSourceFileType() {
        return GeneratorConstants.FILE_TYPE_REMOTE_INTERFACE;
    }
    
    protected void generateClass() throws java.io.IOException {
        jw.write("public interface ", className, " extends java.rmi.Remote ");
        jw.begin();

        JavaInterface intf = port.getJavaInterface();
        for (Iterator it = intf.getMethods(); it.hasNext();) {
            JavaMethod method = (JavaMethod) it.next();
            generateUserMethod(method);
        }
        
        jw.end();  // end interface
    }

    protected void generateUserMethod(JavaMethod method) throws IOException {
        String methodName = method.getName();
        JavaType returnType = getExpandedReturnType(method);
        String returnTypeName = javaTypeToString(returnType);
        boolean voidReturnType = (returnType == null || "void".equals(returnTypeName));
        jw.write("public ");
        if (voidReturnType) {
            jw.write("void");
        } else {
            jw.write(returnTypeName);
        }
        jw.write(" ");
        jw.write(methodName);
        jw.write("(");
        jw.setFirst(", ");
        List parameterList = getExpandedParametersList(method);
        for (Iterator parameters = parameterList.iterator(); parameters.hasNext(); ) {
            JavaParameter parameter = (JavaParameter) parameters.next();
            JavaType parameterType = parameter.getType();
            String paramTypeName = javaTypeToString(parameterType);
            jw.writeNext(paramTypeName, " ", parameter.getName());
        }
        jw.write(")");
        jw.write(" throws java.rmi.RemoteException");
        for (Iterator exceptions = method.getExceptions();
             exceptions.hasNext(); ) {
            String exceptionName = (String) exceptions.next();
            jw.write(", ");
            jw.write(exceptionName);
        }
        jw.eol();
        jw.cr();
    }
}
