/* $Id: extract.c,v 1.8 2002/10/11 19:15:19 te Exp $ */

/*
 * Copyright (c) 2002 Tamer Embaby <tsemba@menanet.net>
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined (__OpenBSD__) || defined (__FreeBSD__)
# include <unistd.h>
# include <sys/param.h>
#endif

#if defined (_WINDOWS)
# define _POSIX_
# include <limits.h>	/* PATH_MAX */
# undef _POSIX_
# include <io.h>
# include <direct.h>
#endif

#include <cheir.h>

#if 0
#define PREAMBLE	\
"/*\n" \
" * $Id: extract.c,v 1.8 2002/10/11 19:15:19 te Exp $\n" \
" * (C) Tamer Embaby <tsemba@menanet.net> \n" \
" * cheir: C Heirarchy: tarcing function calls/source extracting utility\n" \
" */\n\n"
#else
static char *PREAMBLE =
"/*\n"
" * %s\n"
" * (C) Tamer Embaby <tsemba@menanet.net> \n"
" * http://tsemba.tripod.com\n"
" * cheir: C Hierarchy: tracing function calls, source extracting utility\n"
" */\n\n";
#endif

void	xoutput_fctx(struct funcd_ctx *,char *,char *);
int	mkdir_xoutput(char *);
int	xtract_single_funcd(struct funcd_ctx *,char *,char *);
FILE	*xcreate(char  *,char *);
FILE	*xopen(char *,char *);
FILE	*ropen(char *,char *);

extern char *getline(FILE *);

void
xoutput(startfuncs, odir, idir)
	char **startfuncs;
	char *odir, *idir;
{
	char **p;
	struct funcd_ctx *fc;

	if (mkdir_xoutput(odir) < 0)
		return;

	if (verbose != 0)
		printf("xoutput: output directory %s created\n", odir);

	for (p = startfuncs; *p != NULL; p++) {
		fc = getfuncdbynam(*p);
		if (fc != NULL) {
			if (verbose != 0)
				printf("xoutput: extracting %s to %s\n",
				    fc->fd_name, odir);
			xoutput_fctx(fc, odir, idir);
		} else
			printf("extract: %s: not found.  Ignored\n", *p);
	}
#ifdef _MAKEFILE
	mkgen(odir);
#endif
	return;
}

void
xoutput_fctx(fc, xdir, idir)
	struct funcd_ctx *fc;
	char *xdir, *idir;
{
	register struct func *f;

	if (fc->sscop.ss_visit != 0) {
		return;
	}
	xtract_single_funcd(fc, xdir, idir);
	fc->sscop.ss_visit++;

	TAILQ_FOREACH(f, &fc->fd_funcs, f_link) {
		switch (f->f_type) {
			default:
			case FUNC_EXTERNAL:
			case FUNC_RECURSIVE:
				break;
			case FUNC_RESOLVED:
				if (f->f_def == NULL) {
					fprintf(stderr, "xoutput_fctx: "
					    "inconsistency: %s marked resolved "
					    "but doesn't have definition "
					    "pointer\n", f->f_name);
					break;
				}
				xoutput_fctx(f->f_def, xdir, idir);
				break;
		}
	}
	return;
}

