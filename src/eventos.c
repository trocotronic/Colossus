/*
 * $Id: eventos.c,v 1.7 2007-03-19 19:16:36 Trocotronic Exp $ 
 */

#ifndef _WIN32
#include <time.h>
#endif
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "protocolos.h"

Timer *timers = NULL;
Proc *procs = NULL;
extern Canal *canal_debug;

int ProcCache();

void InsertaSenyalEx(int senyal, int (*func)(), int donde)
{
	Senyal *sign, *aux;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
			return;
	}
	sign = BMalloc(Senyal);
	sign->senyal = senyal;
	sign->func = func;
	if (donde == FIN)
	{
		if (!senyals[senyal])
			senyals[senyal] = sign;
		else
		{
			for (aux = senyals[senyal]; aux; aux = aux->sig)
			{
				if (!aux->sig)
				{
					aux->sig = sign;
					break;
				}
			}
		}
	}
	else if (donde == INI)
	{
		sign->sig = senyals[senyal];
		senyals[senyal] = sign;
	}
}

/*!
 * @desc: Elimina una señal
 * @params: $senyal [in] Tipo de señal. Ver InsertaSenyal para más información.
 * 	    $func [in] Función que se desea eliminar.
 * @ret: Devuelve 1 en caso de que sea eliminado; 0, si no.
 * @ver: InsertaSenyal
 * @cat: Señales
 !*/

int BorraSenyal(int senyal, int (*func)())
{
	Senyal *aux, *prev = NULL;
	for (aux = senyals[senyal]; aux; aux = aux->sig)
	{
		if (aux->func == func)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				senyals[senyal] = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void LlamaSenyal(int s, int params, ...)
{
	va_list va;
	void *arg[8];
	int i;
	Senyal *aux;
	if (params > 8)
		return;
	va_start(va, params);
	for (i = 0; i < params; i++)
		arg[i] = va_arg(va, void *);
#ifdef DEBUG
	Debug("Ejecutando senyal %i", s);
#endif
	for (aux = senyals[s]; aux; aux = aux->sig)
	{
		if (aux->func)
		{
			switch(params)
			{
				case 0:
					aux->func();
					break;
				case 1:
					aux->func(arg[0]);
					break;
				case 2:
					aux->func(arg[0], arg[1]);
					break;
				case 3:
					aux->func(arg[0], arg[1], arg[2]);
					break;
				case 4:
					aux->func(arg[0], arg[1], arg[2], arg[3]);
					break;
				case 5:
					aux->func(arg[0], arg[1], arg[2], arg[3], arg[4]);
					break;
				case 6:
					aux->func(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5]);
					break;
				case 7:
					aux->func(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6]);
					break;
				case 8:
					aux->func(arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7]);
					break;
			}
		}
	}
	va_end(va);
}

/*!
 * @desc: Inicia un cronométro.
 * Estos cronómetros permiten ejecutar funciones varias veces en un intervalo de tiempo.
 * @params: $veces [in] Entero que indica el número de veces que se ejecutará la función. Si es 0, la función se ejecuta indefinidamente.
 	    $cada [in] Entero que indica los segundos entre veces que se ejecuta la función.
 	    $func [in] Puntero a la función a ejecutar. Debe ser de tipo int.
 	    $args [in] Puntero genérico que apunta a un tipo de datos que se le pasaran a la función cada vez que sea ejecutada.
 	    $sizearg [in] Tamaño del tipo de datos que se pasa a la función.
 * @ret: Devuelve un recurso tipo Timer que identifica a este cronómetro.
 * @ver: ApagaCrono
 * @cat: Cronometros
 !*/

Timer *IniciaCrono(u_int veces, u_int cada, int (*func)(), void *args)
{
	Timer *aux;
	aux = BMalloc(Timer);
	aux->cuando = microtime() + cada;
	aux->veces = veces;
	aux->cada = cada;
	aux->func = func;
	aux->args = args;
	AddItem(aux, timers);
	return aux;
}

/*!
 * @desc: Detiene un cronómetro
 * @params: $timer [in] Recurso del cronómetro devuelto por IniciaCrono 
 * @ret: Devuelve 1 si se detiene; 0, si no.
 * @ver: IniciaCrono
 * @cat: Cronometros
 !*/

