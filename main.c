/* $Id: main.c,v 1.13 2002/10/11 19:17:05 te Exp $ */

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

/*
 * TODO:
 * strip preprocessor from within function definition. (OK)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined (_WINDOWS)
# define _POSIX_
#endif
#include <limits.h>
#if defined (_WINDOWS)
# undef _POSIX_
#endif
#include <sys/types.h>
#include <sys/stat.h>

#if defined(__FreeBSD__) || defined(linux) || defined (__OpenBSD__)
# include <dirent.h>
# include <unistd.h>
#endif

#if defined(__FreeBSD__) || defined(linux)
# include <getopt.h>
#endif

#if defined (_WINDOWS)
# include <getopt.h>
# include <io.h>
# include <direct.h>
#endif

#include <cheir.h>
#include <stack.h>

extern int	yylex(void);
extern FILE	*yyin;
extern int 	nr_lines;
extern int 	nr_chars;
#define curline (nr_lines+1)
#define RESET_LEX(strm) do { \
	yyrestart(strm); \
	nr_lines = 0; \
	nr_chars = 0; \
} while (0)

struct tokq_t tokens_que;
int tokns = 0;

/* Error(0): see BUGS */
#define PREP_CTX_NONE	0
#define PREP_CTX_IF	1
#define PREP_CTX_ELSE	2
static int prep_context;
static int prep_nest_nack; /* Unclosed '{' context */

#if 0
# define PROGRAMMER_DEBUG
#endif
#ifdef PROGRAMMER_DEBUG
# define log(x)		printf x
#else
# define log(x)
#endif

struct funcd_ctx *getfuncctx(struct mod_ctx *);
int 	push_token(int,int,char *);
void 	clean_tq(void);
int 	getfuncs(struct funcd_ctx *,struct mod_ctx *);
char 	*tokname(int);
void 	addfuncd(struct funcd_ctx *,struct mod_ctx *);
void	addfunc(struct func *,struct funcd_ctx *);
void	addheader(const char *,struct mod_ctx *);
void	dump_funcs(struct func_list *);
int	analyze(FILE *,const char *);
int	isregfile(const char *);
int	iscsource(const char *);
void	resolve(struct func *);
void	f_link(void);
void	output(char **);
void	output_fctx(struct funcd_ctx *,int);
void	indent(int,FILE *);
char	**split(register char *);
int	hash_handler(struct mod_ctx *);

extern	void yyrestart(FILE *);

char *singfile;
char *outdir;		/* Directory to output (extracted) source code */
char *indir;		/* Directory which C files reside */
char *startfuncs;	/* Function name to output/extract */
char *projname;		/* Name of the project; when generating Makefile */
int extract, verbose;
struct mod_ctx_list mods;

