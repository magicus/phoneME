/*
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

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

import javax.microedition.xml.rpc.*;
import javax.xml.namespace.QName;
import javax.xml.rpc.*;

/**
 * An example RPC Midlet which enumerates each of the examples
 * in Appendix A of the JSR 172 specification.
 */
public class RPCMidlet extends MIDlet
    implements CommandListener, Runnable, FaultDetailHandler
{

    private Command exitCommand; // The exit command
    private Command runCommand;
    private Command okCommand;

    private Display display;    // The display for this MIDlet
    private TextBox textBox;

    private List runList;

    private int exampleToRun;

    public RPCMidlet() {
        display = Display.getDisplay(this);
        exitCommand = new Command("Exit", Command.EXIT, 1);
        okCommand = new Command("Ok", Command.SCREEN, 1);
    }

    public void startApp() {
        textBox = new TextBox("RPC Test MIDlet", "", 256, 0);

        textBox.addCommand(okCommand);
        textBox.addCommand(exitCommand);
        textBox.setCommandListener(this);

        runList = new List("172 Appendix A Examples:", Choice.IMPLICIT);
        for (int i = 1; i < 7; i++) {
            runList.append("A.2." + i, null);
        }
        runCommand = new Command("Run", Command.SCREEN, 1);
        runList.addCommand(runCommand);
        runList.addCommand(exitCommand);
        runList.setCommandListener(this);

        display.setCurrent(runList);
    }

    public void pauseApp() {
    }

    public void destroyApp(boolean unconditional) {
    }

    public void commandAction(Command c, Displayable s) {
        if (s != runList && s != textBox) return;

        if (c == exitCommand) {
            destroyApp(false);
            notifyDestroyed();
        } else if (c == runCommand || c == List.SELECT_COMMAND) {
            exampleToRun = runList.getSelectedIndex();
            new Thread(this).start();
        } else if (c == okCommand) {
            display.setCurrent(runList);
        }
    }

    public void run() {
        try {
            textBox.setString("Running example A.2." + (exampleToRun + 1));
            textBox.removeCommand(okCommand);
            display.setCurrent(textBox);

            switch (exampleToRun) {
                case 0:
                    A_2_1();
                    break;

                case 1:
                    A_2_2();
                    break;

                case 2:
                    A_2_3();
                    break;

                case 3:
                    A_2_4();
                    break;

                case 4:
                    A_2_5();
                    break;

                case 5:
                    A_2_6();
                    break;
            }

        } catch (Throwable t) {
            System.err.println("RPCMidlet: invoke failed:");
            t.printStackTrace();

            if (t instanceof JAXRPCException) {
                Throwable t2 = ((JAXRPCException)t).getLinkedCause();

                if (t2 != null && t2 instanceof FaultDetailException) {
                    FaultDetailException e2 = (FaultDetailException)t2;
                    System.err.print("Fault: ");
                    QName faultName = e2.getFaultDetailName();
                    if (faultName != null) {
                        System.err.println("Name: " + faultName.getLocalPart());
                    }
                    Object detail = e2.getFaultDetail();
                    if (detail != null && detail instanceof String) {
                        System.err.println("Detail: " + detail);
                    } else {
                        Object[] customFault = (Object[])detail;
                        for (int i = 0; i < customFault.length; i++) {
                            System.err.println("Fault Field " + i + ": " + customFault[i]);
                        }
                    }
                }
            }
        }

        textBox.setString("Example A.2." + (exampleToRun + 1) + " completed." +
                        " Check stdout for request/response");
        textBox.addCommand(okCommand);
    }

    public Element handleFault(QName faultName) {
        String name = faultName.getLocalPart();
        System.err.println("Handling Fault: " + name);
        Element message = new Element(
            new QName("", "message"),
            Type.STRING);
        Element quote = new Element(
            new QName("", "quote"),
            Type.STRING);
        ComplexType ct = new ComplexType();
        ct.elements = new Element[] {message, quote};

        Element myDetail = new Element(
            new QName("", "myDetail"),
            ct);

        return myDetail;
    }

    private void A_2_1() {
        // integer getEmployeeCount(integer)

        Element getEmployeeCount = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "getEmployeeCount"),
//            new QName("", "getEmployeeCount"),
            Type.INT);

        Element empCount = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "EmpCount"),
            Type.INT);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "getEmployeeCount"),
            getEmployeeCount,
            empCount,
            this);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
