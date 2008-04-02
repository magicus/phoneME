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

/*
 * OsFile.h:
 */

#ifdef __cplusplus
extern "C" {
#endif

#if ENABLE_PCSL
typedef struct _OsFile_Handle {
  void *pcsl_handle;
} *OsFile_Handle;
#endif

OsFile_Handle OsFile_open(const JvmPathChar *filename, const char *mode);
int      OsFile_close(OsFile_Handle handle);
int      OsFile_flush(OsFile_Handle handle);
size_t   OsFile_read(OsFile_Handle handle,
                     void *buffer, size_t size, size_t count);
size_t   OsFile_write(OsFile_Handle handle,
                      const void *buffer, size_t size, size_t count);
long     OsFile_length(OsFile_Handle handle);
bool     OsFile_exists(const JvmPathChar *filename);
long     OsFile_seek(OsFile_Handle handle, long offset, int origin);
int      OsFile_error(OsFile_Handle handle);
int      OsFile_eof(OsFile_Handle handle);
int      OsFile_remove(const JvmPathChar *filename);
bool     OsFile_rename(const JvmPathChar *from, const JvmPathChar *to);

#if USE_IMAGE_MAPPING

struct OsFile_MappedImage {
  address mapped_address;
  // The actual implementation is likely to add fields (e.g., by subclassing
  // in Linux_MappeFile, see OsFile_linux.cpp) for additional information.
};
typedef OsFile_MappedImage* OsFile_MappedImageHandle;

/*
 * Return a preferred address for the binary image created for the given
 * JAR file. On MVM, the implementation should choose an address carefully
 * to avoid conflicts with the images of other applications that may
 * be loaded concorrently.
 */
address OsFile_ImagePreferredAddress(const JvmPathChar* jar_file_name);

/*
 * Map the given file for use as a binary image by BinaryROM.cpp.
 *
 * <preferred_destination>: The implementation should attempt to map 
 *           the file such that the first byte of the file appears at
 *           <preferred_destination> -- if this can be done, no further
 *           relocation of the image is needed. Also, the entire
 *           image may be mapped in read-only mode, except for the
 *           part of the file specified by <rw_offset, rw_length>, which
 *           must always be mapped read/write.
 *
 *           If the image cannot be mapped at <preferred_destination>,
 *           the implementation is free to map the image at any other
 *           address. The actual mapped address is returned in 
 *           OsFile_MappedFile.mapped; In this case, the entire image
 *           must be mapped read-write.
 *
 * <length>: The number of bytes to map (starting from the beginning of the
 *           file.
 *
 * <rw_offset>: from this offset on, all bytes in the file must be mapped
 *           in read-write mode. Note that rw_offset is not guaranteed
 *           to be page aligned, so the implementation may need to round
 *           down to the nearest page boundary.
 *
 * <rw_length>: In this version, it is guaranteed that
 *           rw_offset + rw_length == length.
 */
OsFile_MappedImageHandle OsFile_MapImage(const JvmPathChar* name, 
                                         address preferred_destination,
                                         int length, int rw_offset,
                                         int rw_length);
bool OsFile_UnmapImage(OsFile_MappedImageHandle  mapped_image);

#endif // USE_IMAGE_MAPPING

#ifdef __cplusplus
}
#endif