int
main(argc, argv)
	int argc;
	char **argv;
{
	int optc, fromstdin;
	FILE *in;
#if defined (_WINDOWS)
	char cwd[PATH_MAX];
	struct _finddata_t dent, *de;
	long dird;
#else
	DIR *dir;
	struct dirent *de;
#endif
	char **startfuncsvec;

	projname = indir = outdir = singfile = NULL;
	startfuncs = NULL;
	extract = verbose = 0;
	fromstdin = 0;
	TAILQ_INIT(&tokens_que);
	TAILQ_INIT(&mods);

	if (argv[argc - 1][0] == '-' &&
	    argv[argc - 1][1] == 0) {
		fromstdin = 1;
		argc--;
	}

	while ((optc = getopt(argc, argv, "f:o:xi:s:p:vV") ) != -1) {
		switch (optc) {
			case 'f':
				singfile = optarg;
				break;
			case 'o':
				outdir = optarg;
				break;
			case 'x':
				extract = 1;
				break;
			case 'i':
				indir = optarg;
				if (fromstdin != 0) {
					fprintf(stderr, "cheir: -i option"
					    "specified with stdin.  Ignoring"
					    "stdin\n");
					fromstdin = 0;
				}
				break;
			case 's':
				startfuncs = optarg;
				break;
			case 'v':
				verbose++;
				break;
			case 'p':
				projname = optarg;
				break;
			case 'V':
				printf("%s", VERSION);
				return (0);
		}
	}

	/* Defaults */
	if (startfuncs == NULL)
		startfuncs = "main";

	if (indir == NULL)
		indir = ".";

	if (outdir == NULL)
		outdir = "cheir-xcode";

	if ((startfuncsvec = split(startfuncs) ) == NULL) {
		fprintf(stderr, "cheir: cannot build start functions "
		    "vector.\n");
		return (ERROR_INVAL);
	}

#ifdef _MAKEFILE
	/* Makefile generator */
	if (projname == NULL)
		projname = "__NAME_OF_PROJECT";
	mkinit(projname);
#endif

	if (fromstdin != 0) {
		analyze(stdin, "stdin");
	} else if (singfile != NULL) {
		/* A file in -f flag */
		if ((in = fopen(singfile, "r") ) == NULL) {
			perror("fopen");
			return (1);
		}

		analyze(in, singfile);
		fclose(in);
#ifdef _MAKEFILE
		mkadd(singfile);
#endif
	} else {
		if (verbose != 0)
			printf("==> Starting from (%s)\n", indir);

#if defined (_WINDOWS)
		dird = -1;
#else
		if ((dir  = opendir(indir) ) ==  NULL) {
			perror(indir);
			return (ERROR_IO);
		}
#endif

		for (;;) {
#if defined (_WINDOWS)
#define d_name		name
			if (dird == -1) {
				getcwd(cwd, sizeof(cwd) );
				chdir(indir);
				if ((dird = _findfirst("*.*", &dent) ) < 0) {
					fprintf(stderr, "main: _findfirst: "
					    "error: %s", errno);
					break;
				}
			} else {
				if (_findnext(dird, &dent) < 0) {
					break;
				}
			}
			de = &dent;
#else
			if ((de = readdir(dir) ) == NULL)
				break;
#endif

			if (strcmp(de->d_name, ".") == 0 ||
			    strcmp(de->d_name, "..") == 0)
				continue;

			/* Scan only regular files */
			if (isregfile(de->d_name) == 0)
				continue;

			/* Scan only C files */
			if (iscsource(de->d_name) == 0)
				continue;

			if ((in = fopen(de->d_name, "r") ) == NULL) {
				perror("fopen");
				return (ERROR_IO);
			}

			analyze(in, de->d_name);
			fclose(in);
#ifdef _MAKEFILE
			mkadd(de->d_name);
#endif
		}
#if defined (_WINDOWS)
		chdir(cwd);
#else
		closedir(dir);
#endif
	}
	f_link();
	if (extract != 0)
		xoutput(startfuncsvec, outdir, indir);
	else
		output(startfuncsvec);
	return (0);
}

/*
 * Adds a new function definition context to a module structure.
 */
void
addfuncd(funcd, mod)
	struct funcd_ctx *funcd;
	struct mod_ctx *mod;
{

	if (TAILQ_FIRST(&mod->m_defs) == NULL)
		TAILQ_INSERT_HEAD(&mod->m_defs, funcd, fd_link);
	else
		TAILQ_INSERT_TAIL(&mod->m_defs, funcd, fd_link);
	funcd->fd_mod = mod;
	funcd->fd_mod->s.s_funcdcnt++;
	return;
}

void
addfunc(func, funcd)
	struct func *func;
	struct funcd_ctx *funcd;
{
	struct func *f;

	TAILQ_FOREACH(f, &funcd->fd_funcs, f_link) {
		if (strcmp(f->f_name, func->f_name) == 0) {
			/*
			 * Since we don't return a value
			 * we free memory here
			 */
			free(func);
			return;
		}
	}

	if (TAILQ_FIRST(&funcd->fd_funcs) == NULL)
		TAILQ_INSERT_HEAD(&funcd->fd_funcs, func, f_link);
	else
		TAILQ_INSERT_TAIL(&funcd->fd_funcs, func, f_link);
	func->f_caller = funcd;
	func->f_caller->fd_mod->s.s_funcscnt++;
	return;
}

void
addheader(hnam, mod)
	const char *hnam;
	struct mod_ctx *mod;
{
	struct header *h;

	if ((h = malloc(sizeof(struct header) ) ) == NULL) {
		fprintf(stderr, "addheader: out of dynamic memory\n");
		exit(1);
	}

	memset(h, 0, sizeof(struct header) );
	h->h_name = xstrdup(hnam);

	TAILQ_INSERT_HEAD(&mod->m_headers, h, h_link);
	return;
}

/*
 * Parser code
 */

