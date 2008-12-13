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


/*
 *   
 */

import java.util.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import javax.swing.*;
import javax.swing.tree.*;
import javax.swing.table.*;
import javax.swing.event.*;
import javax.swing.text.NumberFormatter;

class Profile {
    /**
     * This hashtable is a collection of all CallGraphNodes in the callgraph.
     * It's indexed by each CallGraphNode's <b>index</b> member, whose type
     * is Integer.
     */
    Hashtable nodes;

    /**
     * This hashtable is a collection of all CallGraphNodes in the callgraph.
     * It's indexed by each CallGraphNodes's <b>name</b>, whose type is
     * String. <p>
     *
     * Each entry in this hashtable is either a CallGraphNodes (if only
     * one CallGraphNode of that name exists in the callgraph), or
     * a Vector (if two or more CallGraphNodes of that name exist in
     * the callgraph).
     */
    Hashtable nodesByName;

    /**
     * The root of all call records. This root is artificially created.
     * It's the parent of all the parent-less CallGraphNode read from
     * a profile.
     */
    CallGraphNode root;

    /**
     * Number of CallGraphNodes in this profile that has one or more children,
     * including this.root.
     */
    int numParents;

    /**
     * Number of graph files to parse.
     */
    int numFiles;

    /**
     * Duration (in msec) of the period when the profile data was collected.
     */
    double period;

    Profile(String[] files) throws IOException {
        nodes = new Hashtable();
        nodesByName = new Hashtable();
        root = createTopNode(null, -1, "root");
        numParents = 0;
        numFiles = files.length;

        if (isMultifile()) {
            for (int i = 0; i < numFiles; i++) {
                readInputFile(new FileParserContext(files[i], i + 1, this));
            }
        } else {
            readInputFile(new FileParserContext(files[0], 0, this));
        }

        createCallGraph();

        if (numParents > 0) {
            root.endTime = period;
            root.computeSummary();
        }

        root.computePercentage();
    }

    boolean isMultifile() {
        return numFiles > 1;
    }
    
    CallGraphNode createTopNode(CallGraphNode parent, int index, String name) {
        CallGraphNode r = new CallGraphNode(parent);

        r.parent = parent;
        r.parentIdx = parent == null ? null : parent.index;
        r.index = new Integer(index);
        r.name = name;

        nodes.put(r.index, r);

        addToIndexByName(r);
        return r;
    }

    void printRecord(CallGraphNode cgNode, String prefix) {
        if (cgNode.parent != null) {
            System.out.print(prefix);
            System.out.print(cgNode.name);
            System.out.print(", D=" + cgNode.depth);
            System.out.print(", K=" + cgNode.count);
            System.out.print(", C=" + cgNode.kidsCycles);
            System.out.print(", M=" + cgNode.kidsMsec);
            System.out.println();
            prefix += " ";
        }
        if (cgNode.children != null) {
            Vector v = cgNode.children;
            for (int i=0; i<v.size(); i++) {
                printRecord((CallGraphNode)v.elementAt(i), prefix);
            }
        }
    }


    /**
     * Read the entire content of an input file
     */
    void readInputFile(FileParserContext ctx) throws IOException {
        FileReader fr = new FileReader(ctx.file);
        BufferedReader reader = new BufferedReader(fr);
        String line;

        /* Skip the first line. It's for human consumption only */
        line = reader.readLine();
        //line = reader.readLine(); /* need revisit */

        // Read all lines in the input file
        //
        while ((line = reader.readLine()) != null) {
            readInputLine(line, ctx);
        }
    }

    void readInputLine(String line, FileParserContext ctx) {
        StringTokenizer st = new StringTokenizer(line);
        if (st.countTokens() != 15) {
            return;
        }

        // Read the raw content of the input line into the CallGraphNode
        try {
            CallGraphNode cgNode;
            int index = Integer.parseInt(st.nextToken());
            if (index == -1) {
                // We also have a predefined root node in our source file -
                // so modify the existing root node
                cgNode = ctx.root;
                st.nextToken(); // parent token is meaningless for the root
            } else {
                cgNode = new CallGraphNode(ctx.root);
                cgNode.index = new Integer(ctx.indexOffset + index);
                cgNode.parentIdx = new Integer(ctx.indexOffset +
                    Integer.parseInt(st.nextToken()));
            }

            cgNode.depth = Integer.parseInt(st.nextToken());
            cgNode.name = st.nextToken();
            cgNode.count = Integer.parseInt(st.nextToken());

            cgNode.onlyCycles = Long.parseLong(st.nextToken());
            cgNode.onlyMsec   = Double.parseDouble(st.nextToken());
            cgNode.onlyPerc   = Double.parseDouble(st.nextToken());

            if (index != -1) {
                // kidsCylcles is calculated automatically for the root node
                cgNode.kidsCycles = Long.parseLong(st.nextToken());
                cgNode.kidsMsec   = Double.parseDouble(st.nextToken());
                cgNode.kidsPerc   = Double.parseDouble(st.nextToken());

                // Timeline information
                cgNode.startTime  = Double.parseDouble(st.nextToken());
                cgNode.endTime    = Double.parseDouble(st.nextToken());
                if (cgNode.endTime > period) {
                    period = cgNode.endTime;
                }

                // Store new CallGraphNode into the hashtable
                nodes.put(cgNode.index, cgNode);
                // Add it to another index table, searchable by cgNode.name
                addToIndexByName(cgNode);
            }
        } catch (NumberFormatException e) {
            // IMPL_NOTE: warning
        }
    }

    void createCallGraph() {
        // Now all the CallGraphNodes are created. We're ready to
        // construct the call graph
        for (Enumeration e = nodes.elements(); e.hasMoreElements() ;) {
            CallGraphNode cgNode = (CallGraphNode)e.nextElement();

            if (cgNode.parentIdx == null) {
                // This is the root record. Do nothing
            } else {
                CallGraphNode parent;
                parent = (CallGraphNode)nodes.get(cgNode.parentIdx);
                if (parent == null) {
                    System.out.println("WARNING: no parent found for element " + 
                                       cgNode);
                    parent = root;
                }

                if (parent.children == null) {
                    parent.children = new Vector();
                    numParents ++;
                }
                parent.children.addElement(cgNode);
                cgNode.parent = parent;
            }
        }
    }

    /**
     * Add this cgNode to the index of all CallGraphNodes by its name.
     * This index is maintained by the hashtable <b>nodesByName</b>
     */
    void addToIndexByName(CallGraphNode cgNode) {
        Object obj = nodesByName.get(cgNode.name);

        //System.out.println(cgNode.name + "=" + obj);
        if (obj == null) {
            // Not in the table yet, store the cgNode itself
            nodesByName.put(cgNode.name, cgNode);
        }
        else if (obj instanceof Vector) {
            // Already has two more more cgNodes of the same name, append
            // to the list
            ((Vector)obj).addElement(cgNode);
        }
        else {
            // We've seen the second cgNode of the same name. Store both
            // cgNodes inside a Vector and put that Vector in the hashtable.
            //
            // By doing this, we only create Vector for those method names
            // that correspond to more than one CallGraphNode
            Vector v = new Vector(2);
            v.addElement(obj);
            v.addElement(cgNode);
            nodesByName.put(cgNode.name, v);
        }
    }
}

class FileParserContext {
    /**
     * Name of the file being parsed.
     */
    String file;
    
    /**
     * indexOffset is added to every node index found in a source file.
     * This helps to avoid collisions between equal indices in different
     * graph files.
     * indexOffset is defined as fileNo * MAX_INDEX
     * where MAX_INDEX is large enough to ensure that
     * every index in a source file is less than MAX_INDEX.
     */
    int indexOffset;

    /**
     * The root node for all records of the file being parsed.
     */
    CallGraphNode root;

    static final int MAX_INDEX = 10000000;

    FileParserContext(String fileName, int fileNo, Profile prof) {
        file = fileName;
        indexOffset = fileNo * MAX_INDEX;
        if (fileNo == 0) {
            // do not create additional level when parsing
            // exactly one source file;
            // all elements are attached directly to the "root"
            root = prof.root;
        } else {
            // create an auxiliary node named <fileName> which will be
            // the virtual root for all records in current file
            root = prof.createTopNode(prof.root, indexOffset - 1, fileName);
        }
    }
}

class Filter {
    boolean prefix;
    String substr;
    double startTime;
    double endTime;

    Filter(String text, double time) {
        if (text != null) {
            substr = text.trim();
            if (substr.equals("")) {
                substr = null;
            } else if (prefix = substr.charAt(0) == '^') {
                substr = substr.substring(1);
            }
        }
        if (time >= 0.0) {
            startTime = time;
            endTime = time;
        } else {
            startTime = Double.MAX_VALUE;
            endTime = -1.0;
        }
    }

    boolean isEmpty() {
        return substr == null && endTime < 0.0;
    }
    
    boolean recordMatches(CallRecord r) {
        if (substr != null) {
            String name = r.name;
            if (name == null) {
                return false;
            }
            int n = name.indexOf(substr);
            if (n < 0 || (prefix && n != 0)) {
                return false;
            }
        }
        return r.startTime <= startTime && r.endTime >= endTime;
    }

    public boolean equals(Object o) {
        if (o == null) {
            return isEmpty();
        }
        if (!(o instanceof Filter)) {
            return false;
        }
        Filter f = (Filter) o;
        if (substr == null) {
            if (f.substr != null) {
                return false;
            }
        } else {
            if (!substr.equals(f.substr) || prefix != f.prefix) {
                return false;
            }
        }
        return startTime == f.startTime && endTime == f.endTime;
    }

}

class CallRecord {
    final static int DEPTH         = 0;
    final static int PARENT        = 1;
    final static int NAME          = 2;
    final static int COUNT         = 3;
    final static int ONLY_CYCLES   = 4;
    final static int AVG_CYCLES    = 5;
    final static int ONLY_MSEC     = 6;
    final static int ONLY_REL_PERC = 7;
    final static int ONLY_PERC     = 8;
    final static int KIDS_CYCLES   = 9;
    final static int KIDS_MSEC     = 10;
    final static int KIDS_REL_PERC = 11;
    final static int KIDS_PERC     = 12;
    final static int START_TIME    = 13;
    final static int END_TIME      = 14;

