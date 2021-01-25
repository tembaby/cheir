/* $From: C:\\src\\stack/RCS/stack.h,v 1.3 2001/10/09 19:53:38 tamer Exp tamer $ */

/* Tamer Embaby <tsemba@menanet.net> */

/*
 * 1) Unlimited stack size.
 * 2) Generic data types as stack elements, as long as they
 *    are pointers.
 * 3) Easy to use interface.  Less interaction with stack data
 *    structure itself.
 */

#if !defined(_STACK_H)
# define _STACK_H

#define DEFAULT_STACK_SIZE		(-1)		/* Unlimited */
#define STACK_SIZE_UNLIMITED		DEFAULT_STACK_SIZE

#define STK_PUSH(elm,stk) \
	(stk)->s_ops->push((void *)elm,stk)

#define STK_POP(stk) \
	(stk)->s_ops->pop(stk)

#define STK_TOP(stk) \
	(stk)->s_ops->top(stk)

typedef struct stack_elm {
	struct stack_elm 	*se_nxt;
	struct stack_elm 	*se_prv;
	void  			*se_data;
} stack_elm;

struct stack;

struct stack_ops {
	int		(*push)(void *,struct stack *);
	void 		*(*pop)(struct stack *);
	void 		*(*top)(struct stack *);
};

typedef struct stack {
	int			s_size;
	int 			s_maxsize;
	stack_elm 		*s_stack;
	stack_elm 		*s_top;
	struct stack_ops 	*s_ops;
} stack;

stack *newstack(int);
void free_stack(stack *);

#endif /* !_STACK_H */