int
push_token(t, l, id)
	int t, l;
	char *id;
{
	struct tokq *tq;
#define MAX_QTOKENS		40

	if (++tokns > MAX_QTOKENS) {
		tq = TAILQ_FIRST(&tokens_que);
		if (tq == NULL) {
			return (-1);
		}
		TAILQ_REMOVE(&tokens_que, tq, link);
	}
	tq = newtok();
	tq->line = l;
	tq->token = t;
	if (id != NULL)
		tq->id = xstrdup(id);
	if (TAILQ_FIRST(&tokens_que) == NULL)
		TAILQ_INSERT_HEAD(&tokens_que, tq, link);
	else
		TAILQ_INSERT_TAIL(&tokens_que, tq, link);
	return 0;
}

void
clean_tq()
{
	struct tokq *tq;

	while ((tq = TAILQ_FIRST(&tokens_que) ) != NULL) {
		TAILQ_REMOVE(&tokens_que, tq, link);
		if (tq->id != NULL)
			free(tq->id);
		free(tq);
	}
	tokns = 0;
	return;
}

int
getfuncs(funcd, mod)
	struct funcd_ctx *funcd;
	struct mod_ctx *mod;
{
	int token;
	int *tok;
	struct stack *stk;
	struct func *f;
	struct tokq *tq;
	int depth;

	/* Create a stack to hold levels of nested blocks */
	if ((stk = newstack(0) ) == NULL) {
		fprintf(stderr, "getfuncs: not enough memory\n");
		return (-1);
	}

	/* A queue where we put tokens we interested in. */
	clean_tq();

	/* First push the opening function */
	if ((tok = malloc(sizeof(int) ) ) == NULL) {
		fprintf(stderr, "getfuncs: not enough memory\n");
		return (-1);
	}
	*tok = LBRACE;
	STK_PUSH(tok, stk);
	log(("getfuncs: ++ START: pushed default START for %s\n",
	    funcd->fd_name));

	for (;;) {
		if ((token = yylex() ) <= 0) {
			printf("0: <<EOF>> module: %s\n",
			    funcd->fd_mod->m_name);
			clean_tq();
			return (-1);
		}

		if (token == HASH) {
			hash_handler(mod);
			continue;
		}

		if (token != NEWLINE)
			log(("getfuncs: got token %s at %d\n", tokname(token),
			    curline));

		if (token == LBRACE) {	/* { */
			if ((tok = malloc(sizeof(int) ) ) == NULL) {
				fprintf(stderr,
				    "getfuncs: not enough memory\n");
				clean_tq();
				return (-1);
			}
			*tok = RBRACE;

			if (prep_context == PREP_CTX_ELSE) {
				if (prep_nest_nack-- != 0)
					/*IGNORE_PUSH*/
					goto ignore_push;
			} else if (prep_context == PREP_CTX_IF) {
				prep_nest_nack++;
				/*ALLOW_PUSH*/
			}

			STK_PUSH(tok, stk);
			log(("getfuncs: pushed new START at %d\n", curline));
ignore_push:
			;
		}

		if (token == RBRACE) { /* } */
			if (prep_context == PREP_CTX_IF) {
				prep_nest_nack--;
			}

			tok = STK_POP(stk);
			/* Finished */
			if (tok == NULL) {
				log(("getfuncs: -- END: poped <final> START "
				    "at %d\n", curline));
				clean_tq();
				return (0);
			}
			free(tok);
			log(("getfuncs: poped last START at %d\n", curline));
			/* See if it's the last context */
			if (STK_TOP(stk) == NULL) {
				log(("getfuncs: poped <final> START at %d\n",
				    curline));
				clean_tq();
				funcd->fd_eline = curline;
				return (0);
			}
		}

		if (token == LPAREN) {
			if (TAILQ_FIRST(&tokens_que) == NULL)
				continue;

			/*
			 * depth is used to signal constructs of the form:
			 *   func (
			 * l_paren prceded by ID.
			 */
			depth = 0;

#if defined (__FreeBSD__)
			TAILQ_FOREACH_REVERSE(tq, &tokens_que, tokq_t, link) {
#else
			TAILQ_FOREACH_REVERSE(tq, &tokens_que, link, tokq_t) {
#endif
				depth++;
				if (tq->token == ID)
					break;
			}

			if (tq != NULL && depth == 1) {
				f = newfunc(tq->id);
				addfunc(f, funcd);
			}
			clean_tq();
		}

		if (token == ID) {
			push_token(token, 0, yylval.yyv_id);
		} else {
			clean_tq();
		}
	}
	/* NOTREACHED */
	return (0);
}

#define SKIP_TO_NEXT_RPAREN() do { \
	for (;;) { \
		if ((token = yylex() ) <= 0) { \
			log(("2: <<EOF>>\n")); \
			return (NULL); \
		} \
		if (token == RPAREN) { \
			break; \
		} \
	} \
} while (0)