    final static int NUM_FIELDS    = 15;

    String name;
    int count;
    long onlyCycles;
    long kidsCycles;
    double onlyMsec;
    double kidsMsec;
    double onlyPerc;
    double kidsPerc;
    double startTime;
    double endTime;

    // The topmost node that the record belongs to:
    // <root> for single-file profiles or <fileName> for multifile profiles.
    // It is used to calculate the percentage of the record via
    // dividing its onlyCycles or kidsCycles by owner.kidsCycles
    CallRecord owner;

    // If this record represents a delta record, deltaBase points to
    // the record which was compared to, otherwise deltaBase is null
    CallRecord deltaBase;

    CallRecord(CallRecord owner) {
        this.owner = owner == null ? this : owner;
    }

    void add(CallRecord other, boolean addKids) {
        if (other.startTime < this.startTime || this.count == 0) {
            this.startTime = other.startTime;
        }
        if (other.endTime > this.endTime) {
            this.endTime = other.endTime;
        }

        this.count       += other.count;

        this.onlyCycles  += other.onlyCycles;
        this.onlyMsec    += other.onlyMsec;
        this.onlyPerc    += other.onlyPerc;

        if (addKids) {
            this.kidsCycles  += other.kidsCycles;
            this.kidsMsec    += other.kidsMsec;
            this.kidsPerc    += other.kidsPerc;
        }
    }

    void setDelta(CallRecord base) {
        if (deltaBase == null) {
            count      -= base.count;
            onlyCycles -= base.onlyCycles;
            onlyMsec   -= base.onlyMsec;
            onlyPerc   -= base.onlyPerc;
            kidsCycles -= base.kidsCycles;
            kidsMsec   -= base.kidsMsec;
            kidsPerc   -= base.kidsPerc;
            startTime  -= base.startTime;
            endTime    -= base.endTime;
            deltaBase   = base;
        }
    }

    void clearDelta() {
        if (deltaBase != null) {
            count      += deltaBase.count;
            onlyCycles += deltaBase.onlyCycles;
            onlyMsec   += deltaBase.onlyMsec;
            onlyPerc   += deltaBase.onlyPerc;
            kidsCycles += deltaBase.kidsCycles;
            kidsMsec   += deltaBase.kidsMsec;
            kidsPerc   += deltaBase.kidsPerc;
            startTime  += deltaBase.startTime;
            endTime    += deltaBase.endTime;
            deltaBase   = null;
        }
    }

    int depth;

    long getAvgOnlyCycles() {
        if (this.count <= 0) {
            return 0;
        } else {
            return this.onlyCycles / this.count;
        }
    }

    void computePercentage() {
        if (owner.kidsCycles == 0) {
            onlyPerc = 0;
            kidsPerc = 0;
        } else {
            onlyPerc = (double)onlyCycles / owner.kidsCycles;
            kidsPerc = (double)kidsCycles / owner.kidsCycles;
        }
    }
}

/**
 * AveragedCallRecord is used to support non-trivial merging of similar
 * records from different graph files like geometric averaging.
 */
class AveragedCallRecord extends CallRecord {
    static final int ARITHMETIC_MEAN = 0;
    static final int GEOMETRIC_MEAN  = 1;

    static int averagingMethod = ARITHMETIC_MEAN;

    int multipliers;
    double onlyPercProduct;
    double kidsPercProduct;

    AveragedCallRecord(CallRecord owner) {
        super(owner);
        multipliers = 0;
        onlyPercProduct = 1;
        kidsPercProduct = 1;
    }

    void add(CallRecord other, boolean addKids) {
        super.add(other, addKids);
        multipliers++;
        if (owner != null && other.owner.kidsCycles != 0) {
            onlyPercProduct *= (double)other.onlyCycles / other.owner.kidsCycles;
            kidsPercProduct *= (double)other.kidsCycles / other.owner.kidsCycles;
        }
    }

    void computePercentage() {
        if (averagingMethod == ARITHMETIC_MEAN) {
            super.computePercentage();
            return;
        }
        if (multipliers == 0) {
            onlyPerc = 0;
            kidsPerc = 0;
        } else {
            double power = 1 / (double)multipliers;
            onlyPerc = Math.pow(onlyPercProduct, power);
            kidsPerc = Math.pow(kidsPercProduct, power);
        }
    }
}

class CallGraphNode extends CallRecord {
    /**
     * The parent of this CallGraphNode. The parent/child relationship is
     * defined as: <pre>
     *   if methodA calls methodB, and methodB calls methodC
     * then
     *   CallGraphNode (methodA, methodB) is the parent of
     *   CallGraphNode (methodB, methodC)
     * </pre>
     */
    CallGraphNode parent;

    /**
     * A list of all CallGraphNodes whose parent is this CallGraphNode.
     * Is null of this CallGraphNode has no children.
     */
    Vector children;

    /**
     * Is null iff this CallGraphNode has no children.
     */
    CallRecord summary[];

    Integer index;
    Integer parentIdx;

    CallGraphNode(CallRecord owner) {
        super(owner);
    }
    
    /**
     * Compute the summary of each CallGraphNodes that has at least one child.
     */
    void computeSummary() {
        // (1) Merge the summary of all the children. We use a hashtable
        //     to store the summary at this stage for faster searches.

        Vector v = children;
        Hashtable table = new Hashtable();
        int i;

        /* Create myself */
        CallRecord myself = getSummaryRecord(table, this.name);
        myself.add(this, true);

        for (i=0; i<v.size(); i++) {
            CallGraphNode child = (CallGraphNode)v.elementAt(i);
            if (child.children != null) {
                child.computeSummary();
                for (int j=child.summary.length-1; j>=0; j--) {
                    CallRecord chRec = child.summary[j];
                    
                    CallRecord myRec = getSummaryRecord(table, chRec.name);
                    if (!chRec.name.equals(this.name)) {
                        myRec.add(chRec, true);
                    } else {
                        myRec.add(chRec, false);
                    }
                }
            } else {
                CallRecord myRec = getSummaryRecord(table, child.name);
                if (!child.name.equals(this.name)) {
                    myRec.add(child, true);
                } else {
                    myRec.add(child, false);
                }
            }
        }

        // (2) convert the summary from a hashtable to a vector for
        //     better size (use an array instead??)

        summary = new CallRecord[table.size()];
        i = 0;
        for (Enumeration e = table.elements(); e.hasMoreElements() ;) {
            CallRecord rec = (CallRecord)e.nextElement();
            summary[i++] = rec;
        }

        if (isTopNode()) {
            for (i=0; i<summary.length; i++) {
                CallRecord rec = summary[i];
                myself.kidsCycles  += rec.onlyCycles;
                myself.kidsMsec    += rec.onlyMsec;
                myself.kidsPerc    += rec.onlyPerc;
            }
            this.kidsCycles = myself.kidsCycles;
            this.kidsMsec   = myself.kidsMsec;
            this.kidsPerc   = myself.kidsPerc;
        }
    }

    boolean isTopNode() {
        // This node is global root or virtual file root
        // if and only if its index == -1 (mod MAX_INDEX)
        return (index.intValue() + 1) % FileParserContext.MAX_INDEX == 0;
    }

    void computePercentage() {
        // Compute percentage of this node, all of its subnodes
        // and summary records
        super.computePercentage();
        if (children != null) {
            for (int i = 0; i < children.size(); i++) {
                ((CallGraphNode)children.elementAt(i)).computePercentage();
            }
        }
        if (summary != null) {
            for (int i = 0; i < summary.length; i++) {
                summary[i].computePercentage();
            }
        }
    }

    CallGraphNode findChildByName(String nameToFind) {
        for (int i = 0; i < children.size(); i++) {
            CallGraphNode child = (CallGraphNode)children.elementAt(i);
            if (child.name.equals(nameToFind)) {
                return child;
            }
        }
        return null;
    }

    CallRecord findSummaryByName(String nameToFind) {
        for (int i = 0; i < summary.length; i++) {
            if (summary[i].name.equals(nameToFind)) {
                return summary[i];
            }
        }
        return null;
    }

    void setDelta(CallGraphNode base) {
        super.setDelta(base);

        if (children != null && base.children != null) {
            for (int i = 0; i < children.size(); i++) {
                CallGraphNode child = (CallGraphNode)children.elementAt(i);
                CallGraphNode prototype = base.findChildByName(child.name);
                if (prototype != null) {
                    child.setDelta(prototype);
                }
            }
        }
        
        if (summary != null && base.summary != null) {
            for (int i = 0; i < summary.length; i++) {
                CallRecord prototype = base.findSummaryByName(summary[i].name);
                if (prototype != null) {
                    summary[i].setDelta(prototype);
                }
            }
        }
    }

    void clearDelta() {
        super.clearDelta();
        if (children != null) {
            for (int i = 0; i < children.size(); i++) {
                ((CallGraphNode)children.elementAt(i)).clearDelta();
            }
        }
        if (summary != null) {
            for (int i = 0; i < summary.length; i++) {
                summary[i].clearDelta();
            }
        }
    }

    CallRecord getSummaryRecord(Hashtable table, String name) {
        CallRecord rec = (CallRecord)table.get(name);
        if (rec == null) {
            CallRecord owner = isTopNode() ? this : this.owner;
            // Non-trivial averaging is currently supported for top-level
            // summary records except profile.root (for which name == this.name)
            if (parent == null && name != this.name) {
              rec = new AveragedCallRecord(owner);
            } else {
              rec = new CallRecord(owner);
            }
            rec.name = name;
            table.put(name, rec);
        }

        return rec;
    }

    public String toString() {
        if (parent == null) {
            return "-" + name;
        } else {
            return parent.name + "-" + name;
        }
    }
}

class MergedCallGraphNode extends CallGraphNode {
    MergedCallGraphNode(CallGraphNode src, int levels) {
        super(src.owner);
        if (levels > 0 && src.parent != null) {
            this.parent = new MergedCallGraphNode(src.parent, levels-1);
        } else {
            this.parent = new CallGraphNode(src.owner);
            this.parent.name = "....";
        }
        this.name = src.name;
        this.add(src, true);
    }

