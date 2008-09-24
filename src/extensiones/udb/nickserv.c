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
BOTFUNC(NSAcceso);
BOTFUNCHELP(NSHAcceso);
EXTFUNC(NSRegister_U);
EXTFUNC(NSOpts_U);
EXTFUNC(NSInfo_U);
EXTFUNC(NSSuspend);
EXTFUNC(NSLiberar_U);
EXTFUNC(NSSwhois_U);
EXTFUNC(NSForbid_U);
EXTFUNC(NSSendpass_U);
EXTFUNC(NSOptsNick_U);

int NSSigEOS		();
int NSSigDrop	(char *);
extern void NSCambiaInv(Cliente *);
#define NS_OPT_UDB 0x80
#define IsNickUDB(x) (IsReg(x) && atoi(SQLCogeRegistro(NS_SQL, x, "opts")) & NS_OPT_UDB)

bCom nickserv_coms[] = {
	{ "migrar" , NSMigrar , N1 , "Migra tu nick a la BDD." , NSHMigrar } ,
	{ "demigrar" , NSDemigrar , N1 , "Demigra tu nick de la BDD." , NSHDemigrar } ,
	{ "acceso" , NSAcceso , N1 , "Gestiona el acceso de tu nick." , NSHAcceso } ,
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
	InsertaSenyalExt(15, NSOptsNick_U, ext);
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
BOTFUNCHELP(NSHAcceso)
{
	Responde(cl, CLI(nickserv), "Gestiona los accesos de tu nick.");
	Responde(cl, CLI(nickserv), "Es decir, fija la ip o el rango de ips desde las cuales se permite el uso de tu nick.");
	Responde(cl, CLI(nickserv), "Se permite el uso de notación CIDR para especificar un rango de ips.");
	Responde(cl, CLI(nickserv), "Si no se especifica ninguna ip, se elimina el acceso.");
	Responde(cl, CLI(nickserv), "NOTA: Si te equivocas con el acceso, *NO* podrás utilizar este nick. Tendrás que pedir a un operador que te levante el acceso.");
	Responde(cl, CLI(nickserv), " ");
	if (IsOper(cl))
	{
		Responde(cl, CLI(nickserv), "Sintaxis: \00312ACCESO [ip|nick]");
		Responde(cl, CLI(nickserv), "Si especificas una ip, añadirá un acceso a tu nick.");
		Responde(cl, CLI(nickserv), "Si especificas un nick, eliminará el acceso a este nick.");
		Responde(cl, CLI(nickserv), "El acceso de nicks sólo puede borrarse si el usuario ha cambiado de ip y no puede hacerlo él mismo.");
	}
	else
		Responde(cl, CLI(nickserv), "Sintaxis: \00312ACCESO [ip]");
	return 0;
}
BOTFUNC(NSMigrar)
{
	int opts;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, cl->nombre, "pass"), MDString(param[1], 0)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(NS_SQL, cl->nombre, "opts"));
	if (opts & NS_OPT_UDB)
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Ya tienes el nick migrado.");
		return 1;
	}
	PropagaRegistro("N::%s::P %s", cl->nombre, MDString(param[1], 0));
	Responde(cl, CLI(nickserv), "Migración realizada.");
	opts |= NS_OPT_UDB;
	SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts);
	NSCambiaInv(cl);
	return 0;
}
BOTFUNC(NSDemigrar)
{
	int opts;
	if (params < 2)
	{
		Responde(cl, CLI(nickserv), NS_ERR_PARA, fc->com, "tupass");
		return 1;
	}
	if (!IsReg(cl->nombre))
	{
		Responde(cl, CLI(nickserv), NS_ERR_NURG);
		return 1;
	}
	if (strcmp(SQLCogeRegistro(NS_SQL, cl->nombre, "pass"), MDString(param[1], 0)))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(NS_SQL, cl->nombre, "opts"));
	if (!(opts & NS_OPT_UDB))
	{
		Responde(cl, CLI(nickserv), NS_ERR_EMPT, "Tu nick no está migrado.");
		return 1;
	}
	PropagaRegistro("N::%s", cl->nombre);
	Responde(cl, CLI(nickserv), "Demigración realizada.");
	opts &= ~NS_OPT_UDB;
	SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts);
	return 0;
}
BOTFUNC(NSAcceso)
{
	if (params < 2)
	{
		PropagaRegistro("N::%s::A", cl->nombre);
		Responde(cl, CLI(nickserv), "Acceso eliminado.");
	}
	else
	{
		char *c;
		strlcpy(tokbuf, param[1], sizeof(tokbuf));
		if ((c = strchr(tokbuf, '/')))
			*c = '\0';
		if (!EsIp(tokbuf) && !IsOper(cl))
		{
			Responde(cl, CLI(nickserv), NS_ERR_EMPT, "No parece ser una ip válida.");
			return 1;
		}
		if (EsIp(tokbuf))
		{
			PropagaRegistro("N::%s::A %s", cl->nombre, param[1]);
			Responde(cl, CLI(nickserv), "Acceso añadido.");
		}
		else
		{
			PropagaRegistro("N::%s::A", param[1]);
			Responde(cl, CLI(nickserv), "El acceso de \00312%s\003 ha sido eliminado.", param[1]);
		}
	}
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
		SQLInserta(NS_SQL, cl->nombre, "opts", "%i", opts | NS_OPT_UDB);
		if ((pass = SQLCogeRegistro(NS_SQL, cl->nombre, "pass")))
		{
			NSCambiaInv(cl);
			PropagaRegistro("N::%s::P %s", cl->nombre, pass);
		}
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
	if (mod != nickserv->hmod)
		return 1;
	if (IsNickUDB(param[1]))
		PropagaRegistro("N::%s::S %s", param[1], SQLCogeRegistro(NS_SQL, param[1], "suspend"));
	return 0;
}
EXTFUNC(NSLiberar_U)
{
	if (mod != nickserv->hmod)
		return 1;
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
	if (IsNickUDB(param[1]) && (pass = SQLCogeRegistro(NS_SQL, param[1], "pass")))
		PropagaRegistro("N::%s::P %s", param[1], pass);
	return 0;
}
EXTFUNC(NSOptsNick_U)
{
	if (mod != nickserv->hmod)
		return 1;
	if (!strcasecmp(param[2], "PASS"))
	{
		char *pass;
		if (IsNickUDB(param[1]) && (pass = SQLCogeRegistro(NS_SQL, param[1], "pass")))
			PropagaRegistro("N::%s::P %s", param[1], pass);
	}
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
		for (reg = N->arbol->down; reg; reg = sig)
		{
			sig = reg->mid;
			if (!IsReg(reg->item) || !IsNickUDB(reg->item))
				PropagaRegistro("N::%s", reg->item);
		}
	}
	return 0;
}
