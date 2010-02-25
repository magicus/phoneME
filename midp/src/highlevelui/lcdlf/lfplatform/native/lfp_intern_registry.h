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

#ifndef _LFP_INTERN_REGISTRY_H_
#define _LFP_INTERN_REGISTRY_H_

#include <lfpport_displayable.h>
#include <lfpport_item.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a component resource structure for a Displayable.
 * The MidpComponent structure portion will be initialized as:
 * <ul>
 *	<li>type = type argument</li>
 *	<li>modelVersion = 0</li>
 *	<li>next = NULL</li>
 *	<li>child = NULL</li>
 * </ul>
 *
 * The rest of MidpDisplayable structure remains un-initialized. Platform
 * specific layer should populate these remaining data fields before use.
 * 
 *
 * @param type component type of new resource
 * @return pointer to the newly created MidpDisplayable structure,
 * 	   null if failed.
 */
MidpDisplayable* MidpNewDisplayable(MidpComponentType type);

/**
 * Delete all native resource of a Displayable.
 * Resources that will be freed are:
 * <ul>
 *      <li>For each child Item: its platform dependent resource
 *      <li>For each child Item: MidpItem structure
 *      <li>Platform dependent resource
 *      <li>MidpDisplayable structure
 * </ul>
 *
 * @param displayablePtr pointer to the MidpDisplayable structure
 */
void MidpDeleteDisplayable(MidpDisplayable *displayablePtr);

/**
 * Create a component resource structure for an Item.
 * The MidpComponent structure portion will be initialized as:
 * <ul>
 *	<li>type = type argument</li>
 *	<li>modelVersion = 0</li>
 *	<li>next = NULL</li>
 *	<li>child = NULL</li>
 * </ul>
 *
 * The rest of MidpItem structure remains un-initialized. Platform
 * specific layer should populate these remaining data fields before use.
 *
 * @param owner owner screen pointer, null if no owner
 * @param type component type of new resource
 * @return pointer to the newly created MidpItem structure,
 * 	   null if failed.
 */
MidpItem* MidpNewItem(MidpDisplayable *owner, MidpComponentType type);

/**
 * Delete all native resource of an Item.
 * Resources that will be freed are:
 * <ul>
 *      <li>Platform dependent resource
 *      <li>MidpItem structure
 * </ul>
 *
 * @param itemPtr pointer to the MidpItem structure
 */
void MidpDeleteItem(MidpItem *itemPtr);

/**
 * Map a platform widget pointer to its Item's structure pointer.
 * @param owner owner screen's structure pointer
 * @param itemWidgetPtr platform widget pointer to be used as key
 * @return MidpItem* of its Item, null if not found.
 */
MidpItem* MidpFindItem(MidpDisplayable *owner,
		       PlatformItemWidgetPtr itemWidgetPtr);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _LFP_INTERN_REGISTRY_H_ */
