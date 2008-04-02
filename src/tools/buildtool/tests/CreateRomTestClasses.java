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

package tests;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import util.*;

/**
 * Create the romtestclasses.zip that contains classes that need to be
 * romized for the romizer unit tests.
 */
public class CreateRomTestClasses {
    static void usage() {
        System.out.println("Usage: java -jar buildtool.jar " +
                           "romtestclasses cldc_classes.zip " +
                           "cldcx_classes.zip tests.jar <outputfile>");
    }
    static File workspacedir;
    static File outputdir;
    static Vector sourcefiles;
    static PrintWriter pw;

    public static void main(String args[]) throws Throwable {
        if (args.length != 4) {
            usage();
            System.exit(1);
        }
        File cldc_classes_zip  = new File(args[0]);
        File cldcx_classes_zip = new File(args[1]);
        File tests_jar         = new File(args[2]);
        File output            = new File(args[3]);

        FileOutputStream out = new FileOutputStream(output);
        ZipOutputStream zout = new ZipOutputStream(out);

        // Simply copy all entries from cldc_classes_zip and cldcx_classes_zip
        copyZipEntries(zout, cldc_classes_zip,  null);
        copyZipEntries(zout, cldcx_classes_zip, null);

        // We need to filter out the entries in tests_jar if:
        // (1) The test is a negative test case
        // (2) The test may use a feature that's not supported by the
        //     VM's configuration (e.g., floating point or CLDC 1.1 API)
        Filter filter = new Filter();
        copyZipEntries(zout, tests_jar, filter);

        zout.close();
    }

    /**
     * Copy the content of of the given zip file source 
     * to the ZIP file output stream
     */
    static void copyZipEntries(ZipOutputStream zout, File src, Filter filter) 
        throws Throwable
    {
        ZipFile zipSrc = null;

        try {
            zipSrc = new ZipFile(src);
        } catch (Throwable t) {
            System.out.println("Unexpected error:");
            t.printStackTrace();
            System.out.println();
            System.out.println("Your JDK_DIR cannot handle long JAR entries");
            System.out.println("Please upgrade to JDK 1.4.1 or later.");
            System.out.println("JDK version '1.4.1' is known to work.");
            System.exit(-1);
        }
        for (Enumeration e = zipSrc.entries(); e.hasMoreElements(); ) {
            ZipEntry entry = (ZipEntry) e.nextElement();
            String name = entry.getName();
            if (name.startsWith("META-INF")) {
                continue;
            }
            if (filter != null && !filter.includeEntry(entry.getName())) {
                continue;
            }

            int size = (int)entry.getSize();
            byte data[] = new byte[size];
            InputStream in = zipSrc.getInputStream(entry);
            DataInputStream din = new DataInputStream(in);
            din.readFully(data);

            zout.putNextEntry(entry);
            zout.write(data, 0, data.length);
            zout.closeEntry();

            din.close();
        }
    }

    static class Filter {
        static final String include_prefixes[] = {
            "Sanity",
            "com/sun/tck/cldc/lib/",
            "vm/share/rom/ROMOptimizer/functional_test1_b",
            "vm/share/rom/ROMWriter/generate_java_fieldmap2_rom",
            "vm/share/rom/ROMWriter/generate_java_fieldmap1_rom",
            "vm/share/rom/ROMInliner/rom_",
            "util/",
            "bench/NativeMethod.class",
            "rom/",
            "vm/share/natives/sni",
            "vm/share/natives/kni",
            "vm/share/runtime/JVM/JVM_SetHeapLimit1",
            "vm/cpu/share/InterpreterGenerator/FastMemRoutines",
            "vm/share/memory/FinalizerWeakRef",
            "anilib",
            "isolate/tests/Natives",
            "vm/cpu/arm/CodeGenerator_arm",
            "mps/HiddenClass",
            "mps/hidden",
            "vm/cpu/share/CodeGenerator/InitStaticArrayTest",
            "vm/share/compiler/BytecodeCompileClosure/A",
        };
        static final String include_infixes[] = {
            "/SNI_",
        };
        static final String include_suffixes[] = {
            "_rom.class",
        };

        static final String exclude_prefixes[] = {
            "rom/unrestricted/NotRomized.class",
            "rom/restricted/NotRomized.class",
            "rom/hidden/NotRomized.class",
        };
        /**
         * @return true iff the given entry should be included in
         *              the output ZIP file
         */
        public boolean includeEntry(String name) {
            if (!name.endsWith(".class")) {
                return false;
            }

            boolean include = false;
            for (int i=0; i<include_prefixes.length; i++) {
                String prefix = include_prefixes[i];
                if (name.startsWith(prefix)) {
                    include = true;
                    break;                                       
                }
            }
            for (int i=0; i<include_suffixes.length; i++) {
                String prefix = include_suffixes[i];
                if (name.endsWith(prefix)) {
                    include = true;
                    break;                                       
                }
            }
            for (int i=0; i<include_infixes.length; i++) {
                String prefix = include_infixes[i];
                if (name.indexOf(prefix) != -1) {
                    include = true;
                    break;                                       
                }
            }

            if (include) {
                for (int i=0; i<exclude_prefixes.length; i++) {
                    String prefix = exclude_prefixes[i];
                    if (name.startsWith(prefix)) {
                        include = false;
                        break;
                    }
                }
            }

            if (include) {
                System.out.println("Added to ROM: " + name);
            }

            return include;
        }
    }
}
