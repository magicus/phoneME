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
#ifndef _MIDP_AMS_STATUS_H_
#define _MIDP_AMS_STATUS_H_

/**
 * @file
 * @ingroup ams_base
 *
 * @brief Definition of AMS error conditions.
 */

/**
 * IMPL_NOTE:Document BAD_ID_PROPERTY_STATUS.
 */
#define BAD_ID_PROPERTY_STATUS(props) \
    ((props).status == NULL_LEN)
/**
 * IMPL_NOTE:Document OUT_OF_MEM_PROPERTY_STATUS.
 */
#define OUT_OF_MEM_PROPERTY_STATUS(props) \
    ((props).status == OUT_OF_MEM_LEN)
/**
 * IMPL_NOTE:Document CORRUPTED_PROPERTY_STATUS.
 */
#define CORRUPTED_PROPERTY_STATUS(props) \
    ((props).status == SUITE_CORRUPTED_ERROR)
/**
 * IMPL_NOTE:Document READ_ERROR_PROPERTY_STATUS.
 */
#define READ_ERROR_PROPERTY_STATUS(props) \
    ((props).status == IO_ERROR_LEN)
/**
 * IMPL_NOTE:Document BAD_ID_INFO_STATUS.
 */
#define BAD_ID_INFO_STATUS(info) \
    ((info).status == NULL_LEN)
/**
 * IMPL_NOTE:Document OUT_OF_MEM_INFO_STATUS.
 */
#define OUT_OF_MEM_INFO_STATUS(info) \
    ((info).status == OUT_OF_MEM_LEN)
/**
 * IMPL_NOTE:Document SUITE_CORRUPTED_ERR_STATUS
 */
#define SUITE_CORRUPTED_ERR_STATUS(info) \
    ((info).status == SUITE_CORRUPTED_ERROR)
/**
 * IMPL_NOTE:Document READ_ERROR_INFO_STATUS.
 */
#define READ_ERROR_INFO_STATUS(info) \
    ((info).status == IO_ERROR_LEN)

/* @} */

#endif /* _MIDP_AMS_STATUS_H_ */

