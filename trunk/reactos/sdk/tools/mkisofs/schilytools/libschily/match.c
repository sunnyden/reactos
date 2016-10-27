/* @(#)match.c	1.24 10/08/21 Copyright 1985, 1995-2010 J. Schilling */
#include <schily/standard.h>
#include <schily/patmatch.h>
/*
 *	Pattern matching functions
 *
 *	Copyright (c) 1985, 1995-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
/*
 *	The pattern matching functions below are based on the algorithm
 *	presented by Martin Richards in:
 *
 *	"A Compact Function for Regular Expression Pattern Matching",
 *	Software-Practice and Experience, Vol. 9, 527-534 (1979)
 *
 *	Several changes have been made to the original source which has been
 *	written in BCPL:
 *
 *	'/'	is replaced by	'!'		(to allow UNIX filenames)
 *	'(',')' are replaced by	'{', '}'
 *	'\''	is replaced by	'\\'		(UNIX compatible quote)
 *
 *	Character classes have been added to allow "[<character list>]"
 *	to be used.
 *	Start of line '^' and end of line '$' have been added.
 */

#undef	CHAR

#ifdef	__LINE_MATCH
#ifdef	__WIDE_CHAR
#define	patmatch	patwlmatch
#else
#define	opatmatch	opatlmatch
#define	patmatch	patlmatch
#endif
#endif

#ifdef	__WIDE_CHAR
#ifndef	__LINE_MATCH
#define	patcompile	patwcompile
#define	patmatch	patwmatch
#endif
#define	CHAR		wchar_t
#endif

#ifndef	CHAR
typedef	unsigned char	Uchar;
#define	CHAR		Uchar
#endif

#define	ENDSTATE	(-1)

/*
 *	The Interpreter
 */


/*
 *	put adds a new state to the active list
 */
#define	 put(ret, state, sp, n)	{		\
	register int *lstate	= state;	\
	register int *lsp	= sp;		\
	register int ln		= n;		\
						\
	while (lstate < lsp) {			\
		if (*lstate++ == ln) {		\
			ret = lsp;		\
			lsp = 0;		\
			break;			\
		}				\
	}					\
	if (lsp) {				\
		*lstate++ = ln;			\
		ret = lstate;			\
	}					\
}


/*
 *	match a character in class
 */
#define	in_class(found, pat, c)	{			\
	register const CHAR	*lpat	= pat;		\
	register int		lc	= c;		\
	int	lo_bound;				\
	int	hi_bound;				\
							\
	found = FALSE;					\
							\
	if (*lpat == NOT) {				\
		lpat++;					\
		found = TRUE;				\
	}						\
	while (*lpat != RCLASS) {			\
		if (*lpat == QUOTE)			\
			lpat++;				\
		lo_bound = *lpat++;			\
		if (*lpat == RANGE) {			\
			lpat++;				\
			if (*lpat == QUOTE)		\
				lpat++;			\
			hi_bound = *lpat++;		\
		} else {				\
			hi_bound = lo_bound;		\
		}					\
		if (lo_bound <= lc && lc <= hi_bound) {	\
			found = !found;			\
			break;				\
		}					\
	}						\
}

/*
 *	opatmatch - the old external interpreter interface.
 *
 *	Trys to match a string beginning at offset
 *	against the compiled pattern.
 */
#ifndef	__WIDE_CHAR
EXPORT CHAR
*opatmatch(pat, aux, str, soff, slen, alt)
	const CHAR	*pat;
	const int	*aux;
	const CHAR	*str;
	int		soff;
	int		slen;
	int		alt;
{
	int		state[MAXPAT];

	return (patmatch(pat, aux, str, soff, slen, alt, state));
}
#endif

/*
 *	patmatch - the external interpreter interface.
 *
 *	Trys to match a string beginning at offset
 *	against the compiled pattern.
 */
EXPORT CHAR *
patmatch(pat, aux, str, soff, slen, alt, state)
	const CHAR	*pat;
	const int	*aux;
	const CHAR	*str;
	int		soff;
	int		slen;
	int		alt;
	int		state[];
{
	register int	*sp;
	register int	*n;
	register int	*i;
	register int	p;
	register int	q, s, k;
	int		c;
	const CHAR	*lastp = (CHAR *)NULL;

#ifdef	__LINE_MATCH
for (; soff <= slen; soff++) {
#endif

	sp = state;
	put(sp, state, state, 0);
	if (alt != ENDSTATE)
		put(sp, state, sp, alt);

	for (s = soff; ; s++) {
		/*
		 * next char from input string
		 */
		if (s >= slen)
			c = 0;
		else
			c = str[s];
		/*
		 * first complete the closure
		 */
		for (n = state; n < sp; ) {
			p = *n++;		/* next state number */
			if (p == ENDSTATE)
				continue;
			q = aux[p];		/* get next state for pat */
			k = pat[p];		/* get next char from pat */
			switch (k) {

			case REP:
				put(sp, state, sp, p+1);
				/* FALLTHRU */
			case NIL:		/* NIL matches always */
			case STAR:
				put(sp, state, sp, q);
				break;
			case LBRACK:		/* alternations */
			case ALT:
				put(sp, state, sp, p+1);
				if (q != ENDSTATE)
					put(sp, state, sp, q);
				break;
			case START:
				if (s == 0)
					put(sp, state, sp, q);
				break;
			case END:
				if (c == '\0')
					put(sp, state, sp, q);
				break;
			}
		}

		for (i = state; i < sp; ) {
			if (*i++ == ENDSTATE) {
				lastp = &str[s];
				break;
			}
		}
		if (c == 0)
			return ((CHAR *)lastp);

		/*
		 * now try to match next character
		 */
		n = sp;
		sp = state;
		for (i = sp; i < n; ) {
			p = *i++;		/* next active state number */
			if (p == ENDSTATE)
				continue;
			k = pat[p];
			switch (k) {

			case ALT:
			case REP:
			case NIL:
			case LBRACK:
			case START:
			case END:
				continue;
			case LCLASS:
				in_class(q, &pat[p+1], c);
				if (!q)
					continue;
				break;
			case STAR:
				put(sp, state, sp, p);
				continue;
			case QUOTE:
				k = pat[p+1];
			default:
				if (k != c)
					continue;
				/* FALLTHRU */
			case ANY:
				break;
			}
			put(sp, state, sp, aux[p]);
		}
		if (sp == state) {		/* if no new states return */
#ifdef	__LINE_MATCH

			if (lastp || (soff == slen - 1))
				return ((CHAR *)lastp);
			else
				break;
#else
			return ((CHAR *)lastp);
#endif
		}
	}
#ifdef	__LINE_MATCH
}
return ((CHAR *)lastp);
#endif
}


