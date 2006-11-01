/*
 * $Id: hash.h,v 1.1 2006-11-01 11:38:26 Trocotronic Exp $ 
 */

/*!
 * @desc: Estructura de datos Hash. Se utiliza para definir una lista de entradas enlazadas.
 * @params: $item Apunta al primer elemento de la lista.
 	    $items Cantidad de elementos en esa lista.
 * @cat: Programa
 * @ex: 	Hash tabla[100];
 	UnaEstructuraCualquiera *st;
 	...
 	//UnaEstructuraCualquiera debe tener como miembro a hsig
 	st->hsig = tabla[0].item;
 	tabla[0].item = cl;
 	tabla[0].items++;
 !*/	
typedef struct _hash {
	void *item;
	int items;
}Hash;
extern struct _cliente *BuscaClienteEnHash(char *, Hash *);
extern struct _canal *BuscaCanalEnHash(char *, Hash *);
extern void InsertaClienteEnHash(struct _cliente*, char *, Hash *);
extern void InsertaCanalEnHash(struct _canal *, char *, Hash *);
extern int BorraClienteDeHash(struct _cliente *, char *, Hash *);
extern int BorraCanalDeHash(struct _canal *, char *, Hash *);
extern MODVAR Hash uTab[UMAX];
extern MODVAR Hash cTab[CHMAX];
