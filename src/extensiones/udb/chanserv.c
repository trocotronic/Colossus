#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/chanserv.h"
#include "bdd.h"

#ifdef _WIN32
ChanServ *chanserv = NULL;
#else
extern ChanServ *chanserv;
#endif
#define CS_OPT_UDB 0x80
#define CS_AUTOMIGRAR 0x1000
int CSSigDrop(char *);
int CSSigEOS_U();

EXTFUNC(CSOpts_U);
EXTFUNC(CSSuspender_U);
EXTFUNC(CSLiberar_U);
EXTFUNC(CSForbid_U); 
EXTFUNC(CSUnforbid_U); 
EXTFUNC(CSRegister_U);
BOTFUNC(CSMigrar);
BOTFUNCHELP(CSHMigrar);
BOTFUNC(CSDemigrar);
BOTFUNCHELP(CSHDemigrar);
BOTFUNC(CSProteger);
BOTFUNCHELP(CSHProteger);

bCom chanserv_coms[] = {
	{ "migrar" , CSMigrar ,	N1 , "Migra un canal a la base de datos de la red." , CSHMigrar } ,
	{ "demigrar" , CSDemigrar , N1 , "Demigra un canal de la base de datos de la red." , CSHDemigrar } ,
	{ "proteger" , CSProteger , N1 , "Protege un canal para que sólo ciertos nicks puedan entrar." , CSHProteger } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void CargaChanServ(Extension *ext)
{
	int i;
	if (ext->config)
	{
		Conf *cf;
		if ((cf = BuscaEntrada(ext->config, "ChanServ")))
		{
			for (i = 0; i < cf->secciones; i++)
			{
				if (!strcmp(cf->seccion[i]->item, "automigrar"))
					chanserv->opts |= CS_AUTOMIGRAR;
				else if (!strcmp(cf->seccion[i]->item, "funciones"))
					ProcesaComsMod(cf->seccion[i], chanserv->hmod, chanserv_coms);
				else if (!strcmp(cf->seccion[i]->item, "funcion"))
					SetComMod(cf->seccion[i], chanserv->hmod, chanserv_coms);
			}
		}
		else
			ProcesaComsMod(NULL, chanserv->hmod, chanserv_coms);
	}
	else
		ProcesaComsMod(NULL, chanserv->hmod, chanserv_coms);
	InsertaSenyal(SIGN_EOS, CSSigEOS_U);
	InsertaSenyal(CS_SIGN_DROP, CSSigDrop);
	InsertaSenyalExt(9, CSOpts_U, ext);
	InsertaSenyalExt(15, CSSuspender_U, ext);
	InsertaSenyalExt(16, CSLiberar_U, ext);
	InsertaSenyalExt(17, CSForbid_U, ext);
	InsertaSenyalExt(18, CSUnforbid_U, ext);
	InsertaSenyalExt(20, CSRegister_U, ext);
}
void DescargaChanServ(Extension *ext)
{
	BorraSenyal(SIGN_EOS, CSSigEOS_U);
	BorraSenyal(CS_SIGN_DROP, CSSigDrop);
	BorraSenyalExt(9, CSOpts_U, ext);
	BorraSenyalExt(15, CSSuspender_U, ext);
	BorraSenyalExt(16, CSLiberar_U, ext);
	BorraSenyalExt(17, CSForbid_U, ext);
	BorraSenyalExt(18, CSUnforbid_U, ext);
	BorraSenyalExt(20, CSRegister_U, ext);
}
int IsChanUDB(char *canal)
{
	char *opts;
	if ((opts = SQLCogeRegistro(CS_SQL, canal, "opts")))
		return (atoi(opts) & CS_OPT_UDB);
	return 0;
}
int CSSigDrop(char *canal)
{
	if (IsChanUDB(canal))
		PropagaRegistro("C::%s", canal);
	return 0;
}
int CSSigEOS_U()
{
	if (chanserv)
	{
		Udb *bloq, *reg, *sig;
		PropagaRegistro("S::C %s", chanserv->hmod->mascara);
		for (reg = chans->down; reg; reg = sig)
		{
			sig = reg->mid;
			if ((bloq = BuscaBloque(C_FOR_TOK, reg)) && !SQLCogeRegistro(CS_FORBIDS, reg->item, "motivo"))
				SQLInserta(CS_FORBIDS, reg->item, "motivo", bloq->data_char);
			if (!IsChanReg(reg->item) || !IsChanUDB(reg->item))
				PropagaRegistro("C::%s", reg->item);
			else
			{
				if ((bloq = BuscaBloque(C_SUS_TOK, reg)) && !SQLCogeRegistro(CS_SQL, reg->item, "suspend"))
					SQLInserta(CS_SQL, reg->item, "suspend", bloq->data_char);
			}
		}
	}
	return 0;
}
void CSPropagaCanal(char *canal)
{
	SQLRes res;
	SQLRow row;
	char *modos, *c;
	res = SQLQuery("SELECT founder,modos,topic,pass from %s%s where LOWER(item)='%s'", PREFIJO, CS_SQL, strtolower(canal));
	if (!res)
		return;
	row = SQLFetchRow(res);
	modos = row[1];
	if ((c = strchr(modos, '+')))
		modos = c+1;
	if ((c = strchr(modos, '-')))
		*c = 0;
	PropagaRegistro("C::%s::F %s", canal, row[0]);
	PropagaRegistro("C::%s::M %s", canal, modos);
	PropagaRegistro("C::%s::T %s", canal, row[2]);
	PropagaRegistro("C::%s::P %s", canal, row[3]);
	SQLFreeRes(res);
}
EXTFUNC(CSOpts_U)
{
	if (mod != chanserv->hmod || !IsChanUDB(param[1]))
		return 1;
	if (!strcasecmp(param[2], "TOPIC"))
	{
		if (params > 3)
			PropagaRegistro("C::%s::T %s", param[1], Unifica(param, params, 3, -1));
		else
			PropagaRegistro("C::%s::T", param[1]);
	}
	else if (!strcasecmp(param[2], "MODOS"))
	{
		
		if (params > 3)
		{
			char *c, *str;
			strcpy(tokbuf, Unifica(param, params, 3, -1));
			str = strtok(tokbuf, " ");
			if ((c = strchr(str, '-')))
				*c = '\0';
			if (*str == '+')
				str++;
			strcpy(buf, str);
			if ((str = strtok(NULL, " ")))
			{
				strcat(buf, " ");
				strcat(buf, str);
			}
			PropagaRegistro("C::%s::M %s", param[1], buf);
		}
		else
			PropagaRegistro("C::%s::M", param[1]);
	}
	else if (!strcasecmp(param[2], "PASS"))
		PropagaRegistro("C::%s::P %s", param[1], MDString(param[3]));
	else if (!strcasecmp(param[2], "FUNDADOR"))
		PropagaRegistro("C::%s::F %s", param[1], param[3]);
	return 0;
}
EXTFUNC(CSSuspender_U)
{
	if (mod == chanserv->hmod && IsChanUDB(param[1]))
		PropagaRegistro("C::%s::S %s", param[1], Unifica(param, params, 2, -1));
	return 0;
}
EXTFUNC(CSLiberar_U)
{
	if (mod == chanserv->hmod && IsChanUDB(param[1]))
		PropagaRegistro("C::%s::S", param[1]);
	return 0;
}
EXTFUNC(CSForbid_U)
{
	if (mod == chanserv->hmod)
		PropagaRegistro("C::%s::B %s", param[1], Unifica(param, params, 2, -1));
	return 0;
}
EXTFUNC(CSUnforbid_U)
{
	if (mod == chanserv->hmod)
		PropagaRegistro("C::%s::B", param[1]);
	return 0;
}
EXTFUNC(CSRegister_U)
{
	if (mod == chanserv->hmod && (chanserv->opts & CS_AUTOMIGRAR))
	{
		int opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
		SQLInserta(CS_SQL, param[1], "opts", "%i", opts | CS_OPT_UDB);
		CSPropagaCanal(param[1]);
	}
	return 0;
}
BOTFUNCHELP(CSHProteger)
{
	Responde(cl, CLI(chanserv), "Protege un canal por BDD.");
	Responde(cl, CLI(chanserv), "Esta protección consiste en permitir la entrada a determinados nicks.");
	Responde(cl, CLI(chanserv), "Al ser por BDD, aunque los servicios estén caídos, sólo podrán entrar los nicks que coincidan con las máscaras especificadas.");
	Responde(cl, CLI(chanserv), "Así pues, cualquier persona que no figure en esta lista de protegidos, no podrá acceder al canal.");
	Responde(cl, CLI(chanserv), "Si la máscara especificada es un nick, se añadirá la máscara de forma nick!*@*");
	Responde(cl, CLI(chanserv), "Para poder realizar este comando necesitas tener el acceso \00312+e\003.");
	Responde(cl, CLI(chanserv), "Si no especeificas ninguna máscara, toda la lista es borrada.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312PROTEGER #canal [+|-[máscara]]");
	return 0;
}
BOTFUNC(CSProteger)
{
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "PROTEGER #canal {+|-}nick");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSTieneNivel(parv[0], param[1], CS_LEV_EDT))
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (!IsChanUDB(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está migrado.");
		return 1;
	}
	if (BadPtr(param[2] + 1))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER #canal {+|-}nick");
		return 1;
	}
	if (*param[2] == '+')
	{
		PropagaRegistro("C::%s::A::%s -", param[1], param[2] + 1);
		Responde(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido añadida.", param[2] + 1);
	}
	else if (*param[2] == '-')
	{
		PropagaRegistro("C::%s::A::%s", param[1], param[2] + 1);
		Responde(cl, CLI(chanserv), "La entrada \00312%s\003 ha sido eliminada.", param[2] + 1);
	}
	else
	{
		Responde(cl, CLI(chanserv), CS_ERR_SNTX, "PROTEGER {+|-}nick");
		return 1;
	}
	return 0;
}
BOTFUNCHELP(CSHDemigrar)
{
	Responde(cl, CLI(chanserv), "Elimina este canal de la base de datos de la red.");
	Responde(cl, CLI(chanserv), "El canal volverá a su administración anterior a través de los servicios de red.");
	Responde(cl, CLI(chanserv), "NOTA: este comando sólo puede realizarlo el fundador del canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312DEMIGRAR #canal pass");
	return 0;
}
BOTFUNC(CSDemigrar)
{
	int opts;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "DEMIGRAR #canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSEsFundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (!(opts & CS_OPT_UDB))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal no está migrado.");
		return 1;
	}
	PropagaRegistro("C::%s", param[1]);
	Responde(cl, CLI(chanserv), "Demigración realizada.");
	opts &= ~CS_OPT_UDB;
	SQLInserta(CS_SQL, param[1], "opts", "%i", opts);
	return 0;
}
BOTFUNCHELP(CSHMigrar)
{
	Responde(cl, CLI(chanserv), "Migra un canal a la base de datos de la red.");
	Responde(cl, CLI(chanserv), "Una vez migrado, el canal será permanente. Es decir, no se borrará aunque salga el último usuario.");
	Responde(cl, CLI(chanserv), "Conservará los modos y el topic cuando entre el primer usuario. A su vez seguirá mostrándose en el comando /LIST.");
	Responde(cl, CLI(chanserv), "Además, el founder cuando entré y tenga el modo +r siempre recibirá los modos +oq, acreditándole como tal.");
	Responde(cl, CLI(chanserv), "Finalmente, cualquier usuario que entre en el canal y esté vacío, si no es el founder se le quitará el estado de @ (-o).");
	Responde(cl, CLI(chanserv), "Este es independiente de los servicios de red, así pues asegura su funcionamiento siempre aunque no estén conectados.");
	Responde(cl, CLI(chanserv), "NOTA: este comando sólo puede realizarlo el fundador del canal.");
	Responde(cl, CLI(chanserv), " ");
	Responde(cl, CLI(chanserv), "Sintaxis: \00312MIGRAR #canal pass");
	return 0;
}
BOTFUNC(CSMigrar)
{
	int opts;
	if (params < 3)
	{
		Responde(cl, CLI(chanserv), CS_ERR_PARA, "MIGRAR #canal pass");
		return 1;
	}
	if (!IsChanReg(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_NCHR, "");
		return 1;
	}
	if (!IsOper(cl) && IsChanSuspend(param[1]))
	{
		Responde(cl, CLI(chanserv), CS_ERR_SUSP);
		return 1;
	}
	if (!CSEsFundador(cl, param[1])) /* Los operadores de red NO pueden migrar canales que no sean suyos */
	{
		Responde(cl, CLI(chanserv), CS_ERR_FORB, "");
		return 1;
	}
	if (strcmp(SQLCogeRegistro(CS_SQL, param[1], "pass"), MDString(param[2])))
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Contraseña incorrecta.");
		return 1;
	}
	opts = atoi(SQLCogeRegistro(CS_SQL, param[1], "opts"));
	if (opts & CS_OPT_UDB)
	{
		Responde(cl, CLI(chanserv), CS_ERR_EMPT, "Este canal ya está migrado.");
		return 1;
	}
	CSPropagaCanal(param[1]);
	Responde(cl, CLI(chanserv), "Migración realizada.");
	opts |= CS_OPT_UDB;
	SQLInserta(CS_SQL, param[1], "opts", "%i", opts);
	return 0;
}
