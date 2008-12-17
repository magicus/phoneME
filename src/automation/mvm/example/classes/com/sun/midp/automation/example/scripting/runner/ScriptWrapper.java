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

package com.sun.midp.automation.example.scripting;

import java.util.Random;
import java.util.Vector;
import java.io.InputStream;

class ScriptWrapper implements ScriptExtension {

    /**
     * Codes of exteternal functions
     * @see #register
     */
    static final int FUNCTION_PRINT         = 1;
    static final int FUNCTION_RANDOM        = 2;
    static final int FUNCTION_SLEEP         = 3;
    static final int FUNCTION_MILLIS        = 4;
    static final int FUNCTION_SET_TRAP      = 5;
    static final int FUNCTION_RELEASE_TRAP  = 6;
    static final int FUNCTION_RUN_MIDLET    = 7;
    static final int FUNCTION_WAIT          = 8;
    static final int FUNCTION_PRINTLN       = 9;
    static final int FUNCTION_SEND_EVENT    = 10;
    static final int FUNCTION_LAST_EVENT    = 11;
    static final int FUNCTION_SWITCH_TO     = 12;
    static final int FUNCTION_STOP_MIDLET   = 13;
    static final int FUNCTION_CREATE_IMAGE  = 14;
    static final int FUNCTION_VERIFY_JAR    = 15;
    static final int FUNCTION_MIDLET_LIST   = 16;
    static final int FUNCTION_SUBSTRING     = 17;
    static final int FUNCTION_SYS_PROPERTY  = 18;
    static final int FUNCTION_GC            = 19;

    Random random;

    ScriptWrapper() {
        random = new Random();
    }

