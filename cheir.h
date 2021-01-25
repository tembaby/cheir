/* $Id: cheir.h,v 1.13 2002/10/11 19:16:02 te Exp $ */

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

#if !defined (_CHEIR_H)
# define _CHEIR_H

#if defined (_WINDOWS)
# include <bsd_list.h>
# define snprintf	_snprintf
#elif defined (__OpenBSD__) || defined (__FreeBSD__)
# include <sys/queue.h>
#endif

/*#define LEX_DEBUG*/

#define VERSION		"cheir v1.0.8 ($Id: cheir.h,v 1.13 2002/10/11 19:16:02 te Exp $)\n"

#define ERROR_MEM	1
#define ERROR_IO	2
#define ERROR_INVAL	3

#define IF		1
#define FOR		2
#define WHILE		3
#define SWITCH		4
#define LPAREN		5
#define RPAREN		6
#define LBRACE		7
#define RBRACE		8
#define COMMA		9
#define SEMICOL		10
#define ID		11
#define INCLUDEHDR	12
#define LABRA		13
#define RABRA		14
#define HASH		15
#define BSLASH		16
#define NEWLINE		17
#define CONSTINT	18
#define CONSTFLT	19
#define SIZEOF		20
#define RETURN		21
#define ASSIGN		22
#define OPERATOR	23
#define ENDIF		24
#define ELSE		25
#define IFDEF		26
#define IFNDEF		27
#define RBRACKET	28
#define LBRACKET	29

#define DEBUG

struct header {
	TAILQ_ENTRY(header) h_link;
	char *h_name;
};
TAILQ_HEAD(header_list, header);

struct funcd_ctx;

struct func {
	TAILQ_ENTRY(func) f_link;
	char *f_name;
#define FUNC_EXTERNAL	1
#define FUNC_RESOLVED	2
#define FUNC_RECURSIVE	3
	int f_type;
	struct funcd_ctx *f_caller;
	struct funcd_ctx *f_def;
};
TAILQ_HEAD(func_list, func);

struct mod_ctx;

struct funcd_ctx {
	TAILQ_ENTRY(funcd_ctx) fd_link;
	char *fd_name;
#define SCOPE_MOD	1		/* static function */
#define SCOPE_GLOB	2		/* external function (global) */
	int fd_scope;
	int fd_sline;			/* Start line */
	int fd_eline;			/* End line */
	struct mod_ctx *fd_mod;		/* Module */
	struct func_list fd_funcs;
	/* Screen output */
	struct oscope {
		int os_visit;
		int os_line;
	} oscop;
	/* Extract */
	struct sscope {
		int ss_visit;
	} sscop;
};
TAILQ_HEAD(funcd_ctx_list, funcd_ctx);

struct mod_ctx {
	TAILQ_ENTRY(mod_ctx) m_link;
	char *m_name;
	struct header_list m_headers;
	struct funcd_ctx_list m_defs;

	struct stats {
		int s_funcdcnt;		/* Function definition count */
		int s_funcscnt;		/* Functions called in module */
		int s_loc;		/* Lines of code */
	} s;
};
TAILQ_HEAD(mod_ctx_list, mod_ctx);

struct tokq {
	TAILQ_ENTRY(tokq) link;
	int token;
	int line;
	char *id;
};
TAILQ_HEAD(tokq_t, tokq);

/* Used by lexer/parser */
struct yylval {
	char *yyv_id;
	char *yyv_include;
} yylval;

struct	tokq *newtok(void);
struct	funcd_ctx *newfctx(const char *,int);
struct	mod_ctx *newmod(const char *);
char	*xstrdup(const char *);
char	*xstrndup(const char *,int);
struct	func *newfunc(const char *);
void	xoutput(char **,char *,char *);
struct	funcd_ctx *getfuncdbynam(char *);
void	*xmalloc(size_t,const char *);

/* mk.c */
#ifdef _MAKEFILE
int	mkinit(char *);
int	mkadd(char *);
int	mkgen(char *);
#endif

extern int verbose;
extern struct mod_ctx_list mods;

#endif /* !_CHEIR_H */
