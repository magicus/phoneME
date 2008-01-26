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
import com.sun.xml.rpc.util.localization.LocalizableMessageFactory;
import com.sun.xml.rpc.util.localization.Localizer;
import com.sun.xml.rpc.util.localization.Localizable;

import org.netbeans.modules.schema2beansdev.gen.*;

/**
 * We'll generate the Java client stub/proxy for JSR-172.
 * This is intended
 *
 */
public class StubGenerator extends AbstractGenerator implements ProcessorAction {
    // What to use for maxOccurs="unbounded"
    public final static int UNBOUNDED = -1;

    protected boolean useWSIBasicProfile = false;

    // Have we already given the user info on CLDC 1.0 unsupported types?
    protected boolean cldc1_0InfoAlready = false;

    // Have we already given the user info on CLDC 1.0 converted types?
    protected boolean cldc1_0InfoAlreadyConverted = false;

    // qnames is a mapping of all QNames used by the stub.
    protected Map qnames;	// Map<QName, String>

    // varNameToQNames is the inverse of the qnames map
    protected Map varNameToQNames;	// Map<String, QName>

    // typeNames has all Type's (Element or ComplexType) used by the stub.
    protected Map typeNames;	// Map<String, Type>
    // types is the inverse of typeNames
    protected LinkedHashMap types;		// Map<Type, Type>

    // All faults that this stub uses
    protected Map faultNames;	// Map<QName, Type>

    // @see setOriginalTypes
    protected Map originalTypes;	// Map<JavaType, String>

    // All Java class types used by the stub (for instance, {"int", "float"})
    protected Map usedTypes; // Map<String, null>

    /**
     * This table lists all methods in the Stub generated from WSDL Operations.
     */
    protected Map methodNames;	// Map<String, null>

    protected boolean floatDoubleWarning = false;

    protected boolean genDebug = false;

    protected boolean hasUserExceptions = false;

    protected boolean faultDetailHandlerIsInnerClass = false;

    protected String prepOperationMethodName = "_prepOperation";

    /**
     * The package name to use for the basic 172 runtime types.
     */
    protected String javaxMicroeditionXmlRpc = "javax.microedition.xml.rpc";

    public StubGenerator() {
        messageFactory = new LocalizableMessageFactory("com.sun.j2mews.sg.stubgenerator");
        originalTypes = new HashMap();
    }

    /**
     * @param originalTypes Map<JavaType, String> maps all JavaType's that
     * were replaced (@see MakeCldc1_0), and keeps a copy of the original
     * java class name as the value.
     */
    public void setOriginalTypes(Map originalTypes) {
        this.originalTypes = originalTypes;
    }

    public void perform(Model model, Configuration config, Properties options) {
        useWSIBasicProfile = Boolean.valueOf(options.getProperty(ProcessorOptions.USE_WSI_BASIC_PROFILE)).booleanValue(); 
        if ("true".equals(options.getProperty(FLOAT_DOUBLE_TO_STRING))) {
            floatDoubleWarning = true;
        } else {
            floatDoubleWarning = false;
        }

        super.perform(model, config, options);
    }

    protected void generate() throws java.io.IOException {
        //
        // This method gets called for every generated class, make sure
        // that all of our tables are reset.
        //
        qnames = new HashMap();
        varNameToQNames = new HashMap();
        usedTypes = new HashMap();
        typeNames = new HashMap();
        types = new LinkedHashMap();
        faultNames = new HashMap();
        methodNames = new HashMap();

        if (optimize) {
            prepOperationMethodName = "_prep";
        }

        //
        // Go thru and see what the user fault situation is like.
        //
        for (Iterator operations = port.getOperations(); operations.hasNext(); ) {
            com.sun.xml.rpc.processor.model.Operation operation = (com.sun.xml.rpc.processor.model.Operation) operations.next();
            JavaMethod method = operation.getJavaMethod();
            //
            // Does this operation have any user faults?
            //
            if (method.getExceptionsList().size() > 0)
                hasUserExceptions = true;

            //
            // Are we going to define a handleFault method that will conflict
            // with the one in FaultDetailHandler?
            //
            String methodName = method.getName();
            methodNames.put(methodName, method);
            if ("handleFault".equals(methodName))
                faultDetailHandlerIsInnerClass = true;
        }

        super.generate();
    }

    protected String getFullClassName() {
        return env.getNames().stubFor(port, null);
    }

    protected File getSourceFile() {
        return env.getNames().sourceFileForClass(fullClassName, fullClassName, sourceDir, env);
    }

    protected String getSourceFileType() {
        return GeneratorConstants.FILE_TYPE_STUB;
    }
    
    protected void writeImports() throws IOException {
        jw.writeImport("javax.xml.rpc.JAXRPCException");
        jw.writeImport("javax.xml.namespace.QName");
        jw.writeImport(javaxMicroeditionXmlRpc+".Operation");
        jw.writeImport(javaxMicroeditionXmlRpc+".Type");
        jw.writeImport(javaxMicroeditionXmlRpc+".ComplexType");
        jw.writeImport(javaxMicroeditionXmlRpc+".Element");
        if (hasUserExceptions) {
            jw.writeImport(javaxMicroeditionXmlRpc+".FaultDetailException");
            jw.writeImport(javaxMicroeditionXmlRpc+".FaultDetailHandler");
        }
        jw.cr();
    }

    protected void generateClass() throws java.io.IOException {
        JavaInterface javaInterface = port.getJavaInterface();
        String implementsInfo = javaInterface.getName() + ", javax.xml.rpc.Stub";
        if (hasUserExceptions && !faultDetailHandlerIsInnerClass)
            implementsInfo += ", FaultDetailHandler";
        jw.writeClassDecl(className, null, implementsInfo, jw.PUBLIC);

        jw.select(jw.CONSTRUCTOR_SECTION);
        jw.beginConstructor(className);
        String address = port.getAddress();
        if (address == null) {
            jw.writeEol("_propertyNames = new String[0]");
            jw.writeEol("_propertyValues = new Object[0]");
        } else {
            jw.writeEol("_propertyNames = new String[] {ENDPOINT_ADDRESS_PROPERTY}");
            jw.writeEol("_propertyValues = new Object[] {\""+address+"\"}");
        }
        jw.end();
        jw.cr();

        generateProperties();

        jw.select(jw.BODY_SECTION);
        jw.comment("");
        jw.comment(" Begin user methods");
        jw.comment("");
        generateUserMethods(port);
        jw.comment("");
        jw.comment(" End user methods");
        jw.comment("");

        jw.cr();

        if (genDebug) {
            jw.beginMethod("printObjects", "Object o", null, "void", jw.PRIVATE);
            jw.beginIf("o == null");
            jw.writeEol("System.out.print(\"null\")");
            jw.writeEol("return");
            jw.end();
            jw.beginIf("o.getClass().isArray()");
            jw.writeEol("Object[] a = (Object[]) o");
            jw.writeEol("System.out.print(\"{\")");
            jw.beginFor("int i = 0", "i < a.length", "++i");
            jw.beginIf("i > 0");
            jw.writeEol("System.out.print(\", \")");
            jw.end();
            jw.writeEol("System.out.print(\"[\"+i+\"]=\")");
            jw.writeEol("printObjects(a[i])");
            jw.end();
            jw.writeEol("System.out.print(\"}\")");
            jw.endElseBegin();
            jw.writeEol("System.out.print(\"\\\"\"+o+\"\\\"\")");
            jw.end();
            jw.end();
            jw.cr();
        }

        generateQNameVars();
        generateInitTypes();
        generateFaultHandler();

        jw.select(jw.DECL_SECTION);
        jw.cr();
    }

