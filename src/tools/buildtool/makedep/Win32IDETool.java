/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package makedep;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import config.Configurator;

/**
 * Win32IDETool creates a cldc_vm.dsw project file for building
 * the VM inside Visual Studio 6.0. To get started:
 * <ul>
 *   <li> Change to the build\win32_i386_ide subdirectory of
 *        this VM source distribution;
 *   <li> Type "idetool create" in the Command Prompt.
 * </ul>
 *
 * Output structure:
 *
 * $(BuildSpace)/cldc_vm.dsw
 *
 * $(BuildSpace)/javaapi/javaapi.dsp
 *                       jcc.zip
 *                       classes.zip
 *                       NativesTable.cpp
 *
 * $(BuildSpace)/loopgen/loopgen.dsp
 *                      Debug/loopgen.exe
 *
 * $(BuildSpace)/loop/loop.dsp
 *
 * $(BuildSpace)/romgen/romgen.dsp
 * $(BuildSpace)/romgen/Release/romgen.exe
 *
 * $(BuildSpace)/romimage/romimage.dsp
 * $(BuildSpace)/romimage/ROMImage.cpp
 *
 * $(BuildSpace)/cldc_vm/cldc_vm.dsp
 * $(BuildSpace)/cldc_vm/Debug/Interpreter_i386.asm
 * $(BuildSpace)/cldc_vm/Debug/OopMaps.cpp
 *                       Debug/cldc_vm.exe
 *                       Release/cldc_vm.exe
 *                       Product/cldc_vm.exe
 */
public class Win32IDETool extends IDETool {
    public static String DEBUG_LOOPGEN = "Debug loop generator";
    public static String DEBUG_ROMGEN  = "Debug ROM generator";

