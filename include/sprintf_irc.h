/*
 * $Id: sprintf_irc.h,v 1.3 2005-06-29 21:13:45 Trocotronic Exp $ 
 */

#if !defined(SPRINTF_IRC)
#define SPRINTF_IRC

#include <stdarg.h>
#define __attribute__(x)
/*=============================================================================
 * Proto types
 */

extern char *ircvsprintf(register char *str, register const char *format,
    register va_list);
extern char *ircsprintf(register char *str, register const char *format, ...)
    __attribute__ ((format(printf, 2, 3)));

extern const char atoi_tab[4000];

#endif /* SPRINTF_IRC */