    /**
     * Called by ScriptParser before reading of a script file
     * to register pre-defined externals
     */
    public void register(ScriptParser parser) {
        parser.registerExternalFunction("print", FUNCTION_PRINT);
        parser.registerExternalFunction("println", FUNCTION_PRINTLN);     
        parser.registerExternalFunction("random", FUNCTION_RANDOM);
        parser.registerExternalFunction("sleep",  FUNCTION_SLEEP);
        parser.registerExternalFunction("currentTimeMillis", FUNCTION_MILLIS);
        parser.registerExternalFunction("substr", FUNCTION_SUBSTRING);

/*
        parser.registerExternalFunction("setTrap", FUNCTION_SET_TRAP);
        parser.registerExternalFunction("releaseTrap", FUNCTION_RELEASE_TRAP);
        parser.registerExternalFunction("runMidlet", FUNCTION_RUN_MIDLET);
        parser.registerExternalFunction("switchTo", FUNCTION_SWITCH_TO);
        parser.registerExternalFunction("stopMidlet", FUNCTION_STOP_MIDLET);
        parser.registerExternalFunction("sendEvent", FUNCTION_SEND_EVENT);
        parser.registerExternalFunction("lastEvent", FUNCTION_LAST_EVENT);
        parser.registerExternalFunction("waitFor", FUNCTION_WAIT);
        parser.registerExternalFunction("midletList", FUNCTION_MIDLET_LIST);
        parser.registerExternalFunction("systemProperty", FUNCTION_SYS_PROPERTY);
        parser.registerExternalFunction("garbageCollect", FUNCTION_GC);

        parser.registerExternalConstant("CURRENT_MIDLET",   new Constant(-1));
        parser.registerExternalConstant("MANAGER_MIDLET",   Constant.ZERO);
        parser.registerExternalConstant("ON_FIRST_PAINT",
            new Constant("javax.microedition.lcdui.Display.refresh"));

        parser.registerExternalConstant("STOP_EVENT", new Constant(-1));
        parser.registerExternalConstant("KEY_EVENT", new Constant(1));
        parser.registerExternalConstant("PEN_EVENT", new Constant(2));
        parser.registerExternalConstant("COMMAND_EVENT", new Constant(3));
        parser.registerExternalConstant("TIMER_EVENT", new Constant(4));
        parser.registerExternalConstant("PRESSED", new Constant(1));
        parser.registerExternalConstant("RELEASED", new Constant(2));
        parser.registerExternalConstant("REPEATED", new Constant(3));
        parser.registerExternalConstant("TYPED", new Constant(4));
        parser.registerExternalConstant("IME", new Constant(5));
        parser.registerExternalConstant("DRAGGED", new Constant(3));
        parser.registerExternalConstant("SCREEN", new Constant(1));
        parser.registerExternalConstant("REPAINT", new Constant(2));
        parser.registerExternalConstant("CALLED_SERIALLY", new Constant(3));
        parser.registerExternalConstant("MENU_REQUESTED",   new Constant(-1));
        parser.registerExternalConstant("MENU_DISMISSED",   new Constant(-2));
        parser.registerExternalConstant("KEY_0", new Constant(48));
        parser.registerExternalConstant("KEY_1", new Constant(49));
        parser.registerExternalConstant("KEY_2", new Constant(50));
        parser.registerExternalConstant("KEY_3", new Constant(51));
        parser.registerExternalConstant("KEY_4", new Constant(52));
        parser.registerExternalConstant("KEY_5", new Constant(53));
        parser.registerExternalConstant("KEY_6", new Constant(54));
        parser.registerExternalConstant("KEY_7", new Constant(55));
        parser.registerExternalConstant("KEY_8", new Constant(56));
        parser.registerExternalConstant("KEY_9", new Constant(57));
        parser.registerExternalConstant("KEY_ASTERISK", new Constant(42));
        parser.registerExternalConstant("KEY_POUND", new Constant(35));
        parser.registerExternalConstant("KEY_UP", new Constant(-1));
        parser.registerExternalConstant("KEY_DOWN", new Constant(-2));
        parser.registerExternalConstant("KEY_LEFT", new Constant(-3));
        parser.registerExternalConstant("KEY_RIGHT", new Constant(-4));
        parser.registerExternalConstant("KEY_SELECT", new Constant(-5));
        parser.registerExternalConstant("KEY_SOFT1", new Constant(-6));
        parser.registerExternalConstant("KEY_SOFT2", new Constant(-7));
        parser.registerExternalConstant("KEY_CLEAR", new Constant(-8));
        
        parser.registerExternalConstant("KEY_SEND", new Constant(-10));
        parser.registerExternalConstant("KEY_END",  new Constant(-11));
        parser.registerExternalConstant("KEY_POWER", new Constant(-12));
*/
    }
        