    public static void main(String args[]) {
        Win32IDETool tool = new Win32IDETool();
        try {
            tool.execute(args);
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public int parseOneArgument(String args[], int i)
        throws ArrayIndexOutOfBoundsException
    {
        return super.parseOneArgument(args, i);
    }

    public void execute(String args[]) throws Exception {
        parseArguments(args);
        initConfigurator(); // in IDETool
        setUserSettings(queryUserSettings());

        VC6WorkSpace vc6ws = new VC6WorkSpace(this);

        // (1) API project
        VC6JavaAPIProject apiProject = new VC6JavaAPIProject(this);

        // (2) Loop generator project
        VC6VMProject loopgenProject =
            new VC6VMProject(this, "loopgen", "i386", VC6VMProject.LOOPGEN);
        loopgenProject.addDependency(apiProject);

        // (2) Interpreter Loop project
        VC6LoopProject loopProject = null;
        loopProject = new VC6LoopProject(this);
        loopProject.addDependency(loopgenProject);

        VC6VMProject romgenProject = null;
        VC6ROMImageProject romimageProject = null;

        if (isRomizing()) {
            // (3) ROM generator project
            romgenProject =
                new VC6VMProject(this, "romgen", "i386", VC6VMProject.ROMGEN);
            romgenProject.addDependency(apiProject);
            romgenProject.addDependency(loopgenProject);
            romgenProject.addDependency(loopProject);

            romimageProject = new VC6ROMImageProject(this);
            romimageProject.addDependency(romgenProject);
        }

        // (5) Target project
        VC6VMProject targetProject =
            new VC6VMProject(this, "cldc_vm", "i386", VC6VMProject.TARGET);
        targetProject.addDependency(apiProject);
        targetProject.addDependency(loopgenProject);

        if (isRomizing()) {
            targetProject.addDependency(romimageProject);
        }

        vc6ws.addProject(apiProject);
        vc6ws.addProject(loopgenProject);
        vc6ws.addProject(loopProject);
        if (isRomizing()) {
            vc6ws.addProject(romgenProject);
            vc6ws.addProject(romimageProject);
        }
        vc6ws.addProject(targetProject);

        vc6ws.write();
        System.exit(0);
    }

    Vector getBuildOptions() {
        Vector v = new Vector();
        v.addElement("ROMIZING");
        Vector enableFlags = configurator.getFlagNames();
        for (int i=0; i<enableFlags.size(); i++) {
            v.addElement((String)enableFlags.elementAt(i));
        }

        return v;
    }

    Vector getGeneratorOptions() {
        Vector v = new Vector();
        v.addElement(DEBUG_LOOPGEN);
        v.addElement(DEBUG_ROMGEN);

        return v;
    }

    Vector getPaths() {
        Vector v = new Vector();
        v.addElement("javac.exe");
        v.addElement("java.exe");
        v.addElement("jar.exe");
        v.addElement("ml.exe");
        v.addElement("preverify.exe");

        return v;
    }

    /**
     * Read user settings from $BuildSpace/prefs.props, then pop up
     * a dialog to ask for user modification
     */
    Properties queryUserSettings() throws Exception {
        Properties props = new Properties();

        // Put default settings
        Vector v = getBuildOptions();
        for (int i=0; i<v.size(); i++) {
            String name = (String)v.elementAt(i);
            if (name.equals("ROMIZING")) {
                props.put(v.elementAt(i), "true");
            } else {
                props.put(v.elementAt(i), "default");
            }
        }

        props.put(DEBUG_LOOPGEN, "true");
        props.put(DEBUG_ROMGEN,  "false");

        String workspace = getWorkSpaceArg();
        String preverify = workspace + "/build/share/bin/win32_i386/" +
                           "preverify.exe";
        preverify = preverify.replace('/', File.separatorChar);

        props.put("javac.exe",     "");
        props.put("java.exe",      "");
        props.put("jar.exe",       "");
        props.put("ml.exe",        "");
        props.put("preverify.exe", preverify);

        String prefsFile = getOutputFileFullPath("prefs.props");
        try {
            // Load from saved values
            FileInputStream in = new FileInputStream(prefsFile);
            props.load(in);
            in.close();
        } catch (IOException e) {
            // ignore it
        }

        Win32IDESettingsDialog dialog = new Win32IDESettingsDialog();


        // The following call blocks until the user has finished with
        // dialog.
        dialog.getUserInput(props, getBuildOptions(),
                            getGeneratorOptions(),
                            getPaths());

        try {
            FileOutputStream out = new FileOutputStream(prefsFile);
            props.store(out, "");
            out.close();
        } catch (IOException e) {
            // ignore it
        }

        return props;
    }

    public String getExecutablePath(String name) {
        Object path = getUserSettings().get(name);
        if (path != null && (path instanceof String)) {
            String result = getAbsolutePath((String)path);
            //System.out.println(name + " = " + result);
            return result;
        } else {
            return name;
        }
    }

    public boolean isRomizing() {
        return isOptionEnabled("ROMIZING");
    }
}

abstract class IDEOutputFile {
    private IDETool tool;
    private PrintWriter outFile;

    public IDEOutputFile(IDETool tool) {
        this.tool = tool;
    }

    public IDETool tool() {
        return this.tool;
    }

    public void putln() {
        putln("");
    }
    public void putln(String s) {
        outFile.println(s);
    }
    public void put(String s) {
        outFile.print(s);
    }

    public void openOutputFile(String fileName)
        throws Exception
    {
        outFile = tool().openOutputFile(fileName);
    }

    public void closeOutputFile() throws Exception {
        outFile.close();
    }
}

/**
 * Create a workspace for VC6. A workspace contains multiple projects.
 */
class VC6WorkSpace extends IDEOutputFile {
    private Vector projects;

    public VC6WorkSpace(IDETool tool) {
        super(tool);
        this.projects = new Vector();
    }
    public void addProject(IDEProject proj) {
        projects.addElement(proj);
    }

    /**
     * Write all files associated with this workspace, including the .dsw
     * file, .dsp files for all sub-projects, and all generated files.
     */
    public void write() throws Exception {
        writeWorkspaceFile();
        writeProjectFiles();
    }

    void writeProjectFiles() throws Exception {
        for (int i=0; i<projects.size(); i++) {
            IDEProject proj = (IDEProject)projects.elementAt(i);
            proj.write();
        }
    }

    /**
     * Write the .dsw file for this workspace
     */
    void writeWorkspaceFile() throws Exception {
        openOutputFile("cldc_vm.dsw");
        writeHeader();
        writeProjects();
        writeFooter();
        closeOutputFile();
    }

    void writeHeader() {
        putln("Microsoft Developer Studio Workspace File, " +
              "Format Version 6.00");
        putln("# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!");
        writeSeparator();
    }

    void writeSeparator() {
        putln();
        putln("####################################################" +
              "###########################");
        putln();
        putln();
    }

    void writeProjects() throws Exception {
        for (int i=0; i<projects.size(); i++) {
            IDEProject proj = (IDEProject)projects.elementAt(i);
            writeProject(proj);
        }
    }

    void writeProject(IDEProject proj) throws Exception {
        put("Project: \"" + proj.getName() + "\"=.\\");
        put(proj.getOutputDir() + "\\" + proj.getName() + ".dsp - ");
        put("Package Owner=<4>");
        putln();
        putln();
        putln("Package=<5>");
        putln("{{{");
        putln("}}}");
        putln();
        putln("Package=<4>");
        putln("{{{");

        Vector dependencies = proj.getDependencies();
        for (int i=0; i<dependencies.size(); i++) {
            IDEProject other = (IDEProject)dependencies.elementAt(i);
            putln("    Begin Project Dependency");
            putln("    Project_Dep_Name " + other.getName());
            putln("    End Project Dependency");
        }
        putln("}}}");
        writeSeparator();
    }

    void writeFooter() {
        putln("Global:");
        putln();
        putln("Package=<5>");
        putln("{{{");
        putln("}}}");
        putln("");
        putln("Package=<3>");
        putln("{{{");
        putln("}}}");
        writeSeparator();
    }
}

abstract class IDEProject extends IDEOutputFile {
    String name;
    String outputDir;

    /**
     * Other projects that this project depend on.
     */
    Vector dependencies;

    public IDEProject(IDETool tool, String name) {
        super(tool);
        this.name = name;
        dependencies = new Vector();
    }

    public String getName() {
        return name;
    }
    void setOutputDir(String dir) {
        outputDir = dir;
    }
    public String getOutputDir() {
        if (outputDir != null) {
            return outputDir;
        } else {
            return name;
        }
    }

    public abstract void write() throws Exception;

    public void openOutputFile()
        throws Exception
    {
        tool().makeDirExist(getOutputDir());
        openOutputFile(getOutputDir() + File.separator + name + ".dsp");
    }

    public String getRelativeSourceFile(String s) {
        File file = new File(s);
        if (file.isAbsolute()) {
            return s;
        }
        if (tool().isDefaultBuildspace()) {
            // IMPL_NOTE: use the right number of ".."'s
            return "..\\" + s;
        } else {
            return tool().getAbsolutePath(s);
        }
    }

    public void addDependency(IDEProject dependency) {
        dependencies.addElement(dependency);
    }

    public Vector getDependencies() {
        return dependencies;
    }
}

class IDEProjectConfig {
    /* E.g.
     * "makeproj - Win32 Release" (based on "Win32 (x86) External Target")
     */
    public String name;      /* E.g., "makeproj - Win32 Release" */
    public String base;      /* E.g., "Win32 (x86) External Target" */
    public String outputDir; /* E.g., "Release" */

    public boolean isDebug() {
        return (name.indexOf("Debug") >= 0);
    }
    public boolean isRelease() {
        return (name.indexOf("Release") >= 0);
    }
    public boolean isProduct() {
        return (name.indexOf("Product") >= 0);
    }
}

abstract class NMakefile extends IDEOutputFile {
    String fileName;
    public NMakefile(IDETool tool, String fileName) {
        super(tool);
        this.fileName = fileName;
    }

    /**
     * Convenience function for accessing Win32IDETool
     */
    Win32IDETool win32Tool() {
        return (Win32IDETool)tool();
    }

    public void write() throws Exception {
        openOutputFile(fileName);
        writeContents();
        closeOutputFile();
    }
    public void puttab() {
        put("\t");
    }
    public void puttabln(String s) {
        putln("\t" + s);
    }

    abstract void writeContents() throws Exception;
}

abstract class VC6Project extends IDEProject {
    public VC6Project(IDETool tool, String name) {
        super(tool, name);
    }

    /**
     * Convenience function for accessing Win32IDETool
     */
    Win32IDETool win32Tool() {
        return (Win32IDETool)tool();
    }

    abstract public String getTargetTypeString();
    abstract public Vector getAvailableConfigs();

    public void addConfig(Vector v, String configName, String outputDir,
                          String baseName) {
        IDEProjectConfig config = new IDEProjectConfig();
        config.name = getName() + " - " + configName;
        config.base = baseName;
        config.outputDir = outputDir;
        v.addElement(config);
    }

    public String getDefaultConfigName() {
        Vector v = getAvailableConfigs();
        IDEProjectConfig config = (IDEProjectConfig)v.elementAt(0);
        return config.name;
    }

    public void writeHeader() {
        putln("# Microsoft Developer Studio Project File - " +
              "Name=\"" + getName() + "\" - Package Owner=<4>");
        putln("# Microsoft Developer Studio Generated Build File, " +
              "Format Version 6.00");
        putln("# ** DO NOT EDIT **");
        putln();
        putln("# TARGTYPE " + getTargetTypeString());
        putln();
        putln("CFG=" + getDefaultConfigName());
    }

    void nmakemsg(String s) {
        putln("!MESSAGE " + s);
    }

    void writeNMakeMessage() {
        nmakemsg("This is not a valid makefile. " +
                 "To build this project using NMAKE,");
        nmakemsg("use the Export Makefile command and run");
        nmakemsg("");
        nmakemsg("NMAKE /f \"" + getName() +  ".mak\".");
        nmakemsg("");
        nmakemsg("You can specify a configuration when running NMAKE");
        nmakemsg("by defining the macro CFG on the command line. " +
                 "For example:");
        nmakemsg("");
        nmakemsg("NMAKE /f \"" + getName() + ".mak\" CFG=\"" +
                 getDefaultConfigName() + "\"");
        nmakemsg("");
        nmakemsg("Possible choices for configuration are:");
        nmakemsg("");

        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            nmakemsg("\"" + config.name + "\" (based on \"" + config.base +
                     "\")");
        }
        nmakemsg("");
        putln();
    }
}

abstract class VC6NMakeProject extends VC6Project {
    abstract public NMakefile getNMakefile(IDETool tool, String fileName);
    abstract public String getTargetFileName();

    public VC6NMakeProject(IDETool tool, String name) {
        super(tool, name);
    }

    public void write() throws Exception {
        openOutputFile();
        writeHeader();
        writeNMakeMessage();
        writeConfigs();
        writeTarget();
        closeOutputFile();

        String makName = getOutputDir() + File.separator + getName() + ".mak";
        NMakefile nmakefile = getNMakefile(tool(), makName);

        nmakefile.write();
    }