int
mkdir_xoutput(dir)
	char *dir;
{

	/* Even if directory exist */
#if defined (_WINDOWS)
	if (mkdir(dir) < 0) {
#else
	if (mkdir(dir, S_IRWXU|S_IRGRP|S_IXGRP) < 0) {
#endif
		fprintf(stderr, "mkdir_xoutput: %s: %s\n", dir,
		    strerror(errno) );
		return (-1);
	}
	return (0);
}

int
xtract_single_funcd(fc, xdir, idir)
	struct funcd_ctx *fc;
	char *xdir, *idir;
{
	int linenr;
	char *line;
	FILE *ifs, *ofs;
	struct header *hdr;
#if defined (_WINDOWS)
	char buffer[BUFSIZ<<3];
#else
	char buffer[BUFSIZ];
#endif

	linenr = 0;

	if (verbose != 0)
		printf("xtract_func: extracting %s from %s %d -> %d\n",
		    fc->fd_name, fc->fd_mod->m_name, fc->fd_sline,
		    fc->fd_eline);

	/* Output file */
	if ((ofs = xopen(fc->fd_mod->m_name, xdir) ) == NULL) {
		if ((ofs = xcreate(fc->fd_mod->m_name, xdir) ) == NULL) {
			fprintf(stderr, "xtract_func: %s: %s",
			    fc->fd_mod->m_name, strerror(errno) );
			return (-1);
		}
		snprintf(buffer, sizeof(buffer),
		    "/* \n * @Module: %s (%d/%d/%d)\n */\n\n",
		    fc->fd_mod->m_name, fc->fd_mod->s.s_funcdcnt,
		    fc->fd_mod->s.s_funcscnt, fc->fd_mod->s.s_loc);
		fwrite(buffer, strlen(buffer), 1, ofs);

		snprintf(buffer, sizeof(buffer), "/*\n * @Headers:\n */\n");
		fwrite(buffer, strlen(buffer), 1, ofs);

		TAILQ_FOREACH(hdr, &fc->fd_mod->m_headers, h_link) {
			snprintf(buffer, sizeof(buffer), "#include <%s>\n",
			    hdr->h_name);
			fwrite(buffer, strlen(buffer), 1, ofs);
		}
	}

	if ((ifs = ropen(fc->fd_mod->m_name, idir) ) == NULL) {
		fclose(ifs);
		return (-1);
	}

	fwrite("\n", 1, 1, ofs);
	snprintf(buffer, sizeof(buffer), "/*\n * @Routine: %s (%d/%d)\n */\n",
	    fc->fd_name, fc->fd_sline, fc->fd_eline);
	fwrite(buffer, strlen(buffer), 1, ofs);

	for (;;) {
		if ((line = getline(ifs) ) == NULL) {
			break;
		}
		linenr++;

		if (linenr >= fc->fd_sline && linenr <= fc->fd_eline) {
			/*printf("include: current line %d\n", linenr);*/
#if defined (_WINDOWS)
			/*
			 * On Windows: getline will never return \n
			 * so on an empty line it returns empty buffer
			 * (i.e., buffer[0] == 0).  When writing such
			 * a buffer on Windows 2000 the fwrite will fail
			 * with error: No such a file or directory.
			 */
			snprintf(buffer, sizeof(buffer), "%s\n",
			    line);
			if (fwrite(buffer, strlen(buffer), 1, ofs) != 1) {
				printf("fwrite(line): error: %s\n",
				    strerror(errno) );
				break;
			}
#else
			if (fwrite(line, strlen(line), 1, ofs) != 1) {
				printf("fwrite(line): error: %s\n",
				    strerror(errno) );
				break;
			}
			if (fwrite("\n", 1, 1, ofs) != 1) {
				printf("fwrite(\n): error: %s\n",
				    strerror(errno) );
				break;
			}
#endif
		}
		free(line);
	}
	fclose(ifs);
	fclose(ofs);
	return (0);
}

FILE *
xcreate(file, dir)
	char  *file, *dir;
{
	FILE *f;
	char fn[PATH_MAX];
	char buffer[BUFSIZ];

	snprintf(fn, sizeof(fn), "%s/%s", dir, file);
	fn[sizeof(fn)-1] = 0;

	if ((f = fopen(fn, "w") ) == NULL) {
		fprintf(stderr, "xcreate: %s: %s\n", fn, strerror(errno) );
		return (NULL);
	}

	snprintf(buffer, sizeof(buffer), PREAMBLE, VERSION);
	fwrite(buffer, strlen(buffer), 1, f);
	return (f);
}

FILE *
xopen(file, dir)
	char *file, *dir;
{
	int fd;
	FILE *f;
	char fn[PATH_MAX];

	snprintf(fn, sizeof(fn), "%s/%s", dir, file);
	fn[sizeof(fn)-1] = 0;

	/* First we open the file for reading to make sure it exist */
	if ((fd = open(fn, O_WRONLY|O_APPEND, 0) ) < 0) {
		return (NULL);
	}

	lseek(fd, 0, SEEK_END);

	if ((f = fdopen(fd, "a") ) == NULL) {
		fprintf(stderr, "xopen: %s: %s\n", fn, strerror(errno) );
		close(fd);
		return (NULL);
	}
	return (f);
}

FILE *
ropen(file, idir)
	char *file;
	char *idir;
{
	FILE *f;
	char fn[PATH_MAX];

	snprintf(fn, sizeof(fn), "%s/%s", idir, file);
	fn[sizeof(fn)-1] = 0;

	if ((f = fopen(fn, "r") ) == NULL) {
		fprintf(stderr, "ropen: %s: %s\n", fn, strerror(errno) );
		return (NULL);
	}
	return (f);
}