#define	GETFDLINE()	(TAILQ_FIRST(&tokens_que)->line)
#define	GETFDNAME()	((TAILQ_LAST(&tokens_que, tokq_t))->id)
/*
 * getfuncctx() and getfuncs() are tightly coupled, meaning that they
 * have to work together.
 */
struct funcd_ctx *
getfuncctx(mod)
	struct mod_ctx *mod;
{
	int token, prvtoken;
	char *id;
	struct funcd_ctx *fc;

	for (;;) {
		if ((token = yylex() ) <= 0) {
			log(("3: <<EOF>>\n"));
			return (NULL);
		}

		if (token == SEMICOL) {
			clean_tq();
			continue;
		}

		if (token == HASH) {
			hash_handler(mod);
			continue;
		}

		if (token == LPAREN) {
			int depth;

			log(("matched '(' at line %d\n", curline));
			if (prvtoken != ID) {
				/*
				 * This fired error in situations like:
				 * static int *some_var = (int *)0;
				 */
				SKIP_TO_NEXT_RPAREN();
				continue;
			}
			depth = 0;

			/* TODO: gaurd against declspecs(xxx_here) */

			/*
			 * Skip to next ')'. Take into account nesting `('
			 * like int name __P((void));
			 */
			for (;;) {
				token = yylex();
				if (token <= 0) {
					log(("5: <<EOF>>\n"));
					return NULL;
				}
				if (token == LPAREN)
					depth++;
				if (token == RPAREN) {
					if (depth == 0)
						break;
					depth--;
				}
			}
			log(("skip to matching ')' at line %d\n", curline));
			/* cur token is ')' */

			token = yylex();
			if (token <= 0) {
				log(("6: <<EOF>>\n"));
				return (NULL);
			}
			if (token == SEMICOL) {
				log(("semicol; (func proto) cleanup\n"));
				clean_tq();
				prvtoken = 0;
				continue;
			}
			/* skip (old_style/?) to '{' or to ';' */
			for (;;) {
				if (token == LBRACE) {
					log(("-> would start new func\n"));
					fc = newfctx(GETFDNAME(), GETFDLINE() );

					if (strcmp(TAILQ_FIRST(&tokens_que)->id,
					    "static") == 0) {
						fc->fd_scope = SCOPE_MOD;
					} else
						fc->fd_scope = SCOPE_GLOB;

					clean_tq();
					return (fc);
				}
				token = yylex();
				if (token <= 0) {
					log(("7: <<EOF>>\n"));
					return (NULL);
				}
			}
		}

		/* Don't push new line */
		if (token == NEWLINE) {
			continue;
		}

		log(("push %s (%d) line %d\n", tokname(token), token, curline));
		if (token == ID)
			id = yylval.yyv_id;
		else
			id = "dont_care";
		push_token(token, curline, id);

		prvtoken = token;

		/*
		 * SOCK get()
		 * SOCK WINAPI get()
		 * EXTERN SOCK __stdcall get()
		 * int declspec(dllexport) func()
		 * static int fifo()
		 * volatile int exit()
		 * #define LOAD(x)	{ stuff }
		 *
		 * ID   '('            ')'               '{'
		 * func  (  params_XXX  )  old_style_XXX  {    }
		 *          AnyThing
		 *                             ;
		 */
	}
	return (NULL);
}

/*
 * Debugging routines.
 */