    void writeConfigs() {
        putln("# Begin Project");
        putln("# PROP AllowPerConfigDependencies 0");
        putln("# PROP Scc_ProjName \"\"");
        putln("# PROP Scc_LocalPath \"\"");

        String ifstr = "!IF";
        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            putln();
            putln(ifstr + "  \"$(CFG)\" == \"" + config.name + "\"");
            putln();

            String outdir = config.outputDir;
            int debuglib = 0;
            if (outdir.equals("Debug")) {
                debuglib = 1;
            }

            putln("# PROP BASE Use_MFC 0");
            putln("# PROP BASE Use_Debug_Libraries " + debuglib);
            putln("# PROP BASE Output_Dir \"" + outdir + "\"");
            putln("# PROP BASE Intermediate_Dir \"" + outdir + "\"");
            putln("# PROP BASE Cmd_Line \"NMAKE /nologo /f \""+
                  getName()+ ".mak\"\"");
            putln("# PROP BASE Rebuild_Opt \"/a\"");
            putln("# PROP BASE Target_File \"" + getTargetFileName() + "\"");
            putln("# PROP BASE Target_Dir \"\"");
            putln("# PROP Use_MFC 0");
            putln("# PROP Use_Debug_Libraries " + debuglib);
            putln("# PROP Output_Dir \"" + outdir + "\"");
            putln("# PROP Intermediate_Dir \"" + outdir + "\"");
            putln("# PROP Cmd_Line \"NMAKE /nologo /f \"" +
                  getName() + ".mak\"\"");
            putln("# PROP Rebuild_Opt \"/a\"");
            putln("# PROP Target_File \"" + getTargetFileName() + "\"");
            putln("# PROP Target_Dir \"\"");

            ifstr = "!ELSEIF";
        }

        putln();
        putln("!ENDIF");
        putln();
    }

    void writeTarget() {
        putln("# Begin Target");
        putln("");

        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            putln("# Name \"" + config.name + "\"");
        }

        putln();

        String ifstr = "!IF";
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            putln(ifstr + "  \"$(CFG)\" == \"" + config.name + "\"");
            putln("");

            ifstr = "!ELSEIF";
        }

        putln("!ENDIF ");
        putln();
        putln("# Begin Group \"Source Files\"");
        putln();
        putln("# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"");
        writeSourceFiles(tool().getAPISources());
        putln("# End Group");
        putln("# Begin Group \"Header Files\"");
        putln();
        putln("# PROP Default_Filter \"h;hpp;hxx;hm;inl\"");
        putln("# End Group");
        putln("# Begin Group \"Resource Files\"");
        putln();
        putln("# PROP Default_Filter " +
              "\"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"");
        putln("# End Group");
        putln("# End Target");
        putln("# End Project");
    }

    void writeSourceFiles(Vector v) {
        for (int i=0; i<v.size(); i++) {
            putln("# Begin Source File");
            putln();
            putln("SOURCE=" + getRelativeSourceFile((String)v.elementAt(i)));
            putln("# End Source File");
        }
    }
}

class VC6JavaAPIProject extends VC6NMakeProject {
    public VC6JavaAPIProject(IDETool tool) {
        super(tool, "javaapi");
    }

    public String getTargetTypeString() {
        return "\"Win32 (x86) External Target\" 0x0106";
    }

    public Vector getAvailableConfigs() {
        Vector v = new Vector();
        String base = "Win32 (x86) External Target";
        addConfig(v, "Win32 Release", "Release", base);
        return v;
    }

    public String getTargetFileName() {
        return "NativesTable.cpp";
    }

    public NMakefile getNMakefile(IDETool tool, String fileName) {
        return new VC6JavaAPINMakefile(tool, fileName, this);
    }
}

class VC6JavaAPINMakefile extends NMakefile {
    private IDEProject proj;

    public VC6JavaAPINMakefile(IDETool tool, String fileName, IDEProject proj)
    {
        super(tool, fileName);
        this.proj = proj;
    }

    public void writeContents() {
        writeHeader();
        writeJCC();
        writeClassesZip();
        writeNativesTable();
        writeClean();
    }

    void writeHeader() {
        putln("# This file is auto-generated. DO NOT EDIT");
        putln();
        putln("all: NativesTable.cpp");
        putln();
        putln("JAVAC     = " + tool().getExecutablePath("javac.exe"));
        putln("JAR       = " + tool().getExecutablePath("jar.exe"));
        putln("JAVA      = " + tool().getExecutablePath("java.exe"));
        putln("PREVERIFY = " + tool().getExecutablePath("preverify.exe"));
        putln();
        putln();
    }

    void writeJCC() {
        // JCC_SOURCES
        Vector v = tool().getJccSources();
        put("JCC_SOURCES =");
        for (int i=0; i<v.size(); i++) {
            putln(" \\");
            put("  " + proj.getRelativeSourceFile((String)v.elementAt(i)));
        }
        putln();
        putln();

        // jcc
        putln("jcc:");
        puttab();
        putln("-@mkdir jcc > NUL 2> NUL");
        putln();

        // jcc.jar
        putln("jcc.jar: jcc $(JCC_SOURCES)");
        puttabln("@echo Creating JCC tool");
        puttabln("@\"$(JAVAC)\" -d jcc @<<");
        putln("$(JCC_SOURCES)");
        putln("<<");
        puttabln("@\"$(JAR)\" -cf $@ -C jcc .");
        putln();
    }

    void writeClassesZip() {
        String api_ver;
        if (tool().isOptionEnabled("ENABLE_REFLECTION")) {
            api_ver = "CLDC 1.1 plus";
        } else if (tool().isOptionEnabled("ENABLE_CLDC_11")) {
            api_ver = "CLDC 1.1";
        } else {
            api_ver = "CLDC 1.0";
        }

        // API_SOURCES
        Vector v = tool().getAPISources();
        put("API_SOURCES =");
        for (int i=0; i<v.size(); i++) {
            putln(" \\");
            put("  " + proj.getRelativeSourceFile((String)v.elementAt(i)));
        }
        putln();
        putln();

        // tmpclasses
        putln("tmpclasses:");
        puttabln("-@mkdir tmpclasses > NUL 2> NUL");
        putln();

        // classes
        putln("classes:");
        puttabln("-@mkdir classes > NUL 2> NUL");
        putln();


        putln("classes.zip: tmpclasses classes $(API_SOURCES)");
        puttabln("@echo Compiling " +  api_ver + " API classes");
        puttabln("-@rmdir /Q /S tmpclasses > NUL 2> NUL");
        puttabln("-@rmdir /Q /S classes > NUL 2> NUL");
        puttabln("-@mkdir tmpclasses > NUL 2> NUL");
        puttabln("-@mkdir classes > NUL 2> NUL");
        puttabln("@\"$(JAVAC)\" -d tmpclasses @<<");
        putln("$(API_SOURCES)");
        putln("<<");
        puttabln("@echo Preverifying classes");
        puttabln("@\"$(PREVERIFY)\" -classpath classes -d classes tmpclasses");
        puttabln("@echo Creating $@");
        puttabln("@\"$(JAR)\" -cf $@ -C classes .");
        putln();
    }

    void writeNativesTable() {
        putln("NativesTable.cpp: classes.zip jcc.jar");
        puttabln("@echo Creating $@");
        puttabln("@\"$(JAVA)\" -cp jcc.jar JavaCodeCompact " +
                 "-writer CLDC_HI_Natives -o $@ classes.zip");
        putln();
    }

