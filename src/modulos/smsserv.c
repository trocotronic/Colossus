/*
 * $Id: smsserv.c,v 1.2 2006-03-05 18:44:28 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/smsserv.h"

SmsServ *smsserv = NULL;
#define ExFunc(x) TieneNivel(cl, x, smsserv->hmod, NULL)

SOCKFUNC(SmsAbre);
SOCKFUNC(SmsLee);
BOTFUNC(SSSend);
BOTFUNC(SSOpts);
BOTFUNC(SSLista);

int SSSigSQL	();

static bCom smsserv_coms[] = {
	//{ "help" , TSHelp , N1 , "Muestra esta ayuda." , NULL } ,
	{ "send" , SSSend , N1 , "Envía un SMS." , NULL } ,
	{ "set" , SSOpts , N1 , "Fija distintas opciones." , NULL } ,
	{ "lista" , SSLista , N1 , "Muestra tu lista VIP." , NULL } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void SSSet(Conf *, Modulo *);
int SSTest(Conf *, int *);

Sms *cola[MAX_SMS];

ModInfo MOD_INFO(SmsServ) = {
	"SmsServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@rallados.net"
};

int MOD_CARGA(SmsServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
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
	bzero(cola, sizeof(Sms) * MAX_SMS);
	return errores;
}
int MOD_DESCARGA(SmsServ)()
{
	BorraSenyal(SIGN_SQL, SSSigSQL);
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
	if (!(eval = BuscaEntrada(config, "login")))
	{
		Error("[%s:%s] No se encuentra la directriz login.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (BadPtr(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz login esta vacia.", config->archivo, config->item, eval->item, eval->linea);
			error_parcial++;
		}
	}
	if (!(eval = BuscaEntrada(config, "pass")))
	{
		Error("[%s:%s] No se encuentra la directriz pass.", config->archivo, config->item);
		error_parcial++;
	}
	else
	{
		if (BadPtr(eval->data))
		{
			Error("[%s:%s::%s::%i] La directriz pass esta vacia.", config->archivo, config->item, eval->item, eval->linea);
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
		BMalloc(smsserv, SmsServ);
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
			else if (!strcmp(config->seccion[i]->item, "login"))
				ircstrdup(smsserv->login, config->seccion[i]->data);
			else if (!strcmp(config->seccion[i]->item, "pass"))
				ircstrdup(smsserv->pass, config->seccion[i]->data);
		}
	}
	else
		ProcesaComsMod(NULL, mod, smsserv_coms);
	InsertaSenyal(SIGN_SQL, SSSigSQL);
	BotSet(smsserv);
}
SOCKFUNC(SmsAbre)
{
	int i;
	for (i = 0; i < MAX_SMS; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
			break;
	}
	if (i == MAX_SMS) /* algo pasa */
		return 1;
	SockWrite(sck, OPT_CRLF, "GET /7.0/sms.php?login=%s&pass=%s&destino=%u&mensaje=%s HTTP/1.0", smsserv->login, smsserv->pass, cola[i]->numero, cola[i]->texto);
	SockWrite(sck, OPT_CRLF, "Accept: */*");
	SockWrite(sck, OPT_CRLF, "Host: intranet.rallados.net");
	SockWrite(sck, OPT_CRLF, "");
	return 0;
}
SOCKFUNC(SmsLee)
{
	int i;
	for (i = 0; i < MAX_SMS; i++)
	{
		if (cola[i] && cola[i]->sck == sck)
			break;
	}
	if (i == MAX_SMS) /* algo pasa */
		return 1;
	if (*data == '#')
	{
		switch(atoi(data+1))
		{
			case -100:
			case -101:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Falla configuración servidor SMS. Consulte con la administración (err: 1)");
				break;
			case -1:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Faltan datos");
				break;
			case -2:
			case -3:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Esta red no permite enviar SMS");
				break;
			case -4:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Esta red se ha quedado sin créditos. Consulte con la administración (err: 4)");
				break;
			case -5:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Falla autentificación con el servidor SMS. Consulte con la administración (err: 5)");
				break;
			case -6:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Fallan créditos con el servidor SMS. Consulte con la administración (err: 6)");
				break;
			case -7:
				Responde(cola[i]->cl, CLI(smsserv), SS_ERR_EMPT, "Falla la llamada. Consulte con la administración (err: 7)");
				break;
			default:
				Responde(cola[i]->cl, CLI(smsserv), "Su mensaje se ha entregado con éxito");
		}
	}
	return 0;
}
Sms *CreaSms(int numero, char *texto, Cliente *cl)
{
	Sms *aux;
	int i;
	for (i = 0; i < MAX_SMS && cola[i]; i++);
	if (i == MAX_SMS)
		return NULL;
	BMalloc(aux, Sms);
	aux->numero = numero;
	aux->texto = strdup(texto);
	aux->cl = cl;
	aux->sck = SockOpen("intranet.rallados.net", 80, SmsAbre, SmsLee, NULL, NULL, ADD);
	cola[i] = aux;
	return aux;
}
BOTFUNC(SSSend)
{
	int num;
	u_int max;
	char *texto, *usermask = strchr(cl->mask, '!') + 1;
	if (params < 3)
	{
		Responde(cl, CLI(smsserv), SS_ERR_PARA, "SEND nº|nick mensaje");
		return 1;
	}
	if (CogeCache(CACHE_ULTIMO_SMS, usermask, smsserv->hmod->id))
	{
		char buf[512];
		ircsprintf(buf, "No puedes enviar otro sms hasta que no pasen %i minutos.", smsserv->espera / 60);
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, buf);
		return 1;
	}
	if (isdigit(*param[1]))
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
		strcat(texto, "+");
		strcat(texto, smsserv->publi);
	}
	if (!CreaSms(num, texto, cl))
	{
		Responde(cl, CLI(smsserv), SS_ERR_EMPT, "No se puede enviar el sms. Pruebe más tarde.");
		return 1;
	}
	Responde(cl, CLI(smsserv), "Tu mensaje ha sido puesto en cola");
	if (!IsOper(cl))
		InsertaCache(CACHE_ULTIMO_SMS, usermask, 300, smsserv->hmod->id, "%lu", time(0));
	return 0;
}
BOTFUNC(SSOpts)
{
	if (params < 2)
	{
		Responde(cl, CLI(smsserv), SS_ERR_PARA, "SET opcion [valor]");
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
			Responde(cl, CLI(smsserv), SS_ERR_SNTX, "SET lista IN|OUT");
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
			param[1]++;
			if ((val = SQLCogeRegistro(SS_SQL, cl->nombre, "lista")))
			{
				ircsprintf(buf, " %s ", strtolower(param[1]));
				if (!strstr(val, buf))
					strcat(val, &buf[1]);
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
						strcat(buf, c);
						strcat(buf, " ");
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
int SSSigSQL()
{
	if (!SQLEsTabla(SS_SQL))
	{
		if (SQLQuery("CREATE TABLE %s%s ( "
  			"item varchar(255) default NULL, "
  			"numero varchar(16) default NULL, "
  			"lista text NOT NULL "
			");", PREFIJO, SS_SQL))
				Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SS_SQL);
	}
	SQLCargaTablas();
	return 1;
}
