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
import com.sun.xml.rpc.tools.wscompile.CompileTool;
import com.sun.xml.rpc.processor.model.*;
import com.sun.xml.rpc.processor.util.*;
import com.sun.xml.rpc.processor.*;
import com.sun.xml.rpc.processor.util.GeneratedFileInfo;
import com.sun.xml.rpc.tools.wscompile.ActionConstants;
import com.sun.xml.rpc.util.localization.*;

/**
 * We extend the JAX-RPC SI's CompileTool class and replace their
 * generators with ours where needed.
 *
 */
public class CompileTool172 extends CompileTool {
    protected StringBuffer errorMessages = new StringBuffer();
    protected String cldcVersion = "1.0";
    protected boolean expandArguments = true;
    protected LocalizableMessageFactory messageFactory172;
    protected boolean doCompilation = true;
    protected RemoteInterfaceGenerator rig;
    protected StubGenerator sg;

    private Map originalTypes;

    public CompileTool172(OutputStream out, String program) {
        super(out, program);
        originalTypes = new HashMap();
        messageFactory172 = new LocalizableMessageFactory("com.sun.j2mews.sg.wscompile172");

        installReplacementActions();
    }

    protected void installReplacementActions() {
        rig = new RemoteInterfaceGenerator();
        replaceAction(ActionConstants.ACTION_REMOTE_INTERFACE_GENERATOR, rig);

        sg = new StubGenerator();
        sg.setOriginalTypes(originalTypes);
        replaceAction(ActionConstants.ACTION_STUB_GENERATOR, sg);

        // And tell these processors to do nothing
        NullProcessorAction nullProcessor = new NullProcessorAction();
        replaceAction(ActionConstants.ACTION_ENUMERATION_ENCODER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_INTERFACE_SERIALIZER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SOAP_OBJECT_SERIALIZER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SOAP_OBJECT_BUILDER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_LITERAL_OBJECT_SERIALIZER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SOAP_FAULT_SERIALIZER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_FAULT_EXCEPTION_BUILDER_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SERIALIZER_REGISTRY_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SERVICE_GENERATOR, nullProcessor);
        replaceAction(ActionConstants.ACTION_SERVICE_INTERFACE_GENERATOR, nullProcessor);
    }

    /**
     * For example,
     *    replaceAction(ActionConstants.ACTION_STUB_GENERATOR, new StubGenerator());
     */
    public void replaceAction(String name, ProcessorAction pa) {
        actions.put(name, pa);
    }