    void add(CallGraphNode other, boolean addKids, int levels) {
        if (levels > 0) {
            MergedCallGraphNode myParent = (MergedCallGraphNode)this.parent;
            CallGraphNode otherParent = other.parent;
            if (myParent != null && other.parent != null) {
                myParent.add(otherParent, addKids, levels-1);
            }
        }
        super.add(other, addKids);
    }
}

class MergedCallRecord extends CallRecord {
    MergedCallRecord(CallRecord owner) {
        super(owner);
    }
}

abstract class BaseTableModel extends AbstractTableModel implements Comparator {
    JTable table;
    ProfView viewer;

    BaseTableModel(ProfView viewer) {
        this.viewer = viewer;
    }

    String names[] = {
        "Depth", "Parent", "Name", "Count",
        "Cycles", "Avg cycles", "Msec", "Rel %", "%",
        "Cycles_k", "Msec_k", "Rel %_k", "%_k",
        "Start", "End"
    };

    boolean colVisible[] = new boolean[CallRecord.NUM_FIELDS];

    void setOwner(JTable table) {
        this.table = table;
        setTableColumnWidths();
        addMouseListenerToHeaderInTable();
    }

    void setColumnVisible(int colName, boolean visible) {
        colVisible[colName] = visible;
    }

    /**
     * Convert the visible column (seen by user on the screen) to the
     * "real column", which is CallRecord.NAME, CallRecord.COUNT, etc.
     */
    int toRealCol(int visCol) {
        for (int i=0; i<colVisible.length; i++) {
            if (colVisible[i]) {
                if (visCol == 0) {
                    return i;
                } else {
                    visCol --;
                }
            }
        }
        return 0; // shouldn't be here!
    }

    abstract CallRecord[] getDataArray();
    abstract void setDataArray(CallRecord[] newArray);

    /**
     * Returns true if the contents of the data array are CallGraphNode
     * objects.
     */
    abstract boolean isCallGraphNode();

    CallRecord getRecordByRow(int row) {
        return getDataArray()[row];
    }

    /**
     * Implements AbstractTableModel.getColumnCount().
     */
    public int getColumnCount() {
        int n = 0;
        for (int i=0; i<colVisible.length; i++) {
            if (colVisible[i]) {
                n ++;
            }
        }
        return n;
    }

    /**
     * Implements AbstractTableModel.getColumnName().
     */
    public String getColumnName(int visCol) {
        String name = names[toRealCol(visCol)];
        return name;
    }

    /**
     * Implements AbstractTableModel.getRowCount().
     */
    public int getRowCount() {
        return getDataArray().length;
    }

    /**
     * Implements AbstractTableModel.getValueAt().
     *
     * Get the object to display at (row, visCol) in the table.
     */
    public Object getValueAt(int row, int visCol) {
        CallRecord[] dataArray = getDataArray();
        CallRecord rec = dataArray[row];

        switch (toRealCol(visCol)) {
        case CallRecord.PARENT:
            if (isCallGraphNode()) {
                // IMPL_NOTE: change to use (rec instanceof CallGraphNode) instead
                CallGraphNode parent = ((CallGraphNode)rec).parent;
                if (parent == null) {
                    return ".";
                } else {
                    return parent.name;
                }
            } else {
                return "??";
            }
        case CallRecord.NAME:
            return rec.name;
        case CallRecord.COUNT:
            return new Integer(rec.count);
        case CallRecord.DEPTH:
            return new Integer(rec.depth);
        case CallRecord.ONLY_CYCLES:
            return new Long(rec.onlyCycles);
        case CallRecord.AVG_CYCLES:
            return new Long(rec.getAvgOnlyCycles());
        case CallRecord.ONLY_MSEC:
            return new Double(rec.onlyMsec);
        case CallRecord.ONLY_REL_PERC:
            return getPercObject((double)rec.onlyCycles /
                                 viewer.currentTreeNode.cgNode.kidsCycles);
        case CallRecord.ONLY_PERC:
            return getPercObject(rec.onlyPerc);
        case CallRecord.KIDS_CYCLES:
            return new Long(rec.kidsCycles);
        case CallRecord.KIDS_MSEC:
            return new Double(rec.kidsMsec);
        case CallRecord.KIDS_REL_PERC:
            return getPercObject((double)rec.kidsCycles /
                                 viewer.currentTreeNode.cgNode.kidsCycles);
        case CallRecord.START_TIME:
            return new Double(rec.startTime);
        case CallRecord.END_TIME:
            return new Double(rec.endTime);
        case CallRecord.KIDS_PERC:
        default:
            return getPercObject(rec.kidsPerc);
        }
    }

    /**
     * Get the class of the object at the given visible column.
     */
    public Class getColumnClass(int visCol) {
        switch (toRealCol(visCol)) {
        case CallRecord.PARENT:
        case CallRecord.NAME:
            return String.class;
        case CallRecord.COUNT:
        case CallRecord.DEPTH:
            return Integer.class;
        case CallRecord.ONLY_CYCLES:
        case CallRecord.KIDS_CYCLES:
        case CallRecord.AVG_CYCLES:
            return Long.class;
        case CallRecord.ONLY_PERC:
        case CallRecord.KIDS_PERC:
        case CallRecord.ONLY_REL_PERC:
        case CallRecord.KIDS_REL_PERC:
            return String.class;
        case CallRecord.ONLY_MSEC:
        case CallRecord.KIDS_MSEC:
        case CallRecord.START_TIME:
        case CallRecord.END_TIME:
        default:
            return Double.class;
        }
    }

    //----------------------------------------------------------------------
    //
    // Table sorting
    //
    //----------------------------------------------------------------------

