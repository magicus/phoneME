/*
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
package kdp.classparser;

import java.util.*;

/**
 * A class that is used to parse the strings encountered in 
 * a Java class file into their "correct" representation.
 * i.e. in a class file the parameters to the main method
 * are encoded as 
 *            "[Ljava/lang/String;" 
 * and should be displayed as 
 *            "java.lang.String[]"
 *
 *
 * Revision History
 *   $Log: StringParser.java,v $
 *   Revision 1.1.1.1  2000/07/07 13:34:23  jrv
 *   Initial import of kdp code
 *
 *   Revision 1.1.1.1  2000/05/31 19:14:47  ritsun
 *   Initial import of kvmdt to CVS
 *
 *   Revision 1.1  2000/04/25 00:36:43  ritsun
 *   Initial revision
 *
 */
public class StringParser {
    /**
     * Parses the String s into a string that contains
     * a list of parameters and a return value.  The String
     * s must be a properly formatted method signature
     * as read in from a Java class file.
     *
     * @param        s        properly formatted method signature as read in
     *                         from a Java class file.
     * @return      String    string containing a list of method parameters and
     *                         the return value.
     */
    public static String parseSignature (String s)
    {
        //build the return string in retVal
        String    retVal = new String ("");
        //the method parameters are extracted into this string
        String    methodParams;
        //the method return value is extracted into this string
        String    methodReturnVal;
        //used to determine what parameter we are on so a
        // ", " can be put in the string if necessary to
        //separate parameters
        int        timesThrough = 0;

        //index is the index of the character denoting the
        //end of the parameter list
        int        index = s.lastIndexOf (')');

        //extract out the list of method parameters from
        //the method signature
        methodParams = new String (s.substring (1, index));

        //build the parameter list
        retVal = retVal + "Params: ";
        if (methodParams.length () == 0) {
            //there are no parameters
            retVal = retVal + "None";
        } else {
            //loop until there are no parameters left to decode
            while (methodParams.length () != 0) {
                //grab the first parameter out of the list
                String        firstParam = getFirstParam (methodParams);
          
                //if this is not the first parameter we examined put
                //a ", " in the new list to separate the parameters
                if (timesThrough > 0) {
                    retVal = retVal + ", ";
                }

                //convert the class file representation of the first
                //parameter into the "correct" readable format  
                retVal = retVal + parseDataType (firstParam);
         
                //get the length of the method parameter string
                int length = methodParams.length ();
                //get the length of the first parameter
                int firstParamLength = firstParam.length ();
            
                //remove the first parameter from the method parameter
                //string
                methodParams = methodParams.substring (firstParamLength, length);
            
                ++timesThrough;
            }
        } 
      
        //get the length of the input string
        int         length = s.length ();
        //grab the return value by taking a substring out of
        //the input from the end of the parameter list to the end
        methodReturnVal = new String (s.substring (index + 1, length));
      
        //build the final string
        retVal = retVal + "\n\t\t\tReturn: " + getReturnValue (methodReturnVal);
      
        return retVal;
    }
   
    /**
     * Convert the passed in encoded string into it's full form
     *
     * @param        s         an encoded method return value
     *
     * @return       String    the full form of s
     */
    public static String getReturnValue (String s)
    {
        int      lastIndex = s.lastIndexOf (")");

        String   returnValue = s.substring (lastIndex + 1);

        return parseDataType (returnValue);
    }

    /**
     * Copies the first parameter from the input string
     * and returns it.  It does not alter the input string.
     *
     * @param        s        the list of parameters to a method
     *
     * @return        String    the first parameter of the method
     */
    private static String getFirstParam (String s)
    {
        //build the first parameter in this String
        String        retVal = new String ("");

        //examine the first character to determine the parameter
        //type
        switch (s.charAt (0)) {
        case 'V' : retVal = "V"; break; //void

        case 'B' : retVal = "B"; break; //byte

        case 'C' : retVal = "C"; break; //char

        case 'D' : retVal = "D"; break; //double

        case 'F' : retVal = "F"; break; //float

        case 'I' : retVal = "I"; break; //integer

        case 'J' : retVal = "J"; break; //long

            //a class... the class name is terminated by a ";"
        case 'L' : int index = s.indexOf (';');
            retVal = s.substring (0, index + 1);
            break;

        case 'S' : retVal = "S"; break; //short

        case 'Z' : retVal = "Z"; break; //boolean

            //an array...  recursively call without the "["
            //but put the "[" back on after the parameter
            //has been extracted so that it can be pattern matched
            //later for the whole parameter signature
        case '[' : retVal = "[" + getFirstParam (s.substring (1));
            break;

            //something weird... just use the input string
        default : retVal = s; break;
        }
        return retVal;
    }
    