char *
tokname(tok)
{
	static char *toks[] = {
		"unknwon"	/* 0   */,
		"IF"		/* 1   */,
		"FOR"		/* 2   */,
		"WHILE"		/* 3   */,
		"SWITCH"	/* 4   */,
		"LPAREN"	/* 5   */,
		"RPAREN"	/* 6   */,
		"LBRACE"	/* 7   */,
		"RBRACE"	/* 8   */,
		"COMMA"		/* 9   */,
		"SEMICOL"	/* 10  */,
		"ID"		/* 11  */,
		"INCLUDEHDR"	/* 12  */,
		"LABRA"		/* 13  */,
		"RABRA"		/* 14  */,
		"HASH"		/* 15  */,
		"BSLASH"	/* 16  */,
		"NEWLINE"	/* 17  */
		"CONSTINT"	/* 18  */,
		"CONSTFLT"	/* 19  */,
		"SIZEOF"	/* 20  */,
		"RETURN"	/* 21  */,
		"ASSIGN"	/* 22  */,
		"OPERATOR"	/* 23  */,
		"ENDIF"		/* 24  */,
		"ELSE"		/* 25  */,
		"IFDEF"		/* 26  */,
		"IFNDEF"	/* 27  */,
		"RBRACKET"	/* 28  */,
		"LBRACKET"	/* 29  */
	};
	static int toklen = sizeof(toks)/sizeof(toks[0]);

	if (tok <= 0 || tok >= toklen)
		tok = 0;
	fprintf(stdout, "tokname: %d\n", tok);
	fprintf(stdout, "tokname: %s\n", toks[tok]);
	return toks[tok];
}

void
dump_funcs(funcs)
	struct func_list *funcs;
{
	register struct func *f;

	TAILQ_FOREACH(f, funcs, f_link) {
		printf("\t-> %s\n", f->f_name);
	}
	return;
}

int
iscsource(file)
	const char *file;
{
	char * p;

	p = (char *)file + (strlen(file) - 1);

	if (*p == 'c')
		if (*--p == '.')
			return (1);
	return (0);
}

