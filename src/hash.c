/*
 * $Id: hash.c,v 1.7 2007-01-18 12:43:55 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"

Hash uTab[UMAX];
Hash cTab[CHMAX];

/*
 * hasheamos utilizando un multiplicador y un sumando. tenemos de sobras.
 */
unsigned int ELFHash(char* str, unsigned int len)
{
   unsigned int hash = 0;
   unsigned int x    = 0;
   unsigned int i    = 0;

   for(i = 0; i < len; str++, i++)
   {
      hash = (hash << 4) + (ToLower(*str));
      if((x = hash & 0xF0000000L) != 0)
      {
         hash ^= (x >> 24);
         hash &= ~x;
      }
   }

   return (hash & 0x7FFFFFFF);
}
u_int HashCliente(char *clave)
{
	return (ELFHash(clave, strlen(clave))%UMAX);
/*	u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % UMAX);*/
}
u_int HashCanal(char *clave)
{
	return (ELFHash(clave, strlen(clave))%CHMAX);
	/*u_int hash = 0;
	while (*clave)
		hash += (ToLower(*clave++) + INI_SUMD) * INI_FACT;
	return (hash % CHMAX);*/
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
Cliente *BuscaClienteEnHash(char *clave, Hash *tabla)
{
	Cliente *aux;
	u_int hash;
	hash = HashCliente(clave);
	for (aux = (Cliente *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return NULL;
}
Canal *BuscaCanalEnHash(char *clave, Hash *tabla)
{
	Canal *aux;
	u_int hash;
	hash = HashCanal(clave);
	for (aux = (Canal *)tabla[hash].item; aux; aux = aux->hsig)
	{
		if (!strcasecmp(aux->nombre, clave))
			return aux;
	}
	return NULL;
}
void add_item(Item *item, Item **lista)
{
	item->sig = *lista;
	*lista = item;
}
Item *del_item(Item *item, Item **lista, int borra)
{
	Item *aux, *prev = NULL;
	for (aux = *lista; aux; aux = aux->sig)
	{
		if (aux == item)
		{
			if (prev)
				prev->sig = aux->sig;
			else
				*lista = aux->sig;
			if (borra)
				Free(item);
			return (prev ? prev->sig : *lista);
		}
		prev = aux;
	}
	return NULL;
}