    void addMouseListenerToHeaderInTable() { 
        table.setColumnSelectionAllowed(false); 
        MouseAdapter listMouseListener = new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                TableColumnModel columnModel = table.getColumnModel();
                int viewColumn = columnModel.getColumnIndexAtX(e.getX()); 
                int column = table.convertColumnIndexToModel(viewColumn); 
                if (e.getClickCount() == 1 && column != -1) {
                    setSortColumn(column);
                    sort();
                    updateTable(false);
                }
            }
        };
        JTableHeader th = table.getTableHeader(); 
        th.addMouseListener(listMouseListener); 
    }

    /**
     * Implements Comparator.compare
     */
    public int compare(Object o1, Object o2) {
        CallRecord rec1;
        CallRecord rec2;

        if (sortAscending) {
            rec1 = (CallRecord)o1;
            rec2 = (CallRecord)o2;
        } else {
            rec1 = (CallRecord)o2;
            rec2 = (CallRecord)o1;
        }

        switch (sortColumn) {
        case CallRecord.PARENT:
            if (isCallGraphNode()) {
                CallGraphNode parent1 = ((CallGraphNode)rec1).parent;
                CallGraphNode parent2 = ((CallGraphNode)rec2).parent;

                if (parent1 == null) {
                    return -1;
                } else if (parent2 == null) {
                    return 1;
                } else {
                    return parent1.name.compareTo(parent2.name);
                }
            } else {
                return 0;
            }
        case CallRecord.NAME:
            return rec1.name.compareTo(rec2.name);
        case CallRecord.COUNT:
            if (rec1.count == rec2.count) return 0;
            if (rec1.count > rec2.count) return 1;
            if (rec1.count < rec2.count) return -1;
        case CallRecord.DEPTH:
            return rec1.depth - rec2.depth;
        case CallRecord.ONLY_PERC:
            if (rec1.onlyPerc == rec2.onlyPerc) return 0;
            if (rec1.onlyPerc > rec2.onlyPerc) return 1;
            if (rec1.onlyPerc < rec2.onlyPerc) return -1;
        case CallRecord.ONLY_CYCLES:
        case CallRecord.ONLY_MSEC:
        case CallRecord.ONLY_REL_PERC:
            if (rec1.onlyCycles == rec2.onlyCycles) return 0;
            if (rec1.onlyCycles > rec2.onlyCycles) return 1;
            if (rec1.onlyCycles < rec2.onlyCycles) return -1;
        case CallRecord.AVG_CYCLES:
            long rec1_avgCycles = rec1.getAvgOnlyCycles();
            long rec2_avgCycles = rec2.getAvgOnlyCycles();
            if (rec1_avgCycles == rec2_avgCycles) return 0;
            if (rec1_avgCycles > rec2_avgCycles) return 1;
            if (rec1_avgCycles < rec2_avgCycles) return -1;
        case CallRecord.KIDS_PERC:
            if (rec1.kidsPerc == rec2.kidsPerc) return 0;
            if (rec1.kidsPerc > rec2.kidsPerc) return 1;
            if (rec1.kidsPerc < rec2.kidsPerc) return -1;
        case CallRecord.START_TIME:
            if (rec1.startTime == rec2.startTime) return 0;
            if (rec1.startTime > rec2.startTime) return 1;
            if (rec1.startTime < rec2.startTime) return -1;
        case CallRecord.END_TIME:
            if (rec1.endTime == rec2.endTime) return 0;
            if (rec1.endTime > rec2.endTime) return 1;
            if (rec1.endTime < rec2.endTime) return -1;
        case CallRecord.KIDS_CYCLES:
        case CallRecord.KIDS_MSEC:
        case CallRecord.KIDS_REL_PERC:
        default:
            if (rec1.kidsCycles == rec2.kidsCycles) return 0;
            if (rec1.kidsCycles > rec2.kidsCycles) return 1;
            if (rec1.kidsCycles < rec2.kidsCycles) return -1;
            return 0;  // will never happen, of course.
        }
    }

    protected int sortColumn = CallRecord.KIDS_CYCLES;
    protected boolean sortAscending = false;

    /**
     * Set the sorting column. If the same column is set twice consecutively,
     * the sorting direction is reversed.
     */
    public void setSortColumn(int visCol) {
        int column = toRealCol(visCol);
        
        if (sortColumn != column) {
            sortColumn = column;
            if (column == CallRecord.NAME || column == CallRecord.PARENT) {
                sortAscending = true;
            } else {
                sortAscending = false;
            }
        } else {
            sortAscending = !sortAscending;
        }
    }

    /**
     * Sort the data array using the current sortColumn.
     */
    public void sort() {
        CallRecord[] dataArray = getDataArray();
        java.util.List l = Arrays.asList(dataArray);
        Collections.sort(l, this);
        setDataArray((CallRecord[])l.toArray((Object[])dataArray));
    }

    /**
     * Export the current view of the table into a file.
     * IMPL_NOTE: make the file name selectable
     */
    public void export() {
        try {
            CallRecord[] dataArray = getDataArray();
            for (int i=0; i<dataArray.length; i++) {
                CallRecord cr = dataArray[i];
                String name = cr.name;
                System.out.println("Precompile = " + name);
            }
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }

    //----------------------------------------------------------------------
    //
    // Table Column Setup Frame
    //
    //----------------------------------------------------------------------

    SetupFrame setupFrame;

    void setup() {
        if (setupFrame == null) {
            setupFrame = new SetupFrame();
        }
        setupFrame.setVisible(true);
        setupFrame.toFront();
    }

    class SetupFrame extends JFrame implements ActionListener {
        JCheckBox buttons[];

        SetupFrame() {
            super("setup");

            // Put the check boxes in a column in a panel
            JPanel checkPanel = new JPanel();
            checkPanel.setLayout(new GridLayout(0, 1));

            buttons = new JCheckBox[names.length];
            for (int i=0; i<names.length; i++) {
                buttons[i] = new JCheckBox(names[i]);
                buttons[i].setSelected(colVisible[i]);

                checkPanel.add(buttons[i]);
            }

            JButton apply = new JButton("Apply");
            apply.setActionCommand("apply");
            apply.addActionListener(this);

            checkPanel.add(apply);

            this.getContentPane().add(checkPanel);
            this.pack();
            this.setLocation(300, 100);
            this.setVisible(true);

            //setLayout(new BorderLayout());
            //add(checkPanel, BorderLayout.WEST);
            //add(pictureLabel, BorderLayout.CENTER);
            //setBorder(BorderFactory.createEmptyBorder(20,20,20,20));
        }

        public void actionPerformed(java.awt.event.ActionEvent e) {
            if (e.getActionCommand().equals("apply")) {
                boolean changed = false;
                for (int i=0; i<buttons.length; i++) {
                    if (colVisible[i] != buttons[i].isSelected()) {
                        changed = true ;
                        colVisible[i] = buttons[i].isSelected();
                    }
                }
                if (changed) {
                    updateTable(true);
                }
                setVisible(false);
            }
        }
    }

    void setTableColumnWidths() {
        if (this.table == null) {
            return;
        }

        TableColumnModel colModel = table.getColumnModel();

        for (int realCol=0, visCol=0; realCol<names.length; realCol++) {
            if (!colVisible[realCol]) {
                continue;
            }
            int prefWidth;
            switch (realCol) {
            case CallRecord.PARENT:
            case CallRecord.NAME:
                prefWidth = 280;
                break;
            case CallRecord.COUNT:
            case CallRecord.ONLY_CYCLES:
            case CallRecord.AVG_CYCLES:
            case CallRecord.KIDS_CYCLES:
                prefWidth = 70;
                break;
            case CallRecord.DEPTH:
            case CallRecord.ONLY_MSEC:
            case CallRecord.ONLY_PERC:
            case CallRecord.ONLY_REL_PERC:
                prefWidth = 40;
                break;
            case CallRecord.KIDS_MSEC:
            case CallRecord.KIDS_PERC:
            case CallRecord.KIDS_REL_PERC:
            case CallRecord.START_TIME:
            case CallRecord.END_TIME:
            default:
                prefWidth = 50;
            }

            colModel.getColumn(visCol).setPreferredWidth(prefWidth);
            ++ visCol;
        }
    }

    public void updateTable(boolean colsChanged) {
        if (colsChanged) {
            // If the columns has changed, we must do the following
            // to force the table to refresh the column headers.
            table.setModel(new DefaultTableModel());
            table.updateUI();

            table.setModel(this);
            setTableColumnWidths();
        } else {
            table.setModel(this);
        }
        table.updateUI();
    }

    //----------------------------------------------------------------------
    //
    // Percentage viewing
    //
    //----------------------------------------------------------------------

    Object getPercObject(double val) {
        int ival = Math.abs((int)(val * 10000));
        int i = ival / 100;
        int f = ival % 100;

        StringBuffer sbuf = new StringBuffer();
        if (i < 100) {
            sbuf.append(" ");
            if (i < 10) {
                sbuf.append(" ");
            }
        }
        sbuf.append(val < 0 ? "-" : " ");

        sbuf.append(i);
        sbuf.append(".");
        if (f < 10) {
            sbuf.append("0");
        }
        sbuf.append(f);

        return sbuf.toString();
    }
}


class MainTableModel extends BaseTableModel {
    MainTableModel(ProfView viewer) {
        super(viewer);
        setColumnVisible(CallRecord.PARENT,        false);
        setColumnVisible(CallRecord.NAME,          true);
        setColumnVisible(CallRecord.COUNT,         true);
        setColumnVisible(CallRecord.DEPTH,         false);
        setColumnVisible(CallRecord.AVG_CYCLES,    true);
        setColumnVisible(CallRecord.ONLY_CYCLES,   true);
        setColumnVisible(CallRecord.ONLY_MSEC,     true);
        setColumnVisible(CallRecord.ONLY_REL_PERC, false);
        setColumnVisible(CallRecord.ONLY_PERC,     true);
        setColumnVisible(CallRecord.KIDS_CYCLES,   true);
        setColumnVisible(CallRecord.KIDS_MSEC,     true);
        setColumnVisible(CallRecord.KIDS_REL_PERC, false);
        setColumnVisible(CallRecord.KIDS_PERC,     true);
        setColumnVisible(CallRecord.START_TIME,    false);
        setColumnVisible(CallRecord.END_TIME,      false);
    }

    boolean isCallGraphNode() {
        return false;
    }

    CallGraphNode cgNode;

    CallRecord noRecord[] = new CallRecord[0];
    CallRecord singleRecord[] = new CallRecord[1];
    CallRecord children[];
    boolean showChildrenOnly;

    void setDataArray(CallRecord dataArray[]) {
        if (cgNode != null && cgNode.summary != null &&
            dataArray != lastFilterResult) {
            if (showChildrenOnly) {
                children = dataArray;
            } else {
                cgNode.summary = dataArray;
            }
        }
    }

    /**
     * Restrict the TableModel to display only the CallRecords that are
     * reachable (directly or indirectly) from this cgNode.
     */
    void setCurrentDataSet(CallGraphNode cgNode) {
        this.cgNode = cgNode;
        this.children = (cgNode == null || cgNode.children == null) ? null :
                        (CallRecord[]) cgNode.children.toArray(noRecord);
        sort();
    }

    Filter lastFilter;
    CallRecord lastFilterSrc[];
    CallRecord lastFilterResult[];

    CallRecord[] getDataArray() {
        CallRecord array[];

        if (cgNode == null) {
            return noRecord;
        } else if (cgNode.summary == null) {
            singleRecord[0] = cgNode;
            array = singleRecord;
        } else if (showChildrenOnly) {
            array = children;
        } else {
            array = cgNode.summary;
        }

        if (viewer.filter != null) {
            array = filterData(array, viewer.filter);
        }
        return array;
    }

    
    CallRecord[] filterData(CallRecord src[], Filter filter) {
        if (src == lastFilterSrc && filter.equals(lastFilter)) {
            return lastFilterResult;
        }

        lastFilter = filter;
        lastFilterSrc = src;

        Vector v = new Vector();
        for (int i = 0; i < src.length; i++) {
            CallRecord r = src[i];
            if (filter.recordMatches(r)) {
                v.addElement(r);
            }
        }

        CallRecord result[];
        if (src.length != v.size()) {
            if (v.size() == 0) {
                result = noRecord;
            } else {
                result = new CallRecord[v.size()];
                v.toArray((Object[])result);
            }
        } else {
            result = src;
        }

        lastFilterResult = result;
        return result;
    }
}

