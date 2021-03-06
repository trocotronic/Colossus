/*
 * $Id: ipserv.c,v 1.34 2008/04/23 21:13:11 Trocotronic Exp $
 */

#ifndef _WIN32
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "modulos/ipserv.h"
#include "modulos/nickserv.h"

IpServ *ipserv = NULL;
char *forbips[256];
#define ExFunc(x) TieneNivel(cl, x, ipserv->hmod, NULL)

BOTFUNC(ISHelp);
BOTFUNC(ISSetipv);
BOTFUNCHELP(ISHSetipv);
BOTFUNCHELP(ISHSetipv2);
BOTFUNC(ISTemphost);
BOTFUNCHELP(ISHTemphost);
BOTFUNC(ISClones);
BOTFUNCHELP(ISHClones);
BOTFUNC(ISVHost);
BOTFUNCHELP(ISHVHost);

DLLFUNC int ISSigIdOk 		(Cliente *);
char *ISMakeVirtualhost 	(Cliente *, int);
DLLFUNC int ISSigUmode		(Cliente *, char *);
DLLFUNC int ISSigNick		(Cliente *, int);
int ISSigDrop	(char *);
int ISSigEOS	();
int ISSigSQL	();
int ISSigSockClose	();
int ISProcIps	(Proc *);
char *ISIpValida	(char *);

