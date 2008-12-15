/*
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
 *
 */

#ifndef __JAVACALL_DOM_H_
#define __JAVACALL_DOM_H_

/**
 * @file javacall_dom.h
 * @ingroup JSR290DOM
 * @brief Common javacall definitions for DOM
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/**
 * @defgroup JSR290DOM JSR290 DOM API
 *
 * The following API definitions are required by DOM part of the JSR-290.
 *
 * @{
 */
    
#define EVENT_EXCEPTION_OFFSET 0x64
    
/**
 * @enum javacall_dom_exceptions
 * @brief types of DOM exceptions
 */
typedef enum {
    /** 
     * If no exception was thrown
     */
    JAVACALL_DOM_RUNTIME_ERR                 =  0x0,
    /** 
     * If index or size is negative, or greater than the allowed value 
     */
    JAVACALL_DOM_INDEX_SIZE_ERR              =  0x1,
    /**
     * If the specified range of text does not fit into a DOMString
     */    
    JAVACALL_DOM_DOMSTRING_SIZE_ERR          =  0x2,
    /**
     * If any node is inserted somewhere it doesn't belong
     */
    JAVACALL_DOM_HIERARCHY_REQUEST_ERR       =  0x3,
    /**
     * If a node is used in a different document than the one that created it 
     * (that doesn't support it)
     */
    JAVACALL_DOM_WRONG_DOCUMENT_ERR          =  0x4,
    /**
     * If an invalid or illegal character is specified, such as in a name. See 
     * production 2 in the XML specification for the definition of a legal 
     * character, and production 5 for the definition of a legal name 
     * character.
     */
    JAVACALL_DOM_INVALID_CHARACTER_ERR       =  0x5,
    /**
     * If data is specified for a node which does not support data
     */
    JAVACALL_DOM_NO_DATA_ALLOWED_ERR         =  0x6,
    /**
     * If an attempt is made to modify an object where modifications are not 
     * allowed
     */
    JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR =  0x7,
    /**
     * If an attempt is made to reference a node in a context where it does 
     * not exist
     */
    JAVACALL_DOM_NOT_FOUND_ERR               =  0x8,
    /**
     * If the implementation does not support the requested type of object or 
     * operation.
     */
    JAVACALL_DOM_NOT_SUPPORTED_ERR           =  0x9,
    /**
     * If an attempt is made to add an attribute that is already in use 
     * elsewhere
     */
    JAVACALL_DOM_INUSE_ATTRIBUTE_ERR         =  0xA,
    /**
     * If an attempt is made to use an object that is not, or is no longer, 
     * usable.
     */
    JAVACALL_DOM_INVALID_STATE_ERR           =  0xB,
    /**
     * If an invalid or illegal string is specified.
     */
    JAVACALL_DOM_SYNTAX_ERR                  =  0xC,
    /** 
     * If an attempt is made to modify the type of the underlying object. 
     */
    JAVACALL_DOM_INVALID_MODIFICATION_ERR    =  0xD,
    /** 
     * If an attempt is made to create or change an object in a way which is 
     * incorrect with regard to namespaces. 
     */
    JAVACALL_DOM_NAMESPACE_ERR               =  0xE,
    /**
     * If a parameter or an operation is not supported by the underlying 
     * object. 
     */
    JAVACALL_DOM_INVALID_ACCESS_ERR          =  0xF,
    /** 
     * If the type of an object is incompatible with the expected type of the 
     * parameter associated to the object. 
     */
    JAVACALL_DOM_TYPE_MISMATCH_ERR           = 0x10,
    /**
     *  If the <code>Event</code>'s type was not specified by initializing the 
     * event before the method was called. Specification of the Event's type 
     * as <code>null</code> or an empty string will also trigger this 
     * exception. 
     */
    JAVACALL_DOM_EVENTS_UNSPECIFIED_EVENT_TYPE_ERR = EVENT_EXCEPTION_OFFSET,
    /**
     * If the <code>Event</code> object is already dispatched in the tree.
     */
    JAVACALL_DOM_EVENTS_DISPATCH_REQUEST_ERR       = EVENT_EXCEPTION_OFFSET + 1
} javacall_dom_exceptions;

/**
 * @enum javacall_dom_node_types
 * @brief types of DOM nodes
 */
typedef enum {
    /** The node is an <code>Element</code>. */    
    JAVACALL_DOM_ELEMENT_NODE                = 0x1,
    /** The node is an <code>Attr</code>. */    
    JAVACALL_DOM_ATTRIBUTE_NODE              = 0x2,
    /** The node is a <code>Text</code> node. */    
    JAVACALL_DOM_TEXT_NODE                   = 0x3,
    /** The node is a <code>CDATASection</code>. */    
    JAVACALL_DOM_CDATA_SECTION_NODE          = 0x4,
    /** The node is an <code>EntityReference</code>. */    
    JAVACALL_DOM_ENTITY_REFERENCE_NODE       = 0x5,
    /** The node is an <code>Entity</code>. */    
    JAVACALL_DOM_ENTITY_NODE                 = 0x6,
    /** The node is a <code>ProcessingInstruction</code>. */    
    JAVACALL_DOM_PROCESSING_INSTRUCTION_NODE = 0x7,
    /** The node is a <code>Comment</code>. */    
    JAVACALL_DOM_COMMENT_NODE                = 0x8,
    /** The node is a <code>Document</code>. */    
    JAVACALL_DOM_DOCUMENT_NODE               = 0x9,
    /** The node is a <code>DocumentType</code>. */    
    JAVACALL_DOM_DOCUMENT_TYPE_NODE          = 0xA,
    /** The node is a <code>DocumentFragment</code>. */
    JAVACALL_DOM_DOCUMENT_FRAGMENT_NODE      = 0xB,
    /** The node is a <code>Notation</code>. */
    JAVACALL_DOM_NOTATION_NODE               = 0xC
} javacall_dom_node_types;

/**
 * @enum javacall_dom_html_element_types
 * @brief types of HTMLElements
 */
typedef enum {
    /** The Element is a <code>HTMLElement</code>. */
    JAVACALL_DOM_HTML_ELEMENT                  = 0x1,
    /** The HTMLElement is a <code>HTMLFormElement</code>. */
    JAVACALL_DOM_HTML_FORM_ELEMENT             = 0x2,
    /** The HTMLElement is a <code>HTMLInputElement</code>. */
    JAVACALL_DOM_HTML_INPUT_ELEMENT            = 0x3,
    /** The HTMLElement is a <code>HTMLObjectElement</code>. */
    JAVACALL_DOM_HTML_OBJECT_ELEMENT           = 0x4,
    /** The HTMLElement is a <code>HTMLOptionElement</code>. */
    JAVACALL_DOM_HTML_OPTION_ELEMENT           = 0x5,
    /** The HTMLElement is a <code>HTMLTextAreaElement</code>. */
    JAVACALL_DOM_HTML_TEXT_AREA_ELEMENT        = 0x6
} javacall_dom_html_element_types;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_H_ */

