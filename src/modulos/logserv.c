/*
 * $Id: logserv.c,v 1.10 2008-01-21 19:46:45 Trocotronic Exp $ 
 */

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <sys/io.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"
#include "md5.h"
#include "modulos/logserv.h"
#include "modulos/chanserv.h"
#include "modulos/nickserv.h"

LogServ *logserv = NULL;
LogChan *logueos = NULL;
Timer *timercaduca = NULL, *timeranuncia = NULL;
#define ExFunc(x) TieneNivel(cl, x, logserv->hmod, NULL)

BOTFUNC(LSHelp);
BOTFUNC(LSAlta);
BOTFUNCHELP(LSHAlta);
BOTFUNC(LSBaja);
BOTFUNCHELP(LSHBaja);
BOTFUNC(LSEnviar);
BOTFUNCHELP(LSHEnviar);
BOTFUNC(LSOpts);
BOTFUNCHELP(LSHOpts);

int LSSigSQL	();
int LSSigEOS	();
int LSSigCDestroy(Canal *);
int LSSigCDrop(char *);
int LSSigSynch	();
int LSSigSockClose	();
int LSCmdJoin(Cliente *, Canal *);
int LSCmdCMsg(Cliente *, Canal *, char *);
int MiraCaducados();
int AnunciaLogueo();

static bCom logserv_coms[] = {
	{ "help" , LSHelp , N0 , "Muestra esta ayuda." , NULL } ,
	{ "alta" , LSAlta , N1 , "Inicia el logueo de un canal." , LSHAlta } ,
	{ "baja" , LSBaja , N1 , "Detiene el logueo de un canal." , LSHBaja } ,
	{ "enviar" , LSEnviar , N4 , "Envía todos los logs a sus respectivos emails." , LSHEnviar } ,
	{ "set" , LSOpts , N1 , "Fija las distintas opciones del logueo para un canal." , LSHOpts } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};

void LSSet(Conf *, Modulo *);
int LSTest(Conf *, int *);

ModInfo MOD_INFO(LogServ) = {
	"LogServ" ,
	0.1 ,
	"Trocotronic" ,
	"trocotronic@redyc.com"
};

