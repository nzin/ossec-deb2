/* @(#) $Id$ */

/* Copyright (C) 2009 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */
       

#ifndef __OS_ZLIB_H
#define __OS_ZLIB_H

#include "zlib.h"

/* os_compress: Compress a string with zlib. */
int os_compress(char *src, char *dst, int src_size, int dst_size);

/* os_uncompress: Uncompress a string with zlib. */
int os_uncompress(char *src, char *dst, int src_size, int dst_size);

#endif

/* EOF */
