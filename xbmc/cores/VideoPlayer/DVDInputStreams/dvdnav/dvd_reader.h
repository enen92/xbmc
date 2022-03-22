/*
 *  Copyright (C) 2001, 2002 Billy Biggs <vektor@dumbterm.net>,
 *                           Håkan Hjort <d95hjort@dtek.chalmers.se>,
 *                           Björn Englund <d4bjorn@dtek.chalmers.se>
 *
 *  This file is part of libdvdread.
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifdef _MSC_VER
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#endif

#include <sys/types.h>
//#include <inttypes.h>

/*****************************************************************************
* iovec structure: vectored data entry
*****************************************************************************/
#ifdef TARGET_WINDOWS
struct iovec
{
  void *iov_base;     /* Pointer to data. */
  size_t iov_len;     /* Length of data.  */
};
#else
#   include <sys/uio.h>                                      /* struct iovec */
#endif

/**
 * The DVD access interface.
 *
 * This file contains the functions that form the interface to
 * reading files located on a DVD.
 */

/**
 * The current version.
 */
#define DVDREAD_VERSION 904

/**
 * The length of one Logical Block of a DVD.
 */
#define DVD_VIDEO_LB_LEN 2048

/**
 * Maximum length of filenames allowed in UDF.
 */
#define MAX_UDF_FILE_NAME_LEN 2048

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque type that is used as a handle for one instance of an opened DVD.
 */
typedef struct dvd_reader_s dvd_reader_t;

/**
 * Opaque type for a file read handle, much like a normal fd or FILE *.
 */
typedef struct dvd_file_s dvd_file_t;

struct dvd_reader_stream_cb
{
  int(*pf_seek)  (void *p_stream, uint64_t i_pos);
  int(*pf_read)  (void *p_stream, void* buffer, int i_read);
  int(*pf_readv) (void *p_stream, void *p_iovec, int i_blocks);
};
typedef struct dvd_reader_stream_cb dvd_reader_stream_cb;

/**
 * Public type that is used to provide statistics on a handle.
 */
typedef struct {
  off_t size;          /**< Total size of file in bytes */
  int nr_parts;        /**< Number of file parts */
  off_t parts_size[9]; /**< Size of each part in bytes */
} dvd_stat_t;

/**
 * Opens a block device of a DVD-ROM file, or an image file, or a directory
 * name for a mounted DVD or HD copy of a DVD.
 *
 * If the given file is a block device, or is the mountpoint for a block
 * device, then that device is used for CSS authentication using libdvdcss.
 * If no device is available, then no CSS authentication is performed,
 * and we hope that the image is decrypted.
 *
 * If the path given is a directory, then the files in that directory may be
 * in any one of these formats:
 *
 *   path/VIDEO_TS/VTS_01_1.VOB
 *   path/video_ts/vts_01_1.vob
 *   path/VTS_01_1.VOB
 *   path/vts_01_1.vob
 *
 * @param path Specifies the the device, file or directory to be used.
 * @return If successful a read handle is returned. Otherwise 0 is returned.
 *
 * dvd = DVDOpen(path);
 */
dvd_reader_t *DVDOpen( const char * );

/**
 * Closes and cleans up the DVD reader object.
 *
 * You must close all open files before calling this function.
 *
 * @param dvd A read handle that should be closed.
 *
 * DVDClose(dvd);
 */
void DVDClose( dvd_reader_t * );

/**
 *
 */
typedef enum {
  DVD_READ_INFO_FILE,        /**< VIDEO_TS.IFO  or VTS_XX_0.IFO (title) */
  DVD_READ_INFO_BACKUP_FILE, /**< VIDEO_TS.BUP  or VTS_XX_0.BUP (title) */
  DVD_READ_MENU_VOBS,        /**< VIDEO_TS.VOB  or VTS_XX_0.VOB (title) */
  DVD_READ_TITLE_VOBS        /**< VTS_XX_[1-9].VOB (title).  All files in
				  the title set are opened and read as a
				  single file. */
} dvd_read_domain_t;


#ifdef __cplusplus
};
#endif