class GotoFrame extends JFrame implements ListSelectionListener,
                                          ActionListener
{
    Profile profile;
    JTable siteTable, stackTable;
    SiteTableModel siteTableModel;
    StackTableModel stackTableModel;
    ProfView viewer;
    JFormattedTextField levelText;

    GotoFrame(ProfView viewer, Profile profile) {
        this.viewer = viewer;
        this.profile = profile;

        Component sitePanel = createSitePanel();
        Component stackPanel = createStackPanel();

        JSplitPane splitPane = new JSplitPane(JSplitPane.VERTICAL_SPLIT);
        splitPane.setTopComponent(sitePanel);
        splitPane.setBottomComponent(stackPanel);

        this.getContentPane().add(splitPane, BorderLayout.CENTER);
        this.setLocation(70, 30);
        this.setSize(600, 500);
        this.pack();
    }

    Component createSitePanel() {
        // The table
        siteTableModel = new SiteTableModel(viewer);
        siteTable = new ProfileTable(siteTableModel, this);
        siteTableModel.setOwner(siteTable);

        siteTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        ListSelectionModel rowSM = siteTable.getSelectionModel();
        rowSM.addListSelectionListener(this);

        JScrollPane scrollPane = new JScrollPane(siteTable);
        scrollPane.setMinimumSize(new Dimension(180, 50));
        scrollPane.setPreferredSize(new Dimension(600, 300));

        // The button panel
        JPanel btnPanel = new JPanel();
        btnPanel.setLayout(new FlowLayout(FlowLayout.RIGHT));
        btnPanel.setBorder(BorderFactory.createEmptyBorder(2,2,2,2));

        JCheckBox merge = new JCheckBox("Merge callers"); 
        merge.setActionCommand("merge_callers");
        merge.addActionListener(this);

        // Number of levels in merge
        JLabel levelLabel = new JLabel("Levels: ", JLabel.LEFT);
        levelLabel.setAlignmentX(Component.CENTER_ALIGNMENT);

        java.text.NumberFormat numberFormat =
            java.text.NumberFormat.getIntegerInstance();
        NumberFormatter formatter = new NumberFormatter(numberFormat);
        formatter.setMinimum(new Integer(1));
        formatter.setMaximum(new Integer(10000));
        levelText = new JFormattedTextField(formatter);
        levelText.setValue(new Integer(1));
        levelText.setColumns(3);
        levelText.addActionListener(this);

        btnPanel.add(levelLabel);
        btnPanel.add(levelText);
        btnPanel.add(merge);
        
        // put everything together
        JPanel panel = new JPanel();
        panel.setLayout(new BorderLayout());
        panel.add(new JLabel("Call Sites"), BorderLayout.NORTH);
        panel.add(scrollPane, BorderLayout.CENTER);
        panel.add(btnPanel, BorderLayout.SOUTH);

        return panel;
    }

    Component createStackPanel() {
        // The table
        stackTableModel = new StackTableModel(viewer);
        stackTable = new ProfileTable(stackTableModel, this);
        stackTableModel.setOwner(stackTable);

        stackTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        //ListSelectionModel rowSM = stackTable.getSelectionModel();
        //rowSM.addListSelectionListener(this);

        JScrollPane scrollPane = new JScrollPane(stackTable);
        scrollPane.setMinimumSize(new Dimension(180, 50));
        scrollPane.setPreferredSize(new Dimension(600, 300));

        // put everything together
        JPanel panel = new JPanel();
        panel.setLayout(new BorderLayout());
        panel.add(new JLabel("Call Stack"), BorderLayout.NORTH);
        panel.add(scrollPane, BorderLayout.CENTER);

        return panel;
    }

    void display(String name) {
        siteTableModel.setCurrentDataSet(name);
        siteTableModel.updateTable(false);
        siteTableModel.sort();

        ListSelectionModel rowSM = siteTable.getSelectionModel();
        rowSM.clearSelection();

        this.setTitle("All occurances of " + name);
    }

    /**
     * Handles Table selection changes. Implements 
     * ListSelectionListener.valueChanged
     */
    public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting()) {
            return;
        }
        
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (!lsm.isSelectionEmpty()) {
            int selectedRow = lsm.getMinSelectionIndex();
            siteTableRowSelected(selectedRow);
        }
    }

    /**
     * User has selected an item in the site table.
     */
    void siteTableRowSelected(int row) {
        CallGraphNode n = siteTableModel.nodes[row];
        stackTableModel.setCurrentDataSet(n);
        stackTableModel.updateTable(false);

        ListSelectionModel lsm = stackTable.getSelectionModel();
        lsm.clearSelection();
        lsm.addSelectionInterval(0, 0);
    }

    class SiteTableModel extends BaseTableModel {
        String currentName = null;
        boolean merged = false;

        SiteTableModel(ProfView viewer) {
            super(viewer);
            setColumnVisible(CallRecord.PARENT,        true);
            setColumnVisible(CallRecord.NAME,          false);
            setColumnVisible(CallRecord.COUNT,         true);
            setColumnVisible(CallRecord.DEPTH,         true);
            setColumnVisible(CallRecord.ONLY_CYCLES,   false);
            setColumnVisible(CallRecord.ONLY_MSEC,     true);
            setColumnVisible(CallRecord.ONLY_REL_PERC, false);
            setColumnVisible(CallRecord.ONLY_PERC,     true);
            setColumnVisible(CallRecord.KIDS_CYCLES,   false);
            setColumnVisible(CallRecord.KIDS_MSEC,     true);
            setColumnVisible(CallRecord.KIDS_REL_PERC, false);
            setColumnVisible(CallRecord.KIDS_PERC,     true);
            setColumnVisible(CallRecord.START_TIME,    false);
            setColumnVisible(CallRecord.END_TIME,      false);

            setSortColumn(CallRecord.KIDS_PERC);
        }

        boolean isCallGraphNode() {
            return true;
        }

        /**
         * All CallGraphNode to be displayed in this window
         */
        CallGraphNode nodes[] = new CallGraphNode[0];

        CallRecord[] getDataArray() {
            return nodes;
        }
        void setDataArray(CallRecord dataArray[]) {
            nodes = (CallGraphNode[])dataArray;
        }

        /**
         * Restrict this TableModel to display only the CallRecords with
         * the given name.
         */
        void setCurrentDataSet(String name) {
            Object obj = profile.nodesByName.get(name);
            if (obj == null) {
                this.nodes = new CallGraphNode[0];
            }
            else if (obj instanceof CallGraphNode) {
                this.nodes = new CallGraphNode[1];
                this.nodes[0] = (CallGraphNode)obj;
            }
            else {
                Vector v = (Vector)obj;
                if (merged) {
                    v = mergeParents(v);
                }
                this.nodes = new CallGraphNode[v.size()];
                v.toArray((Object[])nodes);
            }
            currentName = name;
        }

        void updateCurrentDataSet(boolean merged) {
            if (currentName != null) {
                this.merged = merged;
                setCurrentDataSet(currentName);
            }
        }

        boolean sameAncestors(CallGraphNode n1, CallGraphNode n2, int levels) {
            for (int i=0; i<levels; i++) {
                String name1 = null, name2 = null;

                if (n1.parent == null && n2.parent == null) {
                    // both n1 and n2 are exactly the same call stack. Why
                    // will this happen??
                    return true;
                }
                if (n1.parent != null) {
                    name1 = n1.parent.name;
                } else {
                    return false;
                }
                if (n2.parent != null) {
                    name2 = n2.parent.name;
                } else {
                    return false;
                }

                if (!name1.equals(name2)) {
                    return false;
                }
                n1 = n1.parent;
                n2 = n2.parent;
            }
            return true;
        }

        /**
         * Create a list of "fake" parent nodes -- if function P may call 
         * function C with different call stacks, create a new node that 
         * combins all those nodes into a single node. This makes it easy
         * to tell how much time is spent in the P->C call, under all
         * circumstances
         */
        Vector mergeParents(Vector nodes) {
            int mergeLevels = 3;
            int i, j;
            Vector mp = new Vector();
            for (i=0; i<nodes.size(); i++) {
                CallGraphNode n1 = (CallGraphNode)nodes.elementAt(i);
                boolean merged = false;
                for (j=0; j<mp.size(); j++) {
                    CallGraphNode n2 = (CallGraphNode)mp.elementAt(j);
                    if (sameAncestors(n1, n2, mergeLevels)) {
                        if (!(n2 instanceof MergedCallGraphNode)) {
                            n2 = new MergedCallGraphNode(n2, mergeLevels);
                            mp.set(j, n2);
                        }

                        ((MergedCallGraphNode)n2).add(n1, true, mergeLevels);
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    mp.addElement(n1);
                }
            }

            return mp;
        }
    }

    class StackTableModel extends BaseTableModel {
        StackTableModel(ProfView viewer) {
            super(viewer);

            setColumnVisible(CallRecord.PARENT,        false);
            setColumnVisible(CallRecord.NAME,          true);
            setColumnVisible(CallRecord.COUNT,         true);
            setColumnVisible(CallRecord.DEPTH,         true);
            setColumnVisible(CallRecord.ONLY_CYCLES,   false);
            setColumnVisible(CallRecord.ONLY_MSEC,     true);
            setColumnVisible(CallRecord.ONLY_REL_PERC, false);
            setColumnVisible(CallRecord.ONLY_PERC,     true);
            setColumnVisible(CallRecord.KIDS_CYCLES,   false);
            setColumnVisible(CallRecord.KIDS_MSEC,     true);
            setColumnVisible(CallRecord.KIDS_REL_PERC, false);
            setColumnVisible(CallRecord.KIDS_PERC,     true);
            setColumnVisible(CallRecord.START_TIME,    false);
            setColumnVisible(CallRecord.END_TIME,      false);

            setSortColumn(CallRecord.DEPTH);
        }

        boolean isCallGraphNode() {
            return true;
        }

        /**
         * All CallGraphNode to be displayed in this window
         */
        CallGraphNode nodes[] = new CallGraphNode[0];

        CallRecord[] getDataArray() {
            return nodes;
        }
        void setDataArray(CallRecord dataArray[]) {
            nodes = (CallGraphNode[])dataArray;
        }

        /**
         * Restrict this TableModel to display only the CallRecords with
         * the given name.
         */
        void setCurrentDataSet(CallGraphNode cgNode) {
            Vector v = new Vector();
            for (;
                 cgNode != null && cgNode.parent != null;
                 cgNode = cgNode.parent) {

                v.addElement(cgNode);
            }

            nodes = new CallGraphNode[v.size()];
            nodes = (CallGraphNode[])v.toArray((Object[])nodes);
        }
    }

    public void actionPerformed(java.awt.event.ActionEvent e) {
        String command = e.getActionCommand();

        if (command.equals("merge_callers")) {
            JCheckBox cb = (JCheckBox)e.getSource();
            siteTableModel.updateCurrentDataSet(cb.isSelected());
            siteTableModel.updateTable(false);
        } 
        else if (command.equals("table_dblclick")) {
            SyncCallGraph((ProfileTable)e.getSource());
        };
    }

    CallGraphNode getNodeFromTable(ProfileTable table, int row) {
        BaseTableModel dataModel = (BaseTableModel)table.getModel();
        return ((CallGraphNode[])dataModel.getDataArray())[row];
    }
    
    void SyncCallGraph(ProfileTable table) {
        ListSelectionModel lsm = table.getSelectionModel();
        if (!lsm.isSelectionEmpty()) {
            int selectedRow = lsm.getMinSelectionIndex();
            if (selectedRow >= 0) {
                CallGraphNode cgNode = getNodeFromTable(table, selectedRow);
                if (table == siteTable) {
                    cgNode = cgNode.parent;
                }
                viewer.gotoGraph(cgNode);
                viewer.frame.toFront();
            }
        }
    }
}

/**
 * This window displays all functions reachable from a function X, regardless
 * of the call stack of X.
 */
class CalleeFrame extends JFrame implements ActionListener
{
    Profile profile;
    JTable calleeTable;
    CalleeTableModel calleeTableModel;
    ProfView viewer;

    CalleeFrame(ProfView viewer, Profile profile) {
        this.viewer = viewer;
        this.profile = profile;

        Component calleePanel = createCalleePanel();
        this.getContentPane().add(calleePanel, BorderLayout.CENTER);
        this.setLocation(30, 80);
        this.setSize(600, 500);
        this.pack();
    }