//                    "http://host.domain/172/A_2_1.php3");
//                    "http://host.domain/172/A_2_1b.php3");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/getEmployeeCount");

        Object o = op.invoke(new Integer(57));

        // o should be of type Integer
        Integer result = (Integer)o;
        System.err.println("Result from A_2_1:\n" + result);
    }

    private void A_2_2() {
        // boolean addGroup(String[])

        Element group = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "group"),
            Type.STRING,
            0,
            Element.UNBOUNDED,
            true);

        ComplexType groupArray = new ComplexType();
        groupArray.elements = new Element[1];
        groupArray.elements[0] = group;

        Element addGroups = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "addGroups"),
            groupArray);

        Element retVal = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "RetVal"),
            Type.BOOLEAN);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "addGroups"),
            addGroups,
            retVal);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/addGroups");

        Object[] groups = new Object[] {"Group1", "Group2", "Group3"};
        Object[] param = new Object[] {groups};

        Object o = op.invoke(param);

        // o should be of type Boolean
        Boolean result = (Boolean)o;
        System.err.println("Result from A_2_2:\n" + result);
    }

    private void A_2_3() {
        // boolean isManager(Name)

        ComplexType nameType = new ComplexType();
        nameType.elements = new Element[2];
        nameType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "firstName"),
            Type.STRING);
        nameType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "lastName"),
            Type.STRING);

        Element isManager = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "isManager"),
            nameType);

        Element retVal = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "RetVal"),
            Type.BOOLEAN);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "isManager"),
            isManager,
            retVal);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/isManager");


        Object[] name = new Object[] {"Guy", "Isamanjer"};

        Object o = op.invoke(name);

        // o should be of type Boolean
        Boolean result = (Boolean)o;
        System.err.println("Result from A_2_3:\n" + result);
    }

    private void A_2_4() {
        // boolean promoteEmployee(Employee)

        ComplexType nameType = new ComplexType();
        nameType.elements = new Element[2];
        nameType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "firstName"),
            Type.STRING);
        nameType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "lastName"),
            Type.STRING);

        ComplexType employeeType = new ComplexType();
        employeeType.elements = new Element[2];
        employeeType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empName"),
            nameType);
        employeeType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empID"),
            Type.INT);

        Element promoteEmployee = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "promoteEmployee"),
            employeeType);

        Element retVal = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "RetVal"),
            Type.BOOLEAN);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "promoteEmployee"),
            promoteEmployee,
            retVal);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/promoteEmployee");


        Object[] name       = new Object[] {"Guy", "Isdoinagreatjob"};
        Object[] employee   = new Object[] {name, new Integer(54)};

        Object o = op.invoke(employee);

        // o should be of type Boolean
        Boolean result = (Boolean)o;
        System.err.println("Result from A_2_4:\n" + result);
    }

    private void A_2_5() {
        // Employee[] getEmployees(Name[])

        ComplexType nameType = new ComplexType();
        nameType.elements = new Element[2];
        nameType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "firstName"),
            Type.STRING);
        nameType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "lastName"),
            Type.STRING);

        ComplexType nameArrayType = new ComplexType();
        nameArrayType.elements = new Element[1];
        nameArrayType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "Name"),
            nameType,
            0,
            Element.UNBOUNDED,
            true);

        ComplexType employeeType = new ComplexType();
        employeeType.elements = new Element[2];
        employeeType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empName"),
            nameType);
        employeeType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empID"),
            Type.INT);

        ComplexType employeeArrayType = new ComplexType();
        employeeArrayType.elements = new Element[1];
        employeeArrayType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "Employee"),
            employeeType,
            0,
            Element.UNBOUNDED,
            true);

        Element getEmployees = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "getEmployees"),
            nameArrayType);

        Element retVal = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "EmployeeArray"),
            employeeArrayType);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "getEmployees"),
            getEmployees,
            retVal);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/getEmployees");


        Object[] name       = new Object[] {"John", "Doe"};
        Object[] name2      = new Object[] {"Skip", "Barber"};
        Object[] nameArray  = new Object[] {name, name2};

        Object[] param      = new Object[] {nameArray};
        Object o = op.invoke(param);

        // o should be an Object[]
        // Object[] empArray = (Object[])o;
        // Object[] empSet = empArray[0];
        // each element of empSet should be an Object[] (en Employee)
        // Object[] emp = (Object[])empSet[0];
        // emp should have two elements, a name and an id
        // Object[] empName = (Object[])emp[0];
        // String firstName = empName[0];
        // String lastName = empName[1];
        // int id = ((Integer)emp[1]).intValue();

        System.err.println("Result from A_2_5:\n");

        printObject(o);

        Object[] empArray = (Object[])o;
        System.err.println("Wrapped Array of size: " + empArray.length);

        Object[] result = (Object[])empArray[0];
        System.err.println("Employee Array of size: " + result.length);

        for (int i = 0; i < result.length; i++) {
            Object[] emp = (Object[])result[i];
            System.err.print("Employee #" + i + ":");
            Object[] eName = (Object[])emp[0];
            String fName = (String)eName[0];
            String lName = (String)eName[1];
            Integer id = (Integer)emp[1];
            System.err.println("\t" + fName + " " + lName + ", " + id);
        }
    }

    private void printObject(Object o) {
        try {
            Object[] oa = (Object[])o;
            for (int i = 0; i < oa.length; i++) {
                printObject(oa[i]);
            }
        } catch (Throwable t) {
            System.err.println(o);
        }
    }

    private void A_2_6() {
        // boolean scheduleMtg(Employee[])

        ComplexType nameType = new ComplexType();
        nameType.elements = new Element[2];
        nameType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "firstName"),
            Type.STRING);
        nameType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "lastName"),
            Type.STRING);

        ComplexType employeeType = new ComplexType();
        employeeType.elements = new Element[2];
        employeeType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empName"),
            nameType);
        employeeType.elements[1] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "empID"),
            Type.INT);

        ComplexType employeeArrayType = new ComplexType();
        employeeArrayType.elements = new Element[1];
        employeeArrayType.elements[0] = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "Employee"),
            employeeType,
            0,
            Element.UNBOUNDED,
            true);

        Element scheduleMtg = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "scheduleMtg"),
            employeeArrayType);

        Element retVal = new Element(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "RetVal"),
            Type.BOOLEAN);

        Operation op = Operation.newInstance(
            new QName("http://www.sun.com/JSR172_AppendixA.xsd", "scheduleMtg"),
            scheduleMtg,
            retVal);

        op.setProperty(Stub.ENDPOINT_ADDRESS_PROPERTY,
                    "http://host.domain:8080/172deploy/ws/172Appendix");
        op.setProperty(Operation.SOAPACTION_URI_PROPERTY,
                    "http://www.sun.com/JSR172UseCases/scheduleMtg");


        Object[] name       = new Object[] {"John", "Doe"};
        Object[] emp        = new Object[] {name, new Integer(420)};
        Object[] name2      = new Object[] {"Skip", "Barber"};
        Object[] emp2       = new Object[] {name2, new Integer(44)};
        Object[] empArray   = new Object[] {emp, emp2};
        Object[] param      = new Object[] {empArray};

        Object o = op.invoke(param);

        // o should be of type Boolean
        Boolean result = (Boolean)o;
        System.err.println("Result from A_2_6:\n" + result);
    }
}
