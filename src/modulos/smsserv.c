/*
 * $Id: smsserv.c,v 1.13 2008/02/13 16:16:10 Trocotronic Exp $
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/smsserv.h"

SmsServ *smsserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, smsserv->hmod, NULL)

SOCKFUNC(SmsAbre);
SOCKFUNC(SmsLee);
SOCKFUNC(SmsCierra);
BOTFUNC(SSHelp);
BOTFUNC(SSSend);
BOTFUNCHELP(SSHSend);
BOTFUNC(SSOpts);
BOTFUNCHELP(SSHOpts);
BOTFUNC(SSLista);
BOTFUNCHELP(SSHLista);
BOTFUNC(SSSaldo);
BOTFUNCHELP(SSHSaldo);

int SSSigSQL	();
int SSSigQuit	();

static bCom smsserv_coms[] = {
	{ "help" , SSHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "send" , SSSend , N1 , "Envía un SMS." , SSHSend } ,
	{ "set" , SSOpts , N1 , "Fija distintas opciones." , SSHOpts } ,
	{ "lista" , SSLista , N1 , "Muestra tu lista VIP." , SSHLista } ,
	{ "saldo" , SSSaldo , N4 , "Informa de los créditos restantes." , SSHSaldo } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void SSSet(Conf *, Modulo *);
int SSTest(Conf *, int *);

DataSock *cola[MAX_COLA];

DataSock *BuscaCola(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
			return cola[i];
	}
	return NULL;
}
DataSock *InsertaCola(char *query, int numero, Cliente *cl)
{
	int i;
	for (i = 0; i < MAX_COLA && cola[i]; i++);
	if (i < MAX_COLA)
	{
		DataSock *dts;
		dts = BMalloc(DataSock);
		if (query)
			ircstrdup(dts->query, query);
		dts->numero = numero;
		dts->cl = cl;
		cola[i] = dts;
		return dts;
	}
	return NULL;
}
int BorraCola(Sock *sck)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
		{
			ircfree(cola[i]->query);
			ircfree(cola[i]);
			return 1;
		}
	}
	return 0;
}

ModInfo MOD_INFO(SmsServ) = {
	"SmsServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(SmsServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(SmsServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(SmsServ).nombre))
			{
				if (!SSTest(modulo.seccion[0], &errores))
					SSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(SmsServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(SmsServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		SSSet(NULL, mod);
	return errores;
}
int MOD_DESCARGA(SmsServ)()
{
	BorraSenyal(SIGN_SQL, SSSigSQL);
	BorraSenyal(SIGN_QUIT, SSSigQuit);
	BotUnset(smsserv);
	return 0;
}
int SSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, smsserv_coms, 1);
	if ((eval = BuscaEntrada(config, "espera")))
	{
		if (BadPtr(eval->data) || atoi(eval->data) < 1)
		{
			Error("[%s:%s::%s::%i] El tiempo de espera entre sms debe ser mayor o igual que 1 minuto", config->archivo, eval->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "id")))
	{
		Error("[%s:%s] No se encuentra la directriz id.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (BadPtr(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz id esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	*errores += error_parcial;
	return error_parcial;
}
void SSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!smsserv)
		smsserv = BMalloc(SmsServ);
	smsserv->espera = 300;
	smsserv->restringido = 0;
	ircfree(smsserv->publi);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, smsserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, smsserv_coms);
			else if (!strcmp(config->seccion[i]->item, "alias"))
			{
				for (p = 0; p < config->seccion[i]->secciones; p++)
				{
					if (!strcmp(config->seccion[i]->seccion[p]->item, "sintaxis"))
						CreaAlias(config->seccion[i]->data, config->seccion[i]->seccion[p]->data, mod);
				}
			}
			else if (!strcmp(config->seccion[i]->item, "publicidad"))
				ircstrdup(smsserv->publi, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "espera"))
				smsserv->espera = atoi(config->seccion[i]->data) * 60;
			else if (!strcmp(config->seccion[i]->item, "restringido"))
				smsserv->restringido = 1;
			else if (!strcmp(config->seccion[i]->item, "id"))
				ircstrdup(smsserv->id, config->seccion[i]->data);
		}
	}
	else
		ProcesaComsMod(NULL, mod, smsserv_coms);
	InsertaSenyal(SIGN_SQL, SSSigSQL);
	InsertaSenyal(SIGN_QUIT, SSSigQuit);
	for (i = 0; i < MAX_COLA; i++)
		cola[i] = NULL;
	BotSet(smsserv);
}
SOCKFUNC(SmsAbre)
{
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	SockWrite(sck, "POST /%s HTTP/1.1", dts->query);
	SockWrite(sck, "Accept: */*");
	SockWrite(sck, "Host: %s", sck->host);
	SockWrite(sck, "Content-type: application/x-www-form-urlencoded");
	SockWrite(sck, "Content-length: %u", strlen(dts->post));
	SockWrite(sck, "Connection: close");
	SockWrite(sck, "");
	SockWrite(sck, dts->post);
	return 0;
}
SOCKFUNC(SmsCierra)
{
	if (!data) /* remoto */
		BorraCola(sck);
	return 0;
}
SOCKFUNC(SmsLee)
{
	DataSock *dts;
	if (!(dts = BuscaCola(sck)))
		return 1;
	if (*data == '#')
	{
		data++;
		if (IsDigit(*data))
		{
			switch(atoi(data))
			{
				case 100:
				case 101:
					Responde(dts->cl, CLI(smsserv), SS_ERR_EMPT, "Falla configuración servidor SMS. Consulte con la administración (err: 1)");
					break;
				case 1:
					Responde(dts->cl, CLI(smsserv), SS_ERR_EMPT, "Faltan datos");
					break;
				case 2:
					Responde(dts->cl, CLI(smsserv), SS_ERR_EMPT, "Esta red no permite enviar SMS");
					break;
				case 3:
					Responde(dts->cl, CLI(smsserv), SS_ERR_EMPT, "Esta red se ha quedado sin créditos. Consulte con la administración (err: 3)");
					break;
				case 0:
					Responde(dts->cl, CLI(smsserv), "Su mensaje se ha entregado con éxito");
					break;
				default:
					ircsprintf(buf, "Error desconocido. Consulte con la administración (err: %i)", atoi(data));
					Responde(dts->cl, CLI(smsserv), SS_ERR_EMPT, buf);
			}
		}
		else if (!strncmp("Creds:", data, 6))
			Responde(dts->cl, CLI(smsserv), "Esta red dispone de \00312%s\003 créditos restantes.", data+6);
	}
	else if (!strncmp(data, "Location:", 9))
	{
		char *c;
		SockClose(sck);
		if ((c = strchr(data+17, '/')))
		{
			*c = '\0';
			ircstrdup(dts->query, c+1);
		}
		dts->sck = SockOpen(data+17, 80, SmsAbre, SmsLee, NULL, SmsCierra);
	}
	return 0;
}
BOTFUNC(SSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(smsserv), "\00312%s\003 permite enviar mensajes SMS a móviles.", smsserv->hmod->nick);
		Responde(cl, CLI(smsserv), "También podrán enviarte mensajes sin que tú tengas que dar tu número de teléfono.");
		Responde(cl, CLI(smsserv), "Tendrás a tu disposición una lista de accesos para que sólo los que tú decidas puedan escribirte o bien para "
								"que ciertas personas no te molesten.");
		Responde(cl, CLI(smsserv), " ");
		Responde(cl, CLI(smsserv), "Servicios prestados:");
		ListaDescrips(smsserv->hmod, cl);
		Responde(cl, CLI(smsserv), " ");
		Responde(cl, CLI(smsserv), "Para más información, \00312/msg %s %s comando", smsserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], smsserv->hmod, param, params))
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, "Opción desconocida.");
	return 0;
}
BOTFUNCHELP(SSHSend)
{
	Responde(cl, CLI(smsserv), "Te permite enviar un SMS a un móvil o usuario.");
	Responde(cl, CLI(smsserv), "Si envías un mensaje a un usuario, éste tendrá que tener su número en esta base de datos y tú deberás figurar en "
							"su lista de acceso.");
	if (smsserv->restringido && !IsOper(cl))
		Responde(cl, CLI(smsserv), "\002NOTA:\002 Esta red no permite el envío de mensajes directamente a números de teléfono, sólo a usuarios de la red.");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312SEND nº|nick mensaje");
	return 0;
}
BOTFUNCHELP(SSHOpts)
{
	Responde(cl, CLI(smsserv), "Fija distintas opciones de configuración.");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), "\00312NUMERO\003 Especifica tu número de móvil para que puedan enviarte SMS sin que tú tengas que dar tu número.");
	Responde(cl, CLI(smsserv), "\00312LISTA\003 Fija la lista como excluyente o incluyente. Si está en IN sólo los nicks que figuren en tu lista podrán "
							"enviarte mensajes. Si está en OUT, los nicks que figuren en tu lista no podrán escribirte.");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312SET numero|lista [valor]");
	return 0;
}
BOTFUNCHELP(SSHLista)
{
	Responde(cl, CLI(smsserv), "Gestiona tu lista de acceso.");
	Responde(cl, CLI(smsserv), "Esta lista sólo puede contener nicks registrados en la red.");
	Responde(cl, CLI(smsserv), "Si tienes el parámetro SET LISTA en IN, sólo los usuarios que figuren en esta lista podrán enviarte SMS.");
	Responde(cl, CLI(smsserv), "Si lo tienes en OUT, los usuarios que tengas en esta lista no podrán enviarte SMS.");
	Responde(cl, CLI(smsserv), "Para listar los usuarios que tienes ingresados, ejecuta este comando sin parámetros.");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312LISTA");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312LISTA +nick");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312LISTA -nick");
	return 0;
}
BOTFUNCHELP(SSHSaldo)
{
	Responde(cl, CLI(smsserv), "Te muestra los créditos restantes que le quedan a tu red.");
	Responde(cl, CLI(smsserv), " ");
	Responde(cl, CLI(smsserv), "Sintaxis: \00312SALDO");
	return 0;
}
BOTFUNC(SSSend)
{
	u_int max, num;
	char *texto, *usermask = strchr(cl->mask, '!') + 1;
	DataSock *dts;
	if (params < 3)
	{
		Responde(cl, CLI(smsserv), SS_ERR_PARA, fc->com, "nº|nick mensaje");
		return 1;
	}
	if (CogeCache(CACHE_ULTIMO_SMS, usermask, smsserv->hmod->id))
	{
		char buf[512];
		ircsprintf(buf, "No puedes enviar otro sms hasta que no pasen %i minutos.", smsserv->espera / 60);
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, buf);
		return 1;
	}
	if (IsDigit(*param[1]))
	{
		if (smsserv->restringido && !IsOper(cl))
		{
			Responde(cl, CLI(smsserv), SS_ERR_EMPT, "Sólo puedes mandar sms a usuarios de esta red");
			return 1;
		}
		if (*param[1] != '6' || strlen(param[1]) != 9 || !(num = atoi(param[1])))
		{
			Responde(cl, CLI(smsserv), SS_ERR_SNTX, "El número de móvil parece incorrecto.");
			return 1;
		}
	}
	else
	{
		char *snum, *val;
		if (!(snum = SQLCogeRegistro(SS_SQL, param[1], "numero")))
		{
			Responde(cl, CLI(smsserv), SS_ERR_EMPT, "Este usuario no tiene asociado ningún móvil.");
			return 1;
		}
		ircsprintf(buf, " %s ", strtolower(param[1]));
		if (!IsOper(cl) && (!(val = SQLCogeRegistro(SS_SQL, param[1], "lista")) || (*val == '1' && !strstr(val, buf)) || (*val == '0' && strstr(val, buf))))
		{
			Responde(cl, CLI(smsserv), SS_ERR_EMPT, "Este usuario no acepta sms suyos");
			return 1;
		}
		num = atoi(Desencripta(snum, MOD_INFO(SmsServ).nombre));
	}
	texto = str_replace(Unifica(param, params, 2, -1), ' ', '+');
	max = 160 - (smsserv->publi ? strlen(smsserv->publi)+1 : 0);
	if (strlen(texto) > max)
	{
		ircsprintf(buf, "El mensaje debe tener un máximo de %i caracteres", max);
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, buf);
		return 1;
	}
	if (max < 160)
	{
		strlcat(texto, "+", sizeof(texto));
		strlcat(texto, smsserv->publi, sizeof(texto));
	}
	ircsprintf(buf, "userid=%s&destinos=%u&mensaje=%s", smsserv->id, num, texto);
	if ((dts = InsertaCola("", 0, cl)))
	{
		ircstrdup(dts->post, buf);
		dts->sck = SockOpen("sms.redyc.com", 80, SmsAbre, SmsLee, NULL, SmsCierra);
	}
	else
	{
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, "No se puede enviar el sms. Pruebe más tarde.");
		return 1;
	}
	Responde(cl, CLI(smsserv), "Tu mensaje ha sido puesto en cola.");
	if (!IsOper(cl))
		InsertaCache(CACHE_ULTIMO_SMS, usermask, 300, smsserv->hmod->id, "%lu", time(0));
	return 0;
}
BOTFUNC(SSOpts)
{
	if (params < 2)
	{
		Responde(cl, CLI(smsserv), SS_ERR_PARA, fc->com, "opcion [valor]");
		return 1;
	}
	if (!strcasecmp(param[1], "numero"))
	{
		if (params < 3)
		{
			SQLBorra(SS_SQL, cl->nombre);
			Responde(cl, CLI(smsserv), "Tu numero ha sido borrado.");
		}
		else
		{
			int num;
			if (*param[2] != '6' || strlen(param[2]) != 9 || !(num = atoi(param[2])))
			{
				Responde(cl, CLI(smsserv), SS_ERR_SNTX, "El número de móvil parece incorrecto.");
				return 1;
			}
			SQLInserta(SS_SQL, cl->nombre, "numero", Encripta(param[2], MOD_INFO(SmsServ).nombre));
			Responde(cl, CLI(smsserv), "Tu número de móvil ha sido actualizado");
		}
	}
	else if (!strcasecmp(param[1], "lista"))
	{
		char *val;
		if (params < 3 || (strcasecmp(param[2], "IN") && strcasecmp(param[2], "OUT")))
		{
			Responde(cl, CLI(smsserv), SS_ERR_SNTX, "Opción desconocida. Sólo: IN|OUT");
			return 1;
		}
		val = SQLCogeRegistro(SS_SQL, cl->nombre, "lista");
		if (!strcasecmp(param[2], "IN"))
		{
			if (val)
			{
				*val = '1';
				SQLInserta(SS_SQL, cl->nombre, "lista", val);
			}
			else
				SQLInserta(SS_SQL, cl->nombre, "lista", "1 ");
		}
		else
		{
			if (val)
			{
				*val = '0';
				SQLInserta(SS_SQL, cl->nombre, "lista", val);
			}
			else
				SQLInserta(SS_SQL, cl->nombre, "lista", "0 ");
		}
	}
	else
	{
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, "Opción incorrecta");
		return 1;
	}
	return 0;
}
BOTFUNC(SSLista)
{
	char *val;
	if (params < 2)
	{
		char *c;
		if (!(val = SQLCogeRegistro(SS_SQL, cl->nombre, "lista")) || strlen(val) < 3)
		{
			Responde(cl, CLI(smsserv), SS_ERR_EMPT, "No tienes ningún contacto");
			return 1;
		}
		Responde(cl, CLI(smsserv), "Lista de contactos");
		strtok(val, " ");
		while ((c = strtok(NULL, " ")))
			Responde(cl, CLI(smsserv), "\00312%s", c);
	}
	else
	{
		if (*param[1] == '+')
		{
			char tmp[BUFSIZE];
			param[1]++;
			if ((val = SQLCogeRegistro(SS_SQL, cl->nombre, "lista")))
			{
				ircsprintf(buf, " %s ", strtolower(param[1]));
				if (!strstr(val, buf))
				{
					strlcpy(tmp, val, sizeof(tmp));
					strlcat(tmp, &buf[1], sizeof(tmp));
					val = tmp;
				}
			}
			else
				ircsprintf(buf, "1 %s ", strtolower(param[1]));
			SQLInserta(SS_SQL, cl->nombre, "lista", val ? val : buf);
			Responde(cl, CLI(smsserv), "Contacto añadido");
		}
		else if (*param[1] == '-')
		{
			param[1]++;
			buf[0] = '\0';
			if ((val = SQLCogeRegistro(SS_SQL, cl->nombre, "lista")))
			{
				char *c;
				for (c = strtok(val, " "); c; c = strtok(NULL, " "))
				{
					if (strcasecmp(c, param[1]))
					{
						strlcat(buf, c, sizeof(buf));
						strlcat(buf, " ", sizeof(buf));
					}
				}
			}
			SQLInserta(SS_SQL, cl->nombre, "lista", buf[0] ? buf : "1 ");
			Responde(cl, CLI(smsserv), "Contacto eliminado");
		}
		else
		{
			Responde(cl, CLI(smsserv), SS_ERR_SNTX, "LISTA {+|-}nick");
			return 1;
		}
	}
	return 0;
}
BOTFUNC(SSSaldo)
{
	DataSock *dts;
	ircsprintf(buf, "userid=%s&cmd=_creditos", smsserv->id);
	if ((dts = InsertaCola("", 0, cl)))
	{
		ircstrdup(dts->post, buf);
		dts->sck = SockOpen("sms.redyc.com", 80, SmsAbre, SmsLee, NULL, SmsCierra);
	}
	else
	{
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, "No se puede conectar con el servidor. Pruebe más tarde.");
		return 1;
	}
	Responde(cl, CLI(smsserv), "Consultando petición. Aguarde...");
	return 0;
}
int SSSigSQL()
{
	SQLNuevaTabla(SS_SQL, "CREATE TABLE IF NOT EXISTS %s%s ( "
  		"item varchar(255) default NULL, "
  		"numero varchar(16) default NULL, "
  		"lista text, "
  		"KEY item (item) "
		");", PREFIJO, SS_SQL);
	return 0;
}
int SSSigQuit(Cliente *cl, char *mensaje)
{
	int i;
	for (i = 0; i < MAX_COLA; i++)
	{
		if (cola[i] && cola[i]->cl == cl)
		{
			SockClose(cola[i]->sck);
			break;
		}
	}
	return 0;
}