    Component createCalleePanel() {
        // The table
        calleeTableModel = new CalleeTableModel(viewer);
        calleeTable = new ProfileTable(calleeTableModel, null);
        calleeTableModel.setOwner(calleeTable);
        calleeTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        JScrollPane scrollPane = new JScrollPane(calleeTable);
        scrollPane.setMinimumSize(new Dimension(180, 50));
        scrollPane.setPreferredSize(new Dimension(600, 300));

        // put everything together
        JPanel panel = new JPanel();
        panel.setLayout(new BorderLayout());
        panel.add(new JLabel("Callees"), BorderLayout.NORTH);
        panel.add(scrollPane, BorderLayout.CENTER);

        return panel;
    }

    void display(String name) {
        calleeTableModel.setCurrentDataSet(name);
        calleeTableModel.updateTable(false);
        calleeTableModel.sort();

        ListSelectionModel rowSM = calleeTable.getSelectionModel();
        rowSM.clearSelection();

        this.setTitle("All callees of " + name);
    }

    class CalleeTableModel extends BaseTableModel {
        String currentName = null;

        CalleeTableModel(ProfView viewer) {
            super(viewer);
            setColumnVisible(CallRecord.PARENT,        false);
            setColumnVisible(CallRecord.NAME,          true);
            setColumnVisible(CallRecord.COUNT,         true);
            setColumnVisible(CallRecord.DEPTH,         false);
            setColumnVisible(CallRecord.ONLY_CYCLES,   false);
            setColumnVisible(CallRecord.ONLY_MSEC,     true);
            setColumnVisible(CallRecord.ONLY_REL_PERC, false);
            setColumnVisible(CallRecord.ONLY_PERC,     true);
            setColumnVisible(CallRecord.KIDS_CYCLES,   false);
            setColumnVisible(CallRecord.KIDS_MSEC,     true);
            setColumnVisible(CallRecord.KIDS_REL_PERC, false);
            setColumnVisible(CallRecord.KIDS_PERC,     true);
            setColumnVisible(CallRecord.START_TIME,    false);
            setColumnVisible(CallRecord.END_TIME,      false);

            setSortColumn(CallRecord.KIDS_PERC);
        }

        boolean isCallGraphNode() {
            return false;
        }

        /**
         * All CallGraphNode to be displayed in this window
         */
        CallRecord nodes[] = new CallRecord[0];

        CallRecord[] getDataArray() {
            return nodes;
        }
        void setDataArray(CallRecord dataArray[]) {
            nodes = dataArray;
        }

        /**
         * Restrict this TableModel to display only the CallRecords with
         * the given name.
         */
        void setCurrentDataSet(String name) {
            Object obj = profile.nodesByName.get(name);
            if (obj == null) {
                this.nodes = new CallRecord[0];
            } else {
                Hashtable h = new Hashtable();

                if (obj instanceof CallGraphNode) {
                    addToDataSet(h, (CallGraphNode)obj);
                } else {
                    Vector v = (Vector)obj;
                    for (int i=0; i<v.size(); i++) {
                        addToDataSet(h, (CallGraphNode)v.elementAt(i));
                    }
                }

                this.nodes = new CallRecord[h.size()];
                int i=0;
                for (Enumeration e = h.elements();
                     e.hasMoreElements(); i++) {
                    this.nodes[i] = (CallRecord)(e.nextElement());
                }
            }
            currentName = name;
        }

        /**
         * Merge all callees of function X, regardless how we get to X.
         */
        void addToDataSet(Hashtable h, CallGraphNode node) {
            if (node == null || node.summary == null) {
                return;
            }

            CallRecord summary[] = node.summary;
            for (int i=0; i<summary.length; i++) {
                CallRecord newRec = summary[i];
                CallRecord oldRec = (CallRecord)h.get(newRec.name);
                if (oldRec == null) {
                    h.put(newRec.name, newRec);
                } else {
                    if (!(oldRec instanceof MergedCallRecord)) {
                        MergedCallRecord mc = new MergedCallRecord(oldRec.owner);
                        mc.name = oldRec.name;
                        mc.add(oldRec, true);
                        oldRec = mc;
                        h.put(oldRec.name, oldRec);
                    }
                    oldRec.add(newRec, true);
                }
            }
        }
    }

    public void actionPerformed(java.awt.event.ActionEvent e) {
        String command = e.getActionCommand();

        if (command.equals("setup_callee_table")) {
            calleeTableModel.setup();
        } 
    }
}

class MyTreeNode extends DefaultMutableTreeNode {
    CallGraphNode cgNode;
    MyTreeNode(CallGraphNode cgNode) {
        super(cgNode.name + 
              " [" + cgNode.onlyCycles + 
              "/"  + cgNode.kidsCycles + "]");
        this.cgNode = cgNode;
    }
}

class ProfileTable extends JTable implements ActionListener, MouseListener,
                                             TableCellRenderer {
    JPopupMenu popup;
    JRadioButtonMenuItem btnAbsolute, btnRelative, btnBoth, btnUnknown;
    JCheckBoxMenuItem btnTimeLine, btnChildrenOnly;
    ActionListener doubleClickListener;
    FontMetrics fm;
    TableColumnModel tcm;
    boolean showToolTip;
    TableCellRenderer renderer;

    void addMenuItem(String caption, String command) {
      JMenuItem item = popup.add(caption);
      item.setActionCommand(command);
      item.addActionListener(this);
    }

    JRadioButtonMenuItem addRadioMenuItem(ButtonGroup group,
                                          String caption, String command) {
      JRadioButtonMenuItem item = new JRadioButtonMenuItem(caption);
      item.setActionCommand(command);
      item.addActionListener(this);
      group.add(item);
      popup.add(item);
      return item;
    }

    JCheckBoxMenuItem addCheckBoxMenuItem(String caption, String command) {
      JCheckBoxMenuItem item = new JCheckBoxMenuItem(caption);
      item.setActionCommand(command);
      item.addActionListener(this);
      popup.add(item);
      return item;
    }

    int isVisible(int column, int score) {
        return ((BaseTableModel)dataModel).colVisible[column] ? score : 0;
    }
    
    void maybePopup(MouseEvent e) {
        if (e.isPopupTrigger()) {
            switch(isVisible(CallRecord.ONLY_PERC,     1) +
                   isVisible(CallRecord.KIDS_PERC,     2) +
                   isVisible(CallRecord.ONLY_REL_PERC, 4) +
                   isVisible(CallRecord.KIDS_REL_PERC, 8)) {
            case 3:
                btnAbsolute.setSelected(true);
                break;
            case 12:
                btnRelative.setSelected(true);
                break;
            case 15:
                btnBoth.setSelected(true);
                break;
            default:
                btnUnknown.setSelected(true);
            }
            popup.show(e.getComponent(), e.getX(), e.getY());
        }
    }

    ProfileTable(AbstractTableModel model, ActionListener doubleClickListener) {
        super(model);
        fm = getFontMetrics(getFont());
        tcm = getColumnModel();
        this.doubleClickListener = doubleClickListener;
        addMouseListener(this);

        popup = new JPopupMenu();
        addMenuItem("Columns...", "table_columns");
        ButtonGroup group = new ButtonGroup();
        btnAbsolute = addRadioMenuItem(group, "Absolute percentage", "percents");
        btnRelative = addRadioMenuItem(group, "Relative percentage", "percents");
        btnBoth = addRadioMenuItem(group, "Both", "percents");
        group.add(btnUnknown = new JRadioButtonMenuItem(""));
        btnTimeLine = addCheckBoxMenuItem("Timeline data", "timeline");
        if (model instanceof MainTableModel) {
            btnChildrenOnly = addCheckBoxMenuItem("Show children only", "children");
        }
        popup.addSeparator();
        addMenuItem("Export for precompilation", "table_precompile");
    }

    public TableCellRenderer getCellRenderer(int row, int column) {
        renderer = super.getCellRenderer(row, column);
        return this;
    }
    
    public Component getTableCellRendererComponent(JTable table, Object value,
                                                   boolean isSelected,
                                                   boolean hasFocus,
                                                   int row, int column) {
        Component c = renderer.getTableCellRendererComponent(
            table, value, isSelected, hasFocus, row, column
        );
        CallRecord[] data = ((BaseTableModel)dataModel).getDataArray();
        if (data[row].deltaBase == null) {
            c.setForeground(ProfView.DEFAULT_COLOR);
        } else {
            c.setForeground(ProfView.DELTA_COLOR);
        }
        return c;
    }

    public void actionPerformed(ActionEvent e) {
        String command = e.getActionCommand();
        BaseTableModel tableModel = (BaseTableModel)dataModel;
        if (command.equals("table_columns")) {
            tableModel.setup();
        } else if (command.equals("table_precompile")) {
            tableModel.export();
        } else if (command.equals("percents")) {
            Object source = e.getSource();
            boolean absolute = source == btnAbsolute || source == btnBoth;
            boolean relative = source == btnRelative || source == btnBoth;
            tableModel.setColumnVisible(CallRecord.ONLY_PERC, absolute);
            tableModel.setColumnVisible(CallRecord.KIDS_PERC, absolute);
            tableModel.setColumnVisible(CallRecord.ONLY_REL_PERC, relative);
            tableModel.setColumnVisible(CallRecord.KIDS_REL_PERC, relative);
            tableModel.updateTable(true);
        } else if (command.equals("timeline")) {
            boolean show = btnTimeLine.getState();
            tableModel.setColumnVisible(CallRecord.START_TIME, show);
            tableModel.setColumnVisible(CallRecord.END_TIME, show);
            tableModel.updateTable(true);
        } else if (command.equals("children")) {
            ((MainTableModel)tableModel).showChildrenOnly =
                btnChildrenOnly.getState();
            tableModel.updateTable(false);
        }
    }

    public void mouseClicked(MouseEvent e) {
        Point p = e.getPoint();
        int row = rowAtPoint(p);
        int col = columnAtPoint(p);
        String s = getValueAt(row, col).toString(); 
        Clipboard clp = Toolkit.getDefaultToolkit().getSystemSelection();
        if (clp == null) {
            clp = Toolkit.getDefaultToolkit().getSystemClipboard();
        }
        clp.setContents(new StringSelection(s), null);
        if (e.getClickCount() == 2 && doubleClickListener != null) {
            doubleClickListener.actionPerformed(
                new ActionEvent(this, 0, "table_dblclick", e.getModifiers())
            );
        }
    }

    public void mouseReleased(MouseEvent e) {
        maybePopup(e);
    }
    
    public void mousePressed(MouseEvent e) {
        maybePopup(e);
    }

    public void mouseEntered(MouseEvent e) {}
    public void mouseExited(MouseEvent e) {}

    public String getToolTipText(MouseEvent e) {
        Point p = e.getPoint();
        int row = rowAtPoint(p);
        int col = columnAtPoint(p);

        String s = getValueAt(row, col).toString();
        showToolTip = fm.stringWidth(s) + 3 > tcm.getColumn(col).getWidth();
        return showToolTip ? s : null;
    }

    public Point getToolTipLocation(MouseEvent e) {
        if (showToolTip) {
            Point p = e.getPoint();
            int row = rowAtPoint(p);
            int col = columnAtPoint(p);
            return getCellRect(row, col, false).getLocation();
        } else {
            return null;
        }
    }
}