int MOD_CARGA(LogServ)(Modulo *mod)
{
	Conf modulo;
	int errores = 0;
	/*if (mainversion != COLOSSUS_VERINT)
	{
		Error("[%s] El módulo ha sido compilado para la versión %i y usas la versión %i", mod->archivo, COLOSSUS_VERINT, mainversion);
		return 1;
	}*/
	//mod->activo = 1;
	if (mod->config)
	{
		if (ParseaConfiguracion(mod->config, &modulo, 1))
		{
			Error("[%s] Hay errores en la configuración de %s", mod->archivo, MOD_INFO(LogServ).nombre);
			errores++;
		}
		else
		{
			if (!strcasecmp(modulo.seccion[0]->item, MOD_INFO(LogServ).nombre))
			{
				if (!LSTest(modulo.seccion[0], &errores))
					LSSet(modulo.seccion[0], mod);
				else
				{
					Error("[%s] La configuración de %s no ha pasado el test", mod->archivo, MOD_INFO(LogServ).nombre);
					errores++;
				}
			}
			else
			{
				Error("[%s] La configuracion de %s es erronea", mod->archivo, MOD_INFO(LogServ).nombre);
				errores++;
			}
		}
		LiberaMemoriaConfiguracion(&modulo);
	}
	else
		LSSet(NULL, mod);
	timercaduca = IniciaCrono(0, 86400, MiraCaducados, NULL);
	return errores;
}
int MOD_DESCARGA(LogServ)()
{
	LogChan *aux, *sig;
	for (aux = logueos; aux; aux = sig)
	{
		sig = aux->sig;
		if (aux->fd != -1)
			close(aux->fd);
		Free(aux);
	}
	BorraSenyal(SIGN_SQL, LSSigSQL);
	BorraSenyal(SIGN_JOIN, LSCmdJoin);
	BorraSenyal(SIGN_CDESTROY, LSSigCDestroy);
	BorraSenyal(SIGN_CMSG, LSCmdCMsg);
	BorraSenyal(SIGN_EOS, LSSigEOS);
	BorraSenyal(CS_SIGN_DROP, LSSigCDrop);
	BorraSenyal(SIGN_SOCKCLOSE, LSSigSockClose);
	ApagaCrono(timercaduca);
	ApagaCrono(timeranuncia);
	BotUnset(logserv);
	return 0;
}
int LSTest(Conf *config, int *errores)
{
	int error_parcial = 0;
	Conf *eval;
	if ((eval = BuscaEntrada(config, "funcion")))
		error_parcial += TestComMod(eval, logserv_coms, 1);
	*errores += error_parcial;
	return error_parcial;
}
void LSSet(Conf *config, Modulo *mod)
{
	int i, p;
	if (!logserv)
		logserv = BMalloc(LogServ);
	if (config)
	{
		for (i = 0; i < config->secciones; i++)
		{
			if (!strcmp(config->seccion[i]->item, "funciones"))
				ProcesaComsMod(config->seccion[i], mod, logserv_coms);
			else if (!strcmp(config->seccion[i]->item, "funcion"))
				SetComMod(config->seccion[i], mod, logserv_coms);
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
		ProcesaComsMod(NULL, mod, logserv_coms);
	InsertaSenyal(SIGN_SQL, LSSigSQL);
	InsertaSenyal(SIGN_JOIN, LSCmdJoin);
	InsertaSenyal(SIGN_CDESTROY, LSSigCDestroy);
	InsertaSenyal(SIGN_CMSG, LSCmdCMsg);
	InsertaSenyal(CS_SIGN_DROP, LSSigCDrop);
	InsertaSenyal(SIGN_EOS, LSSigEOS);
	InsertaSenyal(SIGN_SOCKCLOSE, LSSigSockClose);
	BotSet(logserv);
	mkdir(DIR_LOGS, 0600);
}
int CierraEmail(SmtpData *smtp)
{
	Opts *ofl;
	if (smtp)
	{
		for (ofl = smtp->fps; ofl && ofl->item; ofl++)
		{
			if (ofl->opt != -1)
			{
				LogChan *lc;
				ircsprintf(buf, "%s%s", DIR_LOGS, ofl->item);
				close(ofl->opt);
				unlink(buf);
				for (lc = logueos; lc; lc = lc->sig)
				{
					if (lc->fd == ofl->opt)
					{
						lc->fd = -1;
						break;
					}
				}
			}
		}
	}
	return 0;
}
void PreparaLogueo(char *ruta, int fd)
{
	Opts *fps;
	char canal[64], *email, *c;
	if (fd == -1)
		fd = open(ruta, O_RDONLY | O_BINARY, S_IREAD);
	if ((c = strrchr(ruta, '/')))
		c++;
	else
		c = ruta;
	strlcpy(canal, c, sizeof(canal));
	canal[strlen(canal)-10] = '\0';
	if (lseek(fd, 0, SEEK_SET) == lseek(fd, 0, SEEK_END) || !(email = SQLCogeRegistro(LS_SQL, canal, "email")))
	{
		close(fd);
		unlink(ruta);
	}
	else
	{
		struct stat inode;
		char *t, *md5 = NULL;
		lseek(fd, 0, SEEK_SET);
		if (fstat(fd, &inode) != -1)
		{
			u_long r;
			t = (char *)Malloc(inode.st_size+1);
			t[inode.st_size] = '\0';
			if ((r = read(fd, t, inode.st_size)) == inode.st_size)
				md5 = MDString(t, inode.st_size);
		}
		fps = (Opts *)Malloc(sizeof(Opts)*2);
		fps[0].opt = fd;
		fps[0].item = strdup(c);
		fps[1].opt = 0x0;
		fps[1].item = 0x0;
		ircsprintf(buf, "Log del canal %s", canal);
		EmailArchivos(email, buf, fps, CierraEmail, "Signatura MD5 del archivo: %s", md5 ? md5 : "(no se ha podido calcular)");
	}
}
void EnviaLogueo(LogChan *lc)
{
	ircsprintf(buf, "%s%s%s.log", DIR_LOGS, lc->cn->nombre, lc->envio);
	PreparaLogueo(buf, lc->fd);
}
LogChan *BuscaLogueo(Canal *cn)
{
	LogChan *aux;
	for (aux = logueos; aux; aux = aux->sig)
	{
		if (aux->cn == cn)
			return aux;
	}
	return NULL;
}
LogChan *IniciaLogueo(Canal *cn)
{
	LogChan *aux;
	time_t hora = time(0);
	if ((aux = BuscaLogueo(cn)))
		return aux;
	aux = BMalloc(LogChan);
	aux->cn = cn;
	strftime(aux->envio, sizeof(aux->envio), "%d%m%y", gmtime(&hora));
	ircsprintf(buf, "%s%s%s.log", DIR_LOGS, cn->nombre, aux->envio);
	if ((aux->fd = open(buf, O_RDWR | O_CREAT | O_BINARY | O_APPEND, S_IREAD | S_IWRITE)) == -1)
	{
		Free(aux);
		return NULL;
	}
	EntraBot(CLI(logserv), cn->nombre);
	ProtFunc(P_PRIVADO)((Canal *)cn, CLI(logserv), "A partir de ahora, se grabarán las conversaciones de este canal, a petición de su fundador.");
	AddItem(aux, logueos);
	return aux;
}
int LiberaLogueo(LogChan *lc, int envia)
{
	if (!lc)
		return 0;
	if (lc->fd != -1)
	{
		if (envia)
			EnviaLogueo(lc);
		else
			close(lc->fd);
	}
	if (EsLink(lc->cn->miembro, CLI(logserv)) && !ModuloEsResidente(logserv->hmod, lc->cn->nombre))
		SacaBot(CLI(logserv), lc->cn->nombre, "Logueo desactivado");
	LiberaItem(lc, logueos);
	return 1;
}
int DetieneLogueo(Canal *cn)
{
	LogChan *aux;
	if ((aux = BuscaLogueo(cn)))
		return LiberaLogueo(aux, 1);
	return 0;
}
BOTFUNCHELP(LSHAlta)
{
	Responde(cl, CLI(logserv), "Inicia la grabación de las conversaciones en un canal.");
	Responde(cl, CLI(logserv), "Este comando sólo puede ejecutarlo el fundador de un canal.");
	Responde(cl, CLI(logserv), "Ni la administración ni los operadores de la red pueden usar este comando.");
	Responde(cl, CLI(logserv), " ");
	Responde(cl, CLI(logserv), "Sintaxis: \00312ALTA #canal");
	return 0;
}
BOTFUNCHELP(LSHBaja)
{
	Responde(cl, CLI(logserv), "Detiene la grabación en un canal solicitado con el comando \00312ALTA\003.");
	Responde(cl, CLI(logserv), "Este comando sólo puede ejecutarlo el fundador de ese canal.");
	Responde(cl, CLI(logserv), "Ni la administración ni los operadores de la red pueden usar este comando.");
	Responde(cl, CLI(logserv), " ");
	Responde(cl, CLI(logserv), "Sintaxis: \00312BAJA #canal");
	return 0;
}
BOTFUNCHELP(LSHEnviar)
{
	Responde(cl, CLI(logserv), "Envía inmediatamente los logs a sus respectivos emails y los borra del disco.");
	Responde(cl, CLI(logserv), "Este comando sólo debe utilizarse en circunstancias muy críticas.");
	Responde(cl, CLI(logserv), "No lo use si desconoce su uso.");
	Responde(cl, CLI(logserv), " ");
	Responde(cl, CLI(logserv), "Sintaxis: \003ENVIAR");
	return 0;
}
BOTFUNCHELP(LSHOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(logserv), "Fija las distintas opciones de logueo para su canal.");
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Opciones disponibles:");
		Responde(cl, CLI(logserv), "\00312EMAIL\003 Fija el email al que serán enviados los logs.");
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Para poder utilizar este comando debe ser fundador de dicho canal.");
		Responde(cl, CLI(logserv), "Sintaxis: \00312 SET #canal opcion [valor]");
	}
	else if (!strcasecmp(param[2], "EMAIL"))
	{
		Responde(cl, CLI(logserv), "\00312SET EMAIL");
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Cambia el email al que se envían los logs una vez al día.");
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Sintaxis: \00312SET #canal EMAIL nuevo@email");
	}
	else
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Opción desconocida.");
	return 0;
}		
BOTFUNC(LSHelp)
{
	if (params < 2)
	{
		Responde(cl, CLI(logserv), "\00312%s\003 se encarga  de loguear las conversaciones de los canales.", logserv->hmod->nick);
		Responde(cl, CLI(logserv), "Estos logs se envían periódicamente por mail.");
		Responde(cl, CLI(logserv), "Es una gran ventaja puesto que ya no es necesario disponer de bots ajenos a la red que estén las 24 horas del día online.");
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Comandos disponibles:");
		ListaDescrips(logserv->hmod, cl);
		Responde(cl, CLI(logserv), " ");
		Responde(cl, CLI(logserv), "Para más información, \00312/msg %s %s comando", logserv->hmod->nick, strtoupper(param[0]));
	}
	else if (!MuestraAyudaComando(cl, param[1], logserv->hmod, param, params))
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Opción desconocida.");
	EOI(logserv, 0);
	return 0;
}
BOTFUNC(LSAlta)
{
	if (params < 2)
	{
		Responde(cl, CLI(logserv), LS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Sólo puede dar de alta un canal el propio fundador");
		return 1;
	}
	if (SQLCogeRegistro(LS_SQL, param[1], "email"))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Este canal ya se está logueando");
		return 1;
	}
	SQLInserta(LS_SQL, param[1], "email", SQLCogeRegistro(NS_SQL, cl->nombre, "email"));
	SQLInserta(LS_SQL, param[1], "solicitud", "%lu", time(0));
	SQLInserta(LS_SQL, param[1], "solicitante", cl->nombre);
	IniciaLogueo(BuscaCanal(param[1]));
	Responde(cl, CLI(logserv), "El canal ha sido dado de alta. Empieza la grabación.");
	EOI(logserv, 1);
	return 0;
}
BOTFUNC(LSBaja)
{
	if (params < 2)
	{
		Responde(cl, CLI(logserv), LS_ERR_PARA, fc->com, "#canal");
		return 1;
	}
	if (!CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Sólo puede dar de baja un canal el propio fundador");
		return 1;
	}
	if (!SQLCogeRegistro(LS_SQL, param[1], "email"))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Este canal no se está logueando");
		return 1;
	}
	DetieneLogueo(BuscaCanal(param[1]));
	SQLBorra(LS_SQL, param[1]);
	Responde(cl, CLI(logserv), "El canal ha sido dado de baja.");
	EOI(logserv, 2);
	return 0;
}
BOTFUNC(LSEnviar)
{
	LogChan *lc;
	for (lc = logueos; lc; lc = lc->sig)
		EnviaLogueo(lc);
	Responde(cl, CLI(logserv), "Enviando todos los logs a sus respectivos emails.");
	EOI(logserv, 3);
	return 0;
}
BOTFUNC(LSOpts)
{
	if (params < 3)
	{
		Responde(cl, CLI(logserv), LS_ERR_PARA, fc->com, "#canal parámetros");
		return 1;
	}
	if (!CSEsFundador(cl, param[1]))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Sólo puede dar de alta un canal el propio fundador");
		return 1;
	}
	if (!SQLCogeRegistro(LS_SQL, param[1], "email"))
	{
		Responde(cl, CLI(logserv), LS_ERR_EMPT, "Este canal no se está logueando");
		return 1;
	}
	SQLInserta(LS_SQL, param[1], "email", param[1]);
	EOI(logserv, 4);
	return 0;
}
int LSSigSQL()
{
	if (!SQLEsTabla(LS_SQL))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"email varchar(255) default NULL, "
  			"solicitud int4, "
  			"solicitante varchar(255) default NULL, "
  			"KEY item (item) "
			");", PREFIJO, LS_SQL);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, LS_SQL);
	}
	return 0;
}
int LSCmdJoin(Cliente *cl, Canal *cn)
{
	if (SQLCogeRegistro(LS_SQL, cn->nombre, "email"))
	{
		if (cn->miembros == 1)
			IniciaLogueo(cn);
		ProtFunc(P_PRIVADO)(cl, CLI(logserv), "Se están grabando las conversaciones del canal %s.", cn->nombre);
	}
	return 0;
}
int LSSigCDestroy(Canal *cn)
{
	LogChan *lc;
	if ((lc = BuscaLogueo(cn)))
		return LiberaLogueo(lc, 0);
	return 0;
}
int LSCmdCMsg(Cliente *cl, Canal *cn, char *msg)
{
	LogChan *lc;
	if (!(lc = BuscaLogueo(cn)))
	{
		if (SQLCogeRegistro(LS_SQL, cn->nombre, "email"))
			lc = IniciaLogueo(cn);
		else
			return 1;
	}
	if (lc)
	{
		char tmp[BUFSIZE+16], tmptime[8];
		time_t hora = time(0);
		strftime(tmptime, sizeof(tmptime), "%d%m%y", gmtime(&hora));
		ircsprintf(buf, "%s%s%s.log", DIR_LOGS, cn->nombre, tmptime);
		if (strcmp(tmptime, lc->envio))
		{
			EnviaLogueo(lc);
			strlcpy(lc->envio, tmptime, sizeof(lc->envio));
			lc->fd = open(buf, O_RDWR | O_CREAT | O_BINARY | O_APPEND, S_IREAD | S_IWRITE);
		}
		ircsprintf(tmp, "%lu %s %s\r\n", hora, cl->nombre, msg);
		if (lc->fd == -1)
			lc->fd = open(buf, O_RDWR | O_CREAT | O_BINARY | O_APPEND, S_IREAD | S_IWRITE);
		lseek(lc->fd, 0, SEEK_END);
		write(lc->fd, tmp, strlen(tmp));
#ifdef _WIN32
		_commit(lc->fd);
#else
		fsync(lc->fd);
#endif
	}
	return 0;
}
int MiraCaducados()
{
	Directorio dir;
	if ((dir = AbreDirectorio(DIR_LOGS)))
	{
		char *nombre, tmptime[8];
		time_t hora = time(0);
		strftime(tmptime, sizeof(tmptime), "%d%m%y", gmtime(&hora));
		while ((nombre = LeeDirectorio(dir)))
		{
			if (!strstr(nombre, tmptime))
			{
				ircsprintf(buf, "%s%s", DIR_LOGS, nombre);
				PreparaLogueo(buf, -1);
			}
		}
		CierraDirectorio(dir);
	}
	return 0;
}
int AnunciaLogueo()
{
	LogChan *lc;
	for (lc = logueos; lc; lc = lc->sig)
		ProtFunc(P_PRIVADO)((Cliente *)lc->cn, CLI(logserv), "Se están grabando las conversaciones de este canal.");
	return 0;
}
int LSSigCDrop(char *canal)
{
	SQLBorra(LS_SQL, canal);
	return 0;
}
int LSSigEOS()
{
	SQLRes res;
	SQLRow row;
	MiraCaducados();
	if ((res = SQLQuery("SELECT * FROM %s%s", PREFIJO, LS_SQL)))
	{
		while ((row = SQLFetchRow(res)))
			IniciaLogueo(BuscaCanal(row[0]));
		SQLFreeRes(res);
	}
	if (!timeranuncia)
		timeranuncia = IniciaCrono(0, 7200, AnunciaLogueo, NULL);
	return 0;
}
int LSSigSockClose()
{
	ApagaCrono(timeranuncia);
	timeranuncia = NULL;
	return 0;
}
