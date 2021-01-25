/* $Id: mk.c,v 1.1 2002/05/17 21:07:03 te Exp $ */ 

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
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined (_WINDOWS)
# include <bsd_list.h>
# include <io.h>
#else
# include <sys/queue.h>
# include <unistd.h>
#endif

#include <cheir.h>

static const char *epilog = "#\n# %s#\n\n";
static const char *target = "TARGET = %s\n";
static const char *objs = "\nOBJS = ";
static const char *default_objs_rule = "\n.c.o\t\t:\n\t$(CC) $(CFLAGS) $<\n\n";
static const char *target_link = 
	"\n$(TARGET)\t\t: $(OBJS)\n\t$(CC) -o $(TARGET) $(OBJS)\n\n";

struct cdepi {
	TAILQ_ENTRY(cdepi) c_link;
	char	*c_fnam;
	char	*c_onam;
};
TAILQ_HEAD(dep_list, cdepi);

struct mk {
	char	*m_pnam;		/* Target name */
	struct	dep_list m_deps;	/* List of C files */
} make;

int
mkinit(pnam)
	char *pnam;
{
	
	make.m_pnam = xstrdup(pnam);
	TAILQ_INIT(&make.m_deps);
	return (0);
}

int
mkadd(fnam)
	char *fnam;
{
	int len;
	char *p;
	struct cdepi *cp;

	cp = xmalloc(sizeof(struct cdepi), "mkadd");
	cp->c_fnam = xstrdup(fnam);
	if ((p = strstr(cp->c_fnam, ".c") ) != NULL)
		len = p - cp->c_fnam;
	else
		len = strlen(cp->c_fnam);
	len += 2;
	cp->c_onam = xmalloc(len + 1, "mkadd");
	strncpy(cp->c_onam, cp->c_fnam, len - 2);
	strncat(cp->c_onam, ".o", 2);
	if (TAILQ_FIRST(&make.m_deps) )
		TAILQ_INSERT_HEAD(&make.m_deps, cp, c_link);
	else
		TAILQ_INSERT_TAIL(&make.m_deps, cp, c_link);
	return (0);
}

int
mkgen(odir)
	char *odir;		/* Output directory */
{
	int fd, nbytes;
	struct cdepi *cp;
	char file[PATH_MAX];
	char buffer[BUFSIZ<<1];

#if defined(_WINDOWS)
	snprintf(file, sizeof(file), "%s\\Makefile", odir);
#else
	snprintf(file, sizeof(file), "%s/Makefile", odir);
#endif

#ifdef DEBUG
	if (verbose > 0)
		printf("generating make file: %s\n", file);
#endif
	
	if ((fd = open(file, O_WRONLY|O_CREAT|O_EXCL, 
	    S_IRUSR|S_IWUSR|S_IRGRP) ) < 0) {
		fprintf(stderr, "mkgen: open: %s: %s\n", file, 
		    strerror(errno) );
		return (-1);
	}
	
	/* XXX */
	nbytes = snprintf(buffer, sizeof(buffer), epilog, VERSION);
	if (write(fd, buffer, nbytes) < 0) {
		fprintf(stderr, "mkgen: write: %s: %s\n", file, 
		    strerror(errno) );
		goto error;
	}

	nbytes = snprintf(buffer, sizeof(buffer), target, make.m_pnam);
	if (write(fd, buffer, nbytes) < 0) {
		fprintf(stderr, "mkgen: write: %s: %s\n", file, 
		    strerror(errno) );
		goto error;
	}

	if (write(fd, objs, strlen(objs) ) < 0) {
		fprintf(stderr, "mkgen: write: %s: %s\n", file, 
		    strerror(errno) );
		goto error;
	}

	TAILQ_FOREACH(cp, &make.m_deps, c_link) {
		nbytes = snprintf(buffer, sizeof(buffer), "%s ", cp->c_onam);
		if (write(fd, buffer, nbytes) < 0) {
			fprintf(stderr, "mkgen: write: %s: %s\n", file, 
			    strerror(errno) );
			goto error;
		}
	}

	write(fd, "\n", 1);

	if (write(fd, default_objs_rule, strlen(default_objs_rule) ) < 0) {
		fprintf(stderr, "mkgen: write: %s: %s\n", file, 
		    strerror(errno) );
		goto error;
	}
	
	if (write(fd, target_link, strlen(target_link) ) < 0) {
		fprintf(stderr, "mkgen: write: %s: %s\n", file, 
		    strerror(errno) );
		goto error;
	}
	return (0);

error:
	close(fd);
	return (-1);
}