class ProfileTreeCellRenderer extends DefaultTreeCellRenderer {
    ImageIcon iconJavaInt, iconJavaComp;

    static ImageIcon createImageIcon(String path) {
        java.net.URL imgURL = ProfileTreeCellRenderer.class.getResource(path);
        if (imgURL == null) {
            System.out.println("WARNING: Couldn't find resource: " + path);
            return null;
        }
        return new ImageIcon(imgURL);
    }

    ProfileTreeCellRenderer() {
        super();
        iconJavaInt  = createImageIcon("icons/java_int.gif");
        iconJavaComp = createImageIcon("icons/java_comp.gif");
        ImageIcon iconMethod = createImageIcon("icons/method.gif");
        setOpenIcon(iconMethod);
        setClosedIcon(iconMethod);
        setLeafIcon(createImageIcon("icons/method_leaf.gif"));
    }

    public Component getTreeCellRendererComponent(JTree tree, Object value,
                                                  boolean selected,
                                                  boolean expanded,
                                                  boolean leaf, int row,
                                                  boolean hasFocus) {
        super.getTreeCellRendererComponent(
            tree, value, selected, expanded, leaf, row, hasFocus
        );
        CallGraphNode cgNode = ((MyTreeNode)value).cgNode;
        if (cgNode.deltaBase == null) {
            setForeground(ProfView.DEFAULT_COLOR);
        } else {
            setForeground(ProfView.DELTA_COLOR);
        }
        if (cgNode.name.endsWith("<i>")) {
            setIcon(iconJavaInt);
        } else if (cgNode.name.endsWith("<c>")) {
            setIcon(iconJavaComp);
        }
        return this;
    }

}

class ProfileTree extends JTree implements MouseListener {
    boolean showToolTip;
    Rectangle rect;
    int indent;
    JPopupMenu popup;
    
    public ProfileTree(TreeNode root) {
        super(root);
        ProfileTreeCellRenderer renderer = new ProfileTreeCellRenderer();
        indent = renderer.getOpenIcon().getIconWidth();
        setCellRenderer(renderer);
        ToolTipManager.sharedInstance().registerComponent(this);

        popup = new JPopupMenu();
        popup.add("Export to...").setActionCommand("tree_export");
        popup.addSeparator();
        popup.add("Compare to...").setActionCommand("tree_delta");
        popup.add("Restore original values").setActionCommand("tree_restore");
        addMouseListener(this);
    }

    public void addTreeListener(ActionListener listener) {
        MenuElement[] items = popup.getSubElements();
        for (int i = 0; i < items.length; i++) {
            ((JMenuItem)items[i]).addActionListener(listener);
        }
    }

    public void mouseReleased(MouseEvent e) {
        maybePopup(e);
    }
    
    public void mousePressed(MouseEvent e) {
        maybePopup(e);
    }

    public void mouseClicked(MouseEvent e) {}
    public void mouseEntered(MouseEvent e) {}
    public void mouseExited(MouseEvent e) {}

    void maybePopup(MouseEvent e) {
        if (e.isPopupTrigger()) {
            TreePath path = getPathForLocation(e.getX(), e.getY());
            if (path != null) {
                setSelectionPath(path);
                popup.show(e.getComponent(), e.getX(), e.getY());
            }
        }
    }

    public String getToolTipText(MouseEvent e) {
        TreePath path = getPathForLocation(e.getX(), e.getY());
        if (path == null) {
            showToolTip = false;
        } else {
            rect = getPathBounds(path);
            showToolTip = rect.getX() + rect.getWidth() >
                          getVisibleRect().getWidth();
        }
        return showToolTip ? path.getLastPathComponent().toString() : null;
    }

    public Point getToolTipLocation(MouseEvent e) {
        if (showToolTip) {
            Point p = rect.getLocation();
            p.translate(indent, 0);
            return p;
        } else {
            return null;
        }
    }
}

class ProfileWriter {
    CallGraphNode root;
    boolean normalize;
    int rootIndex;

    ProfileWriter(CallGraphNode root, boolean needToNormalize) {
        this.root = root;
        this.normalize = needToNormalize;
        this.rootIndex = root.index.intValue();
    }

    // Normalizes indices of the graph nodes,
    // so the root node will always be -1 and the other nodes from
    // the multifile profile will obtain their original values
    int getNormalizedIndex(Integer idx) {
        int i = idx.intValue();
        if (!normalize) {
            return i;
        }

        return i == rootIndex ? -1 : i % FileParserContext.MAX_INDEX;
    }
    
    void printNode(PrintStream out, CallGraphNode cgNode) {
        out.print(getNormalizedIndex(cgNode.index));
        out.print('\t');
        out.print(getNormalizedIndex(cgNode.parentIdx));
        out.print('\t');
        out.print(cgNode.depth);
        out.print('\t');
        out.print(cgNode.name);
        out.print('\t');
        out.print(cgNode.count);
        out.print('\t');
        out.print(cgNode.onlyCycles);
        out.print('\t');
        out.print(cgNode.onlyMsec);
        out.print('\t');
        out.print(cgNode.onlyPerc);
        out.print('\t');
        out.print(cgNode.kidsCycles);
        out.print('\t');
        out.print(cgNode.kidsMsec);
        out.print('\t');
        out.print(cgNode.kidsPerc);
        out.print('\t');
        out.print(cgNode.startTime);
        out.print('\t');
        out.print(cgNode.endTime);
        out.println("\t0\t0"); // dummies
        
        Vector v = cgNode.children;
        if (v != null) {
            for (int i = 0; i < v.size(); i++) {
                printNode(out, (CallGraphNode)v.elementAt(i));
            }
        }
    }

    void exportTo(File file) {
        PrintStream out;
        try {
            out = new PrintStream(new FileOutputStream(file));
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return;
        }
        out.println("# Generated by ProfView");
        printNode(out, root);
        out.close();
    }
}

