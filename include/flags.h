/*
 * $Id: flags.h,v 1.2 2004-09-11 15:54:07 Trocotronic Exp $ 
 */

#define ADD 1
#define DEL 2

typedef struct _modes mTab;
struct _modes
{
	u_long mode;
	char flag;
};
#define UMODE_INVISIBLE 	0x1
#define UMODE_OPER 		0x2
#define UMODE_WALLOP  	0x4
#define UMODE_FAILOP  	0x8
#define UMODE_HELPOP  	0x10
#define UMODE_REGNICK  	0x20
#define UMODE_SADMIN  	0x40
#define UMODE_ADMIN  	0x80
#define UMODE_RGSTRONLY  	0x100
#define UMODE_WEBTV  	0x200
#define UMODE_SERVICES  	0x400
#define UMODE_HIDE 		0x800
#define UMODE_NETADMIN 	0x1000
#define UMODE_COADMIN 	0x2000
#define UMODE_WHOIS 		0x4000
#define UMODE_KIX 		0x8000
#define UMODE_BOT 		0x10000
#define UMODE_SECURE 	0x20000
#define UMODE_VICTIM 	0x40000
#define UMODE_DEAF 		0x80000
#define UMODE_HIDEOPER 	0x100000
#define UMODE_SETHOST	0x200000
#define UMODE_STRIPBADWORDS 0x400000
#define UMODE_HIDEWHOIS 	0x800000
#define UMODE_NOCTCP 	0x1000000
#ifdef UDB
#define UMODE_SUSPEND 	0x20000000
#define UMODE_PRIVDEAF	0x80000000
#endif
#define UMODE_LOCOP		0x40000000

#define MODE_CHANOP		0x1
#define MODE_VOICE		0x2
#define MODE_PRIVATE		0x4
#define MODE_SECRET		0x8
#define MODE_MODERATED  	0x10
#define MODE_TOPICLIMIT 	0x20
#define MODE_CHANOWNER	0x40
#define MODE_CHANPROT	0x80
#define MODE_HALFOP		0x100
#define MODE_EXCEPT		0x200
#define MODE_BAN		0x400
#define MODE_INVITEONLY 	0x800
#define MODE_NOPRIVMSGS 	0x1000
#define MODE_KEY		0x2000
#define MODE_LIMIT		0x4000
#define MODE_RGSTR		0x8000
#define MODE_RGSTRONLY 	0x10000
#define MODE_LINK		0x20000
#define MODE_NOCOLOR		0x40000
#define MODE_OPERONLY   	0x80000
#define MODE_ADMONLY   	0x100000
#define MODE_NOKICKS   	0x200000
#define MODE_STRIP	   	0x400000
#define MODE_NOKNOCK		0x800000
#define MODE_NOINVITE  	0x1000000
#define MODE_FLOODLIMIT	0x2000000
#define MODE_MODREG		0x4000000
#define MODE_STRIPBADWORDS	0x8000000
#define MODE_NOCTCP		0x10000000
#define MODE_AUDITORIUM	0x20000000
#define MODE_ONLYSECURE	0x40000000
#define MODE_NONICKCHANGE	0x80000000

extern u_long flag2mode(char , mTab []);
extern char mode2flag(u_long , mTab []);
extern u_long flags2modes(u_long *, char *, mTab []);
extern char *modes2flags(struct _canal *, u_long , mTab []);
extern mTab uModes[];
extern MODVAR mTab chModes[];
