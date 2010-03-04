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

/**
 * @file
 *
 * This header file is interface to the MIDP API for handling control
 * arguments given in the midlet suite's jad file.
 */

#ifndef _MIDP_JAD_CONTROL_ARGS_H_
#define _MIDP_JAD_CONTROL_ARGS_H_

#if ENABLE_CONTROL_ARGS_FROM_JAD

/**
 * @def MAX_JAD_CONTROL_ARGS
 * Maximum number of supported control arguments given in the value of
 * MIDP_CONTROL_ARGS JAD property.
 */
#define MAX_JAD_CONTROL_ARGS 10

/**
 * A structure to hold pointers to the control argument's name and value.
 */
typedef struct tagMIDP_JAD_CONTROL_ARGS {
    const char* pArgName;
    const char* pArgValue;
    int argNameLen;
} MIDP_JAD_CONTROL_ARGS;

/**
 * Finds a value of argument with the given name in the array of control
 * arguments.
 *
 * @param pJadArgs buffer holding the name and value of each control argument
 * @param pArgName name of the argument to find
 * @param ppArgValue address where to save the pointer to the argument's value
 *
 * @return ALL_OK if the given argument name was found, NOT_FOUND otherwise
 */
MIDPError get_jad_control_arg_value(MIDP_JAD_CONTROL_ARGS* pJadArgs,
                                    const char* pArgName,
                                    const char** ppArgValue);
/**
 * Parses a value of MIDP_CONTROL_ARGS property from the midlet suite's
 * JAD file. Syntax of this property is defined by the following grammar:
 * <pre>
 * JAD_CONTROL_PARAM = "MIDP_CONTROL_ARGS" EQUAL *( ";" param)
 * param = report_level_param / log_channels_param / permissions_param /
 * trace_param / assert_param
 * report_level_param = "report_level" EQUAL 1*DIGIT
 * log_channels_param = "log_channels" EQUAL 1*DIGIT *( "," 1*DIGIT)
 * permissions_param  = "allow_all_permissions"
 * trace_param  = "enable_trace" EQUAL bool_val
 * assert_param = "enable_trace" EQUAL bool_val
 * bool_val = 0 / 1
 * </pre>
 *
 * @param pJadProps properties of the midlet suite from its jad file
 * @param pJadArgs [out] buffer where to store pointers to the name and value of
 * each control argument
 * @param maxJadArgs maximal number of arguments that the output buffer can hold
 */
void parse_control_args_from_jad(const MidpProperties* pJadProps,
                                 MIDP_JAD_CONTROL_ARGS* pJadArgs,
                                 int maxJadArgs);

/**
 * Loads the properties of a MIDlet suite from persistent storage.
 *
 * @param suiteId ID of the suite
 * @param pJadProps [out] pointer to a structure containing an array of strings,
 * in a pair pattern of key and value; NULL may be passed if it is not required
 * to read JAD properties
 * @param pJarProps [out] pointer to a structure containing an array of strings,
 * in a pair pattern of key and value; NULL may be passed if it is not required
 * to read JAR properties
 *
 * @return error code (ALL_OK for success)
 */
MIDPError
load_install_properties(SuiteIdType suiteId, MidpProperties* pJadProps,
                        MidpProperties* pJarProps);

#endif

#endif /* _MIDP_JAD_CONTROL_ARGS_H_ */
