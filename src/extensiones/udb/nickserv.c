#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "bdd.h"
#include "modulos/nickserv.h"
#include "modulos/chanserv.h"

#ifdef _WIN32
NickServ *nickserv = NULL;
#else
extern NickServ *nickserv;
#endif

BOTFUNC(NSMigrar);
BOTFUNCHELP(NSHMigrar);
BOTFUNC(NSDemigrar);
BOTFUNCHELP(NSHDemigrar);
EXTFUNC(NSRegister_U);
EXTFUNC(NSOpts_U);
EXTFUNC(NSInfo_U);
EXTFUNC(NSSuspend);
EXTFUNC(NSLiberar_U);
EXTFUNC(NSSwhois_U);
EXTFUNC(NSForbid_U);
EXTFUNC(NSSendpass_U);

int NSSigEOS		();
int NSSigDrop	(char *);
extern void NSCambiaInv(Cliente *);
#define NS_OPT_UDB 0x80
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)

bCom nickserv_coms[] = {
	{ "migrar" , NSMigrar , N1 , "Migra tu nick a la BDD." , NSHMigrar } ,
	{ "demigrar" , NSDemigrar , N1 , "Demigra tu nick de la BDD." , NSHDemigrar } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void CargaNickServ(Extension *ext)
{
	int i;
	if (ext->config)
	{
		Conf *cf;
		if ((cf = BuscaEntrada(ext->config, "NickServ")))
		{
			for (i = 0; i < cf->secciones; i++)
			{
				if (!strcmp(cf->seccion[i]->item, "automigrar"))
					nickserv->opts |= NS_AUTOMIGRAR;
				else if (!strcmp(cf->seccion[i]->item, "funciones"))
					ProcesaComsMod(cf->seccion[i], nickserv->hmod, nickserv_coms);
				else if (!strcmp(cf->seccion[i]->item, "funcion"))
					SetComMod(cf->seccion[i], nickserv->hmod, nickserv_coms);
			}
		}
		else
			ProcesaComsMod(NULL, nickserv->hmod, nickserv_coms);
	}
	else
		ProcesaComsMod(NULL, nickserv->hmod, nickserv_coms);
	InsertaSenyal(SIGN_EOS, NSSigEOS);
	InsertaSenyal(NS_SIGN_DROP, NSSigDrop);
	InsertaSenyalExt(1, NSRegister_U, ext);
	InsertaSenyalExt(3, NSOpts_U, ext);
	InsertaSenyalExt(5, NSSendpass_U, ext);
	InsertaSenyalExt(6, NSInfo_U, ext);
	InsertaSenyalExt(9, NSSuspend, ext);
	InsertaSenyalExt(10, NSLiberar_U, ext);
	InsertaSenyalExt(11, NSSwhois_U, ext);
	InsertaSenyalExt(13, NSForbid_U, ext);
}
void DescargaNickServ(Extension *ext)
{
	BorraSenyal(SIGN_EOS, NSSigEOS);
	BorraSenyal(NS_SIGN_DROP, NSSigDrop);
	BorraSenyalExt(1, NSRegister_U, ext);
	BorraSenyalExt(3, NSOpts_U, ext);
	BorraSenyalExt(5, NSSendpass_U, ext);
	BorraSenyalExt(6, NSInfo_U, ext);
	BorraSenyalExt(9, NSSuspend, ext);
	BorraSenyalExt(10, NSLiberar_U, ext);
	BorraSenyalExt(11, NSSwhois_U, ext);
	BorraSenyalExt(13, NSForbid_U, ext);
}
BOTFUNCHELP(NSHMigrar)
{
	Responde(cl, CLI(nickserv), "Migra tu nick a la base de datos.");
	Responde(cl, CLI(nickserv), "Una vez migrado tu nick, la única forma de ponérselo sólo podrá ser vía /nick tunick:tupass.");
	Responde(cl, CLI(nickserv), "Si la contraseña es correcta, recibirás el modo de usuario +r quedando identificado automáticamente.");
	Responde(cl, CLI(nickserv), "De lo contrario, no podrás utilizar este nick.");
	Responde(cl, CLI(nickserv), "Adicionalmente, puedes utilizar /nick tunick!tupass para desconectar una sesión que pudiere utilizar tu nick.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312MIGRAR tupass");
	return 0;
}
BOTFUNCHELP(NSHDemigrar)
{
	Responde(cl, CLI(nickserv), "Borra tu nick de la BDD.");
	Responde(cl, CLI(nickserv), "Una vez borrado, la identificación procederá con el comando IDENTIFY.");
	Responde(cl, CLI(nickserv), " ");
	Responde(cl, CLI(nickserv), "Sintaxis: \00312DEMIGRAR tupass");
	return 0;
}
BOTFUNC(NSMigrar)
{
	int opts;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "MIGRAR tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, parv[0], "pass"), MDString(param[1])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(NS_SQL, parv[0], "opts"));
	if (opts & NS_OPT_UDB)
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Ya tienes el nick migrado.");
		return 1;
	}
	PropagaRegistro("N::%s::P %s", parv[0], MDString(param[1]));
	Responde(cl, CLI(nickserv), "Migración realizada.");
	opts |= NS_OPT_UDB;
	SQLInserta(NS_SQL, parv[0], "opts", "%i", opts);
	NSCambiaInv(cl);
	return 0;
}
BOTFUNC(NSDemigrar)
{
	int opts;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, "DEMIGRAR tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, parv[0], "pass"), MDString(param[1])))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(NS_SQL, parv[0], "opts"));
	if (!(opts & NS_OPT_UDB))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Tu nick no está migrado.");
		return 1;
	}
	PropagaRegistro("N::%s", parv[0]);
	Responde(cl, CLI(nickserv), "Demigración realizada.");
	opts &= ~NS_OPT_UDB;
	SQLInserta(NS_SQL, parv[0], "opts", "%i", opts);
	return 0;
}
EXTFUNC(NSRegister_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (nickserv->opts & NS_AUTOMIGRAR)
	{
		int opts = atoi(SQLCogeRegistro(NS_SQL, cl->nombre, "opts"));
		char *pass;
		SQLInserta(CS_SQL, cl->nombre, "opts", "%i", opts | NS_OPT_UDB);
		if ((pass = SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
			PropagaRegistro("N::%s::P %s", cl->nombre, pass);
		NSCambiaInv(cl);
	}
	return 0;
}
EXTFUNC(NSOpts_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (!strcasecmp(param[1], "PASS"))
	{
		char *pass;
		if (IsNickUDB(cl->nombre) && (pass = SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
			PropagaRegistro("N::%s::P %s", cl->nombre, pass);
	}
	return 0;
}
EXTFUNC(NSInfo_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (IsNickUDB(param[1]))
		Responde(cl, CLI(nickserv), "Este nick está migrado a la \2BDD");
	return 0;
}
EXTFUNC(NSSuspend)
{
	Cliente *al;
	if (mod != nickserv->hmod)
		return 1;
	if ((al = BuscaCliente(param[1], NULL)))
	{
		ProtFunc(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "+S-r");
		Responde(al, CLI(nickserv), "Tu nick ha sido suspendido por \00312%s\003: %s", cl->nombre, Unifica(param, params, 2, -1));
	}
	if (IsNickUDB(param[1]))
		PropagaRegistro("N::%s::S %s", param[1], SQLCogeRegistro(NS_SQL, param[1], "suspend"));
	return 0;
}
EXTFUNC(NSLiberar_U)
{
	Cliente *al;
	if (mod != nickserv->hmod)
		return 1;
	if ((al = BuscaCliente(param[1], NULL)))
		ProtFunc(P_MODO_USUARIO_REMOTO)(al, CLI(nickserv), "-S");
	if (IsNickUDB(param[1]))
		PropagaRegistro("N::%s::S", param[1]);
	return 0;
}
EXTFUNC(NSSwhois_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (!IsNickUDB(param[1]))
		return 1;
	if (params >= 3)
		PropagaRegistro("N::%s::W %s", param[1], Unifica(param, params, 2, -1));
	else
		PropagaRegistro("N::%s::W", param[1]);
	return 0;
}
EXTFUNC(NSForbid_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (params >= 3)
		PropagaRegistro("N::%s::B %s", param[1], Unifica(param, params, 1, -1));
	else
		PropagaRegistro("N::%s::B", param[1]);
	return 0;
}
EXTFUNC(NSSendpass_U)
{
	char *pass;
	if (mod != nickserv->hmod)
		return 1;
	if (IsNickUDB(cl->nombre) && (pass = SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
		PropagaRegistro("N::%s::P %s", cl->nombre, pass);
	return 0;
}
int NSSigDrop(char *nick)
{
	if (IsNickUDB(nick))
		PropagaRegistro("N::%s", nick);
	return 0;
}
int NSSigEOS()
{
	if (nickserv)
	{
		Udb *reg, *sig;
		PropagaRegistro("S::N %s", nickserv->hmod->mascara);
		for (reg = nicks->down; reg; reg = sig)
		{
			sig = reg->mid;
			if (!IsReg(reg->item) || !IsNickUDB(reg->item))
				PropagaRegistro("N::%s", reg->item);
		}
	}
	return 0;
}