public class ProfView implements TreeSelectionListener, ListSelectionListener,
                                 DocumentListener, ActionListener,
                                 TimeLineListener
{
    public static void main(String argv[]) {
        if (argv.length == 0) {
            System.out.println("No graph file specified!");
            System.out.println("Usage: java -cp ProfView.jar ProfView graphfile1 ... graphfileN");
            System.exit(1);
        }

        ProfView viewer = new ProfView();
        viewer.start(argv);
    }

    final static Color DELTA_COLOR = new Color(0, 0, 160);
    final static Color DEFAULT_COLOR = Color.BLACK;

    JFrame frame;
    ProfileTree tree;
    JTable table;
    MainTableModel tableModel;
    GotoFrame gotoFrame;
    CalleeFrame calleeFrame;
    Profile profile;
    JLabel tableLabel;
    JCheckBox geomMean;
    MyTreeNode topTreeNode;
    MyTreeNode currentTreeNode;
    CallGraphNode deltaTarget;
    JFrame deltaPrompt;
    TimeLine timeline;

    javax.swing.Timer timer;
    JTextField filterText;
    Filter filter;

    void start(String[] files) {
        try {
            // UIManager.setLookAndFeel(
            //     "com.sun.java.swing.plaf.windows.WindowsLookAndFeel");

            this.profile = new Profile(files);
            
            String caption = files[0];
            for (int i = 1; i < files.length; i++) {
                caption += " + " + files[i];
            }
            frame = new JFrame("ProfView " + caption);

            JPanel treePanel = createTreePanel(profile);
            JPanel tablePanel = createTablePanel(profile);

            JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT);
            splitPane.setTopComponent(treePanel);
            splitPane.setBottomComponent(tablePanel);

            frame.getContentPane().add(splitPane, BorderLayout.CENTER);

            frame.pack();
            frame.setLocation(50, 50);
            frame.setSize(900, 600);
            frame.setVisible(true);

            frame.addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    System.exit(0);
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    JScrollPane createTableView(Profile profile) {
        tableModel = new MainTableModel(this);
        table = new ProfileTable(tableModel, this);
        tableModel.setCurrentDataSet(profile.root);
        tableModel.setOwner(table);

        JScrollPane tableView = new JScrollPane(table);
        tableView.setMinimumSize(new Dimension(180, 400));
        tableView.setPreferredSize(new Dimension(680, 400));

        table.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

        ListSelectionModel rowSM = table.getSelectionModel();
        rowSM.addListSelectionListener(this);

        return tableView;
    }

    JPanel createTablePanel(Profile profile) {
        // The table scroll pane
        JScrollPane tableView = createTableView(profile);
        
        // The button panel
        JPanel btnPanel = new JPanel();
        GridBagLayout gridbag = new GridBagLayout();
        GridBagConstraints c = new GridBagConstraints();

        btnPanel.setLayout(gridbag);
        btnPanel.setBorder(BorderFactory.createEmptyBorder(2,2,2,2));

        filterText = new JTextField(20);
        filterText.setActionCommand("filter_main_table");
        filterText.addActionListener(this);
        filterText.getDocument().addDocumentListener(this);

        JLabel filterLabel = new JLabel("Filter:");
        filterLabel.setLabelFor(filterText);

        // used for filter updating after filterText change
        timer = new javax.swing.Timer(500, this);

        JButton callee = new JButton("Callees");
        callee.setActionCommand("callee_main_table");
        callee.addActionListener(this);

        JButton caller = new JButton("Callers");
        caller.setActionCommand("caller_main_table");
        caller.addActionListener(this);

        timeline = new TimeLine(profile.period);
        timeline.setTimeLineListener(this);

        c.insets = new Insets(2, 2, 2, 2);
        c.fill = GridBagConstraints.HORIZONTAL;
        c.gridwidth = GridBagConstraints.REMAINDER;
        c.weightx = 1.0;
        gridbag.setConstraints(timeline, c);
        btnPanel.add(timeline);
        
        c.gridwidth = 1;
        c.weightx = 0.0;
        gridbag.setConstraints(filterLabel, c);
        btnPanel.add(filterLabel);

        c.weightx = 1.0;
        gridbag.setConstraints(filterText, c);
        btnPanel.add(filterText);

        c.weightx = 0.0;
        gridbag.setConstraints(callee, c);
        btnPanel.add(callee);

        gridbag.setConstraints(caller, c);
        btnPanel.add(caller);

        // Put them together
        JPanel tablePanel = new JPanel();
        tablePanel.setLayout(new BorderLayout());
        tableLabel = new JLabel("All calls under <root>");
        tablePanel.add(tableLabel, BorderLayout.NORTH);
        tablePanel.add(btnPanel, BorderLayout.SOUTH);
        tablePanel.add(tableView, BorderLayout.CENTER);

        return tablePanel;
    }

    JPanel createTreePanel(Profile profile) {
        currentTreeNode = topTreeNode = createNodes(profile.root);

        tree = new ProfileTree(topTreeNode);
        tree.addTreeSelectionListener(this);
        tree.addTreeListener(this);
        tree.getSelectionModel().setSelectionMode
            (TreeSelectionModel.SINGLE_TREE_SELECTION);

        JScrollPane scrollPane = new JScrollPane(tree);
        scrollPane.setMinimumSize(new Dimension(180, 400));
        scrollPane.setPreferredSize(new Dimension(600, 500));

        JPanel panel = new JPanel();
        panel.setLayout(new BorderLayout());
        panel.add(new JLabel("Call Graph"), BorderLayout.NORTH);
        panel.add(scrollPane, BorderLayout.CENTER);
        
        if (profile.isMultifile()) {
            // "Geom mean" checkbox is efficient only for multifile profiles
            geomMean = new JCheckBox("Merge using geom mean");
            geomMean.setActionCommand("geom_mean_click");
            geomMean.addActionListener(this);
            JPanel mergePanel = new JPanel();
            mergePanel.setBorder(BorderFactory.createEmptyBorder(2,2,2,2));
            mergePanel.add(geomMean);
            panel.add(mergePanel, BorderLayout.SOUTH);
        }

        return panel;
    }

    MyTreeNode createNodes(CallGraphNode cgNode) {
        MyTreeNode trNode = new MyTreeNode(cgNode);
        if (cgNode.children != null) {
            Vector v = cgNode.children;
            for (int i=0; i<v.size(); i++) {
                CallGraphNode child = (CallGraphNode)v.elementAt(i);
                trNode.add(createNodes(child));
            }
        }

        return trNode;
    }

    /**
     * Handles Tree selection changes.
     */
    public void valueChanged(TreeSelectionEvent e) {
        MyTreeNode n =  (MyTreeNode) tree.getLastSelectedPathComponent();
        if (n != currentTreeNode) {
            currentTreeNode = n;
            if (deltaTarget != null) {
                currentTreeNode.cgNode.setDelta(deltaTarget);
                deltaTarget = null;
                deltaPrompt.setVisible(false);
            }
            refreshTable();
        }
    }
        
    void refreshTable() {
        if (currentTreeNode == null) {
            return;
        }

        tableLabel.setText("All calls under " + currentTreeNode.cgNode.name);

        tableModel.setCurrentDataSet(currentTreeNode.cgNode);
        tableModel.updateTable(false);

        ListSelectionModel rowSM = table.getSelectionModel();
        rowSM.clearSelection();
    }

    /**
     * Handles Table selection changes. Implements 
     * ListSelectionListener.valueChanged
     */
    public void valueChanged(ListSelectionEvent e) {
        if (e.getValueIsAdjusting()) {
            return;
        }
        
        ListSelectionModel lsm = (ListSelectionModel)e.getSource();
        if (!lsm.isSelectionEmpty()) {
            int selectedRow = lsm.getMinSelectionIndex();
            tableRowSelected(selectedRow);
        }
    }

    void tableRowSelected(int row) {
        CallRecord rec = tableModel.getRecordByRow(row);
        timeline.select(rec.startTime, rec.endTime);
        if (gotoFrame != null && gotoFrame.isVisible()) {
            gotoFrame.display(rec.name);
        }
        if (calleeFrame != null && calleeFrame.isVisible()) {
            calleeFrame.display(rec.name);
        }
    }

    /**
     * Handles filter changes. Implements DocumentListener
     */
    public void insertUpdate(javax.swing.event.DocumentEvent e) {
        timer.stop();
        timer.start();
    }

    public void removeUpdate(javax.swing.event.DocumentEvent e) {
        timer.stop();
        timer.start();
    }

    public void changedUpdate(javax.swing.event.DocumentEvent e) {}

    /**
     * Handle TimeLine actions
     */
    public void markerChanged() {
        timer.stop();
        timer.start();
    }

    /**
     * Handle general actions
     */
    public void actionPerformed(java.awt.event.ActionEvent e) {
        String command = e.getActionCommand(); // null means Timer event
        boolean alt = (e.getModifiers() & e.SHIFT_MASK) != 0;
        if (command == null || command.equals("filter_main_table")) {
            timer.stop();
            Filter f = new Filter(filterText.getText(),
                                  timeline.getMarkedTime());
            if (!f.equals(filter)) {
                filter = f.isEmpty() ? null : f;
                refreshTable();
            }
        }
        else if (command.equals("caller_main_table") ||
                 command.equals("table_dblclick") && !alt) {
            if (gotoFrame == null) {
                gotoFrame = new GotoFrame(this, profile);
            }
            if (!gotoFrame.isVisible()) {
                gotoFrame.setVisible(true);
                ListSelectionModel lsm = table.getSelectionModel();
                if (!lsm.isSelectionEmpty()) {
                    tableRowSelected(lsm.getMinSelectionIndex());
                }
            }
            ListSelectionModel lsm = gotoFrame.siteTable.getSelectionModel();
            lsm.clearSelection();
            lsm.addSelectionInterval(0, 0);
            gotoFrame.toFront();
        }
        else if (command.equals("callee_main_table") ||
                 command.equals("table_dblclick") && alt) {
            if (calleeFrame == null) {
                calleeFrame = new CalleeFrame(this, profile);
            }
            if (!calleeFrame.isVisible()) {
                calleeFrame.setVisible(true);
                ListSelectionModel lsm = table.getSelectionModel();
                if (!lsm.isSelectionEmpty()) {
                    tableRowSelected(lsm.getMinSelectionIndex());
                }
            }
            calleeFrame.toFront();
        }
        else if (command.equals("geom_mean_click")) {
            AveragedCallRecord.averagingMethod = 
                ((JCheckBox)e.getSource()).isSelected() ?
                    AveragedCallRecord.GEOMETRIC_MEAN :
                    AveragedCallRecord.ARITHMETIC_MEAN;
            profile.root.computePercentage();
            refreshTable();
        }
        else if (command.equals("tree_export")) {
            File file = chooseFileName(
                "Export " + currentTreeNode.cgNode.name + " subtree to");
            if (file != null) {
                boolean needToNormalize = currentTreeNode != topTreeNode;
                (new ProfileWriter(currentTreeNode.cgNode, needToNormalize)).
                    exportTo(file);
            }
        }
        else if (command.equals("tree_delta")) {
            deltaTarget = currentTreeNode.cgNode;
            showDeltaPrompt();
        }
        else if (command.equals("tree_cancel")) {
            deltaTarget = null;
            deltaPrompt.setVisible(false);
        }
        else if (command.equals("tree_restore")) {
            currentTreeNode.cgNode.clearDelta();
            refreshTable();
        }
    }

    void showDeltaPrompt() {
        if (deltaPrompt == null) {
            deltaPrompt = new JFrame("Calculate profiles Delta");
            JButton cancel = new JButton("Cancel");
            cancel.setActionCommand("tree_cancel");
            cancel.addActionListener(this);

            Container pane = deltaPrompt.getContentPane();
            pane.setLayout(new GridLayout(2, 1));
            pane.add(new JLabel(" Please, click the tree node" +
                                " that will be compared to the selection "));
            pane.add(cancel);
            deltaPrompt.pack();
            deltaPrompt.setLocation(300, 100);
        }
        deltaPrompt.setVisible(true);
    }

    TreePath getTreeTath(CallGraphNode cgNode) {
        if (cgNode.parent == null) {
            return new TreePath(topTreeNode);
        } else {
            TreePath treePath = getTreeTath(cgNode.parent);
            MyTreeNode parentTreeNode = 
                (MyTreeNode)treePath.getLastPathComponent();

            for (Enumeration e = parentTreeNode.children();
                 e.hasMoreElements();) {
                MyTreeNode treeNode = (MyTreeNode)e.nextElement();

                if (treeNode.cgNode == cgNode) {
                    return treePath.pathByAddingChild(treeNode);
                }
            }
            throw new RuntimeException("Shouldn't come to here!");
        }
    }

    void gotoGraph(CallGraphNode cgNode) {
        TreePath treePath = getTreeTath(cgNode);
        tree.scrollPathToVisible(treePath);
        tree.setSelectionPath(treePath);
    }

    File chooseFileName(String caption) {
        JFileChooser filebox = new JFileChooser();
        filebox.setDialogTitle(caption);
        if (filebox.showSaveDialog(frame) == filebox.APPROVE_OPTION) {
            return filebox.getSelectedFile();
        }
        return null;
    }
}