    void writeClean() {
        putln("clean:");
        puttabln("@echo Cleaning output files");
        puttabln("-@rmdir /Q /S jcc          > NUL 2> NUL");
        puttabln("-@rmdir /Q /S tmpclasses   > NUL 2> NUL");
        puttabln("-@rmdir /Q /S classes      > NUL 2> NUL");
        puttabln("-@del /Q jcc.jar           > NUL 2> NUL");
        puttabln("-@del /Q classes.zip       > NUL 2> NUL");
        puttabln("-@del /Q NativesTable.cpp  > NUL 2> NUL");
    }
}

class VC6ROMImageProject extends VC6NMakeProject {
    public VC6ROMImageProject(IDETool tool) {
        super(tool, "romimage");
    }

    public String getTargetTypeString() {
        return "\"Win32 (x86) External Target\" 0x0106";
    }

    public Vector getAvailableConfigs() {
        Vector v = new Vector();
        String base = "Win32 (x86) External Target";
        addConfig(v, "Win32 Release", "Release", base);
        return v;
    }

    public String getTargetFileName() {
        return "ROMImage.cpp";
    }

    public NMakefile getNMakefile(IDETool tool, String fileName) {
        return new VC6ROMImageNMakefile(tool, fileName, this);
    }
}


class VC6ROMImageNMakefile extends NMakefile {
    private IDEProject proj;

    public VC6ROMImageNMakefile(IDETool tool, String fileName, IDEProject proj)
    {
        super(tool, fileName);
        this.proj = proj;
    }

    public void writeContents() {
        writeHeader();
        writeROMImageRule();
        writeClean();
    }

    void writeHeader() {
        putln("# This file is auto-generated. DO NOT EDIT");
        putln();
        putln("all: ROMImage.cpp");
        putln();

        if (win32Tool().isOptionEnabled(win32Tool().DEBUG_ROMGEN)) {
            putln("ROMGEN      = ..\\romgen\\Debug\\romgen.exe");
        } else {
            putln("ROMGEN      = ..\\romgen\\Release\\romgen.exe");
        }
        putln("CLASSES_ZIP = ..\\javaapi\\classes.zip");
        putln();
        putln();
    }

    void writeROMImageRule() {
        String workspace = tool().getWorkSpaceArg();
        String romconfig = workspace + File.separator +
                           "src\\vm\\cldcx_rom.cfg";
        romconfig = tool().getAbsolutePath(romconfig);
        String rompath = workspace + File.separator +
                           "src\\vm";
        rompath = tool().getAbsolutePath(rompath);

        putln("ROMImage.cpp: $(ROMGEN) $(CLASSES_ZIP)");
        puttabln("@echo Creating $@");
        puttabln("\"$(ROMGEN)\" -cp \"$(CLASSES_ZIP)\" \\");
        puttabln("    =HeapCapacity8M \\");
        puttabln("    -romconfig \"" + romconfig + "\" \\");
        puttabln("    -romincludepath \"" + rompath + "\" \\");
        puttabln("    +EnableAllROMOptimizations \\");
        puttabln("    -romize \\");
        puttabln("    +GenerateROMStructs");
        putln();
    }

    void writeClean() {
        putln("clean:");
        puttabln("@echo Cleaning output files");
        puttabln("-@del /Q ROMImage.cpp  > NUL 2> NUL");
        puttabln("-@del /Q ROMLog.txt    > NUL 2> NUL");
        puttabln("-@del /Q ROMStructs.h  > NUL 2> NUL");

    }
}

class VC6LoopProject extends VC6NMakeProject {
    public VC6LoopProject(IDETool tool) {
        super(tool, "loop");
    }

    public String getTargetTypeString() {
        return "\"Win32 (x86) External Target\" 0x0106";
    }

    public Vector getAvailableConfigs() {
        Vector v = new Vector();
        String base = "Win32 (x86) External Target";
        addConfig(v, "Win32 Release", "Release", base);
        return v;
    }

    public String getTargetFileName() {
        return "Dummy.cpp";
    }

    public NMakefile getNMakefile(IDETool tool, String fileName) {
        return new VC6LoopNMakefile(tool, fileName, this);
    }
}

class VC6LoopNMakefile extends NMakefile {
    private IDEProject proj;

    public VC6LoopNMakefile(IDETool tool, String fileName, IDEProject proj)
    {
        super(tool, fileName);
        this.proj = proj;
    }

    public void writeContents() {
        writeHeader();
        writeLoopRule();
        writeClean();
    }

    void writeHeader() {
        putln("# This file is auto-generated. DO NOT EDIT");
        putln();
        putln("all: Dummy.cpp");
        putln();

        if (win32Tool().isOptionEnabled(win32Tool().DEBUG_LOOPGEN)) {
            putln("LOOPGEN      = ..\\loopgen\\Debug\\loopgen.exe");
        } else {
            putln("LOOPGEN      = ..\\loopgen\\Release\\loopgen.exe");
        }
        putln();
        putln();
    }

    void writeLoopRule() {
        putln("Dummy.cpp: $(LOOPGEN)");
        puttabln("@echo Creating Interpreter Loops");

        puttabln("$(LOOPGEN) -generate +GenerateDebugAssembly " +
                 "-outputdir ..\\cldc_vm\\Debug");
        puttabln("$(LOOPGEN) +GenerateOopMaps " +
                 "-outputdir ..\\cldc_vm\\Debug");

        puttabln("$(LOOPGEN) -generateoptimized " +
                 "-outputdir ..\\cldc_vm\\Release");
        puttabln("$(LOOPGEN) +GenerateOopMaps " +
                 "-outputdir ..\\cldc_vm\\Release");

        puttabln("$(LOOPGEN) -generateoptimized " +
                 "-outputdir ..\\cldc_vm\\Product");
        puttabln("$(LOOPGEN) +GenerateOopMaps " +
                 "-outputdir ..\\cldc_vm\\Product");

        if (win32Tool().isRomizing()) {
            if (win32Tool().isOptionEnabled(win32Tool().DEBUG_ROMGEN)) {
                puttabln("$(LOOPGEN) -generate +GenerateDebugAssembly " +
                         "-outputdir ..\\romgen\\Debug");
                puttabln("$(LOOPGEN) +GenerateOopMaps " +
                         "-outputdir ..\\romgen\\Debug");
            } else {
                puttabln("$(LOOPGEN) -generateoptimized " +
                         "-outputdir ..\\romgen\\Release");
                puttabln("$(LOOPGEN) +GenerateOopMaps " +
                         "-outputdir ..\\romgen\\Release");
            }
       }

        putln();
    }

    void writeClean() {
        String junk = " > NUL 2> NUL";
        putln("clean:");
        puttabln("@echo Cleaning output files");
        puttabln("-@del /Q Dummy.cpp" + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Debug\\Interpreter_i386.asm"   + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Release\\Interpreter_i386.asm" + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Product\\Interpreter_i386.asm" + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Debug\\OopMaps.cpp"   + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Release\\OopMaps.cpp" + junk);
        puttabln("-@del /Q ..\\cldc_vm\\Product\\OopMaps.cpp" + junk);

        if (win32Tool().isRomizing()) {
            if (win32Tool().isOptionEnabled(win32Tool().DEBUG_ROMGEN)) {
                puttabln("-@del /Q ..\\romgen\\Debug\\Interpreter_i386.asm"
                         + junk);
                puttabln("-@del /Q ..\\romgen\\Debug\\OopMaps.cpp"
                         + junk);
            } else {
                puttabln("-@del /Q ..\\romgen\\Release\\Interpreter_i386.asm"
                         + junk);
                puttabln("-@del /Q ..\\romgen\\Release\\OopMaps.cpp"
                         + junk);
            }
        }

    }
}

class VMSourceHandler {
    private String arch;
    private String os;
    private IDETool tool;
    private Vector vpaths;
    private IDEProject proj;