int ApagaCrono(Timer *timer)
{
	Timer *aux, *prev = NULL;
	for (aux = timers; aux; aux = aux->sig)
	{
		if (aux == timer)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				timers = aux->sig;
			Free(aux);
			return 1;
		}
		prev = aux;
	}
	return 0;
}
void CompruebaCronos()
{
	Timer *aux, *sig;
	double ms;
	if (refrescando)
		return;
	ms = microtime();
	for (aux = timers; aux; aux = sig)
	{
		sig = aux->sig;
		if (aux->cuando < ms)
		{
			if (aux->args)
				aux->func(aux->args);
			else
				aux->func();
			aux->cuando = ms + aux->cada;
			if (aux->veces)
			{
				if (++aux->lleva == aux->veces)
					ApagaCrono(aux);
			}
		}
	}
}

void ProcesosAuxiliares()
{
	Proc *aux, *sig;
	if (refrescando)
		return;
	for (aux = procs; aux; aux = sig)
	{
		sig = aux->sig;
		aux->func(aux);
	}
}

/*!
 * @desc: Inicia un proceso.
 	Durante la ejecución del programa, puede ser necesario procesar una cantidad de datos suficiente como para no detener el programa.
 	Estas funciones son un principio de hilos o "threads", pero en su forma más primitiva.
 	Por eso tiene un contador que permite partir o dividir el proceso de datos en partes.
 * @params: $func [in] Función a ejecutar.
	Es importante que la propia función especifique próximo tiempo TS para volver a ser ejecutada.
	Cuando se llame a esta función se le pasará como primer parámetro el proceso que la llama.
 * @ex: ProcFunc(MiProc)
 {
 	if (proctime + 3600 < time(NULL)) //Lo hacemos cada hora
 	{
 		if (proc->proc == 50) //Se ha ejecutado 50 veces
 		{
 			proc->proc = 0;
 			proc->time = time(NULL) + 3600; //No se volverá a ejecutar hasta pasada 1 hora
 		}
 		else
 			proc->proc++;
 	}
 	return 0;
}
	...
	IniciaProceso(MiProc);
 * @ver: DetieneProceso
 * @cat: Procesos
 !*/

void IniciaProceso(int (*func)())
{
	Proc *proc;
	for (proc = procs; proc; proc = proc->sig)
	{
		if (proc->func == func)
			return;
	}
	proc = (Proc *)Malloc(sizeof(Proc));
	proc->func = func;
	proc->proc = 0;
	proc->time = (time_t) 0;
	AddItem(proc, procs);
}

/*!
 * @desc: Detiene un proceso.
 * @params: $func [in] Función a detener.
 * @ret: Devuelve 1 si se detiene con éxito; 0, si no.
 * @cat: Procesos
 !*/

int DetieneProceso(int (*func)())
{
	Proc *proc, *prev = NULL;
	for (proc = procs; proc; proc = proc->sig)
	{
		if (proc->func == func)
		{
			if (prev)
				prev->sig = proc->sig;
			else
				procs = proc->sig;
			Free(proc);
			return 1;
		}
		prev = proc;
	}
	return 0;
}
int CargaCache()
{
	if (!SQLEsTabla(SQL_CACHE))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
  			"item varchar(255) default NULL, "
  			"valor varchar(255) default NULL, "
  			"hora int4 default '0', "
  			"owner int4 default '0', "
  			"tipo text default NULL, "
  			"KEY item (item) "
			");", PREFIJO, SQL_CACHE);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, SQL_CACHE);
	}
	else
	{
		if (!SQLEsCampo(SQL_CACHE, "owner"))
			SQLQuery("ALTER TABLE %s%s ADD owner int4 NOT NULL default '0';", PREFIJO, SQL_CACHE);
		if (!SQLEsCampo(SQL_CACHE, "tipo"))
			SQLQuery("ALTER TABLE %s%s ADD tipo VARCHAR(255) default NULL;", PREFIJO, SQL_CACHE);
	}
	SQLCargaTablas();
	IniciaProceso(ProcCache);
	return 1;
}

/*!
 * @desc: Coge un valor de la Caché interna.
 * @params: $tipo [in] Tipo de datos.
 	    $item [in] Ítem a solicitar.
 	    $id [in] Dueño o propietario de esta Caché.
 * @ret: Devuelve el valor del ítem solicitado correspondiente a su dueño.
 * @ver: InsertaCache BorraCache
 * @cat: Cache
 !*/