int
isregfile(file)
	const char *file;
{
	struct stat s;

	if (stat(file, &s) < 0) {
		perror("stat");
		return (0);
	}

#if defined (_WINDOWS)
	if ((_S_IFREG & s.st_mode) != _S_IFREG) {
#else
	if (S_ISREG(s.st_mode) == 0) {
#endif
		return (0);
	}
	return (1);
}

int
analyze(fin, modnam)
	FILE *fin;
	const char *modnam;
{
	struct funcd_ctx *fc;
	struct mod_ctx *mod;
	struct func *f;
	struct header *h;

	mod = newmod(modnam);
	//yyin = fin;
	RESET_LEX(fin);

	if (verbose != 0)
		printf("\n==> starting new module analysis (%s)\n", modnam);

	for (;;) {
		/* Get a function definition */
		if ((fc = getfuncctx(mod) ) == NULL)
			break;

		addfuncd(fc, mod);

		/* Get called functions list */
		if (getfuncs(fc, mod) < 0)
			break;

		if (verbose != 0) {
			printf("function (%s) start (%d) ", fc->fd_name,
	 		    fc->fd_sline);
			printf("end (%d) scope (%s) module (%s):\n",
			    fc->fd_eline,
			    fc->fd_scope == SCOPE_MOD ? "module" : "external",
			    fc->fd_mod->m_name);
			TAILQ_FOREACH(f, &fc->fd_funcs, f_link) {
				printf("\t%s\n", f->f_name);
			}
			printf("\n");
			fflush(stdout);
		}
	}

	if (verbose != 0) {
		printf("\nmodule (%s) includes:\n", mod->m_name);
		TAILQ_FOREACH(h, &mod->m_headers, h_link) {
			printf("\t<%s>\n", h->h_name);
		}
		printf("\n");
	}

	mod->s.s_loc = curline;

	if (TAILQ_FIRST(&mods) == NULL)
		TAILQ_INSERT_HEAD(&mods, mod, m_link);
	else
		TAILQ_INSERT_TAIL(&mods, mod, m_link);
	if (verbose != 0) {
		printf("analyze: module: %s: %d definition%s, %d call%s, %d"
		    " line%s\n",
		    mod->m_name, mod->s.s_funcdcnt,
		    mod->s.s_funcdcnt > 1 ? "s" : "",
		    mod->s.s_funcscnt,
		    mod->s.s_funcscnt > 1 ? "s" : "",
		    mod->s.s_loc, mod->s.s_loc > 1 ? "s" : "");
	}
	return (0);
}

/*
 * Assigns a function calling structure to function definition.
 *
 * TODO: After resolveing function call to function definition,
 * we could insert it in some caching storage for later resolving.
 */
void
resolve(func)
	struct func *func;
{
	struct mod_ctx *mod;
	struct funcd_ctx *fc;

	if (func->f_caller == NULL) {
		fprintf(stderr, "resolve: FATAL: function %s called from "
		    "no where!.  Contact programmer\n",
		    func->f_name);
		exit(4);
	}

	if (func->f_caller->fd_mod == NULL) {
		fprintf(stderr, "resolve: FATAL: function %s called from %s "
		    "while %s has NULL module.  Contact programmer\n",
		    func->f_name, func->f_caller->fd_name,
		    func->f_caller->fd_name);
		exit(4);
	}

	/* Recursive calls */
	if (strcmp(func->f_name, func->f_caller->fd_name) == 0) {
		func->f_def = func->f_caller;
		func->f_type = FUNC_RECURSIVE;
		return;
	}

	func->f_type = FUNC_EXTERNAL;

	/* Try to resolve it in same module */
	TAILQ_FOREACH(mod, &mods, m_link) {
		if (strcmp(mod->m_name, func->f_caller->fd_mod->m_name) == 0)
			break;
	}

	if (mod == NULL) {
		fprintf(stderr, "resolve: FATAL: function %s called from %s "
		    "while %s not on any module.  Contact programmer\n",
		    func->f_name, func->f_caller->fd_name,
		    func->f_caller->fd_name);
		exit(4);
	}

	TAILQ_FOREACH(fc, &mod->m_defs, fd_link) {
		if (strcmp(fc->fd_name, func->f_name) == 0) {
			func->f_def = fc;
			func->f_type = FUNC_RESOLVED;
			return;
		}
	}

	/* Try to resolve it in other modules */
	TAILQ_FOREACH(mod, &mods, m_link) {
		TAILQ_FOREACH(fc, &mod->m_defs, fd_link) {
			if (fc->fd_scope != SCOPE_MOD &&
			    strcmp(func->f_name, fc->fd_name) == 0) {
				func->f_def = fc;
				func->f_type = FUNC_RESOLVED;
				return;
			}
		}
	}
	return;
}

void
f_link()
{
	register struct mod_ctx *mod;
	register struct funcd_ctx *fc;
	register struct func *func;

	if (verbose != 0)
		printf("\n==> linking all function calls ...\n");
	fflush(stdout);

	TAILQ_FOREACH(mod, &mods, m_link) {
		if (verbose != 0) {
			printf("module: %s: %d definition%s, %d call%s\n",
			    mod->m_name, mod->s.s_funcdcnt,
			    mod->s.s_funcdcnt > 1 ? "s" : "",
			    mod->s.s_funcscnt,
			    mod->s.s_funcscnt> 1 ? "s" : "");
		}
		TAILQ_FOREACH(fc, &mod->m_defs, fd_link) {
			TAILQ_FOREACH(func, &fc->fd_funcs, f_link) {
				resolve(func);
			}
		}
	}
	return;
}

char **
split(s)
	register char *s;
{
	int i, mem, slen;
	size_t msize;
	register char *p, *e;
	char **vec;

	mem = i = 0;
	p = s;
	vec = NULL;
	slen = strlen(s);

#if 0
	printf("split: parsing: %s\n", s);
#endif

	while (p < (s + slen) ) {
		e = p;

		/* Igonre LWS */
		while ((*e == ' ' || *e == '\t') && *e != 0)
			e++;
		if (*e == 0)
			break;
		p = e;

		while ((*e != ' ' && *e != '\t') && *e != 0)
			e++;
		msize = ((i + 1) + 1) * sizeof(char *);
		if (vec == NULL)
			vec = malloc(msize);
		else
			vec = realloc(vec, msize);
		if (vec == NULL) {
			fprintf(stderr, "split: out of dynamic memory\n");
			exit(1);
		}
		mem = 1;
		vec[i] = xstrndup(p, e - p);

#if 0
		printf("split: vec[%d] = [%s]\n", i, vec[i]);
#endif
		p = e;
		i++;
	}

	if (mem == 0)
		return (NULL);

	vec[i] = NULL;
#if 0
	printf("split: vec[%d] = NULL\n", i);
#endif
	return (vec);
}

static int oline_number;

void
output(startfuncs)
	char **startfuncs;
{
	char **p;
	struct funcd_ctx *fc;

	oline_number = 0;
	for (p = startfuncs; *p != NULL; p++) {
		fc = getfuncdbynam(*p);
		if (fc != NULL)
			output_fctx(fc, 0);
		else
			printf("%s: not found.  Ignored\n", *p);
	}
	return;
}

#define PRE_PRINT() do { \
	oline_number++; \
	printf("%5d.  ", oline_number); \
} while (0)