    public VMSourceHandler(IDETool tool, IDEProject proj, String arch,
                           String os)
    {
        this.tool = tool;
        this.proj = proj;
        this.arch = arch;
        this.os   = os;

        vpaths = new Vector();
        addVpath("/src/vm/cpu/" + arch);
        addVpath("/src/vm/os/" + os);
        addVpath("/src/vm/share/compiler");
        addVpath("/src/vm/share/handles");
        addVpath("/src/vm/share/interpreter");
        addVpath("/src/vm/share/memory");
        addVpath("/src/vm/share/natives");
        addVpath("/src/vm/share/ROM");
        addVpath("/src/vm/share/reflection");
        addVpath("/src/vm/share/runtime");
        addVpath("/src/vm/share/utilities");
        addVpath("/src/vm/share/verifier");
        addVpath("/src/vm/share/debugger");
        addVpath("/src/vm/share/float");
        addVpath("/src/vm/share/isolate");
        addVpath("/src/vm/share/dynupdate");
        addVpath("/src/vm/share/memoryprofiler");
    }

    public String resolveFileForProject(String fileName) {

        for (int i=0; i<vpaths.size(); i++) {
            String s = (String)vpaths.elementAt(i) + File.separator + fileName;
            File f = new File(s);
            if (f.exists()) {
                return proj.getRelativeSourceFile(s);
            }
        }

        throw new Error("Source file not found: " + fileName);
    }

    public Vector getVpaths() {
        return vpaths;
    }

    private void addVpath(String vpath) {
        String workspace = tool.getWorkSpaceArg();
        vpath = workspace + vpath;
        vpath = vpath.replace('/', File.separatorChar);
        vpaths.addElement(vpath);
    }
}

class VC6VMProject extends VC6Project {
    Database database;
    VMSourceHandler vmSourceHandler;
    int type;

    public final static int LOOPGEN = 1;
    public final static int ROMGEN  = 2;
    public final static int TARGET  = 3;

    /**
     * @param type must be LOOPGEN, ROMGEN or TARGET
     */
    public VC6VMProject(IDETool tool, String name, String arch, int type) {
        super(tool, name);
        vmSourceHandler = new VMSourceHandler(tool, this, arch, "win32");
        this.type = type;
    }

    public String getTargetTypeString() {
        return "\"Win32 (x86) Console Application\" 0x0103";
    }

    public Vector getAvailableConfigs() {
        Vector v = new Vector();
        String base = "Win32 (x86) Console Application";
        switch (this.type) {
        case TARGET:
            addConfig(v, "Win32 Debug", "Debug", base);
            addConfig(v, "Win32 Release", "Release", base);
            addConfig(v, "Win32 Product", "Product", base);
            break;
        case LOOPGEN:
            if (win32Tool().isOptionEnabled(win32Tool().DEBUG_LOOPGEN)) {
                addConfig(v, "Win32 Debug", "Debug", base);
            } else {
                addConfig(v, "Win32 Release", "Release", base);
            }
            break;
        case ROMGEN:
            if (win32Tool().isOptionEnabled(win32Tool().DEBUG_ROMGEN)) {
                addConfig(v, "Win32 Debug", "Debug", base);
            } else {
                addConfig(v, "Win32 Release", "Release", base);
            }
            break;
        }
        return v;
    }

    void openDatabase()  throws Exception {
        Platform platform = new WinGammaPlatform();
        platform.setupFileTemplates();
        long t = platform.defaultGrandIncludeThreshold();

        database = new Database(platform, t);

        tool().makeDirExist(getOutputDir());
        tool().makeDirExist(getOutputDir() + File.separator + "incls");
        database.setOutputDir(tool().getOutputFileFullPath(getOutputDir()));

        Properties globalProps = new Properties();
        if (tool().isOptionEnabled("ENABLE_JAVA_DEBUGGER")) {
            globalProps.put("ENABLE_JAVA_DEBUGGER", "true");
        }
        if (tool().isOptionEnabled("ENABLE_ROM_JAVA_DEBUGGER")) {
            globalProps.put("ENABLE_ROM_JAVA_DEBUGGER", "true");
        }
        if (tool().isOptionEnabled("ENABLE_ISOLATES")) {
            globalProps.put("ENABLE_ISOLATES", "true");
        }
        if (tool().isOptionEnabled("ENABLE_REFLECTION")) {
            globalProps.put("ENABLE_REFLECTION", "true");
        }
        if (this.type == TARGET) {
            if (tool().isOptionEnabled("ENABLE_MONET")) {
                globalProps.put("ENABLE_MONET", "true");
            }
        }
        if (tool().isOptionEnabled("ENABLE_METHOD_TRAPS")) {
            globalProps.put("ENABLE_METHOD_TRAPS", "true");
        }
        if (tool().isOptionEnabled("ENABLE_DYNUPDATE")) {
            globalProps.put("ENABLE_DYNUPDATE", "true");
        }
        globalProps.put("ENABLE_GENERATE_SOURCE_ROM_IMAGE", "true");
        globalProps.put("ENABLE_INTERPRETER_GENERATOR", "true");

        database.get(tool().getPlatformArg(), tool().getDatabaseArg(),
                     globalProps);
        database.compute();
    }

    public void write() throws Exception {
        openDatabase();

        openOutputFile();
        writeHeader();
        writeNMakeMessage();
        writeProjectBlock();
        writeSourceFileRules();
        writeHeaderFileRules();
        writeFooter();
        closeOutputFile();
        writeIncls();
        writeJvmConfig();
    }

    public void writeProjectBlock() {
        putln("# Begin Project");
        putln("# PROP AllowPerConfigDependencies 0");
        putln("# PROP Scc_ProjName \"\"");
        putln("# PROP Scc_LocalPath \"\"");
        putln("CPP=cl.exe");
        putln("MTL=midl.exe");
        putln("RSC=rc.exe");

        String ifstr = "!IF";
        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            putln();
            putln(ifstr + "  \"$(CFG)\" == \"" + config.name + "\"");
            putln();

            String outdir = config.outputDir;
            int debuglib = 0;
            if (outdir.equals("Debug")) {
                debuglib = 1;
            }

            putln("# PROP BASE Use_MFC 0");
            putln("# PROP BASE Use_Debug_Libraries " + debuglib);
            putln("# PROP BASE Output_Dir \"" + outdir + "\"");
            putln("# PROP BASE Intermediate_Dir \"" + outdir + "\"");
            putln("# PROP BASE Target_Dir \"\"");
            putln("# PROP Use_MFC 0");
            putln("# PROP Use_Debug_Libraries " + debuglib);
            putln("# PROP Output_Dir \"" + outdir + "\"");
            putln("# PROP Intermediate_Dir \"" + outdir + "\"");
            putln("# PROP Ignore_Export_Lib 0");
            putln("# PROP Target_Dir \"\"");

            put("# ADD BASE CPP ");
            putBaseCppFlags(config);
            putln("/YX /FR /FD /c");

            put("# ADD CPP /I \".\" ");
            putIncludePaths();
            putCppFlags(config, true);
            putln("/c");

            putln("# ADD BASE RSC /l 0x406 /d \"NDEBUG\"");
            putln("# ADD RSC /l 0x406 /d \"NDEBUG\"");
            putln("BSC32=bscmake.exe");
            putln("# ADD BASE BSC32 /nologo");
            putln("# ADD BSC32 /nologo");
            putln("LINK32=link.exe");

            String linkLine = "LINK32 gdi32.lib user32.lib wsock32.lib " +
                              "/nologo /subsystem:console /incremental:no " +
                              "/machine:I386";

            if (outdir.equals("Debug")) {
                linkLine += " /debug";
            }

            putln("# ADD BASE " + linkLine);
            putln("# ADD " + linkLine);

            ifstr = "!ELSEIF";
        }
        putln();
        putln("!ENDIF");
        putln();

