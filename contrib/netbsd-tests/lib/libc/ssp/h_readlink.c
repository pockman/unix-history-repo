/* $NetBSD: h_readlink.c,v 1.1 2010/12/27 02:04:19 pgoyette Exp $ */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2008\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: h_readlink.c,v 1.1 2010/12/27 02:04:19 pgoyette Exp $");

#include <sys/param.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __FreeBSD__
#include <err.h>
#include <string.h>
#endif

int
main(int argc, char *argv[])
{
#ifdef __FreeBSD__
	char b[512], *sl;
	int n;
	size_t len = atoi(argv[1]);
	sl = malloc(len);
	memset(sl, 'a', len);
	sl[len - 1] = 0;
	unlink("symlink");
	if (symlink(sl, "symlink") == -1)
		err(1, "symlink()");
	n = readlink("symlink", b, len);
	unlink("symlink");
#else
	char b[MAXPATHLEN];
	size_t len = atoi(argv[1]);
	(void)readlink("/", b, len);
#endif
	(void)printf("%s\n", b);
	return 0;
}
