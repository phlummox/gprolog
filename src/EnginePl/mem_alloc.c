/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : Prolog engine                                                   *
 * File  : mem_alloc.c                                                     *
 * Descr.: customized memory allocator                                     *
 * Author: Daniel Diaz                                                     *
 *                                                                         *
 * Copyright (C) 1999,2000 Daniel Diaz                                     *
 *                                                                         *
 * GNU Prolog is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2, or any later version.       *
 *                                                                         *
 * GNU Prolog is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc.  *
 * 59 Temple Place - Suite 330, Boston, MA 02111, USA.                     *
 *-------------------------------------------------------------------------*/

/* $Id$ */

#include "gp_config.h"


/*-------------------------------------------------------------------------*
 * If needed for some machine: redefine here malloc/calloc/realloc/free by *
 * inserting an appropriate file.                                          *
 *-------------------------------------------------------------------------*/

#if defined(M_ix86_linux)

#ifdef HAVE_MMAP
#undef  HAVE_MMAP
#endif
#define HAVE_MMAP 0		/* do not use mmap for big blocks */

#include "dl_malloc.c"

#endif