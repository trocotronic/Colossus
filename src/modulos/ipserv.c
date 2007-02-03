/*
 * $Id: ipserv.c,v 1.30 2007-02-03 22:57:27 Trocotronic Exp $ 
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
#define ExFunc(x) TieneNivel(cl, x, ipserv->hmod, NULL)

BOTFUNC(ISHelp);
BOTFUNC(ISSetipv);
BOTFUNCHELP(ISHSetipv);
BOTFUNCHELP(ISHSetipv2);
BOTFUNC(ISTemphost);
BOTFUNCHELP(ISHTemphost);
BOTFUNC(ISClones);
BOTFUNCHELP(ISHClones);

int ISSigSQL	();
DLLFUNC int ISSigIdOk 		(Cliente *);
char *ISMakeVirtualhost 	(Cliente *, int);
DLLFUNC int ISSigUmode		(Cliente *, char *);
DLLFUNC int ISSigNick		(Cliente *, int);	
int ISSigDrop	(char *);
int ISSigEOS	();

int ISProcIps	(Proc *);


static bCom ipserv_coms[] = {
	{ "help" , ISHelp , N2 , "Muestra esta ayuda." , NULL } ,
	{ "setipv" , ISSetipv , N2 , "Agrega o elimina una ip virtual." , ISHSetipv } ,
	{ "setipv2" , ISSetipv , N2 , "Agrega o elimina una ip virtual de tipo 2." , ISHSetipv2 } ,
	{ "temphost" , ISTemphost , N2 , "Cambia el host de un usuario." , ISHTemphost } ,
	{ "clones" , ISClones , N4 , "Administra la lista de ips con más clones." , ISHClones } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void ISSet(Conf *, Modulo *);
int ISTest(Conf *, int *);

ModInfo MOD_INFO(IpServ) = {
	"IpServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};
	
int MOD_CARGA(IpServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(IpServ).nombre);
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
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(IpServ).nombre);
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
	BorraSenyal(SIGN_UMODE, ISSigUmode);
	BorraSenyal(NS_SIGN_IDOK, ISSigIdOk);
	BorraSenyal(SIGN_POST_NICK, ISSigNick);
	BorraSenyal(SIGN_SQL, ISSigSQL);
	BorraSenyal(NS_SIGN_DROP, ISSigDrop);
	BorraSenyal(SIGN_EOS, ISSigEOS);
	DetieneProceso(ISProcIps);
	BotUnset(ipserv);
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
	IniciaProceso(ISProcIps);
	BotSet(ipserv);
}
BOTFUNCHELP(ISHSetipv)
{
	Responde(cl, CLI(ipserv), "Fija una ip virtual para un determinado nick.");
	Responde(cl, CLI(ipserv), "Este nick debe estar registrado.");
	Responde(cl, CLI(ipserv), "Así, cuando se identifique como propietario de su nick, se le adjudicará su ip virtual personalizada.");
	Responde(cl, CLI(ipserv), "Puede especificarse un tiempo de duración, en días.");
	Responde(cl, CLI(ipserv), "Pasado este tiempo, su ip será eliminada.");
	Responde(cl, CLI(ipserv), "La ip virtual puede ser una palabra, números, etc. pero siempre caracteres alfanuméricos.");
	Responde(cl, CLI(ipserv), "Para que se visualice, es necesario que el nick lleve el modo +x.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV nick [ipv [caducidad]]");
	return 0;
}
BOTFUNCHELP(ISHSetipv2)
{
	Responde(cl, CLI(ipserv), "Fija una ip virtual de tipo 2.");
	Responde(cl, CLI(ipserv), "Actúa de la misma forma que \00312SETIPV\003 pero la ip es de tipo 2.");
	Responde(cl, CLI(ipserv), "Este tipo consiste en añadir el sufijo de la red al final de la ip virtual.");
	Responde(cl, CLI(ipserv), "Así, la ip que especifiques será adjuntada del sufijo \00312%s\003 de forma automática.", ipserv->sufijo);
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV2 nick [ipv [caducidad]]");
	return 0;
}
BOTFUNCHELP(ISHTemphost)
{
	Responde(cl, CLI(ipserv), "Cambia el host de un usuario.");
	Responde(cl, CLI(ipserv), "Este host sólo se visualizará si el nick tiene el modo +x.");
	Responde(cl, CLI(ipserv), "Si el usuario cierra su sesión, el host es eliminado de tal forma que al volverla a iniciar ya no lo tendrá.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312TEMPHOST nick host");
	return 0;
}
BOTFUNCHELP(ISHClones)
{
	Responde(cl, CLI(ipserv), "Gestiona la lista de ips con más clones.");
	Responde(cl, CLI(ipserv), "Estas ips o hosts tienen un número concreto de clones permitidos.");
	Responde(cl, CLI(ipserv), "Si se sobrepasa el límite, los usuarios son desconectados.");
	Responde(cl, CLI(ipserv), "Es importante distinguir entre ip y host. Si el usuario resuelve a host, deberá introducirse el host.");
	Responde(cl, CLI(ipserv), " ");
	Responde(cl, CLI(ipserv), "Sintaxis: \00312CLONES {ip|host} [número]");
	return 0;
}

BOTFUNC(ISHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), "\00312%s\003 se encarga de gestionar las ips virtuales de la red.", ipserv->hmod->nick);
		Responde(cl, CLI(ipserv), "Según esté configurado, cambiará la clave de cifrado cada \00312%i\003 horas.", ipserv->cambio / 3600);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Comandos disponibles:");
		ListaDescrips(ipserv->hmod, cl);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Para más información, \00312/msg %s %s comando", ipserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], ipserv->hmod, param, params))
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Opción desconocida.");
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
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
	if (params >= 3)
	{
		if (!strcasecmp(param[0], "SETIPV2"))
		{
			buf[0] = '\0';
			ircsprintf(buf, "%s.%s", param[2], ipserv->sufijo);
			ipv = buf;
		} 
		else
			ipv = param[2];
		SQLInserta(IS_SQL, param[1], "ip", ipv);
		if (params == 4)
			SQLInserta(IS_SQL, param[1], "caduca", "%lu", time(0) + atol(param[3]) * 8600);
		Responde(cl, CLI(ipserv), "Se ha añadido la ip virtual a \00312%s\003.", ipv);
		if (params == 4)
			Responde(cl, CLI(ipserv), "Esta ip caducará dentro de \00312%i\003 días.", atoi(param[3]));
	}
	else
	{
		SQLBorra(IS_SQL, param[1]);
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
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	ProtFunc(P_CAMBIO_HOST_REMOTO)(al, param[2]);
	Responde(cl, CLI(ipserv), "El host de \00312%s\003 se ha cambiado por \00312%s\003.", param[1], param[2]);
	EOI(ipserv, 2);
	return 0;
}
BOTFUNC(ISClones)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, fc->com, "{ip|host} [nº]");
		return 1;
	}
	if (params >= 3)
	{
		if (atoi(param[2]) < 1)
		{
			Responde(cl, CLI(ipserv), IS_ERR_SNTX, "El número de clones debe ser un número entero");
			return 1;
		}
		SQLInserta(IS_CLONS, param[1], "clones", param[2]);
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 se ha añadido con \00312%s\003 clones.", param[1], param[2]);
	}
	else
	{
		SQLBorra(IS_CLONS, param[1]);
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 ha sido eliminada.", param[1]);
	}
	EOI(ipserv, 3);
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
	if (!SQLEsTabla(IS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"ip varchar(255) default NULL, "
  			"caduca int4 default '0', "
  			"KEY item (item) "
			");", PREFIJO, IS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_SQL);
	}
	if (!SQLEsTabla(IS_CLONS))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"clones varchar(255) default NULL, "
  			"KEY item (item) "
			");", PREFIJO, IS_CLONS);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_CLONS);
	}
	SQLCargaTablas();
	return 0;
}
int ISCompruebaCifrado()
{
	char clave[128];
	char tiempo[BUFSIZE];
	FILE *fp;
	ircsprintf(clave, "a%lu", Crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv->cambio));
	if ((fp = fopen("ts", "r")))
	{
		if (fgets(tiempo, BUFSIZE, fp))
		{
			if (atol(tiempo) + ipserv->cambio < time(0))
			{
				escribe:
				if (fp)
					fclose(fp);
				if ((fp = fopen("ts", "w")))
				{
					tiempo[0] = '\0';
					ircsprintf(tiempo, "%lu\n", time(0));
					fputs(tiempo, fp);
					fclose(fp);
				}
				return 0;
			}
			else
				fclose(fp);
		}
		else
			goto escribe;
	}
	else
		goto escribe;
	return 0;
}
int ISSigDrop(char *nick)
{
	SQLBorra(IS_SQL, nick);
	return 0;
}
int ISSigEOS()
{
	ISCompruebaCifrado();
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
	if (IsId(al) && (vip = SQLCogeRegistro(IS_SQL, al->nombre, "ip")))
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
				ProtFunc(P_NOTICE)(al, CLI(ipserv), "*** Protección IP: tu dirección virtual es %s", cifrada);
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
		if (!(res = SQLQuery("SELECT item from %s%s where (caduca != '0' AND caduca < %i) LIMIT 30 OFFSET %i", PREFIJO, IS_SQL, time(0), proc->proc)) || !SQLNumRows(res))
		{
			if (proc->time) /* no es la primera llamada de ese proc */
				ISCompruebaCifrado();
			proc->proc = 0;
			proc->time = time(0);
			return 1;
		}
		while ((row = SQLFetchRow(res)))
		{
			SQLBorra(IS_SQL, row[0]);
			Senyal1(IS_SIGN_DROP, row[0]);
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
