/*
 * $Id: hash.c,v 1.4 2005-06-29 21:13:50 Trocotronic Exp $ 
 */

#include <stdio.h>
#include "struct.h"
#include "ircd.h"

Hash uTab[UMAX];
Hash cTab[CHMAX];

/*
 * hasheamos utilizando un multiplicador y un sumando. tenemos de sobras.
 */
u_int HashCliente(char *clave)
{
	u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % UMAX);
}
u_int HashCanal(char *clave)
{
	u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % CHMAX);
}
void InsertaClienteEnHash(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	hash = HashCliente(clave);
	us->hsig = tabla[hash].item;
	tabla[hash].item = us;
	tabla[hash].items++;
}
void InsertaCanalEnHash(Canal *ca, char *clave, Hash *tabla)
{
	u_int hash = 0;
	hash = HashCanal(clave);
	ca->hsig = tabla[hash].item;
	tabla[hash].item = ca;
	tabla[hash].items++;
}
int BorraClienteDeHash(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Cliente *aux, *prev = NULL;
	hash = HashCliente(clave);
	for (aux = (Cliente *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (aux == us)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				tabla[hash].item = aux->hsig;
			tabla[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
int BorraCanalDeHash(Canal *ca, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Canal *aux, *prev = NULL;
	hash = HashCanal(clave);
	for (aux = (Canal *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (aux == ca)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				tabla[hash].item = aux->hsig;
			tabla[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
Cliente *BuscaClienteEnHash(char *clave, Cliente *lugar, Hash *tabla)
{
	Cliente *aux;
	u_int hash;
	hash = HashCliente(clave);
	for (aux = (Cliente *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
Canal *BuscaCanalEnHash(char *clave, Canal *lugar, Hash *tabla)
{
	Canal *aux;
	u_int hash;
	hash = HashCanal(clave);
	for (aux = (Canal *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
