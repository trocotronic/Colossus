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
void inserta_cliente_en_hash(Cliente *us, char *clave)
{
	u_int hash = 0;
	hash = hash_cliente(clave);
	us->hsig = uTab[hash].item;
	uTab[hash].item = us;
	uTab[hash].items++;
}
void inserta_canal_en_hash(Canal *ca, char *clave)
{
	u_int hash = 0;
	hash = hash_canal(clave);
	ca->hsig = cTab[hash].item;
	cTab[hash].item = ca;
	cTab[hash].items++;
}
int borra_cliente_de_hash(Cliente *us, char *clave)
{
	u_int hash = 0;
	Cliente *aux, *prev = NULL;
	hash = hash_cliente(clave);
	for (aux = (Cliente *)uTab[hash].item; aux; aux = aux->hsig)
	{
		if (aux == us)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				uTab[hash].item = aux->hsig;
			uTab[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
int borra_canal_de_hash(Canal *ca, char *clave)
{
	u_int hash = 0;
	Canal *aux, *prev = NULL;
	hash = hash_canal(clave);
	for (aux = (Canal *)cTab[hash].item; aux; aux = aux->hsig)
	{
		if (aux == ca)
		{
			if (prev)
				prev->hsig = aux->hsig;
			else
				cTab[hash].item = aux->hsig;
			cTab[hash].items--;
			return 1;
		}
		prev = aux;
	}
	return 0;
}
Cliente *busca_cliente_en_hash(char *clave, Cliente *lugar)
{
	Cliente *aux;
	u_int hash;
	hash = hash_cliente(clave);
	for (aux = (Cliente *)uTab[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
Canal *busca_canal_en_hash(char *clave, Canal *lugar)
{
	Canal *aux;
	u_int hash;
	hash = hash_canal(clave);
	for (aux = (Canal *)cTab[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return lugar;
}