        putln("# Begin Target");
        putln();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            putln("# Name \"" + config.name + "\"");
        }
    }

    void putBaseCppFlags(IDEProjectConfig config) {
        put("/nologo ");

        if (config.isDebug()) {
            put("/W3 /Gm /Zi /Od ");
        } else if (config.isRelease()) {
            put("/W3 /O2 /Zi /Od ");
        } else {
            put("/W3 /O2 ");
        }

        Vector v = getBaseCppDefines(config);
        for (int i=0; i<v.size(); i++) {
            put("/D \"" + v.elementAt(i) + "\" ");
        }
    }

    void putCppFlags(IDEProjectConfig config, boolean usePCH) {
        putBaseCppFlags(config);

        Vector v = getCppDefines(config);
        for (int i=0; i<v.size(); i++) {
            put("/D \"" + v.elementAt(i) + "\" ");
        }
        if (usePCH) {
            put("/FR /Yu\"incls/_precompiled.incl\" /FD ");
        }
    }

    Vector getBaseCppDefines(IDEProjectConfig config) {
        Vector v = new Vector();
        v.addElement("WIN32");

        if (config.isDebug()) {
            v.addElement("DEBUG");
            v.addElement("AZZERT");
        } else if (config.isRelease()) {
            v.addElement("NDEBUG");
        } else {
            v.addElement("PRODUCT");
            v.addElement("NDEBUG");
        }

        return v;
    }

    Vector getCppDefines(IDEProjectConfig config) {
        Vector v = new Vector();

        if (this.type == TARGET && win32Tool().isRomizing()) {
            v.addElement("ROMIZING");
        }
        return v;
    }

    void putIncludePaths() {
        Vector vpaths = vmSourceHandler.getVpaths();
        for (int i=0; i<vpaths.size(); i++) {
            String path = (String)vpaths.elementAt(i);
            put("/I \"" + getRelativeSourceFile(path) + "\" ");
        }
    }

    void writeSourceFileRules() throws Exception  {
        Vector sourceFiles = new Vector();

        for (Iterator iter = database.getAllFiles().iterator();
             iter.hasNext(); ) {
            FileList fl = (FileList) iter.next();
            String fileName = fl.getName();
            if (isFileIncludedInSources(fileName)) {
                fileName = vmSourceHandler.resolveFileForProject(fileName);
                sourceFiles.addElement(fileName);
            }
        }

        // Add extra source specific to this VM. For example, a
        // loopgen VM would add InterpreterSkeleton.cpp
        addExtraSources(sourceFiles);

        boolean added_pch = false;
        putln("# Begin Group \"Source Files\"");
        putln();
        putln("# PROP Default_Filter " +
              "\"cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90;asm\"");
        for (int i=0; i<sourceFiles.size(); i++) {
            String fileName = (String)sourceFiles.elementAt(i);

            putln("# Begin Source File");
            putln();
            putln("SOURCE=" + fileName);

            if (!mayUsePrecompiledHeader(fileName)) {
                putln("#  SUBTRACT CPP /YX /Yc /Yu");
            } else {
                if (!added_pch) {
                    putln("#  ADD CPP /Yc\"incls/_precompiled.incl\"");
                    added_pch = true;
                }
            }
            putln("# End Source File");
        }

        // Write the asm files as well
        switch (this.type) {
        case TARGET:
        case ROMGEN:
            writeAsmAndOopMapsFileRules();
            break;
        }

        putln("# End Group");
    }

    void writeAsmAndOopMapsFileRules() throws Exception {
        writeAsmAndOopMapsFileRules(true);
        writeAsmAndOopMapsFileRules(false);

        createDummyAsmAndOopMapsFiles(true);
        createDummyAsmAndOopMapsFiles(false);
    }

    void writeAsmAndOopMapsFileRules(boolean isAsm) {
        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);
            String srcName, stem, ext;

            if (isAsm) {
                stem = "Interpreter_i386";
                ext = ".asm";
            } else {
                stem = "OopMaps";
                ext = ".cpp";
            }
            srcName = config.outputDir + "\\" + stem + ext;
            String mlpath = win32Tool().getExecutablePath("ml.exe");

            putln("# Begin Source File");
            putln();
            putln("SOURCE=\"" + srcName + "\"");
            putln();

            if (v.size() > 1) {
                putln("!IF  \"$(CFG)\" == \"" + config.name + "\"");
                putln();
            }

            putln("# PROP Ignore_Default_Tool 1");
            putln("# Begin Custom Build - Performing Custom Build Step on " +
                  "$(InputName).asm");
            putln("OutDir=.\\" + config.outputDir);
            putln("InputPath=\\\"" + config.outputDir +
                  "\\" + stem + ext + "\"");
            putln("InputName=" + stem);

            putln();
            putln("\"$(OutDir)\\$(InputName).obj\" : $(SOURCE) " +
                  "\"$(INTDIR)\" \"$(OUTDIR)\"");
            if (isAsm) {
                putln("    \"" + mlpath + "\" /nologo /c /coff /Zi /FR " +
                      "/Fo\"$(OutDir)\\$(InputName).obj\" " + srcName);
            } else {
                put("    \"cl.exe\" /c /I . ");
                putIncludePaths();
                putCppFlags(config, false);
                putln(" /Fo\"$(OutDir)\\$(InputName).obj\" " + srcName);
            }
            putln("# End Custom Build");
            putln();

            if (v.size() > 1) {
                // We need to write the Exclude_From_Build only if there
                // are more than one config in this project.
                for (int j=0; j<v.size(); j++) {
                    if (i == j) {
                        continue;
                    }
                    IDEProjectConfig other = (IDEProjectConfig)v.elementAt(j);
                    putln("!ELSEIF  \"$(CFG)\" == \"" + other.name + "\"");
                    putln();
                    putln("# PROP Exclude_From_Build 1");
                    putln();
                }
                putln("!ENDIF");
                putln();
            }
            putln("# End Source File");
            putln();
        }
    }

    void createDummyAsmAndOopMapsFiles(boolean isAsm) throws Exception {
        Vector v = getAvailableConfigs();
        for (int i=0; i<v.size(); i++) {
            IDEProjectConfig config = (IDEProjectConfig)v.elementAt(i);

            String dirName = getOutputDir() + "\\" + config.outputDir;
            File f = new File(dirName);
            if (!f.exists()) {
                f.mkdir();
            }

            if (isAsm) {
                String asmName = dirName + "\\Interpreter_i386.asm";
                PrintWriter writer = tool().openOutputFile(asmName);
                writer.println("\t;Dummy file.");
                writer.println("\t;Will be overwritten during compilation.");
                writer.println("\t;This file is auto-generated.");
                writer.println("\t;Do not edit.");
                writer.close();
            } else {
                String asmName = dirName + "\\OopMaps.cpp";
                PrintWriter writer = tool().openOutputFile(asmName);
                writer.println("// Dummy file.");
                writer.println("// Will be overwritten during compilation.");
                writer.println("// This file is auto-generated.");
                writer.println("// Do not edit.");
                writer.close();
            }
        }
    }

    void writeHeaderFileRules() {
        Vector headerFiles = new Vector();

        for (Iterator iter = database.getAllFiles().iterator();
             iter.hasNext(); ) {
            FileList fl = (FileList) iter.next();
            String fileName = fl.getName();
            if (isFileIncludedInHeaders(fileName)) {
                fileName = vmSourceHandler.resolveFileForProject(fileName);
                headerFiles.addElement(fileName);
            }
        }

        putln("# Begin Group \"Header Files\"");
        putln();
        putln("# PROP Default_Filter \"h;hpp;hxx;hm;inl;fi;fd\"");
        for (int i=0; i<headerFiles.size(); i++) {
            String fileName = (String)headerFiles.elementAt(i);

            putln("# Begin Source File");
            putln();
            putln("SOURCE=" + fileName);
            putln("# End Source File");
        }
        putln("# End Group");

        putln("# Begin Group \"Config Files\"");
        putln();
        putln("# PROP Default_Filter \"h;hpp;hxx;hm;inl;fi;fd\"");
        putln("# Begin Source File");
        putln();
        putln("SOURCE=jvmconfig.h");
        putln("# End Source File");
        putln("# End Group");
    }

    void writeFooter() {
        putln("# Begin Group \"Resource Files\"");
        putln();
        putln("# PROP Default_Filter " +
              "\"ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe\"");
        putln("# End Group");
        putln("# End Target");
        putln("# End Project");
    }

    void writeIncls() throws Exception {
        database.put();
    }

    /* override this to implement file filtering */
    public boolean isFileIncludedInSources(String fileName) {
        fileName = fileName.toLowerCase();
        if (!fileName.endsWith(".cpp") && !fileName.endsWith(".c")) {
            return false;
        }

        if (fileName.equals("nativestable.cpp")) { // this file is generated
            return false;
        }

        if (fileName.equals("romimage.cpp")) { // this file is generated
            return false;
        }

        return true;
    }

    /* override this to implement file filtering */
    public boolean isFileIncludedInHeaders(String fileName) {
        fileName = fileName.toLowerCase();
        if (!fileName.endsWith(".hpp") && !fileName.endsWith(".h")) {
            return false;
        }

        return true;
    }

    /* override this to implement file filtering */
    public boolean mayUsePrecompiledHeader(String fileName) {
        fileName = fileName.toLowerCase();
        if (fileName.endsWith("romimage.cpp")) {
            return false;
        }
        if (fileName.endsWith("romskeleton.cpp")) {
            return false;
        }
        if (fileName.endsWith("nativestable.cpp")) {
            return false;
        }
        if (fileName.endsWith("os_win32.cpp")) {
            return false;
        }

        return true;
    }

    /**
     * Override this to include extra files. You must add a pathname
     * that's accessible from this project. E.g., "./NativesTable.cpp"
     */
    public void addExtraSources(Vector lst) {
        lst.addElement("../javaapi/NativesTable.cpp");
        VMSourceHandler h = this.vmSourceHandler;

        switch (this.type) {
        case TARGET:
            if (!win32Tool().isRomizing()) {
                lst.addElement(h.resolveFileForProject("ROMSkeleton.cpp"));
            } else {
                lst.addElement("..\\romimage\\ROMImage.cpp");
            }
            break;
        case LOOPGEN:
            lst.addElement(h.resolveFileForProject("InterpreterSkeleton.cpp"));
            lst.addElement(h.resolveFileForProject("ROMSkeleton.cpp"));
            lst.addElement(h.resolveFileForProject("OopMapsSkeleton.cpp"));
            break;
        case ROMGEN:
            lst.addElement(h.resolveFileForProject("ROMSkeleton.cpp"));
            break;
        }
    }

    /**
     * Write the jvmconfig.h file for this project
     */
    void writeJvmConfig() throws Exception {
        /*
         * If user has chosen non-default value for the ENABLE_XXX flags,
         * pass these values to tool.configurator, which creates jvmconfig.h.
         */

        Hashtable env = new Hashtable();
        Properties userSettings = tool().getUserSettings();
        for (Enumeration keys = userSettings.keys(); keys.hasMoreElements();) {
            String key = (String)keys.nextElement();
            if (key.startsWith("ENABLE")) {
                String value = (String)userSettings.get(key);
                if (key.equals("ENABLE_MONET") && this.type != TARGET) {
                    // ENABLE_MONET must be set to false for loopgen and romgen
                    value = "false";
                }
                if (value.equals("true") || value.equals("false")) {
                   env.put(key, value);
                   env.put(key + "__BY", "idetool");
                }
            }
        }

        switch (this.type) {
        case ROMGEN:
        case LOOPGEN:
            // the generators must have INTERPRETER and ROM generators
            env.put("ENABLE_INTERPRETER_GENERATOR",     "true");
            env.put("ENABLE_INTERPRETER_GENERATOR__BY", "idetool");
            env.put("ENABLE_ROM_GENERATOR",             "true");
            env.put("ENABLE_ROM_GENERATOR__BY",         "idetool");
        }

        String filename = getOutputDir() + File.separator + "jvmconfig.h";
        String fullpath = tool().getOutputFileFullPath(filename);
        Vector extra = new Vector();
        extra.addElement("USE_UNICODE_FOR_FILENAMES=1");

        // Append product-specific definitions
        {
            String productName = tool().configurator.getProductName();
            if (productName != null) {
                extra.addElement("JVM_NAME=\"" + productName + "\"");
            }
            String releaseVersion = tool().configurator.getReleaseVersion();
            if (releaseVersion != null) {
                extra.addElement("JVM_RELEASE_VERSION=\"" + releaseVersion + "\"");
            }
        }

        win32Tool().configurator.write(fullpath, env, extra);
    }
}

