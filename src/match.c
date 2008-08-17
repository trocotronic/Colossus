#ifndef NULL
#define NULL 0
#endif
extern const char NTL_tolower_tab[];
#define ToLower(c) (NTL_tolower_tab[(int)(c)])
/*
 * match()
 *  written by binary
 */
static int match2(char *mask, char *name)
{
	unsigned char *m;		/* why didn't the old one use registers */
	unsigned char *n;		/* because registers suck -- codemastr */
	unsigned char cm;
	unsigned char *wsn;
	unsigned char *wsm;

	m = (unsigned char *)mask;

	cm = *m;

#define lc(x) ToLower(x)

	n = (unsigned char *)name;
	if (cm == '*')
	{
		if (m[1] == '\0')	/* mask is just "*", so true */
			return 0;
	}
	else if (cm != '?' && lc(cm) != lc(*n))
		return 1;	/* most likely first chars won't match */
	else if ((*m == '\0') && (*n == '\0'))
		return 0;  /* true: both are empty */
	else if (*n == '\0')
		return 1; /* false: name is empty */
	else
	{
		m++;
		n++;
	}
	cm = lc(*m);
	wsm = (unsigned char *)NULL;
	wsn = (unsigned char *)NULL;
	while (1)
	{
		if (cm == '*')	/* found the * wildcard */
		{
			m++;	/* go to next char of mask */
			if (!*m)	/* if at end of mask, */
				return 0;	/* function becomes true. */
			while (*m == '*')	/* while the char at m is "*" */
			{
				m++;	/* go to next char of mask */
				if (!*m)	/* if at end of mask, */
					return 0;	/* function becomes true. */
			}
			cm = *m;
			if (cm == '\\')	/* don't do ? checking if a \ */
			{

				cm = *(++m);	/* just skip this char, no ? checking */
				/* In case of something like: '*\', return false. */
				if (!*m)
					return 1;
			}
			else if (cm == '?')	/* if it's a ? */
			{
				do
				{
					m++;	/* go to the next char of both */
					if (!*n)
						return 1; /* false: no character left */
					n++;
					if (!*n)	/* if end of test string... */
						return (!*m ? 0 : 1);	/* true if end of mask str, else false */
				}
				while (*m == '?');	/* while we have ?'s */
				cm = *m;
				if (!cm)	/* last char of mask is ?, so it's true */
					return 0;
			}
			cm = lc(cm);
			while (lc(*n) != cm)
			{	/* compare */
				if (!*n)	/* if at end of n string */
					return 1;	/* function becomes false. */
				n++;	/* go to next char of n */
			}
			wsm = m;	/* mark after where wildcard found */
			cm = lc(*(++m));	/* go to next mask char */
			wsn = n;	/* mark spot first char was found */
			n++;	/* go to next char of n */
			continue;
		}
		if (cm == '?')	/* found ? wildcard */
		{
			cm = lc(*(++m));	/* just skip and go to next */
			if (!*n)
				return 1; /* false: no character left */
			n++;
			if (!*n)	/* return true if end of both, */
				return (cm ? 1 : 0);	/* false if end of test str only */
			continue;
		}
		if (cm == '\\')	/* next char will not be a wildcard. */
		{		/* skip wild checking, don't continue */
			cm = lc(*(++m));
			n++;
		}
		/* Complicated to read, but to save CPU time.  Every ounce counts. */
		if (lc(*n) != cm)	/* if the current chars don't equal, */
		{
			if (!wsm)	/* if there was no * wildcard, */
				return 1;	/* function becomes false. */
			n = wsn + 1;	/* start on char after the one we found last */
			m = wsm;	/* set m to the spot after the "*" */
			cm = lc(*m);
			while (cm != lc(*n))
			{	/* compare them */
				if (!*n)	/* if we reached end of n string, */
					return 1;	/* function becomes false. */
				n++;	/* go to next char of n */
			}
			wsn = n;	/* mark spot first char was found */
		}
		if (!cm)	/* cm == cn, so if !cm, then we've */
			return 0;	/* reached end of BOTH, so it matches */
		m++;		/* go to next mask char */
		n++;		/* go to next testing char */
		cm = lc(*m);	/* pointers are slower */
	}
}

/* Old match() plus some optimizations from bahamut */
/*!
 * @desc: Establece una relación entre dos cadenas, una con comodines.
 * @params: $mask [in] Patrón con comodines.
 	    $name [in] Cadena a comparar.
 * @ret: Devuelve 0 si coincide y se ajusta al patrón; 0, si no.
 * @cat: Programa
 !*/

int match(char *mask, char *name)
{
	if (mask[0] == '*' && mask[1] == '!') {
		mask += 2;
		while (*name != '!' && *name)
			name++;
		if (!*name)
			return 1;
		name++;
	}

	if (mask[0] == '*' && mask[1] == '@') {
		mask += 2;
		while (*name != '@' && *name)
			name++;
		if (!*name)
			return 1;
		name++;
	}
	return match2(mask,name);
}
