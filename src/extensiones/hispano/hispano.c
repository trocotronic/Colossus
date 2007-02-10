#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"

IRCFUNC(*nick);
IRCFUNC(m_nick);
extern int b642int(char *);
extern void InsertaClienteEnNumerico(Cliente *, char *, Hash *);

ModInfo MOD_INFO(Hispano) = {
	"Hispano" ,
	0.1,
	"Trocotronic" ,
	"trocotronic@telefonica.net"
};
int MOD_CARGA(Hispano)(Extension *ext, Protocolo *prot)
{
	Comando *com;
	if ((com = BuscaComando("NICK")))
	{
		nick = com->funcion[0];
		BorraComando("NICK", nick);
	}
	InsertaComando("NICK", "N", m_nick, INI, MAXPARA);
	return 0;
}
int MOD_DESCARGA(Hispano)(Extension *ext, Protocolo *prot)
{
	BorraComando("NICK", m_nick);
	InsertaComando("NICK", "N", nick, INI, MAXPARA);
	return 0;
}
IRCFUNC(m_nick)
{
	if (parc > 3)
	{
		Cliente *al = NULL;
		if (parc > 8)
		{
			struct in_addr tmp;
			tmp.s_addr = htonl(base64toint(parv[7]));
			al = NuevoCliente(parv[1], parv[4], parv[5], inet_ntoa(tmp), cl->nombre, parv[5], parv[6], parv[9]);
			al->numeric = b642int(parv[8]);
			al->trio = strdup(parv[8]);
			InsertaClienteEnNumerico(al, parv[8], uTab);
		}
		if (BuscaModulo(parv[1], modulos))
		{
			ProtFunc(P_QUIT_USUARIO_REMOTO)(al, &me, "Nick protegido.");
			ReconectaBot(parv[1]);
		}
		LlamaSenyal(SIGN_POST_NICK, 2, al, 0);
		LlamaSenyal(SIGN_UMODE, 2, al, parv[6]);
	}
	else
	{
		LlamaSenyal(SIGN_PRE_NICK, 2, cl, parv[1]);
		if (strcasecmp(parv[1], cl->nombre))
			ProcesaModosCliente(cl, "-r");
		CambiaNick(cl, parv[1]);
		LlamaSenyal(SIGN_POST_NICK, 2, cl, 1);
	}
	return 0;
}