class Win32IDESettingsDialog extends JFrame implements ActionListener {
    boolean okPressed;
    Object lock = new Object();
    Hashtable widgets;
    Properties props;
    Vector buildOptions, generatorOptions, paths;

    public void getUserInput(Properties props, Vector buildOptions,
                            Vector generatorOptions, Vector paths)
        throws Exception
    {
        this.props = props;
        this.buildOptions = buildOptions;
        this.generatorOptions = generatorOptions;
        this.paths = paths;

        searchPaths();
        createUI();
        waitUI();
        updateProperties();
    }

    private void searchPaths() throws Exception {
        Vector v = getSystemPaths();

        for (int n=0; n<paths.size(); n++) {
            String name = (String)paths.elementAt(n);
            String path = (String)props.get(name);

            if (path != null && !path.equals("")) {
                // we already have a default value
                continue;
            }
            path = "- please enter full path of " + name;
            for (int i=0; i<v.size(); i++) {
                String p = (String)v.elementAt(i);
                String s = p + File.separator + name;
                File f = new File(s);
                if (f.exists()) {
                    path = s;
                    break;
                }
            }

            props.put(name, path);
        }
    }

    private Vector getSystemPaths() throws Exception {
        Vector v = new Vector();

        Process proc = Runtime.getRuntime().exec("cmd.exe /c path");
        InputStream in = proc.getInputStream();
        InputStreamReader inr = new InputStreamReader(in);
        BufferedReader reader = new BufferedReader(inr);
        String data = reader.readLine();
        reader.close();

        if (!data.toLowerCase().startsWith("path=")) {
            return v;
        } else {
            data = data.substring(5);
        }

        StringTokenizer st = new StringTokenizer(data, File.pathSeparator);
        while (st.hasMoreTokens()) {
            String path = st.nextToken();
            v.addElement(path);
        }
        return v;
    }