#ifndef	__LINE_MATCH
/*
 *	The Compiler
 */

typedef	struct args {
	const CHAR	*pattern;
	int		*aux;
	int		patp;
	int		length;
	CHAR		Ch;
} arg_t;

LOCAL	void	nextitem __PR((arg_t *));
LOCAL	int	prim	 __PR((arg_t *));
LOCAL	int	expr	 __PR((arg_t *, int *));
LOCAL	void	setexits __PR((int *, int, int));
LOCAL	int	join	 __PR((int *, int, int));

/*
 *	'read' the next character from pattern
 */
#define	rch(ap)						\
{							\
	if (++(ap)->patp >= (ap)->length)		\
		(ap)->Ch = 0;				\
	else						\
		(ap)->Ch = (ap)->pattern[(ap)->patp];	\
}

/*
 *	get the next item from pattern
 */
LOCAL void
nextitem(ap)
	arg_t	*ap;
{
	if (ap->Ch == QUOTE)
		rch(ap);
	rch(ap);
}

/*
 *	parse a primary
 */
LOCAL int
prim(ap)
	arg_t	*ap;
{
	int	a  = ap->patp;
	int	op = ap->Ch;
	int	t;

	nextitem(ap);
	switch (op) {

	case '\0':
	case ALT:
	case RBRACK:
		return (ENDSTATE);
	case LCLASS:
		while (ap->Ch != RCLASS && ap->Ch != '\0')
			nextitem(ap);
		if (ap->Ch == '\0')
			return (ENDSTATE);
		nextitem(ap);
		break;
	case REP:
		t = prim(ap);
		if (t == ENDSTATE)
			return (ENDSTATE);
		setexits(ap->aux, t, a);
		break;
	case LBRACK:
		a = expr(ap, &ap->aux[a]);
		if (a == ENDSTATE || ap->Ch != RBRACK)
			return (ENDSTATE);
		nextitem(ap);
		break;
	}
	return (a);
}

/*
 *	parse an expression (a sequence of primaries)
 */
LOCAL int
expr(ap, altp)
	arg_t	*ap;
	int	*altp;
{
	int	exits = ENDSTATE;
	int	a;
	int	*aux = ap->aux;
	CHAR	Ch;

	for (;;) {
		a = prim(ap);
		Ch = ap->Ch;
		if (Ch == ALT || Ch == RBRACK || Ch == '\0') {
			exits = join(aux, exits, a);
			if (Ch != ALT)
				return (exits);
			*altp = ap->patp;
			altp = &aux[ap->patp];
			nextitem(ap);
		} else
			setexits(aux, a, ap->patp);
	}
}

/*
 *	set all exits in a list to a specified value
 */
LOCAL void
setexits(aux, list, val)
	int	*aux;
	int	list;
	int	val;
{
	int	a;

	while (list != ENDSTATE) {
		a = aux[list];
		aux[list] = val;
		list = a;
	}
}

/*
 *	concatenate two lists
 */
LOCAL int
join(aux, a, b)
	int	*aux;
	int	a;
	int	b;
{
	int	t;

	if (a == ENDSTATE)
		return (b);
	t = a;
	while (aux[t] != ENDSTATE)
		t = aux[t];
	aux[t] = b;
	return (a);
}

/*
 *	patcompile - the external compiler interface.
 *
 *	The pattern is compiled into the aux array.
 *	Return value on success, is the outermost alternate which is != 0.
 *	Error is indicated by return of 0.
 */
EXPORT int
patcompile(pat, len, aux)
	const CHAR	*pat;
	int		len;
	int		*aux;
{
	arg_t	a;
	int	alt = ENDSTATE;
	int	i;

	a.pattern = pat;
	a.length  = len;
	a.aux	  = aux;
	a.patp    = -1;

	for (i = 0; i < len; i++)
		aux[i] = ENDSTATE;
	rch(&a);
	i = expr(&a, &alt);
	if (i == ENDSTATE)
		return (0);
	setexits(aux, i, ENDSTATE);
	return (alt);
}
#endif	/* LMATCH */