void
output_fctx(fc, level)
	struct funcd_ctx *fc;
	int level;
{
	register struct func *f;

	PRE_PRINT();
	indent(level, stdout);
	if (fc->oscop.os_visit != 0) {
		printf("(*) %s (%s) see line %d\n", fc->fd_name,
		    fc->fd_mod->m_name, fc->oscop.os_line);
		return;
	}
	printf("-> %s (%s)\n", fc->fd_name, fc->fd_mod->m_name);
	fc->oscop.os_line = oline_number;
	fc->oscop.os_visit++;

	level++;
	TAILQ_FOREACH(f, &fc->fd_funcs, f_link) {
		switch (f->f_type) {
			case FUNC_EXTERNAL:
				PRE_PRINT();
				indent(level, stdout);
				printf("%s ", f->f_name);
				printf("(external)\n");
				break;
			case FUNC_RECURSIVE:
				PRE_PRINT();
				indent(level, stdout);
				printf("%s ", f->f_name);
				printf("(recursive)\n");
				break;
			case FUNC_RESOLVED:
				if (f->f_def == NULL) {
					fprintf(stderr, "output_fctx: "
					    "inconsistency: %s marked resolved "
					    "but doesn't have definition "
					    "pointer\n", f->f_name);
					break;
				}
				output_fctx(f->f_def, level);
				break;
			default:
				printf("(BULLSHIT)\n");
				break;
		}
	}
	return;
}

#define DEFINITION_ADD(ldef,sd) do { \
	if (TAILQ_FIRST(ldef) == NULL) \
		TAILQ_INSERT_HEAD(ldef, sd, g_link); \
	else \
		TAILQ_INSERT_TAIL(ldef, sd, g_link); \
} while (0)
#define DEFINITION_FREE(ldef) do { \
	struct _generic_ptr *g; \
	while ((g = TAILQ_FIRST(ldef) ) != NULL) { \
		TAILQ_REMOVE(ldef, g, g_link); \
		free(g); \
	} \
} while (0)