    protected void generateProperties() throws IOException {
        jw.select(jw.DECL_SECTION);
        jw.writeEol("private String[] _propertyNames");
        jw.writeEol("private Object[] _propertyValues");

        jw.select(jw.BODY_SECTION);
        jw.beginMethod("_setProperty", "String name, Object value");
        //jw.writeEol("name = name.intern()");
        jw.writeEol("int size = _propertyNames.length");
        jw.beginFor("int i = 0", "i < size", "++i");
        jw.beginIf("_propertyNames[i].equals(name)");
        jw.writeEol("_propertyValues[i] = value");
        jw.writeEol("return");
        jw.end();  // if
        jw.end();  // for
        jw.comment("Need to expand our array for a new property");
        jw.writeEol("String[] newPropNames = new String[size + 1]");
        jw.writeEol("System.arraycopy(_propertyNames, 0, newPropNames, 0, size)");
        jw.writeEol("_propertyNames = newPropNames");
        jw.writeEol("Object[] newPropValues = new Object[size + 1]");
        jw.writeEol("System.arraycopy(_propertyValues, 0, newPropValues, 0, size)");
        jw.writeEol("_propertyValues = newPropValues");
        jw.cr();
        jw.writeEol("_propertyNames[size] = name");
        jw.writeEol("_propertyValues[size] = value");
        jw.end();
        jw.cr();

        jw.beginMethod("_getProperty", "String name", null, "Object");
        //jw.writeEol("name = name.intern()");
        jw.beginFor("int i = 0", "i < _propertyNames.length", "++i");
        jw.beginIf("_propertyNames[i].equals(name)");
        jw.writeEol("return _propertyValues[i]");
        jw.end();  // if
        jw.end();  // for
        jw.beginIf("ENDPOINT_ADDRESS_PROPERTY.equals(name) || USERNAME_PROPERTY.equals(name) || PASSWORD_PROPERTY.equals(name)");
        jw.writeEol("return null");
        jw.end();  // if
        jw.beginIf("SESSION_MAINTAIN_PROPERTY.equals(name)");
        jw.writeEol("return new java.lang.Boolean(false)");
        jw.end();  // if
        jw.writeEol("throw new JAXRPCException(", localize(getMessage("stubgenerator.unrecognizedProperty", "name")), ")");
        jw.end();
        jw.cr();

        jw.beginMethod(prepOperationMethodName, "Operation op", null, "void", jw.PROTECTED);
        jw.beginFor("int i = 0", "i < _propertyNames.length", "++i");
        jw.writeEol("op.setProperty(_propertyNames[i], _propertyValues[i].toString())");
        jw.end();  // for
        jw.end();
        jw.cr();
    }

    protected void generateUserMethods(Port port) throws java.io.IOException {
        Iterator operations = port.getOperations();
        while (operations.hasNext()) {
            com.sun.xml.rpc.processor.model.Operation operation = (com.sun.xml.rpc.processor.model.Operation) operations.next();
            JavaInterface portInterface = port.getJavaInterface();
            generateUserMethods(operation, portInterface);
        }
    }
    