static bCom ipserv_coms[] = {
	{ "help" , ISHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "vhost" , ISVHost , N1 , "Te cambia tu ip virtual." , ISHVHost } ,
	{ "setipv" , ISSetipv , N2 , "Agrega o elimina una ip virtual." , ISHSetipv } ,
	{ "setipv2" , ISSetipv , N2 , "Agrega o elimina una ip virtual de tipo 2." , ISHSetipv2 } ,
	{ "temphost" , ISTemphost , N2 , "Cambia el host de un usuario." , ISHTemphost } ,
	{ "clones" , ISClones , N4 , "Administra la lista de ips con m�s clones." , ISHClones } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void ISSet(Conf *, Modulo *);
int ISTest(Conf *, int *);

ModInfo MOD_INFO(IpServ) = {
	"IpServ" ,
	1.0 ,
	"Trocotronic" ,
	"trocotronic@redyc.com" ,
	"QQQQQPPPPPGGGGGHHHHHWWWWWRRRRR"
};

int MOD_CARGA(IpServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El m�dulo ha sido compilado para la versi�n %i y usas la versi�n %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuraci�n de %s", mod->archivo, MOD_INFO(IpServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(IpServ).nombre))
			{
				if (!ISTest(modulo.seccion[0], &errores))
					ISSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuraci�n de %s no ha pasado el test", mod->archivo, MOD_INFO(IpServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(IpServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		ISSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(IpServ)()
{
	int i;
	BorraSenyal(SIGN_UMODE, ISSigUmode);
	BorraSenyal(NS_SIGN_IDOK, ISSigIdOk);
	BorraSenyal(SIGN_POST_NICK, ISSigNick);
	BorraSenyal(SIGN_SQL, ISSigSQL);
	BorraSenyal(NS_SIGN_DROP, ISSigDrop);
	BorraSenyal(SIGN_EOS, ISSigEOS);
	BorraSenyal(SIGN_SOCKCLOSE, ISSigSockClose);
	DetieneProceso(ISProcIps);
	BotUnset(ipserv);
	for (i = 0; forbips[i]; i++)
		ircfree(forbips[i]);
	return 0;
}
int ISTest(Conf *config, int *errores)
{
	int i, error_parcial = 0;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funcion"))
			error_parcial += TestComMod(config->seccion[i], ipserv_coms, 1);
	}
	*errores += error_parcial;
	return error_parcial;
}
void ISSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!ipserv)
		ipserv = BMalloc(IpServ);
	ipserv->clones = 3;
	ircstrdup(ipserv->sufijo, "virtual");
	ipserv->cambio = 86400;
	ipserv->pvhost = 0;
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, ipserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, ipserv_coms);
			else if (!strcmp(config->seccion[i]->item, "clones"))
				ipserv->clones = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "sufijo"))
				ircstrdup(ipserv->sufijo, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "cambio"))
				ipserv->cambio = atoi(config->seccion[i]->data) * 3600;
			else if (!strcmp(config->seccion[i]->item, "pref_vhost"))
				ipserv->pvhost = 1;
			else if (!strcmp(config->seccion[i]->item, "maxlist"))
				ipserv->maxlist = atoi(config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "prohibir"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
					ircstrdup(forbips[p], config->seccion[i]->seccion[p]->data);
				forbips[p] = NULL;
			}
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
		}
	}
	else
		ProcesaComsMod(NULL, mod, ipserv_coms);
	InsertaSenyal(SIGN_UMODE, ISSigUmode);
	InsertaSenyal(NS_SIGN_IDOK, ISSigIdOk);
	InsertaSenyal(SIGN_POST_NICK, ISSigNick);
	InsertaSenyal(SIGN_SQL, ISSigSQL);
	InsertaSenyal(NS_SIGN_DROP, ISSigDrop);
	InsertaSenyal(SIGN_EOS, ISSigEOS);
	InsertaSenyal(SIGN_SOCKCLOSE, ISSigSockClose);
	BotSet(ipserv);
}
BOTFUNCHELP(ISHSetipv)
{
	Responde(cl, CLI(ipserv), "Fija una ip virtual para un determinado nick.");
	Responde(cl, CLI(ipserv), "Este nick debe estar registrado.");
	Responde(cl, CLI(ipserv), "As�, cuando se identifique como propietario de su nick, se le adjudicar� su ip virtual personalizada.");
	Responde(cl, CLI(ipserv), "Puede especificarse un tiempo de duraci�n, en d�as.");
	Responde(cl, CLI(ipserv), "Pasado este tiempo, su ip ser� eliminada.");
	Responde(cl, CLI(ipserv), "La ip virtual puede ser una palabra, n�meros, etc. pero siempre caracteres alfanum�ricos.");
	Responde(cl, CLI(ipserv), "Para que se visualice, es necesario que el nick lleve el modo +x.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV nick [ipv [caducidad]]");
	return 0;
}
BOTFUNCHELP(ISHSetipv2)
{
	Responde(cl, CLI(ipserv), "Fija una ip virtual de tipo 2.");
	Responde(cl, CLI(ipserv), "Act�a de la misma forma que \00312SETIPV\003 pero la ip es de tipo 2.");
	Responde(cl, CLI(ipserv), "Este tipo consiste en a�adir el sufijo de la red al final de la ip virtual.");
	Responde(cl, CLI(ipserv), "As�, la ip que especifiques ser� adjuntada del sufijo \00312%s\003 de forma autom�tica.", ipserv->sufijo);
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV2 nick [ipv [caducidad]]");
	return 0;
}
BOTFUNCHELP(ISHTemphost)
{
	Responde(cl, CLI(ipserv), "Cambia el host de un usuario.");
	Responde(cl, CLI(ipserv), "Este host s�lo se visualizar� si el nick tiene el modo +x.");
	Responde(cl, CLI(ipserv), "Si el usuario cierra su sesi�n, el host es eliminado de tal forma que al volverla a iniciar ya no lo tendr�.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312TEMPHOST nick host");
	return 0;
}
BOTFUNCHELP(ISHClones)
{
	Responde(cl, CLI(ipserv), "Gestiona la lista de ips con m�s clones.");
	Responde(cl, CLI(ipserv), "Estas ips o hosts tienen un n�mero concreto de clones permitidos.");
	Responde(cl, CLI(ipserv), "Si se sobrepasa el l�mite, los usuarios son desconectados.");
	Responde(cl, CLI(ipserv), "Es importante distinguir entre ip y host. Si el usuario resuelve a host, deber� introducirse el host.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312CLONES {ip|host} [n�mero]");
	return 0;
}
BOTFUNCHELP(ISHVHost)
{
	Responde(cl, CLI(ipserv), "Personaliza tu host virtual.");
	Responde(cl, CLI(ipserv), "Si no especificas ning�n par�metro, tu host vitual se eliminar�.");
	Responde(cl, CLI(ipserv), "Estas ips personalizadas no pueden acabar con menos de 3 letras despu�s del �ltimo punto (.)");
	Responde(cl, CLI(ipserv), "S�lo puedes usar este comando una vez cada 24 horas.");
	Responde(cl, CLI(ipserv), "Las ips personalizadas pueden contener colores, negrita y subrayado");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312VHOST [vhost]");
	return 0;
}
BOTFUNC(ISHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), "\00312%s\003 se encarga de gestionar las ips virtuales de la red.", ipserv->hmod->nick);
		Responde(cl, CLI(ipserv), "Seg�n est� configurado, cambiar� la clave de cifrado cada \00312%i\003 horas.", ipserv->cambio / 3600);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Comandos disponibles:");
		ListaDescrips(ipserv->hmod, cl);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Para m�s informaci�n, \00312/msg %s %s comando", ipserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], ipserv->hmod, param, params))
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Opci�n desconocida.");
	EOI(ipserv, 0);
	return 0;
}
BOTFUNC(ISSetipv)
{
	char *ipv;
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "nick [ipv [caducidad]]");
		return 1;
	}
	if (!IsReg(param[1]))
	{
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no est� registrado.");
		return 1;
	}
	if (params >= 3)
	{
		Cliente *al;
		if (strlen(param[2]) > 30)
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "La ip virtual no puede tener m�s de 30 caracteres.");
			return 1;
		}
		if (ipserv->sufijo && !strcasecmp(param[0], "SETIPV2"))
		{
			ircsprintf(buf, "%s.%s", param[2], ipserv->sufijo);
			ipv = buf;
		}
		else
			ipv = param[2];
		SQLInserta(IS_SQL, param[1], "ipvirtual", ipv);
		if (params == 4)
			SQLInserta(IS_SQL, param[1], "ipcaduca", "%lu", time(0) + atol(param[3]) * 8600);
		Responde(cl, CLI(ipserv), "Se ha a�adido la ip virtual \00312%s\003 a \00312%s\003.", ipv, param[1]);
		if (params == 4)
			Responde(cl, CLI(ipserv), "Esta ip caducar� dentro de \00312%i\003 d�as.", atoi(param[3]));
		if (ProtFunc(P_CAMBIO_HOST_REMOTO) && (al = BuscaCliente(param[1])))
			ProtFunc(P_CAMBIO_HOST_REMOTO)(al, ipv);
		if ((al = BuscaCliente(param[1])))		
			ProtFunc(P_NOTICE)(al, CLI(ipserv), "*** Protecci�n IP: tu direcci�n virtual es %s", ipv);
	}
	else
	{	
		SQLInserta(IS_SQL, param[1], "ipvirtual", NULL);
		Responde(cl, CLI(ipserv), "La ip virtual de \00312%s\003 ha sido eliminada.", param[1]);
	}
	EOI(ipserv, 1);
	return 0;
}
BOTFUNC(ISTemphost)
{
	Cliente *al;
	if (!ProtFunc(P_CAMBIO_HOST_REMOTO))
	{
		Responde(cl, CLI(ipserv), ERR_NSUP);
		return 1;
	}
	if (params < 3)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "nick nuevohost");
		return 1;
	}
	if (!(al = BuscaCliente(param[1])))
	{
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este usuario no est� conectado.");
		return 1;
	}
	ProtFunc(P_CAMBIO_HOST_REMOTO)(al, param[2]);
	Responde(cl, CLI(ipserv), "El host de \00312%s\003 se ha cambiado por \00312%s\003.", param[1], param[2]);
	ProtFunc(P_NOTICE)(al, CLI(ipserv), "*** Protecci�n IP: tu direcci�n virtual es %s", param[2]);
	EOI(ipserv, 2);
	return 0;
}
BOTFUNC(ISClones)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "[+|-]{ip|host|patr�n} [n�] [Nota]");
		return 1;
	}
	if (*param[1] == '+')
	{
		char *nota = NULL;
		time_t fecha = time(NULL);
		param[1]++;
		if (atoi(param[2]) < 1)
		{
			Responde(cl, CLI(ipserv), IS_ERR_SNTX, "El n�mero de clones debe ser un n�mero entero");
			return 1;
		}

		SQLInserta(IS_CLONS, param[1], "clones", param[2]);
		
		if (params > 3) { //Comprobamos nota o creamos una.
		  	nota = Unifica(param, params, 3, -1);
			SQLInserta(IS_CLONS, param[1], "nota", "%s por %s a las %s\t", nota, cl->nombre, Fecha(&fecha));		  			
		}
		else
		 	SQLInserta(IS_CLONS, param[1], "nota", "A�adido por %s a las %s\t", cl->nombre, Fecha(&fecha));		
		
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 se ha a�adido con \00312%s\003 clones.", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		if (!SQLCogeRegistro(IS_CLONS, param[1], "clones"))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "La ip/host no tiene asignado clones.");
			return 1;
		}
		SQLBorra(IS_CLONS, param[1]);
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 le han sido eliminados todos los clones.", param[1]);
	}
	else
	{
		SQLRes res;
		SQLRow row;
		u_int i;
		char *rep;
		rep = str_replace(param[1], '*', '%');
		if (!(res = SQLQuery("SELECT item,clones,nota from %s%s where item LIKE '%s'", PREFIJO, IS_CLONS, rep)))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "No se han encontrado coincidencias.");
			return 1;
		}
		Responde(cl, CLI(ipserv), "*** Ip/Host que coinciden con \00312%s\003 ***", param[1]);
		for (i = 0; i < ipserv->maxlist && (row = SQLFetchRow(res)); i++)
			Responde(cl, CLI(ipserv), "Ip/Host: \00312%s\003 Clones: \00312%s\003 Nota: \00312%s\003", row[0], row[1], row[2]);
		Responde(cl, CLI(ipserv), "Resultado: \00312%i\003/\00312%i", i, SQLNumRows(res));
		SQLFreeRes(res);
	}
	EOI(ipserv, 3);
	return 0;
}
BOTFUNC(ISVHost)
{
	char *ipv;
	if (params < 2)
	{
		SQLInserta(IS_SQL, cl->nombre, "ipvirtual", NULL);
		Responde(cl, CLI(ipserv), "Vhost eliminado");
	}
	else
	{
		if ((ipv = ISIpValida(param[1])))
		{
			ircsprintf(buf, "Vhost no permitido: %s", ipv);
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, buf);
			return 1;
		}
		if (CogeCache(IS_CACHE_VHOST, cl->nombre, ipserv->hmod->id))
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "No puedes efectuar un cambio de vhost hasta que no transcurran 24h desde el �ltimo cambio.");
			return 1;
		}
		if (ipserv->pvhost && ipserv->sufijo)
		{
			ircsprintf(buf, "%s.%s", param[1], ipserv->sufijo);
			ipv = buf;
		}
		else
			ipv = param[1];
		SQLInserta(IS_SQL, cl->nombre, "ipvirtual", ipv);
		InsertaCache(IS_CACHE_VHOST, cl->nombre, 86400, ipserv->hmod->id, ipv);
		ProtFunc(P_NOTICE)(cl, CLI(ipserv), "*** Protecci�n IP: tu direcci�n virtual es %s", ipv);
		Responde(cl, CLI(ipserv), "Vhost cambiado a \00312%s", ipv);
	}
	EOI(ipserv, 4);
	return 0;
}
int ISSigNick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		Cliente *aux;
		int i = 0, clons = ipserv->clones;
		char *cc;
		for (aux = clientes; aux; aux = aux->sig)
		{
			if (EsServidor(aux) || EsBot(aux))
				continue;
			if (!strcmp(cl->host, aux->host))
				i++;
		}
		if ((cc = SQLCogeRegistro(IS_CLONS, cl->host, "clones")))
			clons = atoi(cc);
		if (i > clons)
			ProtFunc(P_QUIT_USUARIO_REMOTO)(cl, CLI(ipserv), "Demasiados clones.");
	}
	return 0;
}
int ISSigUmode(Cliente *cl, char *modos)
{
	if ((cl->modos & UMODE_HIDE) && strchr(modos, 'x'))
		ISMakeVirtualhost(cl, 1);
	return 0;
}
int ISSigIdOk(Cliente *al)
{
	if (al->modos & UMODE_HIDE)
		ISMakeVirtualhost(al, 1);
	return 0;
}
int ISSigSQL()
{
	/*SQLNuevaTabla(IS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"ip varchar(255) default NULL, "
  		"caduca int4 default '0', "
  		"KEY item (item) "
		");", PREFIJO, IS_SQL);*/
	SQLNuevaTabla(IS_CLONS, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"clones varchar(255) default NULL, "
		"nota text,"
  		"KEY item (item) "
		");", PREFIJO, IS_CLONS);
	LlamaSenyal(SIGN_SQL2, 1);
	return 0;
}
int ISSigDrop(char *nick)
{
	SQLBorra(IS_SQL, nick);
	return 0;
}
int ISSigEOS()
{
	IniciaProceso(ISProcIps);
	return 0;
}
int ISSigSockClose()
{
	DetieneProceso(ISProcIps);
	return 0;
}
char *CifraIpTEA(char *ipreal)
{
	static char cifrada[512], clave[BUFSIZE];
	char *p;
	int ts = 0;
	unsigned int ourcrc, v[2], k[2], x[2];
	ourcrc = Crc32(ipreal, strlen(ipreal));
	ircsprintf(clave, "a%lu", Crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv->cambio));
	p = cifrada;
	while (1)
  	{
		x[0] = x[1] = 0;
		k[0] = base64toint(clave);
		k[1] = base64toint(clave + 6);
    		v[0] = (k[0] & 0xffff0000) + ts;
    		v[1] = ourcrc;
    		tea(v, k, x);
    		inttobase64(p, x[0], 6);
    		p[6] = '.';
    		inttobase64(p + 7, x[1], 6);
		if (strchr(p, '[') == NULL && strchr(p, ']') == NULL)
			break;
    		else
		{
			if (++ts == 65536)
			{
				strlcpy(p, ipreal, sizeof(cifrada));
				break;
			}
		}
	}
	return cifrada;
}
char *ISMakeVirtualhost(Cliente *al, int mostrar)
{
	char *cifrada, buf[BUFSIZE], *sufix, *vip;
	if (!ProtFunc(P_CAMBIO_HOST_REMOTO))
		return NULL;
	cifrada = CifraIpTEA(al->host);
	sufix = ipserv->sufijo ? ipserv->sufijo : "virtual";
	if (IsId(al) && (vip = SQLCogeRegistro(IS_SQL, al->nombre, "ipvirtual")))
		cifrada = vip;
	else
	{
		ircsprintf(buf, "%s.%s", cifrada, sufix);
		cifrada = buf;
	}
	if (!al->vhost || strcmp(al->vhost, cifrada))
	{
		if (al->modos & UMODE_HIDE)
		{
			ProtFunc(P_CAMBIO_HOST_REMOTO)(al, cifrada);
			if (mostrar)
				ProtFunc(P_NOTICE)(al, CLI(ipserv), "*** Protecci�n IP: tu direcci�n virtual es %s", cifrada);
		}
		ircstrdup(al->vhost, cifrada);
	}
	return al->vhost;
}
int ISProcIps(Proc *proc)
{
	SQLRes res;
	SQLRow row;
	if (proc->time + 1800 < time(0)) /* lo hacemos cada 30 mins */
	{
		if (!(res = SQLQuery("SELECT item from %s%s where (ipcaduca != '0' AND ipcaduca < %i) LIMIT 30 OFFSET %i", PREFIJO, IS_SQL, time(0), proc->proc)) || !SQLNumRows(res))
		{
			proc->proc = 0;
			proc->time = time(0);
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			SQLInserta(IS_SQL, row[0], "ipvirtual", NULL);
			LlamaSenyal(IS_SIGN_DROP, 1, row[0]);
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
char *ISIpValida(char *ip)
{
	char *c, *allow = "_-.��������������������������������������������";
	int i;
	if (strlen(ip) > 30)
		return "vhost demasiado largo";
	for (c = ip; *c; c++)
	{
		if (!IsAlnum(*c) && !strchr(allow, *c))
			return "caractereres no permitidos";
	}
	if (!strchr(ip, '.') && strlen(ip) < 4)
		return "si no contiene ning�n . debe tener m�s de 4 caracteres";
	if ((c = strrchr(ip, '.')) && (strlen(c) < 5 || strchr(c, '-') || strchr(c, '_')))
		return "debe tener m�s de 4 caracteres despu�s del �ltimo . y no contener - ni _";
	for (i = 0; forbips[i]; i++)
	{
		if (!match(forbips[i], ip))
			return forbips[i];
	}
	return NULL;
}
