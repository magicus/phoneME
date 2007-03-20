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

/**
 * @file native_handlers_info.h
 * @ingroup CHAPI
 * @brief Content handlers database access interface
 */

#ifndef _JAVACALL_REGISTRY_H_
#define _JAVACALL_REGISTRY_H_

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/

#define ERROR_BAD_PARAMS -1L
#define ERROR_NO_MEMORY	-2L
#define ERROR_NOT_FOUND -3L
#define ERROR_HANDLER_ALREADY_EXISTS -4L
#define ERROR_ACCESS_DENIED	-5L
#define ERROR_NO_MORE_ELEMENTS 259L
#define ERROR_BUFFER_TOO_SMALL 234L



typedef struct _registry_value_pair {
	const unsigned short* key;
	const unsigned short* value;
} registry_value_pair;


//register handler
int register_handler(
        const unsigned short* content_handler_id,
		const unsigned short* content_handler_friendly_name,
		const unsigned short* suite_id,
        const unsigned short* class_name,
		int flag,
        const unsigned short** types,     int nTypes,
        const unsigned short** suffixes,  int nSuffixes,
        const unsigned short** actions,   int nActions,  
        const unsigned short** locales,   int nLocales,
        const unsigned short** action_names, int nActionNames,
        const unsigned short** accesses,  int nAccesses,
		const registry_value_pair* additional_keys, int nKeys,
		const unsigned short* default_icon_path);


int enum_handlers(int* pos_id, /*OUT*/ short*  buffer, int* length);

int enum_handlers_by_suffix(const unsigned short* suffix, int* pos_id, /*OUT*/ short*  buffer, int* length);

int enum_handlers_by_type(const unsigned short* mimetype, int* pos_handle, /*OUT*/ short*  buffer, int* length);

int enum_handlers_by_action(const unsigned short* action, int* pos_handle, /*OUT*/ short*  buffer, int* length);

int enum_handlers_by_suit_id(const unsigned short* suit_id, int* pos_handle, /*OUT*/ short*  buffer, int* length);

int enum_actions(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length);

int enum_action_locales(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length);

int get_local_action_name(const unsigned short* content_handler_id, const unsigned short* action, const unsigned short* locale, short*  buffer, int* length);

int enum_suffixes(const unsigned short* content_handler_id, int* pos_id, /*OUT*/ short*  buffer, int* length);

int enum_types(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length);

int enum_trusted_callers(const unsigned short* content_handler_id, int* pos_id, /*OUT*/ short*  buffer, int* length);

int get_class_name(const unsigned short* content_handler_id, /*OUT*/ short*  buf, int* length);

int get_content_handler_friendly_name(const unsigned short* content_handler_id, /*OUT*/ short*  buffer, int* length);

int get_suite_id(const unsigned short* content_handler_id, /*OUT*/ short*  buffer, int* length);

int get_flag(const unsigned short* content_handler_id,/*OUT*/  int* val);

int get_handler_info(const unsigned short* content_handler_id,
				   /*OUT*/  short*  suit_id, int* suit_id_len,
				   short*  classname, int* classname_len,
				   int *flag);

int is_trusted(const unsigned short* content_handler_id, const unsigned short* caller_id);

int is_action_supported(const unsigned short* content_handler_id, const unsigned short* action);

int unregister_handler(const unsigned short* content_handler_id);

#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */

#endif  /* _JAVACALL_REGISTRY_H_ */