    /**
     * Called by Script Engine whenever an external function is called
     *
     * @param function - a code of registred function to be called
     * @param argv       - arguments passed to the function
     * @return result value of the function
     */
    public Constant invoke(int function, Constant[] args) {
        switch (function) {

        case FUNCTION_PRINT:
        case FUNCTION_PRINTLN:
            if (args != null) {
                for (int i = 0; i < args.length; i++) {
                    System.out.print(args[i].asString());
                }
            }
            if (function == FUNCTION_PRINTLN) {
                    System.out.println();
            }
            break;
            
        case FUNCTION_RANDOM:
            return args == null ?
                         new Constant(random.nextDouble()) :
                         new Constant(random.nextInt(args[0].asInteger()));
     
        case FUNCTION_SLEEP:
            if (args == null) {
                    Thread.yield();
            } else {
                    try {
                            Thread.sleep(args[0].asInteger());
                    } catch (Exception e) {}
            }
            break;

        case FUNCTION_MILLIS:
            return new Constant(System.currentTimeMillis());

        case FUNCTION_SUBSTRING:
            String s = args[0].asString();
            int start = args[1].asInteger();
            if (start < 0) {
                start += s.length();
            }
            if (args.length < 3) {
                return new Constant(s.substring(start));
            }
            int len = args[2].asInteger();
            if (len < 0) {
                len += (s.length() - start);
            }
            return new Constant(s.substring(start, len));

/*
        case FUNCTION_SET_TRAP:
            int callCount = args.length < 2 ? 1 : args[1].asInteger();
            int trapAction = args.length < 3 ? MethodTrap.ACTION_STOP_ISOLATE :
                                             args[2].asInteger();
            return new Constant(MethodTrap.setTrap(args[0].asString(), callCount,
                                                                                         trapAction, 0));

        case FUNCTION_RELEASE_TRAP:
            MethodTrap.releaseTrap(args[0].asInteger());
            break;

        case FUNCTION_RUN_MIDLET:
            String exitConditions[] = null;
            if (args.length >= 3) {
                exitConditions = new String[args.length - 2];
                for (int i = 2; i < args.length; i++) {
                    exitConditions[i - 2] = args[i].asString();
                }
            }
            Integer handle = variant().registerMidlet(args[0].asString(),
                                                                                                args[1].asString(),
                                                                                                exitConditions);
            variant().launchMidlet(handle);
            return new Constant(handle);

        case FUNCTION_SWITCH_TO:
            long result = variant().switchToMidlet((Integer)args[0].getValue());
            return new Constant(new Long(result));

        case FUNCTION_STOP_MIDLET:
            variant().stopMidlet((Integer)args[0].getValue());
            return Constant.NULL;

        case FUNCTION_SEND_EVENT:
            int[] iargs = new int[args.length-1];
            for (int i=1; i<args.length; i++) {
                    iargs[i-1] = args[i].asInteger();
            }
            result = variant().sendEventToMidlet((Integer)args[0].getValue(), iargs);
            return new Constant(new Long(result));

        case FUNCTION_LAST_EVENT:
            result = variant().lastEvent((Integer)args[0].getValue());
            return new Constant(new Long(result));

        case FUNCTION_WAIT:
            Object value = args[0].getValue();
            if (value instanceof Integer) { 
                    variant().waitExit((Integer)value);
                    return Constant.TRUE;
            } else if (value instanceof Long) {
                    return new Constant(variant().waitFor(((Long)value).longValue()));
            }
            return Constant.FALSE;

        case FUNCTION_CREATE_IMAGE:
            return new Constant(
                    Util.createAppImage(args[0].asString(), args[1].asString(), 0));

        case FUNCTION_VERIFY_JAR:
            return new Constant(Util.verify(args[0].asString()));

        case FUNCTION_MIDLET_LIST:
            Installer installer =
                Installer.getInstaller(variant().getSecurityDomain());
            String[] appList = installer.list();
            if (appList == null || appList.length == 0) {
                return Constant.NULL;
            }
            Vector midlets = new Vector(appList.length);
            for (int i = 0; i < appList.length; i++) {
                MIDletSuite midletSuite = installer.getMIDletSuite(appList[i]);
                Constant storage = new Constant(File.getStorageRoot() +
                                                                                appList[i] + "suite.jar");
                for (int j = 1; ; j++) {
                    String midletName = midletSuite.getProperty("MIDlet-" + j);
                    if (midletName == null) {
                        break;
                    }
                    midletName = new MIDletInfo(midletName).classname;
                    Variable[] midlet = new Variable[2];
                    midlet[0] = new Variable(new Constant(midletName));
                    midlet[1] = new Variable(storage);
                    midlets.addElement(new Constant(midlet));
                }
            }
            return arrayFromVector(midlets);

        case FUNCTION_SYS_PROPERTY:
            return new Constant(getPropertyValue(args[0].asInteger()));

        case FUNCTION_GC:
            garbageCollect();
            break;
*/

        default:
            // Unknown or unimplemented function, return NULL
        }
        return Constant.NULL;
    }

    static Constant arrayFromVector(Vector v) {
        if (v.size() == 0) {
            return Constant.NULL;
        }
        Variable[] array = new Variable[v.size()];
        for (int i = 0; i < v.size(); i++) {
            array[i] = new Variable((Constant)v.elementAt(i));
        }
        return new Constant(array);
    }
    
    static void runScript(InputStream input) {
        Script script = ScriptParser.parse(input, new ScriptWrapper());
        (new Thread(script)).start();
    }
}
