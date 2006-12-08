/*
 * @(#)AbstractNamedNode.java	1.10 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.tools.jdwpgen;

import java.util.*;
import java.io.*;

abstract class AbstractNamedNode extends Node {

    NameNode nameNode;
    String name;

    public String name() {
        return name;
    }

    void prune() {
        Iterator it = components.iterator();

        if (it.hasNext()) {
            Node nameNode = (Node)it.next();

            if (nameNode instanceof NameNode) {
                this.nameNode = (NameNode)nameNode;
                this.name = this.nameNode.text();
                it.remove();
            } else {
                error("Bad name: " + name);
            }
        } else {
            error("empty");
        }
        super.prune();
    }

    void constrain(Context ctx) {
        nameNode.constrain(ctx);
        super.constrain(ctx.subcontext(name));
    }

    void document(PrintWriter writer) {
        writer.println("<h4><a name=" + name + ">" + name + 
                       " Command Set</a></h4>");
        for (Iterator it = components.iterator(); it.hasNext();) {
            ((Node)it.next()).document(writer);
        }
    }

    String javaClassName() {
        return name();
    }

    void genJavaClassSpecifics(PrintWriter writer, int depth) {
    }

    String javaClassImplements() {
        return ""; // does not implement anything, by default
    }

    void genJavaClass(PrintWriter writer, int depth) {
        writer.println();
        genJavaComment(writer, depth);
        indent(writer, depth);
        if (depth != 0) {
            writer.print("static ");
        }
        writer.print("class " + javaClassName());
        writer.println(javaClassImplements() + " {");
        genJavaClassSpecifics(writer, depth+1);
        for (Iterator it = components.iterator(); it.hasNext();) {
            ((Node)it.next()).genJava(writer, depth+1);
        }
        indent(writer, depth);
        writer.println("}");
    }

    void genCInclude(PrintWriter writer) {
        if (nameNode instanceof NameValueNode) {
            writer.println("#define " + context.whereC + 
                           " " + nameNode.value());
        }
        super.genCInclude(writer);
    }
}
