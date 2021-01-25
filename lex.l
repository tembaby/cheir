/* $Id: lex.l,v 1.8 2002/05/26 11:14:46 te Exp $ */

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

%x string comment inclusion

IDINIT		[_a-zA-Z]
IDCHARS		[_a-zA-Z0-9]
DIGIT		[0-9]

%{

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cheir.h>

#ifdef LEX_DEBUG
# define log(x)		printf x
#else
# define log(x)		
#endif

int nr_lines = 0, nr_chars = 0;
int preprocessor = 0;
int escape_seq = 0;
int include_dq_start = 0;
#if defined (_WINDOWS)
char incbuf[BUFSIZ<<3], *incp;
#else
char incbuf[BUFSIZ<<2], *incp;
#endif

void 	adv_c(void);
void 	adv_l(void);
int	id(void);
int	header(void);
int	include_end(void);

%}

%%
if	{ log(("if\n"));	adv_c(); return IF; }
for	{ log(("for\n"));	adv_c(); return FOR; }
while	{ log(("while\n"));	adv_c(); return WHILE; }
switch	{ log(("switch\n"));	adv_c(); return SWITCH; }
sizeof	{ adv_c(); return SIZEOF; }
return	{ adv_c(); return RETURN; }
endif	{ adv_c(); return ENDIF; }
else	{ adv_c(); return ELSE; }
ifdef	{ adv_c(); return IFDEF; }
ifndef	{ adv_c(); return IFNDEF; }

include	{ 
		adv_c(); 
		if (preprocessor == 0) {
			/* A special hack for `include' ID */
			return id(); 
		}
		BEGIN(inclusion);
	}

"#"	{ log(("hash\n")); 	preprocessor = 1; return HASH; }
"("	{ log(("l_paren\n"));	adv_c(); return LPAREN; }
")"	{ log(("r_paren\n"));	adv_c(); return RPAREN; }
"{"	{ log(("l_barce\n"));	adv_c(); return LBRACE; }
"}"	{ log(("r_brace\n"));	adv_c(); return RBRACE; }
"["	{ log(("r_bracket\n"));	adv_c(); return RBRACKET; }
"]"	{ log(("l_bracket\n"));	adv_c(); return LBRACKET; }
","	{ log(("comma\n"));	adv_c(); return COMMA; }
";"	{ log(("semicolon\n"));	adv_c(); return SEMICOL; }
"\\"	{ adv_c(); return BSLASH; }

"="	{ adv_c(); return ASSIGN; }
"/="	{ adv_c(); return ASSIGN; }
"+="	{ adv_c(); return ASSIGN; }
"-="	{ adv_c(); return ASSIGN; }
"%="	{ adv_c(); return ASSIGN; }
"|="	{ adv_c(); return ASSIGN; }
"&="	{ adv_c(); return ASSIGN; }
"<<="	{ adv_c(); return ASSIGN; }
">>="	{ adv_c(); return ASSIGN; }
"*="	{ adv_c(); return ASSIGN; }
"^="	{ adv_c(); return ASSIGN; }

"+"	{ adv_c(); return OPERATOR; }
"-"	{ adv_c(); return OPERATOR; }
"*"	{ adv_c(); return OPERATOR; }
"/"	{ adv_c(); return OPERATOR; }
"%"	{ adv_c(); return OPERATOR; }
"~"	{ adv_c(); return OPERATOR; }
"^"	{ adv_c(); return OPERATOR; }
"!"	{ adv_c(); return OPERATOR; }
"=="	{ adv_c(); return OPERATOR; }
"<="	{ adv_c(); return OPERATOR; }
"<"	{ adv_c(); return OPERATOR; }
"!="	{ adv_c(); return OPERATOR; }
">"	{ adv_c(); return OPERATOR; }
">="	{ adv_c(); return OPERATOR; }
">>"	{ adv_c(); return OPERATOR; }
"<<"	{ adv_c(); return OPERATOR; }
"&"	{ adv_c(); return OPERATOR; }
"|"	{ adv_c(); return OPERATOR; }
"?"	{ adv_c(); return OPERATOR; }
":"	{ adv_c(); return OPERATOR; }

{DIGIT}*		{
				log(("DIGIT: %s\n", yytext));
				adv_c();
				return CONSTINT;
			}
{DIGIT}+"."{DIGIT}*	{
				log(("FLOAT: %s\n", yytext));
				adv_c();
				return CONSTFLT;
			}

{IDINIT}+{IDCHARS}*	{
				log(("ID: %s\n", yytext));
				adv_c();
				return id(); 
			}

"/*"		{ BEGIN(comment); }
<comment>.	{ adv_c(); }
<comment>\n	{ adv_l(); }
<comment>"*/"	{ BEGIN(INITIAL); }

\"		{ 
			adv_c();
			BEGIN(string); 
		}
<string>\"	{ 
			adv_c();
			if (escape_seq == 0)
				BEGIN(INITIAL); 
			escape_seq = 0;
		}
<string>\\	{
			adv_c();
			escape_seq = !escape_seq;
		}
<string>\n	{ adv_l(); }
<string>.	{ 
			adv_c(); 
			escape_seq = 0;
		}

<inclusion>("<"|"\"")	{
				if (include_dq_start != 0) {
					include_dq_start = 0;
					return (include_end() );
				}
				if (yytext[0] == '"')
					include_dq_start = 1;
				adv_c();
				incp = incbuf;
				log(("include: buffer adjusted\n"));
			}
<inclusion>[[:space:]]	{ adv_c(); /* Eat spaces */ }
<inclusion>(">"|"\"")	{ 
				return (include_end() );
#if 0
				adv_c();
				*incp = 0;
				yylval.yyv_include = strdup(incbuf);
				BEGIN(INITIAL); 
				log(("include: %s\n", incbuf));
				return header();
#endif
			}
<inclusion>.	{ 
			adv_c();
			if ((incp - incbuf) > sizeof(incbuf) - 1) {
				fprintf(stderr, "buffer overrun\n");
				exit(1);
			}
			*incp++ = yytext[0]; 
			log(("include: [%c]\n", yytext[0]));
		}

\n		{ 
			if (preprocessor) {
				preprocessor = 0;
			}
			adv_l(); 
			return NEWLINE;
		}

.		{ /*log(("->[%s]\n", yytext));*/ adv_c(); /*return 23;*/ }
%%

void adv_c()
{

	nr_chars += yyleng;
	return;
}

void adv_l()
{

	nr_lines++;
	return;
}

int
header()
{

	yylval.yyv_include = strdup(incbuf);
	if (yylval.yyv_include == NULL) {
		fprintf(stderr, "lex: include: out of dynamic memory\n");
		exit(1);
	}
	return INCLUDEHDR;
}

int
id()
{
	
	yylval.yyv_id = strdup(yytext);
	if (yylval.yyv_id == NULL) {
		fprintf(stderr, "lex: id: out of dynamic memory\n");
		exit(1);
	}
	return ID; 
}

int
include_end()
{
	
	adv_c();
	*incp = 0;
	yylval.yyv_include = strdup(incbuf);
	BEGIN(INITIAL); 
	log(("include: %s\n", incbuf));
	return header();
}

int yywrap()
{

	if (verbose != 0)
		printf("lex: finished, nr_chars=%d, nr_lines=%d\n",
		    nr_chars, nr_lines);
	return 1;
}