struct funcd_ctx *
getfuncdbynam(fnam)
	char *fnam;
{
	int scop_modnr, scop_globnr;
	char *modnam, *p;
	struct mod_ctx *mod;
	struct funcd_ctx *fc, *fc_smod, *fc_sglob;
	struct _generic_ptr {
		TAILQ_ENTRY(_generic_ptr) g_link;
		void *g_ptr;
	};
	TAILQ_HEAD(_gptr_list_t, _generic_ptr);
	struct _gptr_list_t sdefs, gdefs;
	struct _generic_ptr *gp;

	/*
	 * Lists of all static/external function definition found
	 * for a given routine.
	 */
	TAILQ_INIT(&sdefs);
	TAILQ_INIT(&gdefs);

	modnam = NULL;
	scop_modnr = scop_globnr = 0;

	/* Resolve module_name:function_name notation. */
	if ((p = strchr(fnam, ':') ) != NULL) {
		modnam = xstrndup(fnam, p - fnam);
		fnam += p - fnam + 1;
	}

	fc_smod = fc_sglob = NULL;
	TAILQ_FOREACH(mod, &mods, m_link) {
		TAILQ_FOREACH(fc, &mod->m_defs, fd_link) {
			if (strcmp(fnam, fc->fd_name) == 0) {
				/*
				 * If we're instructed to extract a certain
				 * routine from a specified module we
				 * don't take into account the scope
				 * of this routine, we just match it (if
				 * we could) and exit all loops.
				 */
				if (modnam != NULL &&
				    strcmp(modnam, fc->fd_mod->m_name) == 0) {
					fc_sglob = fc;
					scop_globnr = 1;
					goto found;
				}

				gp = xmalloc(sizeof(*gp), "_gptr");
				if (fc->fd_scope == SCOPE_GLOB) {
#if defined (FAVOR_FIRST)
					if (fc_sglob == NULL)
#endif
						fc_sglob = fc;
					gp->g_ptr = fc;
					DEFINITION_ADD(&gdefs, gp);
					scop_globnr++;
				} else if (fc->fd_scope == SCOPE_MOD) {

#if defined (FAVOR_FIRST)
			/*
			 * If we found more than one occurence for function
			 * definition, this function returns the last one
			 * found.  Define FAVOR_FIRST to return the first
			 * occurence.
			 */
					if (fc_smod == NULL)
#endif
						fc_smod = fc;
					gp->g_ptr = fc;
					DEFINITION_ADD(&sdefs, gp);
					scop_modnr++;
				} else {
					fprintf(stderr, "ERROR: function has "
					    "unknown scope type!\n");
					free(gp);
					return (NULL);
				}
			}
		}
	}

found:
	if (modnam != NULL)
		free(modnam);

	if (verbose != 0) {
		printf("==> %s: %d static definition%s, %d external "
		    "definition%s\n", fnam, scop_modnr,
		    scop_modnr > 1 ? "s" : "", scop_globnr,
		    scop_globnr > 1 ? "s" : "");
		fflush(stdout);
	}

	if (fc_sglob != NULL) {
		if (scop_globnr > 1) {
			printf("%s: defined extrnal more than once!\n", fnam);
			TAILQ_FOREACH(gp, &gdefs, g_link) {
				fc = gp->g_ptr;
				printf("%s:  in module %s line %d\n", fnam,
				    fc->fd_mod->m_name, fc->fd_sline);
			}
			printf("%s: try [module:function].  ", fnam);
#ifdef FAVOR_FIRST
			printf("For now will favor first ...\n");
#else
			printf("For now will favor last ...\n");
#endif
		}
		DEFINITION_FREE(&sdefs);
		DEFINITION_FREE(&gdefs);
		return (fc_sglob);
	}

	if (scop_modnr > 1) {
		printf("%s: defined static more than once.\n", fnam);
		TAILQ_FOREACH(gp, &sdefs, g_link) {
			fc = gp->g_ptr;
			printf("%s:  in module %s line %d\n", fnam,
			    fc->fd_mod->m_name, fc->fd_sline);
		}
		printf("%s: try -s [module:function].");
		DEFINITION_FREE(&sdefs);
		DEFINITION_FREE(&gdefs);
		return (NULL);
	}

	DEFINITION_FREE(&sdefs);
	DEFINITION_FREE(&gdefs);
	return (fc_smod);
}

void
indent(level, strm)
	FILE *strm;
{
	int i;

	for (i = 0; i < level; i++)
		printf("\t");
	return;
}

int
hash_handler(mod)
	struct mod_ctx *mod;
{
	int token, prvtok;
	int skipnextnl = 0;

	/*
	 * Preprocessor; skip to the next new line
	 * taking into account \ followed by \n
	 *
	 * This totally eats the preprocessor macro.
	 *
	 * And in encountered include directive it will
	 * add it to `mod' struct header list.
	 */

	prvtok = HASH;

	for (;;) {
		if ((token = yylex() ) <= 0) {
			log(("hash_hand: <<EOF>> at %d\n", curline));
			return (-1);
		}

		if ((token == IF || token == IFDEF || token == IFNDEF) &&
		    prvtok == HASH) {
			log((">> #IF[[n]DEF] %s:%d\n",
			    mod->m_name, curline));
			prep_nest_nack = 0;
			prep_context = PREP_CTX_IF;
		}

		if (token == ELSE && prvtok == HASH) {
			log((">> #ELSE %s:%d\n", mod->m_name, curline));
			log((">> #ELSE nack=%d\n", prep_nest_nack));
			prep_context = PREP_CTX_ELSE;
		}

		if (token == ENDIF && prvtok == HASH) {
			log((">> #ENDIF %s:%d\n", mod->m_name, curline));
			prep_context = PREP_CTX_NONE;
			prep_nest_nack = 0;
		}

		if (token == BSLASH) {
			skipnextnl = 1;
			continue;
		}

		if (token == NEWLINE) {
			if (skipnextnl == 0)
				break;
			skipnextnl = 0;
		}

		if (token == INCLUDEHDR)
			addheader(yylval.yyv_include, mod);
		prvtok = token;
	}
	return (0);
}
