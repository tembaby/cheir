/* $Id: mem.c,v 1.4 2002/04/16 03:23:42 te Exp $ */

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
#include <stdlib.h>
#include <string.h>

#include <cheir.h>

struct tokq *
newtok()
{
	struct tokq *tq;

	tq = xmalloc(sizeof(struct tokq), "newtok");
	return (tq);
}

struct funcd_ctx *
newfctx(fnam, slin)
	const char *fnam;
       	int slin;
{
	struct funcd_ctx *fc;

	fc = xmalloc(sizeof(struct funcd_ctx), "newfctx");
	TAILQ_INIT(&fc->fd_funcs);
	fc->fd_name = strdup(fnam);
	fc->fd_sline = slin;
	return (fc);
}

struct mod_ctx *
newmod(mnam)
	const char *mnam;
{
	struct mod_ctx *m;

	m = xmalloc(sizeof(struct mod_ctx), "newmod");
	TAILQ_INIT(&m->m_headers);
	TAILQ_INIT(&m->m_defs);
	m->m_name = xstrdup(mnam);
	return (m);
}

char *
xstrdup(s)
	const char *s;
{
	char *d;

	if ((d = strdup(s) ) == NULL) {
		fprintf(stderr, "xstrdup: out of dynamix memory\n");
		exit(1);
	}
	return (d);
}

char *
xstrndup(s, n)
	const char *s;
{
	char *d;

	d = xmalloc(n + 1, "xstrndup");
	strncpy(d, s, n);
	d[n] = 0;
	return (d);
}

struct func *
newfunc(fnam)
	const char *fnam;
{
	struct func *f;

	f = xmalloc(sizeof(struct func), "newfunc");
	f->f_name = xstrdup(fnam);
	return (f);
}

void *
xmalloc(size, func)
	size_t size;
	const char *func;
{
	void *p;
	
	if ((p = malloc(size) ) == NULL) {
		fprintf(stderr, "%s: out of dynamic memory\n", func);
		exit(ERROR_MEM);
	}
	memset(p, 0, size);
	return (p);
}
