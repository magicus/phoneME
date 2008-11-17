/*
 * @(#)jzconf.h	1.12 06/10/16
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

#ifndef _INCLUDED_JZCONF_H
#define _INCLUDED_JZCONF_H

#define deflateInit_	CVMzlibDeflateInit_
#define deflate	CVMzlibDeflate
#define deflateEnd	CVMzlibDeflateEnd
#define inflateInit_ 	CVMzlibInflateInit_
#define inflate	CVMzlibInflate
#define inflateEnd	CVMzlibInflateEnd
#define deflateInit2_	CVMzlibDeflateInit2_
#define deflateSetDictionary CVMzlibDeflateSetDictionary
#define deflateCopy	CVMzlibDeflateCopy
#define deflateReset	CVMzlibDeflateReset
#define deflateParams	CVMzlibDeflateParams
#define inflateInit2_	CVMzlibInflateInit2_
#define inflateSetDictionary CVMzlibInflateSetDictionary
#define inflateSync	CVMzlibInflateSync
#define inflateSyncPoint CVMzlibInflateSyncPoint
#define inflateReset	CVMzlibInflateReset
#define compress	CVMzlibCompress
#define compress2	CVMzlibCompress2
#define uncompress	CVMzlibUncompress
#define adler32	CVMzlibAdler32
#define crc32		CVMzlibCrc32
#define get_crc_table CVMzlibGet_crc_table

#define Byte		CVMzlibByte
#define uInt		CVMzlibUInt
#define uLong		CVMzlibULong
#define Bytef	        CVMzlibBytef
#define charf		CVMzlibCharf
#define intf		CVMzlibIntf
#define uIntf		CVMzlibUIntf
#define uLongf	CVMzlibULongf
#define voidpf	CVMzlibVoidpf
#define voidp		CVMzlibVoidp

#define zlibVersion     CVMzlibVersion
#define zError          CVMzlibError
#define zcalloc         CVMzlibCalloc
#define zcfree          CVMzlibCfree

/*
 * Mapping for zip_util.c
 */
#define allocZip        CVMzutilAllocZip
#define InitializeZip   CVMzutilInitializeZip
#define DestroyZip      CVMzutilDestroyZip
#define InflateFully    CVMzutilInflateFully

#define z_off_t         CVMzlibOff_t

/*
 * Mapping for infblock.h
 */
#define inflate_blocks_state CVMzlibInflate_blocks_state
#define inflate_blocks_statef CVMzlibInflate_blocks_statef

#define inflate_blocks        CVMzlibInflate_blocks
#define inflate_blocks_new    CVMzlibInflate_blocks_new
#define inflate_blocks_reset  CVMzlibInflate_blocks_reset
#define inflate_blocks_free   CVMzlibInflate_blocks_free
#define inflate_blocks_sync_point   CVMzlibInflate_blocks_sync_point
#define inflate_set_dictionary   CVMzlibInflate_set_dictionary

/*
 * inftrees.h
 */
#define inflate_huft_s CVMzlibInflate_huft_s
#define inflate_huft CVMzlibInflate_huft

#define inflate_trees_bits CVMzlibInflate_trees_bits 
#define inflate_trees_dynamic CVMzlibInflate_trees_dynamic 
#define inflate_trees_fixed CVMzlibInflate_trees_fixed 

/*
 * infcodes.h
 */
#define inflate_codes_state CVMzlibInflate_codes_state
#define inflate_codes_statef CVMzlibInflate_codes_statef

#define inflate_codes_new CVMzlibInflate_codes_new
#define inflate_codes CVMzlibInflate_codes
#define inflate_codes_free CVMzlibInflate_codes_free

/*
 * infutil.h
 */
#define inflate_flush CVMzlibInflate_flush

/*
 * inffast.h
 */
#define inflate_fast CVMzlibInflate_fast

/*
 *  misc
 */
#define deflate_copyright CVMzlibDeflate_copyright
#define inflate_copyright CVMzlibInflate_copyright
#define inflate_mask CVMzlibInflate_mask
#define _tr_init CVMzlib_tr_init
#define _tr_stored_block CVMzlib_tr_stored_block
#define _tr_align CVMzlib_tr_align
#define _tr_flush_block CVMzlib_tr_flush_block
#define _tr_tally CVMzlib_tr_tally
#define _length_code CVMzlib_length_code
#define _dist_code CVMzlib_dist_code
#define z_errmsg CVMzlib_errmsg

#endif /* _INCLUDED_JZCONF_H */
