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

/** \file JarFileInfo.hpp
 * Definitions used by the JarFile reader.
 * These define the layout of the zip file compression used by jar files
 */

#define TMPBUFFERSIZE 64  // Must be >= LOCHDRSIZ

/*=========================================================================
 * JAR Data Stream structure
 *=======================================================================*/

#define LOCSIG (('P' << 0) + ('K' << 8) + (3 << 16) + (4 << 24))
#define CENSIG (('P' << 0) + ('K' << 8) + (1 << 16) + (2 << 24))
#define EXTSIG (('P' << 0) + ('K' << 8) + (7 << 16) + (8 << 24))
#define ENDSIG (('P' << 0) + ('K' << 8) + (5 << 16) + (6 << 24))

/*
 * Supported compression types
 */
#define STORED      0
#define DEFLATED    8

/*
 * Header sizes including signatures
 */
#define LOCHDRSIZ 30
#define EXTHDRSIZ 16
#define CENHDRSIZ 46
#define ENDHDRSIZ 22

/*
 * Header field access macros
 */
#define CH(b, n) ((jint)(((unsigned char *)(b))[n]))
#define SH(b, n) ((jint)(CH(b, n) | (CH(b, n+1) << 8)))
#define LG(b, n) ((jint)(SH(b, n) | (SH(b, n+2) << 16)))

#define GETSIG(b) LG(b, 0)      /* signature */

/*
 * Macros for getting local file header (LOC) fields
 */
#define LOCVER(b) SH(b, 4)      /* version needed to extract */
#define LOCFLG(b) SH(b, 6)      /* encrypt flags */
#define LOCHOW(b) SH(b, 8)      /* compression method */
#define LOCTIM(b) LG(b, 10)     /* modification time */
#define LOCCRC(b) LG(b, 14)     /* uncompressed file crc-32 value */
#define LOCSIZ(b) LG(b, 18)     /* compressed size */
#define LOCLEN(b) LG(b, 22)     /* uncompressed size */
#define LOCNAM(b) SH(b, 26)     /* filename size */
#define LOCEXT(b) SH(b, 28)     /* extra field size */

/*
 * Offsets for central directory header (CEN) fields
 */
#define CENVEM_OFFSET 4      /* version made by */
#define CENVER_OFFSET 6      /* version needed to extract */
#define CENFLG_OFFSET 8      /* general purpose bit flags */
#define CENHOW_OFFSET 10     /* compression method */
#define CENTIM_OFFSET 12     /* file modification time (DOS format) */
#define CENCRC_OFFSET 16     /* crc of uncompressed data */
#define CENSIZ_OFFSET 20     /* compressed size */
#define CENLEN_OFFSET 24     /* uncompressed size */
#define CENNAM_OFFSET 28     /* length of filename */
#define CENEXT_OFFSET 30     /* length of extra field */
#define CENCOM_OFFSET 32     /* file comment length */
#define CENDSK_OFFSET 34     /* disk number start */
#define CENATT_OFFSET 36     /* internal file attributes */
#define CENATX_OFFSET 38     /* external file attributes */
#define CENOFF_OFFSET 42     /* offset of local header */

/*
 * Macros for getting central directory header (CEN) fields
 */
#define CENVEM(b) SH(b, CENVEM_OFFSET)
#define CENVER(b) SH(b, CENVER_OFFSET)
#define CENFLG(b) SH(b, CENFLG_OFFSET)
#define CENHOW(b) SH(b, CENHOW_OFFSET)
#define CENTIM(b) LG(b, CENTIM_OFFSET)
#define CENCRC(b) LG(b, CENCRC_OFFSET)
#define CENSIZ(b) LG(b, CENSIZ_OFFSET)
#define CENLEN(b) LG(b, CENLEN_OFFSET)
#define CENNAM(b) SH(b, CENNAM_OFFSET)
#define CENEXT(b) SH(b, CENEXT_OFFSET)
#define CENCOM(b) SH(b, CENCOM_OFFSET)
#define CENDSK(b) SH(b, CENDSK_OFFSET)
#define CENATT(b) SH(b, CENATT_OFFSET)
#define CENATX(b) LG(b, CENATX_OFFSET)
#define CENOFF(b) LG(b, CENOFF_OFFSET)

/*
 * Offsets for end of central directory header (END) fields
 */
#define ENDSUB_OFFSET 8      /* number of entries on this disk */
#define ENDTOT_OFFSET 10     /* total number of entries */
#define ENDSIZ_OFFSET 12     /* central directory size */
#define ENDOFF_OFFSET 16     /* central directory offset */
#define ENDCOM_OFFSET 20     /* size of zip file comment */

/*
 * Macros for getting end of central directory header (END) fields
 */
#define ENDSUB(b) SH(b, ENDSUB_OFFSET)
#define ENDTOT(b) SH(b, ENDTOT_OFFSET)
#define ENDSIZ(b) LG(b, ENDSIZ_OFFSET)
#define ENDOFF(b) LG(b, ENDOFF_OFFSET)
#define ENDCOM(b) SH(b, ENDCOM_OFFSET)

/**
 * This class describes a JAR file entry being processed by the
 * JarFile class. You can use JarFile::current_entry_info() to obtain
 * the JarInfoEntry of the entry that's currently being processed.
 */
class JarInfoEntry {
public:
  /// Offset of local directory -- this value will never change as long
  /// as the JarFile is open.
  juint locOffset;

  /// Master central offset of the JarFile. -- this value will never
  /// change as long as the JarFile is open. We can search all the entries
  /// in the JarFile starting from this offset.
  juint cenOffset;

  /// Offset of the first central directory that has not yet been examined.
  /// This field is used in two places:
  /// - JarFile::do_entries() use it to iterate over all entries in a JarFile.
  /// - JarFile::find_entry() use it to mark the first entry whose information
  ///   has not yet been cached.
  juint nextCenOffset;

  /// The central header of the current Jar entry. This information is
  /// used by JarFile::load_entry to find the data of the entry.
  unsigned char centralHeader[CENHDRSIZ];

  /// the uncompressed len of the current Jar entry.
  int length;

#if ENABLE_ROM_GENERATOR
  /// the total number of entries in the central directory -- this value will
  /// never change as long as the JarFile is open.
  unsigned int totalEntryCount;
#endif
};

typedef unsigned int (*GetByteFunctionType)(void *);