    protected void generateUserMethods(Operation operation,
                                       JavaInterface portInterface) throws java.io.IOException {
        JavaMethod method = operation.getJavaMethod();
        // methodName is the name we should give our method
        String methodName = method.getName();
        // operationName is the name of the operation as the web service sees
        // it.
        QName operationQName = operation.getName();
        String operationName = operationQName.getLocalPart();
        if (operationQName.getNamespaceURI() == null || "".equals(operationQName.getNamespaceURI()))
            operationQName = new QName(port.getName().getNamespaceURI(), operationName);
        jw.cr();

        // Make sure we're document/literal here.
        if (operation.getStyle() != com.sun.xml.rpc.wsdl.document.soap.SOAPStyle.DOCUMENT || operation.getUse() != com.sun.xml.rpc.wsdl.document.soap.SOAPUse.LITERAL) {
            String style = styleToString(operation.getStyle());
            String use = useToString(operation.getUse());
            Localizable msg = getMessage("stubgenerator.wrongStyleOrUse",
                                         operationName, style, use);
            commentWarning(msg);
            return;
        }

        if (!isValid(operation, usedTypes)) {
            Localizable msg = getMessage("stubgenerator.operationHasInvalidType",
                                         operationName);
            commentWarning(msg);
            return;
        }

        JavaType origReturnType = method.getReturnType();
        JavaType returnType = getExpandedReturnType(method);
        String returnTypeName = javaTypeToString(returnType);
        boolean voidReturnType = (returnType == null || "void".equals(returnTypeName));

        Map usedNames = new HashMap();
        // Make sure we don't create any variables with the same name as
        // a method.
        usedNames.putAll(methodNames);

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
        int parameterCount = parameterList.size();
        
        for (Iterator parameters = parameterList.iterator(); parameters.hasNext(); ) {
            JavaParameter parameter = (JavaParameter) parameters.next();
            JavaType parameterType = parameter.getType();
            String paramTypeName = javaTypeToString(parameterType);
            jw.writeNext(paramTypeName, " ", parameter.getName());
            usedNames.put(parameter.getName(), parameter);
        }
        jw.write(")");
        jw.write(" throws java.rmi.RemoteException");
        boolean operationHasUserExceptions = false;
        for (Iterator exceptions = method.getExceptions();
             exceptions.hasNext(); ) {
            operationHasUserExceptions = true;
            String exceptionName = (String) exceptions.next();
            jw.write(", ");
            jw.write(exceptionName);
        }
        jw.write(" ");
        jw.begin();
        
        String resultObjVar = makeUniq("resultObj", usedNames);  // reserve this variable name
        String resultVar = makeUniq("result", usedNames);  // reserve this variable name

        String inputVar = null;
        String inputObjectVar = null;
        if (method.getParameterCount() > 0) {
            // Make sure we iterate over parameters from the original list
            if (method.getParameterCount() > 1) {
                onWarning(getMessage("stubgenerator.moreThanOneParameter",
                                     methodName));
            }
            for (Iterator parameters = method.getParameters();
                 parameters.hasNext(); ) {
                JavaParameter parameter = (JavaParameter) parameters.next();

                Parameter p = parameter.getParameter();
                Block parameterBlock = p.getBlock();

                JavaType parameterType = parameter.getType();
                String paramTypeName = javaTypeToString(parameterType);
                Type resultTypeVar = convertJavaTypeToType(parameterType);
                boolean isNillable = p.getType().isNillable();
                //System.out.println("methodName="+methodName+" paramTypeName="+paramTypeName+" isNillable="+isNillable);
                if (!isNillable) {
                    // Double check due to a feature of JAX-RPC.
                    isNillable = forceNillable(parameterType, methodName);
                }
                boolean isOptional = false;
                Element resultElement = getElement(parameterBlock.getName(),
                                                   resultTypeVar,
                                                   false, isNillable,
                                                   isOptional);
                resultElement.setTopLevel(true);
                inputVar = resultElement.getVarName();
            }

            // Make sure to use the parameters from our copy of the list.
            Iterator parameters = parameterList.iterator();
            jw.comment("Copy the incoming values into an Object array if needed.");
            boolean multipleParams = parameterList.size() > 1 ||
                parameterList != method.getParametersList();
            String multipleParamsVar = null;
            if (multipleParams) {
                multipleParamsVar = instanceOf("input", "Object", usedNames);
                jw.writeEol("Object[] ", multipleParamsVar, " = new Object["+parameterList.size(), "]");
            }
            for (int paramNum = 0; parameters.hasNext(); ++paramNum) {
                JavaParameter parameter = (JavaParameter) parameters.next();
                String paramJavaName = parameter.getName();
                JavaType parameterType = parameter.getType();
                inputObjectVar = genMakeInputObject(parameterType,
                                                    usedNames,
                                                    paramJavaName);
                if (multipleParams) {
                    jw.writeEol(multipleParamsVar, "["+paramNum, "] = ", inputObjectVar);
                }
            }
            if (multipleParams) {
                inputObjectVar = multipleParamsVar;
            }
        }
        if (genDebug) {
            jw.writeEol("System.out.print(\"Input objects: \")");
            jw.writeEol("printObjects(", inputObjectVar, ")");
            jw.writeEol("System.out.println()");
        }

        String outputVar = null;
        if (!voidReturnType) {
            Type outputTypeVar = convertJavaTypeToType(origReturnType);
            QName returnTypeQName = null;
            Response response = operation.getResponse();
            boolean isNillable = false;
            boolean isOptional = false;
            for (Iterator it = response.getParameters(); it.hasNext(); ) {
                Parameter p = (Parameter) it.next();
                Block block = p.getBlock();
                returnTypeQName = block.getName();
                isNillable = p.getType().isNillable();
                /*  Trust the nillable from the type
                if (!isNillable) {
                    isNillable = forceNillable(returnType, methodName);
                }
                */
                // There should only be 1 Parameter in the return.
            }
            Element outputElement = getElement(returnTypeQName, outputTypeVar,
                                               false, isNillable, isOptional);
            outputElement.setTopLevel(true);
            outputVar = outputElement.getVarName();
        }

        jw.cr();
        jw.write("Operation op = Operation.newInstance(", getQNameVar(operationQName), ", ");
        jw.write(inputVar, ", ", outputVar);
        if (operationHasUserExceptions) {
            if (faultDetailHandlerIsInnerClass)
                jw.write(", _myFaultDetailHandler");
            else
                jw.write(", this");
        }
        jw.writeEol(")");
        jw.writeEol(prepOperationMethodName, "(op)");
        String soapAction = operation.getSOAPAction();
        if (soapAction == null) {
            // See Section 8.2.1.  If not set, it shall be the empty string.
            soapAction = "";
        }
        jw.writeEol("op.setProperty(Operation.SOAPACTION_URI_PROPERTY, ", JavaUtil.instanceFrom("String", operation.getSOAPAction()), ")");

        if (!voidReturnType)
            jw.writeEol("Object ", resultObjVar);
        jw.beginTry();
        if (!voidReturnType) {
            jw.write(resultObjVar, " = ");
        }
        jw.writeEol("op.invoke(", inputObjectVar, ")");
        jw.endCatch("JAXRPCException e");
        String causeVar = makeUniq("cause", usedNames);
        jw.writeEol("Throwable ", causeVar, " = e.getLinkedCause()");
        jw.beginIf(causeVar+" instanceof java.rmi.RemoteException");
        jw.writeEol("throw (java.rmi.RemoteException) ", causeVar);
        if (operationHasUserExceptions) {
            jw.endElseBeginIf(causeVar+" instanceof FaultDetailException");
            String fdeVar = makeUniq("fde", usedNames);
            String fdeNameVar = makeUniq("fdeName", usedNames);
            jw.writeEol("FaultDetailException ", fdeVar, " = (FaultDetailException) ", causeVar);
            jw.write("QName ", fdeNameVar);
            jw.writeEol(" = ", fdeVar, ".getFaultDetailName()");
            genMakeFaults(operation, fdeVar, fdeNameVar, usedNames);
        }
        jw.end();
        jw.writeEol("throw e");
        jw.end();
        if (genDebug) {
            jw.writeEol("System.out.print(\"Output objects: \")");
            jw.writeEol("printObjects(", resultObjVar, ")");
            jw.writeEol("System.out.println()");
        }

        if (!voidReturnType) {
            jw.writeEol(returnTypeName, " ", resultVar);
            jw.comment("Convert the result into the right Java type.");
            if (origReturnType == returnType) {
                genMakeOutputObject(resultObjVar, false, resultVar, returnType,
                                    usedNames);
            } else {
                jw.comment("Unwrapped return value");
                JavaStructureType jst = (JavaStructureType) origReturnType;
                // There's only 1 member.
                JavaStructureMember jsm = (JavaStructureMember) jst.getMembers().next();
                genMakeOutputObject(resultObjVar, false, resultVar,
                                    jsm, 0, 1, usedNames);
            }
            jw.writeEol("return ", resultVar);
        }
        jw.end();
    }

