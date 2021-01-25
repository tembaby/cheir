/* $From: C:\\src\\stack/RCS/stack.c,v 1.2 2001/10/09 19:45:30 tamer Exp tamer $ */

/* Tamer S. Embaby <tsemba@menanet.net> */

/* Sun Jun 25 15:05:39 EET 2000 */
/* This code is free as long as this comment and the above remain intact. */

#include <stdio.h>
#include <stdlib.h>

#include <stack.h>

#define STACK_DEBUG

/* Default stack operations. */
static int 	push(void *,stack *);
static void  	*pop(stack *);
static void  	*top(stack *);

/* Stack `doubly linked list' manpulation routines. */
static stack_elm *stk_alloc(void);
static void 	stk_set_data(void *,stack_elm *);
static void 	stk_plug(stack *,stack_elm *);
static void 	stk_detach(stack *,stack_elm *);
static void 	stk_free(stack_elm *);

stack *newstack(int size)
{
	stack *stk;

	stk = malloc(sizeof *stk);
	if (stk == NULL)
		return NULL;

	stk->s_ops = malloc(sizeof *(stk->s_ops) );
	if (stk->s_ops == NULL) {
		free(stk);
		return NULL;
	}

	stk->s_ops->push = push;
	stk->s_ops->pop = pop;
	stk->s_ops->top = top;
	stk->s_size = 0;
	stk->s_maxsize = (size > 0) ? size : DEFAULT_STACK_SIZE;
	stk->s_stack = stk->s_top = NULL;
	return stk;
}

void free_stack(stack *stk)
{
	stack_elm *elm, *tmp;

	elm = stk->s_stack;
	while (elm) {
		tmp = elm->se_nxt;
		stk_detach(stk, elm);
		stk_free(elm);
		elm = tmp;
	}

	free(stk->s_ops);
	free(stk);
	return;
}

static int push(void *data, stack *stk)
{
	stack_elm *elm;

	if (stk == NULL)
		return 0;

	if (stk->s_size >= stk->s_maxsize &&
	    stk->s_maxsize != STACK_SIZE_UNLIMITED)
		return 0;

	elm = stk_alloc();
	if (elm == NULL)
		return 0;

	stk_set_data(data, elm);
	stk_plug(stk, elm);
	stk->s_size++;
	return 1;
}

/* Returns pointer to data, data could be malloc()'d, so caller must free()
 * its memory. */
static void *pop(stack *stk)
{
	void *data;

	data = NULL;

	if (stk->s_top) {
		data = stk->s_top->se_data;
		stk_detach(stk, stk->s_top);
		/*stk_free(stk->s_top);*/ /* XXX This frees the stack element memory? */
		stk->s_size--;
	}
	return data;
}

static void *top(stack *stk)
{

	if (stk->s_top)
		return stk->s_top->se_data;
	return NULL;
}

static stack_elm *stk_alloc()
{
	stack_elm *elm;

	elm = malloc(sizeof *elm);
	if (elm == NULL)
		return NULL;

	elm->se_nxt = elm->se_prv = NULL;
	elm->se_data = NULL;
	return elm;
}

static void stk_set_data(void *data, stack_elm *elm)
{

	elm->se_data = data;
	return;
}

static void stk_plug(stack *stk, stack_elm *elm)
{

	if (stk->s_top == NULL) {
		stk->s_top = elm;
		stk->s_stack = elm;
		return;
	}

	stk->s_top->se_nxt = elm;
	elm->se_prv = stk->s_top;
	stk->s_top = elm;
	return;
}

static void stk_detach(stack *stk, stack_elm *elm)
{

	if (elm->se_prv) {
		stk->s_top = elm->se_prv;
		stk->s_top->se_nxt = NULL;
		return;
	}

	stk->s_top = stk->s_stack = NULL;
	return;
}

static void stk_free(stack_elm *elm)
{

	/*free(elm->se_data);*/ /* XXX uncomment me */
	return;
}
