/*
 * $Id: flags.c,v 1.2 2004-09-11 16:08:03 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"

mTab uModes[] = {
	{ UMODE_INVISIBLE , 'i' },
	{ UMODE_OPER , 'o' },
	{ UMODE_WALLOP , 'w' },
	{ UMODE_FAILOP , 'g' },
	{ UMODE_HELPOP , 'h' },
	{ UMODE_REGNICK , 'r' },
	{ UMODE_SADMIN , 'a' },
	{ UMODE_ADMIN , 'A' },
	{ UMODE_RGSTRONLY , 'R' },
	{ UMODE_WEBTV , 'V' },
#ifdef UDB
	{ UMODE_SERVICES , 'k' },
#else
	{ UMODE_SERVICES , 'S' },
#endif
	{ UMODE_HIDE , 'x' },
	{ UMODE_NETADMIN , 'N' },
	{ UMODE_COADMIN , 'C' },
	{ UMODE_WHOIS , 'W' },
	{ UMODE_KIX , 'q' },
	{ UMODE_BOT , 'B' },
	{ UMODE_SECURE , 'z' },
	{ UMODE_VICTIM , 'v' },
	{ UMODE_DEAF , 'd' },
	{ UMODE_HIDEOPER , 'H' },
	{ UMODE_SETHOST , 't' },
	{ UMODE_STRIPBADWORDS , 'G' },
	{ UMODE_HIDEWHOIS , 'p' },
	{ UMODE_NOCTCP , 'T' },
#ifdef UDB
	{ UMODE_SUSPEND , 'S' },
	{ UMODE_PRIVDEAF, 'D' },
#endif
	{ UMODE_LOCOP , 'O' },
	{0x0, 0x0}
};

mTab chModes[] = {
	{ MODE_CHANOP , 'o' },
	{ MODE_VOICE , 'v' },
	{ MODE_PRIVATE , 'p' },
	{ MODE_SECRET , 's' },
	{ MODE_MODERATED , 'm' },
	{ MODE_TOPICLIMIT , 't' },
	{ MODE_CHANOWNER , 'q' },
	{ MODE_CHANPROT , 'a' },
	{ MODE_HALFOP , 'h' },
	{ MODE_EXCEPT , 'e' },
	{ MODE_BAN , 'b' },
	{ MODE_INVITEONLY , 'i' },
	{ MODE_NOPRIVMSGS , 'n' },
	{ MODE_KEY , 'k' },
	{ MODE_LIMIT , 'l' },
	{ MODE_RGSTR , 'r' },
	{ MODE_RGSTRONLY , 'R' },
	{ MODE_LINK , 'L' },
	{ MODE_NOCOLOR , 'c' },
	{ MODE_OPERONLY , 'O' },
	{ MODE_ADMONLY , 'A' },
	{ MODE_NOKICKS , 'Q' },
	{ MODE_STRIP , 'S' },
	{ MODE_NOKNOCK , 'K' },
	{ MODE_NOINVITE , 'V' },
	{ MODE_FLOODLIMIT , 'f' },
	{ MODE_MODREG , 'M' },
	{ MODE_STRIPBADWORDS , 'G' },
	{ MODE_NOCTCP , 'C' },
	{ MODE_AUDITORIUM , 'u' },
	{ MODE_ONLYSECURE , 'z' },
	{ MODE_NONICKCHANGE , 'N' },
	{0x0, 0x0}
};

u_long flag2mode(char flag, mTab tabla[])
{
	mTab *aux = &tabla[0];
	while (aux->flag != 0x0)
	{
		if (aux->flag == flag)
			return aux->mode;
		aux++;
	}
	return 0L;
}
char mode2flag(u_long mode, mTab tabla[])
{
	mTab *aux = &tabla[0];
	while (aux->flag != 0x0)
	{
		if (aux->mode == mode)
			return aux->flag;
		aux++;
	}
	return (char)NULL;
}
u_long flags2modes(u_long *modos, char *flags, mTab tabla[])
{
	char f = ADD;
	if (!*modos)
		*modos = 0L;
	while (*flags)
	{
		switch(*flags)
		{
			case '+':
				f = ADD;
				break;
			case '-':
				f = DEL;
				break;
			default:
				if (f == ADD)
					*modos |= flag2mode(*flags, tabla);
				else
					*modos &= ~flag2mode(*flags, tabla);
		}
		flags++;
	}
	return *modos;
}
char *modes2flags(Canal *cn, u_long modes, mTab tabla[])
{
	char f = ADD;
	mTab *aux = &tabla[0];
	char *flags;
	int i = 0;
	flags = buf;
	while (aux->flag != 0x0)
	{
		if (modes & aux->mode)
			*flags++ = aux->flag;
		aux++;
	}
	if (cn)
	{
		if (cn->clave)
		{
			strcat(flags, " ");
			strcat(flags, cn->clave);
		}
		if (cn->link)
		{
			strcat(flags, " ");
			strcat(flags, cn->link);
		}
		if (cn->flood)
		{
			strcat(flags, " ");
			strcat(flags, cn->flood);
		}
	}
	else
		*flags = '\0';
	//printf("%s\n",fl);
	return buf;
}
