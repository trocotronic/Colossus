/*
 * $Id: hash.c,v 1.3 2004-12-31 12:27:57 Trocotronic Exp $ 
 */

#include <stdio.h>
#include "struct.h"
#include "ircd.h"

Hash uTab[UMAX];
Hash cTab[CHMAX];

/*
 * hasheamos utilizando un multiplicador y un sumando. tenemos de sobras.
 */
u_int hash_cliente(char *clave)
{
	u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % UMAX);
}
u_int hash_canal(char *clave)
{
	u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % CHMAX);
}
void inserta_cliente_en_hash(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	hash = hash_cliente(clave);
	us->hsig = tabla[hash].item;
	tabla[hash].item = us;
	tabla[hash].items++;
}
void inserta_canal_en_hash(Canal *ca, char *clave, Hash *tabla)
{
	u_int hash = 0;
	hash = hash_canal(clave);
	ca->hsig = tabla[hash].item;
	tabla[hash].item = ca;
	tabla[hash].items++;
}
int borra_cliente_de_hash(Cliente *us, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Cliente *aux, *prev = NULL;
	hash = hash_cliente(clave);
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
int borra_canal_de_hash(Canal *ca, char *clave, Hash *tabla)
{
	u_int hash = 0;
	Canal *aux, *prev = NULL;
	hash = hash_canal(clave);
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
Cliente *busca_cliente_en_hash(char *clave, Cliente *lugar, Hash *tabla)
{
	Cliente *aux;
	u_int hash;
	hash = hash_cliente(clave);
	for (aux = (Cliente *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
Canal *busca_canal_en_hash(char *clave, Canal *lugar, Hash *tabla)
{
	Canal *aux;
	u_int hash;
	hash = hash_canal(clave);
	for (aux = (Canal *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