    /**
     * Add a few parameters/features specific to JSR-172.
     * Remove a few parameters/features that don't apply.
     * Always make sure the wsi feature is on.
     */
    protected boolean parseArguments(String[] args) {
        boolean hasFeatures = false;
        boolean optimize = false;
        boolean showCldc1_0Info = false;
        boolean hasKeep = false;
        for (int i = 0; i < args.length; ++i) {
            String arg = args[i].intern();
            if (arg == "-cldc1.0") {
                cldcVersion = "1.0";
            } else if (arg == "-cldc1.1") {
                cldcVersion = "1.1";
            } else if (arg == "-cldc1.0info") {
                showCldc1_0Info = true;
            } else if (arg == "-define" || arg == "-gen:server" ||
                       arg == "-gen:both") {
                // The server side only features are not supported here.
                onError(get172Message("wscompile172.noServiceSide"));
                return false;
            } else if (args[i].startsWith("-f:") ||
                       args[i].startsWith("-features:")) {
                hasFeatures = true;
                boolean hasWSIFeatureSet = false;
                String featureString = args[i].substring(args[i].startsWith("-f:") ? 3 : 10);
                StringTokenizer tokenizer = new StringTokenizer(featureString, ",");
                String unprocessedFeatures = null;
                while (tokenizer.hasMoreTokens()) {
                    String feature = tokenizer.nextToken().trim().intern();
                    if (feature == "rpcliteral") {
                        onError(get172Message("wscompile172.featureNotSupported", "rpcliteral"));
                        return false;
                    } else if (feature == "wsi") {
                        hasWSIFeatureSet = true;
                    } else if (feature == "donotunwrap") {
                        expandArguments = false;
                        continue;
                    }
                    if (unprocessedFeatures == null)
                        unprocessedFeatures = "-f:";
                    else
                        unprocessedFeatures += ",";
                    unprocessedFeatures += feature;
                }
                if (!hasWSIFeatureSet) {
                    if (unprocessedFeatures == null)
                        unprocessedFeatures = "-f:";
                    unprocessedFeatures += ",wsi";
                    hasWSIFeatureSet = true;
                }
                if (unprocessedFeatures != null)
                    args[i] = unprocessedFeatures;
                continue;
            } else if (arg == "-O") {
                optimize = true;
                continue;
            } else if (arg == "-keep") {
                hasKeep = true;
                continue;
            } else {
                // Skip the resetting of the argument.
                continue;
            }
            args[i] = "";
        }
        if (!hasFeatures) {
            String[] newArgs = new String[args.length+1];
            System.arraycopy(args, 0, newArgs, 0, args.length);
            newArgs[args.length] = "-f:wsi";
            args = newArgs;
        }
        if (!hasKeep) {
            String[] newArgs = new String[args.length+1];
            System.arraycopy(args, 0, newArgs, 0, args.length);
            newArgs[args.length] = "-keep";
            args = newArgs;
        }
        if (cldcVersion.equals("1.0"))
            properties.setProperty(StubGenerator.FLOAT_DOUBLE_TO_STRING, "true");
        else
            properties.setProperty(StubGenerator.FLOAT_DOUBLE_TO_STRING, "false");
        if (showCldc1_0Info)
            properties.setProperty(StubGenerator.SHOW_ALL_CLDC1_0_INFO, "true");
        else
            properties.setProperty(StubGenerator.SHOW_ALL_CLDC1_0_INFO, "false");
        if (expandArguments)
            properties.setProperty(StubGenerator.EXPAND_ARGUMENTS, "true");
        else
            properties.setProperty(StubGenerator.EXPAND_ARGUMENTS, "false");
        if (optimize)
            properties.setProperty(StubGenerator.OPTIMIZE, "true");
        else
            properties.setProperty(StubGenerator.OPTIMIZE, "false");

        return super.parseArguments(args);
    }

    public Localizable getVersion() {
        return get172Message("wscompile172.version", super.getVersion());
    }

    protected void compileGeneratedClasses() {
        if (doCompilation) {
            super.compileGeneratedClasses();
        } else {
            // Do nothing, so as to skip the compilation step.
        }
    }
    
    protected void registerProcessorActions(Processor processor) {
        rig.setLocalizer(localizer);
        rig.setEnvironment(environment);
        sg.setLocalizer(localizer);
        sg.setEnvironment(environment);
        if (cldcVersion.equals("1.0")) {
            MakeCldc1_0 coercer = new MakeCldc1_0(originalTypes);
            coercer.setEnvironment(environment);
            processor.add(coercer);
        }
        super.registerProcessorActions(processor);
    }

    public Map getOriginalTypes() {
        return originalTypes;
    }

    public Iterator getGeneratedFiles() {
        return environment.getGeneratedFiles();
    }

    public Model getCompileToolModel() {
        return (Model) processor.getModel();
    }
    
    public Object getCompileToolEnvironment() {
        return environment;
    }

    public void onError(Localizable msg) {
        String text = localizer.localize(msg);
        errorMessages.append(text);
        errorMessages.append("\n");
        super.onError(msg);
    }

    public String getErrorMessages() {
        return errorMessages.toString();
    }

    protected Localizable get172Message(String key) {
        return messageFactory172.getMessage(key);
    }

    protected Localizable get172Message(String key, String arg) {
        return messageFactory172.getMessage(key, new Object[] {arg});
    }

    protected Localizable get172Message(String key, Localizable l) {
        return messageFactory172.getMessage(key, new Object[] {l});
    }

    protected Localizable get172Message(String key, Object[] args) {
        return messageFactory172.getMessage(key, args);
    }

    protected void usage() {
        report(get172Message("wscompile172.usage", program));
    }
}