    /**
     * Convert JavaType into the a Type which the stub will use.
     */
    protected Type convertJavaTypeToType(JavaType type) throws IOException {
        //jw.comment("type="+type);
        String typeName = javaTypeToString(type).intern();
        //jw.comment("typeName="+typeName);
        if (type instanceof JavaSimpleType) {
            Type result = Type.toType(typeName);
            if (result == Type.UNKNOWN) {
                onError(getMessage("stubgenerator.unknownSimpleType", typeName));
            }
            return result;
        } else if (type instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) type;
            int membersCount = jst.getMembersCount();
            List elements = new ArrayList(membersCount);	// List<Type>
            for (Iterator members = jst.getMembers(); members.hasNext(); ) {
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                //jw.comment("jsm="+jsm);
                String memberName = jsm.getName();
                JavaType memberType = jsm.getType();

                boolean isArray = false;
                boolean isNillable = false;
                boolean isOptional = false;
                if (memberType instanceof JavaArrayType)
                    isArray = true;
                QName memberQName = null;
                Object memberOwner = jsm.getOwner();
                //jw.comment("memberOwner="+memberOwner);
                if (memberOwner instanceof AbstractType) {
                    AbstractType at = (AbstractType) memberOwner;
                    //jw.comment("at.getName="+at.getName());
                    memberQName = at.getName();
                    if (at.isNillable())
                        isNillable = true;
                } else if (memberOwner instanceof LiteralElementMember) {
                    LiteralElementMember lem = (LiteralElementMember) memberOwner;
                    memberQName = lem.getName();
                    if (lem.isNillable()) {
                        isNillable = true;
                    }
                    if (!lem.isRequired())
                        isOptional = true;
                    //jw.comment("lem.gettype="+lem.getType());
                } else {
                    jw.comment("!!!!!!! memberOwner is of unknown type: "+memberOwner);
                }
                /*
                if (!isNillable) {
                    isNillable = forceNillable(memberType, memberQName.toString());
                }
                */
                //jw.comment("memberQName="+memberQName+" isNillable="+isNillable+" isOptional="+isOptional+" isArray="+isArray);
                if (memberQName == null) {
                    onError(getMessage("stubgenerator.unknownQNameOfMember",
                                       memberName));
                }

                Type memberTypeVarName = convertJavaTypeToType(memberType);
                Element element = getElement(memberQName, memberTypeVarName,
                                             isArray, isNillable, isOptional);
                elements.add(element);
            }
            ComplexType cType = getComplexType(type, elements);
            return cType;
        } else if (type instanceof JavaArrayType) {
            JavaArrayType jat = (JavaArrayType) type;
            JavaType elementType = jat.getElementType();
            //jw.comment("jat="+jat+" elementType="+elementType);
            String elementTypeName = javaTypeToString(elementType);
            Type elementTypeVarName = convertJavaTypeToType(elementType);
            return elementTypeVarName;
        } else if (type instanceof JavaEnumerationType) {
            JavaEnumerationType jet = (JavaEnumerationType) type;
            JavaType baseType = jet.getBaseType();
            return convertJavaTypeToType(baseType);
        } else {
            commentWarning(getMessage("stubgenerator.unknownType", "convertJavaTypeToType", type.toString()));
        }
        return Type.UNKNOWN;
    }

    /**
     * Generate code to convert a java bean graph (most likely from the
     * formal parameter) into an Object[] as the spec defines.  See
     * Section 8.2.4.
     */
    protected String genMakeInputObject(JavaType type,
                                        Map usedNames,
                                        String inputExpr) throws IOException {
        //jw.comment("type="+type);
        String typeName = javaTypeToString(type).intern();
        if (type instanceof JavaSimpleType) {
            return JavaUtil.toObject(inputExpr, typeName, true);
        } else if (type instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) type;
            int membersCount = jst.getMembersCount();
            String varName = instanceOf(type, "Object", usedNames);
            List resultExprs = new ArrayList(membersCount);	// List<String>
            jw.writeEol("Object[] ", varName);
            jw.beginIf(inputExpr+" == null");
            jw.writeEol(varName, " = null");
            jw.endElseBegin();
            for (Iterator members = jst.getMembers(); members.hasNext(); ) {
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                JavaType memberType = jsm.getType();
                String memberTypeName = javaTypeToString(memberType);
                String curInputExpr = inputExpr+"."+jsm.getReadMethod()+"()";
                String resultExpr = genMakeInputObject(memberType,
                                                       usedNames, curInputExpr);
                resultExprs.add(resultExpr);
            }
            jw.writeEol(varName, " = new Object["+membersCount, "]");
            Iterator resultExprIt = resultExprs.iterator();
            for (int elementNum = 0; resultExprIt.hasNext(); ++elementNum) {
                String resultExpr = (String) resultExprIt.next();
                jw.write(varName, "["+elementNum, "] = ");
                jw.writeEol(resultExpr);
            }
            jw.end();
            return varName;
        } else if (type instanceof JavaArrayType) {
            JavaArrayType jat = (JavaArrayType) type;
            JavaType elementType = jat.getElementType();
            if (elementType instanceof JavaSimpleType) {
                //jw.comment(inputExpr+" is already the right type (array of simple things).");
                return inputExpr;
            }

            String elementTypeName = javaTypeToString(elementType);
            String varName = instanceOf(type, "Object", usedNames);
            jw.writeEol("Object[] ", varName);
            jw.beginIf(inputExpr+" == null");
            //jw.writeEol(varName, " = null");
            //jw.comment("Arrays can only be empty, not null");
            //jw.writeEol("throw new IllegalArgumentException(\"", inputExpr, " == null\")");
            // See section 8.2.4, null arrays are the same as a 0 length array.
            jw.writeEol(varName, " = new Object[0]");
            jw.endElseBegin();
            jw.write(varName, " = new Object[");
            jw.writeEol(inputExpr, ".length]");
            String indexVar = instanceOf(type, "Index", usedNames);
            String curInputExpr = inputExpr+"["+indexVar+"]";
            jw.beginFor("int "+indexVar+" = 0",
                        indexVar+" < "+inputExpr+".length", "++"+indexVar);
            String resultExpr = genMakeInputObject(elementType,
                                                   usedNames, curInputExpr);
            jw.cr();
            jw.write(varName, "[", indexVar);
            jw.writeEol("] = ", resultExpr);
            jw.end();
            jw.end();
            return varName;
        } else if (type instanceof JavaEnumerationType) {
            JavaEnumerationType jet = (JavaEnumerationType) type;
            JavaType baseType = jet.getBaseType();
            return genMakeInputObject(baseType, usedNames,
                                      inputExpr+".getValue()");
        } else {
            jw.comment(" Hit unknown type: type="+type+" for "+inputExpr);
            commentWarning(getMessage("stubgenerator.unknownType", "genMakeInputObject", type.toString()));
            return "unknown type ("+type+") for "+inputExpr;
        }
    }

    /**
     * Generate code to convert an Object[] into a java bean graph.
     * See Section 8.2.6.
     */
    protected void genMakeOutputObject(String objName, boolean objNameIsArray,
                                       String resultName,
                                       JavaType type,
                                       Map usedNames) throws IOException {
        //jw.comment("type="+type);
        String typeName = javaTypeToString(type).intern();
        if (type instanceof JavaSimpleType) {
            if (objNameIsArray)
                objName = "((Object)"+objName+")";
            jw.writeEol(resultName, " = ",
                        JavaUtil.fromObject(typeName, objName));
        } else if (type instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) type;
            jw.beginIf(objName+" == null");
            jw.writeEol(resultName, " = null");
            jw.endElseBegin();
            jw.write(resultName, " = new ");
            jw.writeEol(typeName, "()");
            int membersCount = jst.getMembersCount();
            Iterator members = jst.getMembers();
            for (int elementNum = 0; members.hasNext(); ++elementNum) {
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                JavaType memberType = jsm.getType();
                String memberTypeName = javaTypeToString(memberType);
                String varName = instanceOf(memberType, usedNames);
                jw.writeEol(memberTypeName, " ", varName);
                genMakeOutputObject(objName, objNameIsArray, varName,
                                    jsm, elementNum, membersCount, usedNames);
                jw.writeEol(resultName+"."+jsm.getWriteMethod()+"("+varName+")");
            }
            jw.end();
        } else if (type instanceof JavaArrayType) {
            JavaArrayType jat = (JavaArrayType) type;
            JavaType elementType = jat.getElementType();
            String elementTypeName = javaTypeToString(elementType);
            if (elementType instanceof JavaSimpleType) {
                //
                // The SPI will return an array of the right type
                // for simple types.
                // Don't need to go thru and copy it over 1 by 1.
                //
                jw.write(resultName, " = (", elementTypeName, "[]) ");
                jw.writeEol(objName);
            } else {
                jw.beginIf(objName+" == null");
                jw.writeEol(resultName, " = null");
                jw.endElseBegin();
                if (!objNameIsArray)
                    objName = "((Object[])"+objName+")";
                String sizeVar = instanceOf(type, "Size", usedNames);
                jw.write("int ", sizeVar, " = ");
                jw.writeEol(objName, ".length");
                jw.write(resultName, " = new ", elementTypeName);
                jw.writeEol("[", sizeVar, "]");
                String indexVar = instanceOf(type, "Index", usedNames);
                String curResultExpr = resultName+"["+indexVar+"]";
                String curObjExpr = objName+"["+indexVar+"]";
                jw.beginFor("int "+indexVar+" = 0",
                            indexVar+" < "+sizeVar, "++"+indexVar);
                genMakeOutputObject(curObjExpr, false, curResultExpr,
                                    elementType,
                                    usedNames);
                jw.end();
                jw.end();
            }
        } else if (type instanceof JavaEnumerationType) {
            JavaEnumerationType jet = (JavaEnumerationType) type;
            JavaType baseType = jet.getBaseType();
            jw.beginIf(objName+" == null");
            jw.writeEol(resultName, " = null");
            jw.endElseBegin();
            String varName = instanceOf(type, usedNames);
            jw.writeEol(javaTypeToString(baseType), " ", varName);
            genMakeOutputObject(objName, objNameIsArray, varName,
                                baseType, usedNames);
            jw.write(resultName, " = ");
            jw.writeEol(typeName, ".fromValue(", varName, ")");
            jw.end();
        } else {
            commentWarning(getMessage("stubgenerator.unknownType", "genMakeOutputObject", type.toString()));
        }
    }

    protected void genMakeOutputObject(String objName,
                                       boolean objNameIsArray,
                                       String resultName,
                                       JavaStructureMember jsm,
                                       int elementNum, int membersCount,
                                       Map usedNames) throws IOException {
        String memberName = jsm.getName();
        JavaType memberType = jsm.getType();
        String memberObjName = makeUniq(memberName+"Obj", usedNames);
        boolean memberObjNameIsArray;
        //
        // Declare a new variable, memberObjName, and initalize it's value
        // to be objName[elementNum]
        //
        if (memberType instanceof JavaSimpleType ||
            memberType instanceof JavaEnumerationType ||
            (memberType instanceof JavaArrayType &&
             ((JavaArrayType)memberType).getElementType() instanceof JavaSimpleType)) {
            jw.write("Object ", memberObjName, " = ");
            memberObjNameIsArray = false;
        } else {
            jw.write("Object[] ", memberObjName, " = (Object[]) ");
            memberObjNameIsArray = true;
        }
        if (objNameIsArray)
            jw.write(objName);
        else
            jw.write("((Object[])", objName, ")");
        jw.write("["+elementNum, "]");
        jw.eol();
        genMakeOutputObject(memberObjName, memberObjNameIsArray,
                            resultName,
                            memberType,
                            usedNames);
    }

    /**
     * Generate code to convert a JAXRPCException into service
     * specific exceptions.
     */
    protected void genMakeFaults(Operation operation,
                                 String fdeVar, String fdeNameVar,
                                 Map usedNames) throws IOException {
        for (Iterator faults = operation.getFaults(); faults.hasNext(); ) {
            Fault fault = (Fault) faults.next();
            Block faultBlock = fault.getBlock();
            QName exceptionName = faultBlock.getName();
            JavaException jexcep = fault.getJavaException();

            boolean wrappedSimpleType;
            if (jexcep.getMembersCount() == 1 &&
                faultBlock.getType() instanceof LiteralSimpleType) {
                //
                // JAX-RPC is wrapping the simple type up into a
                // complexType, which is fine for the Java side, but
                // we need to correctly describe the schema type as being
                // simple (in the Type's).
                //
                wrappedSimpleType = true;
            } else {
                wrappedSimpleType = false;
            }

            Type exceptionType = convertJavaTypeToType(jexcep);
            Element exceptionElement;
            if (wrappedSimpleType && exceptionType instanceof ComplexType) {
                ComplexType cType = (ComplexType) exceptionType;
                // Pull out the first element, and use it instead.
                exceptionElement = cType.getElement(0);
                exceptionName = exceptionElement.getQName();
            } else {
                exceptionElement = getElement(exceptionName,
                                              exceptionType);
            }
            exceptionElement.setTopLevel(true);
            //jw.comment("exception var name ="+exceptionElement.getVarName());
            faultNames.put(exceptionName, exceptionElement);

            jw.beginIf("\""+exceptionName.getLocalPart()+"\".equals("+fdeNameVar+".getLocalPart()) && \""+exceptionName.getNamespaceURI()+"\".equals("+fdeNameVar+".getNamespaceURI())");
            boolean firstMember = true;
            int elementNum = 0;
            List paramNames = new ArrayList(jexcep.getMembersCount());
            for (Iterator members = jexcep.getMembers(); members.hasNext(); ++elementNum) {
                firstMember = false;
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                String paramName = instanceOf(jsm.getName(), "_param", usedNames);
                paramNames.add(paramName);
                JavaType memberType = jsm.getType();
                String memberTypeName = javaTypeToString(memberType);
                // Declare each parameter and give it a default value.
                jw.writeEol(memberTypeName, " ", paramName); // + " = " + JavaUtil.nullValueForType(memberTypeName));
                String objName;
                if (wrappedSimpleType) {
                    objName = fdeVar+".getFaultDetail()";
                } else {
                    objName = "((Object[])"+fdeVar+".getFaultDetail())["+elementNum+"]";
                }
                genMakeOutputObject(objName,
                                    false, paramName, memberType,
                                    usedNames);
            }
            jw.write("throw new ");
            jw.write(javaTypeToString(jexcep));
            jw.write("(");
            jw.setFirst(", ");
            for (Iterator paramNamesIt = paramNames.iterator(); paramNamesIt.hasNext(); ) {
                String paramName = (String) paramNamesIt.next();
                jw.writeNext(paramName);
            }
            jw.writeEol(")");
            jw.end();
        }
    }

    /**
     * Check all types associated with an Operation and make sure they are
     * all valid.
     */
    public boolean isValid(Operation operation,
                           Map usedTypes) {
        JavaMethod method = operation.getJavaMethod();
        JavaType returnType = method.getReturnType();
        String operationName = operation.getName().getLocalPart();
        if (!isValid("Operation "+operationName+" return type", returnType,
                     usedTypes))
            return false;
        for (Iterator parameters = method.getParameters(); parameters.hasNext(); ) {
            JavaParameter parameter = (JavaParameter) parameters.next();
            if (!isValid("Operation "+operationName+" parameter "+parameter.getName(),
                         parameter.getType(), usedTypes))
                return false;
        }
        for (Iterator faults = operation.getFaults(); faults.hasNext(); ) {
            Fault fault = (Fault) faults.next();
            Block faultBlock = fault.getBlock();
            JavaException jexcep = fault.getJavaException();
            if (!isValid("Operation "+operationName+" fault "+fault.getName(),
                         jexcep, usedTypes))
                return false;
        }
        return true;
    }
    
    public boolean isValid(String contextInfo, JavaType type, Map usedTypes) {
        String typeName = javaTypeToString(type);
        if (typeName == null)
            return true;
        typeName = typeName.intern();
        if (type instanceof JavaSimpleType) {
            usedTypes.put(typeName, null);
            if (originalTypes.containsKey(type)) {
                String originalTypeName = (String) originalTypes.get(type);
                if (showCldc1_0Info) {
                    onWarning(getMessage("stubgenerator.cldc1_0.warning",
                                         contextInfo, originalTypeName, typeName));
                } else if (!cldc1_0InfoAlreadyConverted) {
                    onWarning(getMessage("stubgenerator.cldc1_0.generalInfoConverted"));
                    cldc1_0InfoAlreadyConverted = true;
                }
                return true;
            }
            if (typeName == "float" || typeName == "Float" ||
                typeName == "java.lang.Float" || typeName == "double" ||
                typeName == "Double" || typeName == "java.lang.Double") {
                if (showCldc1_0Info) {
                    onInfo(getMessage("stubgenerator.cldc1_0.info",
                                      contextInfo, typeName));
                } else if (!cldc1_0InfoAlready) {
                    onInfo(getMessage("stubgenerator.cldc1_0.generalInfo"));
                    cldc1_0InfoAlready = true;
                }
                return true;
            }
            if (typeName == "javax.xml.soap.SOAPElement") {
                onError(getMessage("stubgenerator.unknownSimpleType", typeName));
                return false;
            }
            if (!isValidType(typeName)) {
                onError(getMessage("stubgenerator.invalidType",
                                   contextInfo, typeName));
                return false;
            }
        } else if (type instanceof JavaStructureType) {
            JavaStructureType jst = (JavaStructureType) type;
            Iterator members = jst.getMembers();
            while (members.hasNext()) {
                JavaStructureMember jsm = (JavaStructureMember) members.next();
                if (!isValid(contextInfo+"."+jsm.getName(),
                             jsm.getType(), usedTypes))
                    return false;
            }
        } else if (type instanceof JavaArrayType) {
            JavaArrayType jat = (JavaArrayType) type;
            if (!isValid(contextInfo+"[]",
                         jat.getElementType(), usedTypes))
                return false;
        } else if (type instanceof JavaEnumerationType) {
            JavaEnumerationType jet = (JavaEnumerationType) type;
            JavaType baseType = jet.getBaseType();
            if (!isValid(contextInfo+" enumeration", baseType, usedTypes))
                return false;
        } else {
            onWarning(getMessage("stubgenerator.unknownType", "isValid", type.toString()));
        }
        return true;
    }

    protected void commentWarning(Localizable msg) throws IOException {
        comment(msg);
        onWarning(msg);
    }
    
    protected void comment(Localizable msg) throws IOException {
        jw.comment(localizer.localize(msg));
    }

    protected String styleToString(com.sun.xml.rpc.wsdl.document.soap.SOAPStyle style) {
        if (style == com.sun.xml.rpc.wsdl.document.soap.SOAPStyle.DOCUMENT)
            return "document";
        else if (style == com.sun.xml.rpc.wsdl.document.soap.SOAPStyle.RPC)
            return "rpc";
        else
            return "unknown";
    }

    protected String useToString(com.sun.xml.rpc.wsdl.document.soap.SOAPUse use) {
        if (use == com.sun.xml.rpc.wsdl.document.soap.SOAPUse.LITERAL)
            return "literal";
        else if (use == com.sun.xml.rpc.wsdl.document.soap.SOAPUse.ENCODED)
            return "encoded";
        else
            return "unknown";
    }

    protected boolean forceNillable(JavaType javatype, String methodName) {
        String type = javaTypeToString(javatype);
        if (!type.equals(JavaUtil.fromObjectType(type))) {
            //onWarning(getMessage("stubgenerator.forcingNillable",
            //                     type, methodName));
            return true;
        }
        return false;
    }
    
    protected String getQNameVar(QName name) {
        if (qnames.containsKey(name))
            return (String) qnames.get(name);
        String qnameVar;
        if (optimize)
            qnameVar = makeUniq("_q", varNameToQNames);
        else
            qnameVar = makeVarName(name, "_qname_", varNameToQNames);
        varNameToQNames.put(qnameVar, name);
        qnames.put(name, qnameVar);
        return qnameVar;
    }

    /**
     * Generate code to define each QName used in the stub.
     */
    protected void generateQNameVars() throws IOException {
        List sortedNames = new ArrayList(qnames.keySet());
        // Make it a stable listing.
        Collections.sort(sortedNames, new QNameComparator());
        for (Iterator it = sortedNames.iterator(); it.hasNext(); ) {
            Object o = it.next();
            QName name = (QName) o;
            jw.write("protected static final QName ");
            jw.write((String)qnames.get(name));
            jw.write(" = new QName(");
            if (name.getNamespaceURI() == null)
                jw.write("null");
            else {
                jw.write("\"");
                jw.write(name.getNamespaceURI());
                jw.write("\"");
            }
            jw.write(", \"");
            jw.write(name.getLocalPart());
            jw.writeEol("\")");
        }
    }

    /**
     * Provide an ordering for QNames.
     */
    protected static class QNameComparator implements Comparator {
        public QNameComparator() {}

        public int compare(Object o1, Object o2) {
            QName qn1 = (QName) o1;
            QName qn2 = (QName) o2;
            int result = qn1.getNamespaceURI().compareTo(qn2.getNamespaceURI());
            if (result == 0)
                result = qn1.getLocalPart().compareTo(qn2.getLocalPart());
            return result;
        }
    }

    /**
     * This class mimics javax.microedition.xml.rpc.Type and will
     * convert itself into java code that creates that Type.
     */
    protected static class Type {
        protected String varName;
        
        protected Type(String varName) {
            this.varName = varName;
        }

        public String getVarName() {
            return varName;
        }

        public String getClassName() {
            throw new UnsupportedOperationException();
        }

        public boolean isTopLevel() {
            return false;
        }

        public final static Type BOOLEAN = new Type("Type.BOOLEAN");
        public final static Type BYTE = new Type("Type.BYTE");
        public final static Type DOUBLE = new Type("Type.DOUBLE");
        public final static Type FLOAT = new Type("Type.FLOAT");
        public final static Type INT = new Type("Type.INT");
        public final static Type LONG = new Type("Type.LONG");
        public final static Type SHORT = new Type("Type.SHORT");
        public final static Type STRING = new Type("Type.STRING");
        public final static Type UNKNOWN = new Type("UNKNOWN");

        /**
         * Convert the normal java type into the enumeration of Type.
         * @param name The java class name (e.g., "int").
         */
        public static Type toType(String name) {
            Type result = (Type) toTypeMap.get(name);
            if (result == null) {
                return UNKNOWN;
            }
            return result;
        }

        private final static Map toTypeMap;	// Map<String, Type>
        static {
            toTypeMap = new HashMap();
            toTypeMap.put("boolean", BOOLEAN);
            toTypeMap.put("Boolean", BOOLEAN);
            toTypeMap.put("java.lang.Boolean", BOOLEAN);
            toTypeMap.put("byte", BYTE);
            toTypeMap.put("Byte", BYTE);
            toTypeMap.put("java.lang.Byte", BYTE);
            toTypeMap.put("int", INT);
            toTypeMap.put("Integer", INT);
            toTypeMap.put("java.lang.Integer", INT);
            toTypeMap.put("long", LONG);
            toTypeMap.put("Long", LONG);
            toTypeMap.put("java.lang.Long", LONG);
            toTypeMap.put("short", SHORT);
            toTypeMap.put("Short", SHORT);
            toTypeMap.put("java.lang.Short", SHORT);
            toTypeMap.put("float", FLOAT);
            toTypeMap.put("Float", FLOAT);
            toTypeMap.put("java.lang.Float", FLOAT);
            toTypeMap.put("double", DOUBLE);
            toTypeMap.put("Double", DOUBLE);
            toTypeMap.put("java.lang.Double", DOUBLE);
            toTypeMap.put("String", STRING);
            toTypeMap.put("java.lang.String", STRING);
        }

        public void write(Writer out) throws IOException {
            throw new UnsupportedOperationException();
        }

        public boolean equals(Object o) {
            if (o == this)
                return true;
            if (!(o instanceof Type))
                return false;
            Type e = (Type) o;
            if (!(varName == null ? e.varName == null : varName.equals(e.varName)))
                return false;
            return true;
        }

        public int hashCode() {
            return (varName == null ? 0 : varName.hashCode());
        }

        public String toString() {
            try {
                StringWriter sw = new StringWriter();
                write(sw);
                return sw.toString();
            } catch (IOException e) {
                return varName;
            } catch (UnsupportedOperationException e) {
                return varName;
            }
        }
    }

    /**
     * This class mimics javax.microedition.xml.rpc.Element and will
     * convert itself into java code that creates that type.
     */
    protected static class Element extends Type {
        private String name;
        private QName qName;
        private Type type;
        private int minOccurs;
        private int maxOccurs;
        private boolean nillable;
        private boolean topLevel;

        protected Element(String varName, String name, 
                          QName qName, Type type,
                          int minOccurs, int maxOccurs,
                          boolean nillable) {
            super(varName);
            this.name = name;
            this.qName = qName;
            if (type == null)
                throw new IllegalArgumentException("type == null");
            this.type = type;
            this.minOccurs = minOccurs;
            this.maxOccurs = maxOccurs;
            this.nillable = nillable;
        }

        public String getClassName() {
            return "Element";
        }

        public void setVarName(String name) {
            varName = name;
        }

        public void setTopLevel(boolean value) {
            topLevel = value;
        }

        public boolean isTopLevel() {
            return topLevel;
        }

        public QName getQName() {
            return qName;
        }

        public void write(Writer out) throws IOException {
            out.write(varName);
            out.write(" = new ");
            out.write(getClassName());
            out.write("(");
            out.write(name);
            out.write(", ");
            out.write(type.getVarName());
            if (minOccurs != 1 || maxOccurs != 1 || nillable != false) {
                out.write(", "+minOccurs);
                out.write(", "+maxOccurs);
                out.write(", "+nillable);
            }
            out.write(");\n");
        }

        /**
         * This equals does not take topLevel into account, since toplevel
         * is not a value that's reflected in the external data type.
         */
        public boolean equals(Object o) {
            if (o == this)
                return true;
            if (!(o instanceof Element))
                return false;
            Element e = (Element) o;
            if (!(name == null ? e.name == null : name.equals(e.name)))
                return false;
            if (!(qName == null ? e.qName == null : qName.equals(e.qName)))
                return false;
            if (!(type == null ? e.type == null : type.equals(e.type)))
                return false;
            if (!(minOccurs == e.minOccurs))
                return false;
            if (!(maxOccurs == e.maxOccurs))
                return false;
            if (!(nillable == e.nillable))
                return false;
            return true;
        }

        public int hashCode() {
            int result = 17;
            result = 37*result + (name == null ? 0 : name.hashCode());
            result = 37*result + (qName == null ? 0 : qName.hashCode());
            result = 37*result + (type == null ? 0 : type.hashCode());
            result = 37*result + minOccurs;
            result = 37*result + maxOccurs;
            result = 37*result + (nillable ? 0 : 1);
            return result;
        }
    }

    /**
     * This class mimics javax.microedition.xml.rpc.ComplexType and will
     * convert itself into java code that creates that type.
     */
    protected static class ComplexType extends Type {
        private List elements;	// List<Element>

        protected ComplexType(String varName, List elements) {
            super(varName);
            this.elements = elements;
        }

        public String getClassName() {
            return "ComplexType";
        }
        
        public void setVarName(String name) {
            varName = name;
        }

        public Element getElement(int index) {
            return (Element) elements.get(index);
        }

        public void write(Writer out) throws IOException {
            out.write(varName);
            out.write(" = new ");
            out.write(getClassName());
            out.write("();\n");
            out.write(varName);
            int size = elements.size();
            out.write(".elements = new Element["+size);
            out.write("];\n");
            Iterator it = elements.iterator();
            for (int elementNum = 0; it.hasNext(); ++elementNum) {
                Element e = (Element) it.next();
                out.write(varName);
                out.write(".elements["+elementNum);
                out.write("] = ");
                out.write(e.getVarName());
                out.write(";\n");
            }
        }
        
        public boolean equals(Object o) {
            if (o == this)
                return true;
            if (!(o instanceof ComplexType))
                return false;
            ComplexType ct = (ComplexType) o;
            if (elements.size() != ct.elements.size())
                return false;
            for (Iterator it = elements.iterator(), it2 = ct.elements.iterator();
                 it.hasNext(); ) {
                Element type = (Element) it.next();
                Element type2 = (Element) it2.next();
                if (!(type == null ? type2 == null : type.equals(type2)))
                    return false;
            }
            return true;
        }

        public int hashCode() {
            int result = 17;
            for (Iterator it = elements.iterator(); it.hasNext(); ) {
                Element type = (Element) it.next();
                result = 37*result + (type == null ? 0 : type.hashCode());
            }
            return result;
        }
    }

    protected Element getElement(QName name, Type type) {
        return getElement(name, type, 1, 1, false);
    }

    protected Element getElement(QName name, Type type,
                                 int minOccurs, int maxOccurs,
                                 boolean nillable) {
        String varName;
        if (optimize)
            varName = "_t";
        else
            varName = makeVarName(name, "_type_", null);
        Element result = new Element(varName, getQNameVar(name), name, type,
                                     minOccurs, maxOccurs,
                                     nillable);
        if (types.containsKey(result)) {
            // It might already be defined.
            //System.out.println("type already defined: "+result);
            Object o = types.get(result);
            // Return the one that was created first.
            return (Element) o;
        }
        // Make sure the name is unique.
        varName = makeUniq(varName, typeNames);
        result.setVarName(varName);
        //System.out.println("new Element: "+result);

        types.put(result, result);
        typeNames.put(varName, result);
        return result;
    }
    
    protected Element getElement(QName name, Type type,
                                        boolean isArray,
                                        boolean nillable, boolean optional) {
        return getElement(name, type,
                          optional ? 0 : 1,
                          isArray ? UNBOUNDED : 1,
                          nillable);
    }

    protected ComplexType getComplexType(JavaType type, List elements) {
        String name = type.getName();
        int pos;
        // Remove everything before the .
        pos = name.lastIndexOf('.');
        if (pos > 0)
            name = name.substring(pos+1, name.length());

        // Get rid of any trailing []
        int arrayDimensions = 0;
        while (name.endsWith("[]")) {
            name = name.substring(0, name.length()-2);
            ++arrayDimensions;
        }
        for (int i = 0; i < arrayDimensions; ++i)
            name += "Array";

        return getComplexType(name, elements);
    }
    
    protected ComplexType getComplexType(String name, List elements) {
        String varName;
        if (optimize)
            varName = "_c";
        else
            varName = "_complexType_"+env.getNames().validJavaMemberName(name);
        ComplexType result = new ComplexType(varName, elements);
        if (types.containsKey(result)) {
            // It might already be defined.
            //System.out.println("type already defined: "+result);
            Object o = types.get(result);
            return (ComplexType)o;
        }
        // Make sure the name is unique.
        varName = makeUniq(varName, typeNames);
        result.setVarName(varName);
        //System.out.println("new ComplexType: "+result);

        types.put(result, result);
        typeNames.put(varName, result);
        return result;
    }

    /**
     * Generate code to initialize all of the Type's used by the stub.
     */
    protected void generateInitTypes() throws IOException {
        for (Iterator it = types.keySet().iterator(); it.hasNext(); ) {
            Type type = (Type) it.next();
            String varName = type.getVarName();
            if (type.isTopLevel()) {
                jw.write("protected static final ");
                jw.write(type.getClassName());
                jw.writeEol(" ", varName);
            }
        }

        jw.write("static ");
        jw.begin();
        jw.comment("Create all of the Type's that this stub uses, once.");
        for (Iterator it = types.keySet().iterator(); it.hasNext(); ) {
            Type type = (Type) it.next();
            String varName = type.getVarName();
            if (!type.isTopLevel())
                jw.writeEol(type.getClassName(), " ", varName);
            type.write(jw);
        }
        jw.end();
        jw.cr();
    }

    /**
     * Generate code for the implementation of FaultDetailHandler.
     */
    protected void generateFaultHandler() throws IOException {
        List sortedNames = new ArrayList(faultNames.keySet());
        if (sortedNames.size() == 0)
            return;
        if (faultDetailHandlerIsInnerClass) {
            jw.select(jw.DECL_SECTION);
            jw.writeEol("private MyFaultDetailHandler _myFaultDetailHandler = new MyFaultDetailHandler()");
        }

        jw.select(jw.BODY_SECTION);
        
        if (faultDetailHandlerIsInnerClass) {
            jw.write("private static class MyFaultDetailHandler implements FaultDetailHandler ");
            jw.begin();
            jw.writecr("public MyFaultDetailHandler() {}");
        }
        jw.write("public Element handleFault(QName name) ");
        jw.begin();

        // Make it a stable listing.
        Collections.sort(sortedNames, new QNameComparator());
        for (Iterator it = sortedNames.iterator(); it.hasNext(); ) {
            Object o = it.next();
            QName name = (QName) o;
            Type t = (Type) faultNames.get(name);
            jw.beginIf("\""+name.getLocalPart()+"\".equals(name.getLocalPart()) && \""+name.getNamespaceURI()+"\".equals(name.getNamespaceURI())");
            jw.writeEol("return "+t.getVarName());
            jw.end();
        }
        jw.writeEol("return null");
        jw.end();
        if (faultDetailHandlerIsInnerClass) {
            jw.end();
        }
        jw.cr();
    }
}