    /**
     * Parses an input string representing a Java
     * primitive ID or class name, or array as
     * extracted from a Java class file.  It returns
     * the "correct" readable representation of the
     * string.
     *
     * @param        s        input string as read from a Java class file.
     *
     * @return        String    the resulting string
     */
    public static String parseDataType (String s)
    {
        //build the result in here by branching based on
        //the first character of the input string
        //and placing the appropriate word form of the
        //datatype or class name in the result string
        String        retVal = new String ("");

        switch (s.charAt (0)) {
        case 'V' : retVal = "void"; break;

        case 'B' : retVal = "byte"; break;

        case 'C' : retVal = "char"; break;

        case 'D' : retVal = "double"; break;

        case 'F' : retVal = "float"; break;

        case 'I' : retVal = "int"; break;

        case 'J' : retVal = "long"; break;

        case 'L' : retVal = parseClassNameRemoveFirst (s); break;

        case 'S' : retVal = "short"; break;

        case 'Z' : retVal = "boolean"; break;

        case '[' : retVal = parseDataType (s.substring (1, s.length ())) + "[]"; break;

        default : retVal = s; break;
        }
        return retVal;
    }

    /**
     * Extracts the parameters from the passed in method signature and
     * puts them in a linked list of strings.
     *
     * @param        s              Encoded method signature
     *
     * @return       LinkedList     the parameters of a method
     *                              in a linked list of strings
     */
    public static LinkedList getParametersAsLL (String s)
    {
        LinkedList         paramList = new LinkedList ();
      
        //the method parameters are extracted into this string
        String    methodParams;

        //index is the index of the character denoting the
        //end of the parameter list
        int        index = s.lastIndexOf (')');

        //extract out the list of method parameters from
        //the method signature
        methodParams = new String (s.substring (1, index));

        if (methodParams.length () == 0) {
            return null;
        } else {
            //loop until there are no parameters left to decode
            while (methodParams.length () != 0) {
                //grab the first parameter out of the list
                String        firstParam = getFirstParam (methodParams);

                paramList.add (parseDataType (firstParam));
            
                //get the length of the method parameter string
                int length = methodParams.length ();
                //get the length of the first parameter
                int firstParamLength = firstParam.length ();

                //remove the first parameter from the method parameter string
                methodParams = methodParams.substring (firstParamLength, length);
            }
        }

        return paramList;
    }

    /**
     * Counts the number of words used by the parameters
     *
     * @param        s              Encoded method signature
     *
     * @return       count of words
     */
    public static int getParameterCount (String s) {
        //the method parameters are extracted into this string
        String    methodParams;

        //index is the index of the character denoting the
        //end of the parameter list
        int        index = s.lastIndexOf (')');
        int wordCount = 0;

        //extract out the list of method parameters from
        //the method signature
        methodParams = new String (s.substring (1, index));

        if (methodParams.length () == 0) {
            return 0;
        } else {
            //loop until there are no parameters left to decode
            while (methodParams.length () != 0) {
                //grab the first parameter out of the list
                String        firstParam = getFirstParam (methodParams);

                switch (firstParam.charAt (0)) {
                case 'V' :
                    break;

                case 'B' :
                case 'C' :
                case 'F' :
                case 'I' :
                case 'L' :
                case 'S' :
                case 'Z' :
                case '[' :
                    wordCount++;
                    break;

                case 'D' :
                case 'J' :
                    wordCount += 2;
                    break;

                default :
                    return 0;
                }

                //get the length of the method parameter string
                int length = methodParams.length ();
                //get the length of the first parameter
                int firstParamLength = firstParam.length ();

                //remove the first parameter from the method parameter string
                methodParams = methodParams.substring (firstParamLength, length);
            }
        }

        return wordCount;
    }

    /**
     * A simpler version of class name parsing.
     * Just replaces "/" with ".".
     *
     * @param        s        the input string
     *
     * @return        String    the parsed output string
     */
    public static String parseClassName (String s)
    {
        String        retVal = new String (s);

        retVal = retVal.replace ('/', '.');

        return retVal;
    }

    /**
     * Another local method that converts a class name to
     * it's "correct" form.  It replaces "/" with "." and
     * removes ";" and removes the first character, often
     * an "L".
     */
    private static String parseClassNameRemoveFirst (String s)
    {
        String        retVal = new String (s);

        retVal = retVal.substring (1);

        retVal = retVal.replace ('/', '.');
        retVal = retVal.replace (';', ' ');
        retVal = retVal.trim ();

        return retVal;
    }
}
