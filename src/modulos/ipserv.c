/*
 * $Id: ipserv.c,v 1.21 2005-12-04 14:09:23 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "bdd.h"
#include "modulos/ipserv.h"
#include "modulos/nickserv.h"

IpServ ipserv;
#define ExFunc(x) TieneNivel(cl, x, ipserv.hmod, NULL)

BOTFUNC(ISHelp);
BOTFUNC(ISSetipv);
BOTFUNC(ISTemphost);
BOTFUNC(ISClones);
#ifdef UDB
BOTFUNC(ISOpts);
BOTFUNC(ISDns);
BOTFUNC(ISNolines);
#endif

int ISSigSQL	();
#ifndef UDB
int ISSigIdOk 		(Cliente *);
char *ISMakeVirtualhost 	(Cliente *, int);
int ISSigUmode		(Cliente *, char *);
int ISSigNick		(Cliente *, int);	
#endif
int ISSigDrop	(char *);
int ISSigEOS	();

int ISProcIps	(Proc *);


static bCom ipserv_coms[] = {
	{ "help" , ISHelp , N2 } ,
	{ "setipv" , ISSetipv , N2 } ,
	{ "setipv2" , ISSetipv , N2 } ,
	{ "temphost" , ISTemphost , N2 } ,
	{ "clones" , ISClones , N4 } ,
#ifdef UDB
	{ "set" , ISOpts , N4 } ,
	{ "dns" , ISDns , N4 } ,
	{ "nolines" , ISNolines , N3 } ,
#endif
	{ 0x0 , 0x0 , 0x0 }
};

void ISSet(Conf *, Modulo *);
int ISTest(Conf *, int *);

ModInfo MOD_INFO(IpServ) = {
	"IpServ" ,
	0.10 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};
	
int MOD_CARGA(IpServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	if (!mod->config)
	{
		Error("[%s] Falta especificar archivo de configuración para %s", mod->archivo, MOD_INFO(IpServ).nombre);
		errores++;
	}
	else
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
	return errores;
}	
int MOD_DESCARGA(IpServ)()
{
#ifndef UDB
	BorraSenyal(SIGN_UMODE, ISSigUmode);
	BorraSenyal(NS_SIGN_IDOK, ISSigIdOk);
	BorraSenyal(SIGN_POST_NICK, ISSigNick);
#endif
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
	int i;
	for (i = 0; i < config->secciones; i++)
	{
		if (!strcmp(config->seccion[i]->item, "funciones"))
			ProcesaComsMod(config->seccion[i], mod, ipserv_coms);
		else if (!strcmp(config->seccion[i]->item, "funcion"))
			SetComMod(config->seccion[i], mod, ipserv_coms);
#ifndef UDB
		else if (!strcmp(config->seccion[i]->item, "clones"))
			ipserv.clones = atoi(config->seccion[i]->data);
#endif
		else if (!strcmp(config->seccion[i]->item, "sufijo"))
			ircstrdup(ipserv.sufijo, config->seccion[i]->data);
		else if (!strcmp(config->seccion[i]->item, "cambio"))
			ipserv.cambio = atoi(config->seccion[i]->data) * 3600;
	}
#ifndef UDB
	InsertaSenyal(SIGN_UMODE, ISSigUmode);
	InsertaSenyal(NS_SIGN_IDOK, ISSigIdOk);
	InsertaSenyal(SIGN_POST_NICK, ISSigNick);
#endif
	InsertaSenyal(SIGN_SQL, ISSigSQL);
	InsertaSenyal(NS_SIGN_DROP, ISSigDrop);
	InsertaSenyal(SIGN_EOS, ISSigEOS);
	IniciaProceso(ISProcIps);
	BotSet(ipserv);
}
BOTFUNC(ISHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), "\00312%s\003 se encarga de gestionar las ips virtuales de la red.", ipserv.hmod->nick);
		Responde(cl, CLI(ipserv), "Según esté configurado, cambiará la clave de cifrado cada \00312%i\003 horas.", ipserv.cambio / 3600);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Comandos disponibles:");
		FuncResp(ipserv, "SETIPV", "Agrega o elimina una ip virtual.");
		FuncResp(ipserv, "SETIPV2", "Agrega o elimina una ip virtual de tipo 2.");
		FuncResp(ipserv, "TEMPHOST", "Cambia el host de un usuario.");
#ifdef UDB
		FuncResp(ipserv, "NOLINES", "Controla las excepciones para una ip.");
#endif
		FuncResp(ipserv, "CLONES", "Administra la lista de ips con más clones.");
#ifdef UDB
		FuncResp(ipserv, "SET", "Fija algunos parámetros de la red.");
#endif
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Para más información, \00312/msg %s %s comando", ipserv.hmod->nick, strtoupper(param[0]));
	}
	else if (!strcasecmp(param[1], "SETIPV") && ExFunc("SETIPV"))
	{
		Responde(cl, CLI(ipserv), "\00312SETIPV");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Fija una ip virtual para un determinado nick.");
		Responde(cl, CLI(ipserv), "Este nick debe estar registrado.");
		Responde(cl, CLI(ipserv), "Así, cuando se identifique como propietario de su nick, se le adjudicará su ip virtual personalizada.");
		Responde(cl, CLI(ipserv), "Puede especificarse un tiempo de duración, en días.");
		Responde(cl, CLI(ipserv), "Pasado este tiempo, su ip será eliminada.");
		Responde(cl, CLI(ipserv), "La ip virtual puede ser una palabra, números, etc. pero siempre caracteres alfanuméricos.");
		Responde(cl, CLI(ipserv), "Para que se visualice, es necesario que el nick lleve el modo +x.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "SETIPV2") && ExFunc("SETIPV2"))
	{
		Responde(cl, CLI(ipserv), "\00312SETIPV2");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Fija una ip virtual de tipo 2.");
		Responde(cl, CLI(ipserv), "Actúa de la misma forma que \00312SETIPV\003 pero la ip es de tipo 2.");
		Responde(cl, CLI(ipserv), "Este tipo consiste en añadir el sufijo de la red al final de la ip virtual.");
		Responde(cl, CLI(ipserv), "Así, la ip que especifiques será adjuntada del sufijo \00312%s\003 de forma automática.", ipserv.sufijo);
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312SETIPV2 {+|-}nick [ipv [caducidad]]");
	}
	else if (!strcasecmp(param[1], "TEMPHOST") && ExFunc("TEMPHOST"))
	{
		Responde(cl, CLI(ipserv), "\00312TEMPHOST");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Cambia el host de un usuario.");
		Responde(cl, CLI(ipserv), "Este host sólo se visualizará si el nick tiene el modo +x.");
		Responde(cl, CLI(ipserv), "Si el usuario cierra su sesión, el host es eliminado de tal forma que al volverla a iniciar ya no lo tendrá.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312TEMPHOST nick host");
	}
	else if (!strcasecmp(param[1], "CLONES") && ExFunc("CLONES"))
	{
		Responde(cl, CLI(ipserv), "\00312CLONES");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Gestiona la lista de ips con más clones.");
		Responde(cl, CLI(ipserv), "Estas ips o hosts tienen un número concreto de clones permitidos.");
		Responde(cl, CLI(ipserv), "Si se sobrepasa el límite, los usuarios son desconectados.");
		Responde(cl, CLI(ipserv), "Es importante distinguir entre ip y host. Si el usuario resuelve a host, deberá introducirse el host.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312CLONES +{ip|host} número");
		Responde(cl, CLI(ipserv), "Añade una ip|host con un número propio de clones.");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312CLONES -{ip|host}");
		Responde(cl, CLI(ipserv), "Borra una ip|host.");
	}
#ifdef UDB
	else if (!strcasecmp(param[1], "SET") && ExFunc("SET"))
	{
		Responde(cl, CLI(ipserv), "\00312SET");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Establece varios parámetros de la red.");
		Responde(cl, CLI(ipserv), "\00312QUIT_IPS\003 Establece el mensaje de desconexión para ips con capacidad personal de clones.");
		Responde(cl, CLI(ipserv), "\00312QUIT_CLONES\003 Fija el mensaje de desconexión cuando se supera el número de clones permitidos en la red.");
		Responde(cl, CLI(ipserv), "\00312CLONES\003 Indica el número de clones permitidos en la red.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312SET quit_ips|quit_clones|clones valor");
	}
	else if (!strcasecmp(param[1], "DNS") && ExFunc("DNS"))
	{
		Responde(cl, CLI(ipserv), "\00312DNS");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Establece una resolución inversa de una ip.");
		Responde(cl, CLI(ipserv), "Este comando permite hacer una relación a partir de una ip hacia un host.");
		Responde(cl, CLI(ipserv), "Cuando conecte un usuario al servidor, si su ip está en DNS, se le asignará automáticamente este host relacionado, sin necesidad de resoluciones inversas.");
		Responde(cl, CLI(ipserv), "NOTA: Debe ser una ip");
		Responde(cl, CLI(ipserv), "Si no se especifica ningún host, la ip es eliminada de la lista.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312DNS ip [host]");
	}
	else if (!strcasecmp(param[1], "NOLINES") && ExFunc("NOLINES"))
	{
		Responde(cl, CLI(ipserv), "\00312NOLINES");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Gestiona las excepciones para una ip.");
		Responde(cl, CLI(ipserv), "Con estas excepciones, la conexión es capaz de saltarse restricciones.");
		Responde(cl, CLI(ipserv), "Excepciones:");
		Responde(cl, CLI(ipserv), "\00312G\003 - GLines");
		Responde(cl, CLI(ipserv), "\00312Z\003 - ZLines");
		Responde(cl, CLI(ipserv), "\00312Q\003 - QLines");
		Responde(cl, CLI(ipserv), "\00312S\003 - Shuns");
		Responde(cl, CLI(ipserv), "\00312T\003 - Throttle");
		Responde(cl, CLI(ipserv), "La ip que tenga estas excepciones será inmune.");
		Responde(cl, CLI(ipserv), "Sin parámetros, se borran las excepciones para esa ip.");
		Responde(cl, CLI(ipserv), " ");
		Responde(cl, CLI(ipserv), "Sintaxis: \00312NOLINES ip [nolines]");
		Responde(cl, CLI(ipserv), "Ejemplo: \00312NOLINES 127.0.01 GZQST");
	}
		
#endif
	else
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNC(ISSetipv)
{
	char mod = !strcasecmp(param[0], "SETIPV2") ? 1 : 0;
	char *ipv;
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "SETIPV +-nick [ipv [caducidad]]");
		return 1;
	}
	if (!IsReg(param[1] + 1))
	{
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no está registrado.");
		return 1;
	}
#ifdef UDB
	if (!IsNickUDB(param[1] + 1))
	{
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este nick no está migrado.");
		return 1;
	}
#endif
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			Responde(cl, CLI(ipserv), IS_ERR_PARA, "SETIPV +nick ipv [caducidad]");
			return 1;
		}
		param[1]++;
		if (mod)
		{
			buf[0] = '\0';
			ircsprintf(buf, "%s.%s", param[2], ipserv.sufijo);
			ipv = buf;
		} 
		else
			ipv = param[2];
		SQLInserta(IS_SQL, param[1], "ip", ipv);
		if (params == 4)
			SQLInserta(IS_SQL, param[1], "caduca", "%lu", time(0) + atol(param[3]) * 8600);
#ifdef UDB
		PropagaRegistro("N::%s::V %s", param[1], ipv);
#endif
		Responde(cl, CLI(ipserv), "Se ha añadido la ip virtual a \00312%s\003.", ipv);
		if (params == 4)
			Responde(cl, CLI(ipserv), "Esta ip caducará dentro de \00312%i\003 días.", atoi(param[3]));
	}
	else if (*param[1] == '-')
	{
		param[1]++;
		SQLBorra(IS_SQL, param[1]);
#ifdef UDB
		PropagaRegistro("N::%s::V", param[1]);
#endif
		Responde(cl, CLI(ipserv), "La ip virtual de \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "SETIPV {+|-}nick [ipv [caducidad]]");
		return 1;
	}
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
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "TEMPHOST nick nuevohost");
		return 1;
	}
	if (!(al = BuscaCliente(param[1], NULL)))
	{
		Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Este usuario no está conectado.");
		return 1;
	}
	ProtFunc(P_CAMBIO_HOST_REMOTO)(al, param[2]);
	Responde(cl, CLI(ipserv), "El host de \00312%s\003 se ha cambiado por \00312%s\003.", param[1], param[2]);
	return 0;
}
#ifdef UDB
BOTFUNC(ISOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "SET quit_ips|quit_clones|clones valor");
		return 1;
	}
	if (!strcasecmp(param[1], "QUIT_IPS"))
	{
		PropagaRegistro("S::T %s", Unifica(param, params, 2, -1));
		Responde(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión para ips con número personal de clones.");
	}
	else if (!strcasecmp(param[1], "QUIT_CLONES"))
	{
		PropagaRegistro("S::Q %s", Unifica(param, params, 2, -1));
		Responde(cl, CLI(ipserv), "Se ha cambiado el mensaje de desconexión al rebasar el número de clones permitidos en la red.");
	}
	else if (!strcasecmp(param[1], "CLONES"))
	{
		int clons = atoi(param[2]);
		if (clons < 1 || clons > 1024)
		{
			Responde(cl, CLI(ipserv), IS_ERR_EMPT, "El número de clones debe estar entre 1 y 1024.");
			return 1;
		}
		PropagaRegistro("S::S %c%s", CHAR_NUM, param[2]);
		Responde(cl, CLI(ipserv), "El número de clones permitidos en la red se ha cambiado a \00312%s", param[2]);
	}
	else
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "SET quit_ips|quit_clones|clones valor");
		return 1;
	}
	return 0;
}
BOTFUNC(ISDns)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "DNS ip [host]");
		return 1;
	}
	if (!EsIp(param[1]))
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "No parece ser una ip");
		return 1;
	}
	if (params == 3)
	{
		PropagaRegistro("I::%s::H %s", param[1], param[2]);
		Responde(cl, CLI(ipserv), "Se ha cambiado la resolución de la ip \00312%s\003 a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("I::%s::H", param[1]);
		Responde(cl, CLI(ipserv), "Se ha eliminado al resolución de la ip \00312%s", param[1]);
	}
	return 0;
}
BOTFUNC(ISNolines)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "NOLINES ip [G|Z|Q|S|T]");
		return 1;
	}
	if (params == 3)
	{
		char *c;
		for (c = param[2]; !BadPtr(c); c++)
		{
			if (!strchr("GZQST", *c))
			{
				Responde(cl, CLI(ipserv), IS_ERR_EMPT, "Sólo se aceptan las nolines G|Z|Q|S|T");
				return 1;
			}
		}
		PropagaRegistro("I::%s::E %s", param[1], param[2]);
		Responde(cl, CLI(ipserv), "Se han cambiado las excepciones para la ip \00312%s\003 a \00312%s", param[1], param[2]);
	}
	else
	{
		PropagaRegistro("I::%s::E", param[1]);
		Responde(cl, CLI(ipserv), "Se han eliminado las excepciones para la ip \00312%s", param[1]);
	}
	return 0;
}
#endif
BOTFUNC(ISClones)
{
	if (params < 2)
	{
		Responde(cl, CLI(ipserv), IS_ERR_PARA, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	if (*param[1] == '+')
	{
		if (params < 3)
		{
			Responde(cl, CLI(ipserv), IS_ERR_PARA, "CLONES +{ip|host} nº");
			return 1;
		}
		if (atoi(param[2]) < 1)
		{
			Responde(cl, CLI(ipserv), IS_ERR_SNTX, "CLONES +{ip|host} número");
			return 1;
		}
		param[1]++;
#ifdef UDB
		PropagaRegistro("I::%s::S %c%s", param[1], CHAR_NUM, param[2]);
#else
		SQLInserta(IS_CLONS, param[1], "clones", param[2]);
#endif
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 se ha añadido con \00312%s\003 clones.", param[1], param[2]);
	}
	else if (*param[1] == '-')
	{
		param[1]++;
#ifdef UDB
		PropagaRegistro("I::%s::S", param[1]);
#else
		SQLBorra(IS_CLONS, param[1]);
#endif
		Responde(cl, CLI(ipserv), "La ip/host \00312%s\003 ha sido eliminada.", param[1]);
	}
	else
	{
		Responde(cl, CLI(ipserv), IS_ERR_SNTX, "CLONES {+|-}{ip|host} [nº]");
		return 1;
	}
	return 0;
}	
#ifndef UDB
int ISSigNick(Cliente *cl, int nuevo)
{
	if (!nuevo)
	{
		Cliente *aux;
		int i = 0, clons = ipserv.clones;
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
#endif
int ISSigSQL()
{
	if (!SQLEsTabla(IS_SQL))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"ip varchar(255) default NULL, "
  			"caduca int4 default '0' "
			");", PREFIJO, IS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_SQL);
	}
#ifndef UDB
	if (!SQLEsTabla(IS_CLONS))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"clones varchar(255) default NULL "
			");", PREFIJO, IS_CLONS))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, IS_CLONS);
	}
#endif
	SQLCargaTablas();
	return 0;
}
int ISCompruebaCifrado()
{
	char clave[128];
#ifndef UDB
	char tiempo[BUFSIZE];
	FILE *fp;
#endif
	ircsprintf(clave, "a%lu", Crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv.cambio));
#ifdef UDB
	PropagaRegistro("S::L %s", clave);
#else
	if ((fp = fopen("ts", "r")))
	{
		if (fgets(tiempo, BUFSIZE, fp))
		{
			if (atol(tiempo) + ipserv.cambio < time(0))
			{
				fclose(fp);
				escribe:
				if ((fp = fopen("ts", "w")))
				{
					tiempo[0] = '\0';
					ircsprintf(tiempo, "%lu\n", time(0));
					fputs(tiempo, fp);
					fclose(fp);
				}
				return 0;
			}
		}
		else
			goto escribe;
	}
	else
		goto escribe;
#endif
	return 0;
}
int ISSigDrop(char *nick)
{
	SQLBorra(IS_SQL, nick);
	return 0;
}
int ISSigEOS()
{
#ifdef UDB
	PropagaRegistro("S::I %s", ipserv.hmod->mascara);
	PropagaRegistro("S::J %s", ipserv.sufijo);
#endif
	ISCompruebaCifrado();
	return 0;
}
#ifndef UDB
char *CifraIpTEA(char *ipreal)
{
	static char cifrada[512], clave[BUFSIZE];
	char *p;
	int ts = 0;
	unsigned int ourcrc, v[2], k[2], x[2];
	ourcrc = Crc32(ipreal, strlen(ipreal));
	ircsprintf(clave, "a%lu", Crc32(conf_set->clave_cifrado, strlen(conf_set->clave_cifrado)) + (time(0) / ipserv.cambio));
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
				strcpy(p, ipreal);
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
	sufix = ipserv.sufijo ? ipserv.sufijo : "virtual";
	if (IsId(al))
	{
		if (!(vip = SQLCogeRegistro(IS_SQL, al->nombre, "ip")))
		{
			buf[0] = '\0';
			ircsprintf(buf, "%s.%s", cifrada, sufix);
			cifrada = buf;
		}
		else
			cifrada = vip;
	}
	else
	{
		buf[0] = '\0';
		ircsprintf(buf, "%s.%s", cifrada, sufix);
		cifrada = buf;
	}
	if (al->vhost && strcmp(al->vhost, cifrada))
	{
		if (al->modos & UMODE_HIDE)
			ProtFunc(P_CAMBIO_HOST_REMOTO)(al, al->vhost);
		if (mostrar)
			ProtFunc(P_NOTICE)(al, CLI(ipserv), "*** Protección IP: tu dirección virtual es %s", cifrada);
	}
	return al->vhost;
}
#endif
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
#ifdef UDB
			PropagaRegistro("N::%s::V", row[0]);
#endif
		}
		proc->proc += 30;
		SQLFreeRes(res);
	}
	return 0;
}