char *CogeCache(char *tipo, char *item, int id)
{
	SQLRes *res;
	SQLRow row;
	char *tipo_c, *item_c;
	static char row0[BUFSIZE];
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	res = SQLQuery("SELECT valor,hora FROM %s%s WHERE item='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, item_c, tipo_c, id);
	Free(item_c);
	Free(tipo_c);
	if (res)
	{
		time_t hora;;
		row = SQLFetchRow(res);
		hora = (time_t)atoul(row[1]);
		if (hora && hora < time(0))
		{
			SQLFreeRes(res);
			BorraCache(tipo, item, id);
			return NULL;
		}
		else
		{
			if (BadPtr(row[0]))
			{
				SQLFreeRes(res);
				return NULL;
			}
			strlcpy(row0, row[0], sizeof(row0));
			SQLFreeRes(res);
			return row0;
		}
	}
	return NULL;
}

/*!
 * @desc: Inserta un dato en la Caché interna.
 * @params: $tipo [in] Tipo de datos a insertar, a definir por el usuario.
 	    $item [in] Ítem a insertar.
 	    $off [in] Tiempo de validez, en segundos. Pasado estos segundos, esta entrada se borrará automáticamente.
 	    $id [in] Propietario o dueño de la entrada.
 	    $valor [in] Cadena con formato que corresponde al valor de ese ítem. Puede ser NULL si no tiene valor.
 	    $... [in] Argumentos variables según la cadena de formato.
 * @ver: CogeCache BorraCache
 * @cat: Cache
 !*/
 
void InsertaCache(char *tipo, char *item, int off, int id, char *valor, ...)
{
	char *tipo_c, *item_c, *valor_c = NULL, buf[BUFSIZE];
	va_list vl;
	buf[0] = '\0';
	if (!BadPtr(valor))
	{
		va_start(vl, valor);
		ircvsprintf(buf, valor, vl);
		va_end(vl);
		valor_c = SQLEscapa(buf);
	}
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	if (CogeCache(tipo, item, id))
		SQLQuery("UPDATE %s%s SET valor='%s', hora=%lu WHERE item='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, valor_c ? valor_c : item_c, off ? time(0) + off : 0, item_c, tipo_c, id);
	else
		SQLQuery("INSERT INTO %s%s (item,valor,hora,tipo,owner) values ('%s','%s',%lu,'%s',%i)", PREFIJO, SQL_CACHE, item_c, valor_c ? valor_c : item_c, off ? time(0) + off : 0, tipo_c, id);
	Free(item_c);
	Free(tipo_c);
	ircfree(valor_c);
}
int ProcCache(Proc *proc)
{
	u_long ts = time(0);
	if ((u_long)proc->time + 1800 < ts) /* lo hacemos cada 30 mins */
	{
		SQLQuery("DELETE from %s%s where hora < %lu AND hora !=0", PREFIJO, SQL_CACHE, ts);
		proc->proc = 0;
		proc->time = ts;
		return 1;
	}
	return 0;
}

/*!
 * @desc: Elimina una entrada de la Caché interna.
 * @params: $tipo [in] Tipo de datos.
 	    $item [in] Ítem a eliminar.
 	    $id [in] Dueño o propietario de la entrada.
 * @ver: CogeCache InsertaCache
 * @cat: Cache
 !*/
 
void BorraCache(char *tipo, char *item, int id)
{
	char *tipo_c, *item_c;
	tipo_c = SQLEscapa(tipo);
	item_c = SQLEscapa(item);
	SQLQuery("DELETE FROM %s%s WHERE LOWER(item)='%s' AND tipo='%s' AND owner=%i", PREFIJO, SQL_CACHE, strtolower(item_c), tipo_c, id);
	Free(item_c);
	Free(tipo_c);
}
int SigPostNick(Cliente *cl, int nuevo)
{
	Nivel *niv;
	if (IsId(cl) && (niv = BuscaNivel("ROOT")) && !strcasecmp(conf_set->root, cl->nombre))
		cl->nivel |= niv->nivel;
	return 0;
}
int SigCDestroy(Canal *cn)
{
	if (canal_debug && !strcasecmp(cn->nombre, canal_debug->nombre))
		canal_debug = NULL;
	return 0;
}