    private void addOne(JPanel panel, GridBagLayout gridbag,
                        GridBagConstraints c, JComponent comp,
                        boolean indent) {
        c.fill = GridBagConstraints.BOTH;
        c.weightx = 1.0;
        if (indent) {
            c.insets = new Insets(0, 20, 0, 0);
        } else {
            c.insets = new Insets(5, 0, 0, 0);
        }
        c.gridwidth = GridBagConstraints.REMAINDER;
        gridbag.setConstraints(comp, c);
        panel.add(comp);
    }

    private void addTwo(JPanel panel, GridBagLayout gridbag,
                        GridBagConstraints c, JComponent one,
                        JComponent two, boolean indent) {
        c.fill = GridBagConstraints.BOTH;
        c.weightx = 1.0;
        c.ipadx = 2;
        c.gridwidth = 1;
        if (indent) {
            c.insets = new Insets(0, 20, 0, 0);
        } else {
            c.insets = new Insets(5, 0, 0, 0);
        }
        gridbag.setConstraints(one, c);
        panel.add(one);

        c.insets = new Insets(0, 0, 0, 0);
        c.gridwidth = GridBagConstraints.REMAINDER;
        gridbag.setConstraints(two, c);
        panel.add(two);
    }

    class MyScrollablePanel extends JPanel implements Scrollable {
        JPanel otherPanel;
        MyScrollablePanel(JPanel otherPanel) {
            this.otherPanel = otherPanel;
        }
        public Dimension getPreferredSize() {
            Dimension d = super.getPreferredSize();
            return d;
        }

        public Dimension getPreferredScrollableViewportSize() {
            Dimension d1 = super.getPreferredSize();
            Dimension d2 = otherPanel.getPreferredSize();
            return new Dimension((int)(d1.getWidth()),
                                 (int)(d2.getHeight()));
        }

        public boolean getScrollableTracksViewportWidth() {
            return false;
        }

        public boolean getScrollableTracksViewportHeight() {
            return false;
        }

        public int getScrollableUnitIncrement(Rectangle visibleRect,
                                              int orientation,
                                              int direction) {
            return 10;
        }

        public int getScrollableBlockIncrement(Rectangle visibleRect,
                                               int orientation,
                                               int direction) {
            return 50;
        }
    }

    private void createUI() {
        widgets = new Hashtable();

        JPanel rightPanel = new JPanel();
        JPanel leftPanel = new MyScrollablePanel(rightPanel);

        GridBagLayout gridbag = new GridBagLayout();
        GridBagConstraints c = new GridBagConstraints();

        leftPanel.setLayout(gridbag);
        rightPanel.setLayout(new GridLayout(0, 1));

        // Build options
        addOne(leftPanel, gridbag, c, new JLabel("Build Options:"), false);
        String items3[] = {
            "true",
            "false",
            "default",
        };
        String items2[] = {
            "true",
            "false",
        };

        for (Enumeration e = buildOptions.elements() ; e.hasMoreElements() ;) {
            String name = (String)e.nextElement();
            String value = (String)props.get(name);

            JLabel label = new JLabel(name);
            JComboBox box;

            if (name.startsWith("ENABLE")) {
                box = new JComboBox(items3);
            } else {
                box = new JComboBox(items2);
            }

            if (value.equals("true")) {
                box.setSelectedIndex(0);
            } else if (value.equals("false")) {
                box.setSelectedIndex(1);
            } else {
                box.setSelectedIndex(2);
            }

            addTwo(leftPanel, gridbag, c, label, box, true);
            widgets.put(name, box);
        }

        // Generator options
        addOne(leftPanel, gridbag, c, new JLabel("Generator Options:"), false);
        for (Enumeration e = generatorOptions.elements();
             e.hasMoreElements() ;) {
            String name = (String)e.nextElement();
            boolean defValue = ((String)props.get(name)).equals("true");

            JCheckBox box = new JCheckBox(name);
            if (defValue) {
                box.setSelected(true);
            }
            addOne(leftPanel, gridbag, c, box, true);
            widgets.put(name, box);
        }

        // Paths
        rightPanel.add(new JLabel("Tool Paths:"));
        for (Enumeration e = paths.elements() ; e.hasMoreElements() ;) {
            String name = (String)e.nextElement();
            String path = (String)props.get(name);

            JLabel label = new JLabel(name);
            JTextField textField = new JTextField();
            textField.setColumns(30);
            textField.setText(path);
            rightPanel.add(label);
            rightPanel.add(textField);
            widgets.put(name, textField);
        }

        JButton ok = new JButton("  OK  ");
        ok.setActionCommand("ok");
        ok.addActionListener(this);

        JButton cancel = new JButton("Cancel");
        cancel.setActionCommand("cancel");
        cancel.addActionListener(this);

        JPanel buttons = new JPanel();
        buttons.setLayout(new FlowLayout());
        buttons.add(ok);
        buttons.add(cancel);

        JPanel rightOuterPanel = new JPanel();
        rightOuterPanel.setLayout(new BorderLayout());
        rightOuterPanel.add(rightPanel, BorderLayout.NORTH);

        JScrollPane leftPane = new JScrollPane(leftPanel,
                           ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS,
                           ScrollPaneConstants.HORIZONTAL_SCROLLBAR_ALWAYS);

        JSplitPane split = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,
                                          leftPane, rightOuterPanel);
        getContentPane().add(split);
        getContentPane().add(buttons, BorderLayout.SOUTH);

        getRootPane().setDefaultButton(ok);

        this.pack();
        this.setLocation(200, 200);
    }

    private void waitUI() throws Exception {
        this.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                setVisible(false);
                synchronized (lock) {
                    lock.notifyAll();
                }
            }
        });

        okPressed = false;
        this.setVisible(true);
        synchronized (lock) {
            lock.wait();
        }

        if (!okPressed) {
            System.out.println("IDE creation cancelled");
            System.exit(1);
        }
    }

    private void updateProperties() {
        for (Enumeration e = widgets.keys(); e.hasMoreElements() ;) {
            String name = (String)e.nextElement();
            JComponent comp = (JComponent)widgets.get(name);

            if (comp instanceof JCheckBox) {
                boolean sel = ((JCheckBox)comp).isSelected();
                if (sel) {
                    props.put(name, "true");
                } else {
                    props.put(name, "false");
                }
            } else if (comp instanceof JComboBox) {
                JComboBox box = (JComboBox)comp;
                String value = (String)box.getSelectedItem();
                props.put(name, value);
            } else if (comp instanceof JTextField) {
                String value = ((JTextField)comp).getText();
                props.put(name, value);
            }
        }
    }

    public void actionPerformed(java.awt.event.ActionEvent e) {
        if (e.getActionCommand().equals("ok")) {
            okPressed = true;
        }
        setVisible(false);
        synchronized (lock) {
            lock.notifyAll();
        }
    }
}

// IMPL_NOTE: add default debug command line args for ROM generator
// IMPL_NOTE: disable optimization for float
