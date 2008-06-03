#include <time.h>
#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "modulos/gameserv.h"
#include "modulos/gameserv/kyrhos.h"

Kyrhos *kyrhos = NULL;
KyrhosUser *kusers = NULL;

int KyrhosSockClose();
int KyrhosEOS();
int KyrhosSQL();
int KyrhosPMsg(Cliente *, Cliente *, char *, int);
int ProcesaKyrhos(Cliente *, char *);

#define P(x) Responde(kus->cl, kyrhos->cl, x)
#define C0(x) (param [0] && !strcasecmp(param[0], x))
#define C1(x) (param [1] && !strcasecmp(param[1], x))
#define C2(x) (param [2] && !strcasecmp(param[2], x))
#define ID(x) (1<<x)
#define OD() ObjetosDejados(kus)
#define Tiene(x) (kus->objetos & ID(x))
#define Usa(x) (kus->usando & ID(x))
#define Muerto(x) (kus->muertos & ID(x))

int Procesa13G(KyrhosUser *);
int Procesa14G(KyrhosUser *);

int Procesa13H(KyrhosUser *);
int Procesa14H(KyrhosUser *);
int Procesa15H(KyrhosUser *);
int Procesa16H(KyrhosUser *);

int Procesa13I(KyrhosUser *);

int Procesa13J(KyrhosUser *);

KyrhosObj objs[] = {
	{ "guantes" , POS14H , 1 , 0 , 0 , 1 , 2 } ,
	{ "pala" , POS13J , 0 , 0 , 0 , 0 , 1 } ,
	{ "migas" , POS13J , 0 , 0 , 5 , 0 , 3 } 
};
KyrhosEnemigo enems[] = {
	{ "lagarto" , POS14G , 1 , 1 , 8 , 0.5 , 0 }
};
KyrhosPos poses[] = {
	{ POS16H , P_NORTE , P_NADA , P_NADA , P_NADA , Procesa16H } ,
	{ POS14G , P_NADA , P_ESTE , P_NADA , P_NADA , Procesa14G } ,
	{ POS14H , P_NORTE , P_NADA , P_SUR , P_OESTE , Procesa14H } ,
	{ POS15H , P_NORTE , P_NADA , P_SUR , P_NADA , Procesa15H } ,
	{ POS13H , P_NADA , P_ESTE , P_SUR , P_OESTE , Procesa13H } ,
	{ POS13I , P_NADA , P_ENTRAR , P_NADA , P_OESTE , Procesa13I } ,
	{ POS13J , P_NADA , P_NADA , P_NADA , P_SALIR , Procesa13J } ,
	{ POS13G , P_NADA , P_ESTE , P_NADA , P_NADA , Procesa13G } ,
	{ 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 }
};
Opts ords[] = {
	{ P_ENTRAR , "Entrar" } ,
	{ P_SALIR , "Salir" } ,
	{ P_SUBIR , "Subir" } ,
	{ P_BAJAR , "Bajar" } ,
	{ P_NORTE , "Norte" } ,
	{ P_ESTE , "Este" } ,
	{ P_SUR , "Sur" } ,
	{ P_OESTE , "Oeste" } ,
	{ 0x0 , 0x0 }
};
//int (*poses[MAX_POS][MAX_POS])(KyrhosUser *) = {
//		/* 	 A		    B	  	  C	    		D      	   E			 F		    G      	  H    		I      	   J      	 K      	    L      	  M      		N      	   O      	 P 	   */
//* 1 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 2 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 3 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 4 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 5 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 6 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 7 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 8 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 9 */ 	{    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 10 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 11 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 12 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 13 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 14 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    , Procesa14G , Procesa14H ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 15 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    , Procesa15H ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//* 16 */ {    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    , Procesa16H ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    ,    NULL    } ,
//};
KyrhosUser *BuscaKyrhosUser(Cliente *cl)
{
	KyrhosUser *kus;
	for (kus = kusers; kus; kus = kus->sig)
	{
		if (kus->cl == cl)
			return kus;
	}
	return NULL;
}
KyrhosUser *InsertaKyrhosUser(Cliente *cl)
{
	KyrhosUser *kus;
	//u_int i;
	if ((kus = BuscaKyrhosUser(cl)))
		return kus;
	kus = BMalloc(KyrhosUser);
	kus->cl = cl;
	kus->pos = &poses[0];
	kus->vida = P_EXP;
	kus->ataque = kus->defensa = 1;
	kus->experiencia = 1.0;
	//for (i = 0; i < O_OBJS; i++)
	//	kus->posobjs[i] = objs[i].pos;
	AddItem(kus, kusers);
	return kus;
}
int BorraKyrhosUser(KyrhosUser *kus)
{
	LiberaItem(kus, kusers);
	return 0;
}
char *Articulo(int tipo, int personal)
{
	switch(tipo)
	{
		case 0:
			if (personal)
				return "el";
			else
				return "un";
		case 1:
			if (personal)
				return "la";
			else
				return "una";
		case 2:
			if (personal)
				return "los";
			else
				return "unos";
		case 3:
			if (personal)
				return "las";
			else
				return "unas";
	}
	return "null";
}
void ObjetosDejados(KyrhosUser *kus)
{
	int i;
	for (i = 0; i < O_OBJS; i++)
	{
		if (kus->posobjs[i] == kus->pos->xy)
			Responde(kus->cl, kyrhos->cl, "Aquí hay %s %s.", Articulo(objs[i].art, 0), objs[i].nombre);
	}
}
char *Herida(u_int r, u_int max)
{
	u_int p;
	if (max <= 4)
		p = 1;
	else
		p = max / 4;
	if (r <= p)
		return "roza";
	else if (p < r && r <= 2*p)
		return "araña";
	else if (2*p < r && r <= 3*p)
		return "golpea";
	else if (3*p < r)
		return "destroza";
	else
		return "uh?";
}
void Despide(KyrhosUser *kus)
{
	P("Tu misión ha llegado a su fin. No has podido sobrevivir a tu nuevo destino.");
	P("Qué pasará con Lahlia? Es tarde para saberlo. Has muerto sin pena ni gloria, sin que nadie se enterara.");
	BorraKyrhosUser(kus);
}
void DetieneLucha(KyrhosUser *kus)
{
	ApagaCrono(kus->klu->tim);
	Free(kus->klu->ken);
	ircfree(kus->klu);
}
int Ataque(KyrhosLucha *klu)
{
	int r;
	char *h;
	//if (Aleatorio(1,1000) <= 500) /* ataca el usuario */
	//{
		r1:
		r = Aleatorio(0, klu->kus->ataque) - Aleatorio(0, klu->ken->defensa);
		if (r > 0) /* ataca */
		{
			char c;
			h = Herida(r, klu->kus->ataque);
			c = ToUpper(*h);
			Responde(klu->kus->cl, kyrhos->cl, "%c%ss a %s %s.", c, h+1, Articulo(klu->ken->art, 1), klu->ken->nombre);
			klu->ken->vida -= r;
			klu->esquiv &= ~0x1;
		}
		else
		{
			char c;
			if (klu->esquiv & 0x1)
				goto r1;
			h = Articulo(klu->ken->art, 1);
			c = ToUpper(*h);
			Responde(klu->kus->cl, kyrhos->cl, "%c%s %s esquiva tu ataque.", c, h+1, klu->ken->nombre);
			klu->esquiv |= 0x1;
		}
	//}
	//else
	//{
		r2:
		r = Aleatorio(0, klu->ken->ataque) - Aleatorio(0, klu->kus->defensa);
		if (r > 0) /* ataca */
		{
			char c;
			h = Articulo(klu->ken->art, 1);
			c = ToUpper(*h);
			Responde(klu->kus->cl, kyrhos->cl, "%c%s %s te %s.", c, h+1, klu->ken->nombre, Herida(r, klu->ken->ataque));
			klu->kus->vida -= r;
			klu->esquiv &= ~0x2;
		}
		else
		{
			if (klu->esquiv & 0x2)
				goto r2;
			Responde(klu->kus->cl, kyrhos->cl, "Esquivas el ataque de %s %s.", Articulo(klu->ken->art, 1), klu->ken->nombre);
			klu->esquiv |= 0x2;
		}
	//}
	if (klu->kus->vida <= 0 || klu->ken->vida <= 0)
	{
		if (klu->kus->vida <= 0)
		{
			Responde(klu->kus->cl, kyrhos->cl, "Has muerto");
			Despide(klu->kus);
		}
		else
		{
			u_int i = (u_int)klu->kus->experiencia;
			Responde(klu->kus->cl, kyrhos->cl, "Has conseguido acabar con %s %s", Articulo(klu->ken->art, 1), klu->ken->nombre);
			klu->kus->experiencia += klu->ken->experiencia;
			if ((u_int)klu->kus->experiencia != i)
				Responde(klu->kus->cl, kyrhos->cl, "Ahora tienes %u puntos de experiencia.", (u_int)klu->kus->experiencia);
			for (i = 0; i < E_ENEMS; i++)
			{
				if (!strcmp(klu->ken->nombre, enems[i].nombre))
					break;
			}
			if (i == E_ENEMS)
				Responde(klu->kus->cl, kyrhos->cl, "Ha ocurrido un error grave (1). Comuníquelo a su autor.");
			klu->kus->muertos |= ID(i);
		}
		DetieneLucha(klu->kus);
	}
	return 0;
}
KyrhosLucha *EmpiezaAtaque(KyrhosUser *kus, u_int ken)
{
	KyrhosLucha *klu;
	klu = BMalloc(KyrhosLucha);
	klu->kus = kus;
	klu->ken = (KyrhosEnemigo *)Malloc(sizeof(KyrhosEnemigo));
	memcpy(klu->ken, &enems[ken], sizeof(KyrhosEnemigo));
	klu->tim = IniciaCrono(0, 1, Ataque, klu);
	return klu;
}
KyrhosPos *BuscaPos(u_int xy)
{
	int i;
	for (i = 0; poses[i].func; i++)
	{
		if (poses[i].xy == xy)
			return &poses[i];
	}
	return NULL;
}
int Procesa14G(KyrhosUser *kus)
{
	P("Delante tuyo tienes una enorme roca de piedra granito, en medio de una pradera. Es como una grandiosa pelota de piedra, tan alta que no puedes ver la parte superior. Debe tener unos 3 metros de diámetro.");
	P("Parece caída del cielo. Alrededor suyo hay un círculo de arena.");
	if (Muerto(E_LAGARTO))
		P("Aquí yace el cuerpo de un lagarto brutalmente atacado.");
	else
		P("También hay un lagarto que busca comida.");
	return 0;
}
int Procesa13H(KyrhosUser *kus)
{
	P("Es la orilla de un riachuelo. Es un tanto especial. Su agua es verde. Un verde pestilente. Es como un enorme moco viscoso. No puedes ver ni el fondo.");
	P("No es muy ancho pero ni por todo el oro del mundo pondrías un pie en ese agua. A tu lado hay un cartel de madera con un palo clavado al suelo.");
	P("A tu derecha puedes entrever un molino abandonado. A tu izquierda, una playa pequeña con arena blanda.");
	return 0;
}
int Procesa14H(KyrhosUser *kus)
{
	P("Detrás tuyo queda Veldania, tu pueblo. Prefieres no mirar. Te trae muchos recuerdos. Respirando fondo miras hacia delante");
	P("Estás en un camino de tierra. Todavía húmedo de la lluvia que cayó hace dos días. Puedes ver muchos árboles que con sus hojas hacen una profunda sombra.");
	P("A tu derecha, hay un tronco podrido en el que han crecido muchas setas. A tu izquierda puedes ver una roca enorme que parece que lleva algo escrito.");
	P("A lo lejos puedes oir el murmullo de un riachuelo.");
	return 0;
}
int Procesa15H(KyrhosUser *kus)
{
	P("Estás en las murallas de la ciudad. Delante tuyo tienes una enorme puerta de madera con cadenas. Nunca has recordado que esa puerta estuviera levantada. Tal vez en la época del rey Kyrhos cuando conquistaba lejanas tierras.");
	P("También está Muktar, el guardián de la puerta. Era gran amigo de tu padre cuando eran jóvenes. Incluso alguna vez había venido a tu casa a comer algo.");
	P("Todavía está dormido y no se ha despertado con los primeros rayos.");
	return 0;
}
int Procesa16H(KyrhosUser *kus)
{
	P("Te encuentras en tu tierra natal: Veldania. Ya han pasado tres días desde los funerales para el rey Kyrhos.");
	P("Cansado de esperar decides ir en busca de la princesa Lahlia. Tan sólo coges un sayo con un poco de comida y te decides a embarcarte en una aventura que no sabes a dónde te llevará.");
	P("Tal vez consigas a Lahlia. Tal vez tengas una muerte segura. Pero esto no te frena. Cierras la vieja y podrida puerta de tu casa y miras hacia el cielo.");
	P("Los primeros rayos de sol se asoman por el valle. Todavía no se ve ningún veldanio por las calles. Antes de partir, giras la cabeza e intentas memorizar lo que ha sido hasta ahora tu casa. Seguramente nunca más vuelvas a verla.");
	return 0;
}
int Procesa13I(KyrhosUser *kus)
{
	P("Delante tuyo tienes un molino muy viejo. Sus aspas aún tienen algunos trozos de tela que antaño servían para hacerlas girar.");
	P("La puerta está media carcomida. Sus paredes blancas apenas se mantienen en pie.");
	P("Estás rodeado de maleza. A un lado puedes ver el río verde.");
	return 0;
}
int Procesa13J(KyrhosUser *kus)
{
	P("Oh! Al abrir la puerta te ha caído una biga a la cabeza y te ha dejado inconsciente.");
	if (--kus->vida > 0)
	{
		P("Afortunadamente, al cabo de un rato consigues recuperar el sentido. Poco a poco puedes ver su interior.");
		P("Es un molino con dos plantas. El techo tiene un agujero enorme. Por él pasan algunos rayos de luz que lo iluminan todo.");
		P("El suelo de la segunda planta se ha desplomado y está lleno de escombros.");
		P("A un lado puedes ver un extenso mecanismo que se une con las aspas del molino. Posiblemente se utilizaba tiempo atrás para moler grano u otra faena.");
		P("También hay un pequeño horno en el que se debían cocinar algunos alimentos. Encima de éste, está una enorme pala para sacarlos de dentro.");
	}
	else
	{
		P("Te has ido desangrando lentamente hasta llegar a tu muerte.");
		Despide(kus);
	}
	return 0;
}
int Procesa13G(KyrhosUser *kus)
{
	P("Es un embarcadero. A un lado hay un cartel indicativo. La arena es muy blanda. Podrías hacer castillos pero inmediatamente piensas en tu labor con Lahlia.");
	P("Parece que algo reluce debajo.");
	return 0;
}
void Salidas(KyrhosUser *kus)
{
	int i, tipo;
	buf[0] = '\0';
	for (i = 1; i < 5; i++)
	{
		if ((tipo = *(((u_int *)kus->pos)+i)) != P_NADA)
		{
			strlcat(buf, ", ", sizeof(buf));
			strlcat(buf, BuscaOptItem(tipo, ords), sizeof(buf));
		}
	}
	Responde(kus->cl, kyrhos->cl, "Salidas: %s", &buf[2]);
}
int FijarPos(KyrhosUser *kus, int pos)
{
	u_int nueva = 0;
	KyrhosPos *p;
	if (pos == P_NORTE)
	{
		if (((kus->pos->xy)>>16) == 0x0)
			return 0;
		nueva = ((kus->pos->xy&0xFFFF0000)>>1)|(kus->pos->xy&0x0000FFFF);
	}
	else if (pos == P_ESTE)
	{
		if (((kus->pos->xy)&0xFFFF) == 0x8000)
			return 0;
		nueva = ((kus->pos->xy&0x0000FFFF)<<1)|(kus->pos->xy&0xFFFF0000);
	}
	else if (pos == P_SUR)
	{
		if (((kus->pos->xy)>>16) == 0x8000)
			return 0;
		nueva = ((kus->pos->xy&0xFFFF0000)<<1)|(kus->pos->xy&0x0000FFFF);
	}
	else if (pos == P_OESTE)
	{
		if (((kus->pos->xy)&0xFFFF) == 0x0)
			return 0;
		nueva = ((kus->pos->xy&0x0000FFFF)>>1)|(kus->pos->xy&0xFFFF0000);
	}
	if (!(p = BuscaPos(nueva)))
		return 0;
	kus->pos = p;
	return 1;
}
int KyrhosPMsg(Cliente *cl, Cliente *bl, char *msg, int resp)
{
	if (bl == kyrhos->cl)
		ProcesaKyrhos(cl, msg);
	return 0;
}
int ProcesaKyrhos(Cliente *cl, char *msg)
{
	char *param[256], par[BUFSIZE];
	int params, i;
	KyrhosUser *kus = NULL;
	strlcpy(par, msg, sizeof(par));
	for (i = 0, param[i] = strtok(par, " "); param[i]; param[++i] = strtok(NULL, " "));
	params = i;
	if (!(kus = BuscaKyrhosUser(cl)))
	{
		if (C0("EMPIEZA"))
		{
			kus = InsertaKyrhosUser(cl);
			Procesa16H(kus);
			Salidas(kus);
			return 0;
		}
		else
		{
			Responde(cl, kyrhos->cl, "Para iniciar el juego escribe \00312/msg %s EMPIEZA", kyrhos->nick);
			return 1;
		}
	}
	if (kus->klu && !C0("RENDIR"))
	{
		P("Ahora no. Estás luchando a vida o muerte!");
		return 0;
	}
	if (C0("N") || C0("E") || C0("S") || C0("O") ||
		C0("NORTE") || C0("ESTE") || C0("SUR") || C0("OESTE"))
	{
		int val;
		char c = ToUpper(*param[0]);
		if (c == 'N' && kus->pos->N == P_NORTE)
			val = FijarPos(kus, P_NORTE);
		else if (c == 'S' && kus->pos->S == P_SUR)
			val = FijarPos(kus, P_SUR);
		else if (c == 'E' && kus->pos->E == P_ESTE)
			val = FijarPos(kus, P_ESTE);
		else if (c == 'O' && kus->pos->O == P_OESTE)
			val = FijarPos(kus, P_OESTE);
		else
		{
			P("No puedes ir en esa dirección.");
			return 1;
		}
		if (!val)
		{
			P("No puedes ir en esa dirección.");
			return 1;
		}
		kus->pos->func(kus);
		OD();
		Salidas(kus);
	}
	if (C0("SUBIR")|| C0("BAJAR") || C0("ENTRAR") || C0("SALIR"))
	{
		int v = BuscaOpt(param[0], ords), i;
		for (i = 1; i < 5; i++)
		{
			if (*(((u_int *)kus->pos)+i) == v)
				break;
		}
		if (i == 5 || !FijarPos(kus, i))
		{
			P("No puedes ir en esa dirección.");
			return 1;
		}
		kus->pos->func(kus);
		OD();
		Salidas(kus);
	}
	else if (C0("EXAMINAR") || C0("EX"))
	{
		switch(kus->pos->xy)
		{
			case POS14H:
				if (C1("tronco"))
				{
					P("Es un tronco bastante seco. Hay mucha vegetación en él.");
					if (!Tiene(O_GUANTES))
					{
						P("Mirando en su interior puedes ver unos guantes de cuero bastante roñosos.");
						P("Seguramente son de algún caballero que los tiró.");
					}
					break;
				}
			case POS14G:
				if (C1("roca") || C1("piedra"))
				{
					P("Es una enorme roca redonda. Te acercas y ves que hay una inscripción que rodea la piedra.");
					P("Son símbolos que nunca has visto. No parece tener ningún sentido. No obstante, sabes que ahí se guarda un gran secreto.");
					break;
				}
			case POS13H:
				if (C1("cartel"))
				{
					P("Es un cartel en el que se puede leer:");
					P("\"Prohibido bañarse. Río Kefer.");
					P("Este río proviene de Los Picos Efeos.\"");
					break;
				}
			case POS13J:
				if (C1("horno"))
				{
					P("Es un horno muy viejo. Ya no crees que pueda usarse para hacer pan. Sin embargo, en el fondo parece que hay unas cuantas migas que podrían quitarte el hambre que tienes.");
					P("Pero están muy lejos! No puedes alcanzarlas con la mano.");
					break;
				}
			default:
				P("No ves nada que te llame la atención.");
		}
	}
	else if (C0("COGER") || C0("C"))
	{
		int i;
		for (i = 0; i < O_OBJS; i++)
		{
			if (C1(objs[i].nombre))
				break;
		}
		if (i == O_OBJS)	
			P("Recapacitas y te das cuenta de que no vale la pena cogerlo.");
		else if (Tiene(i))
			Responde(kus->cl, kyrhos->cl, "Ya %s tienes.", Articulo(objs[i].art, 1));
		else if ((kus->posobjs[i] && kus->posobjs[i] != kus->pos->xy) || (!kus->posobjs[i] && objs[i].pos != kus->pos->xy))
			P("Eres ciego? Este objeto no está.");
		else
		{
			switch(i)
			{
				case O_GUANTES:
					P("Con mucho asco coges los guantes llenos de moho. Dentro hay una cucaracha que sale corriendo antes de que te los guardes.");
					break;
				case O_PALA:
					P("Haciendo malabarismos por fin consigues asir la pala. Puede serte útil para coger cosas a larga distancia.");
					break;
			}
			kus->objetos |= ID(i);
			kus->posobjs[i] = 0;
		}	
	}
	else if (C0("USAR") || C0("U"))
	{
		int i;
		for (i = 0; i < O_OBJS; i++)
		{
			if (Tiene(i) && C1(objs[i].nombre))
				break;
		}
		if (i == O_OBJS)
			P("No tienes este objeto.");
		else if (Usa(i))
			Responde(kus->cl, kyrhos->cl, "Ya estás usando %s %s.", Articulo(objs[i].art, 1), objs[i].nombre);
		else
		{
			switch(i)
			{
				case O_GUANTES:
					P("Soplas un poco y te pones los guantes en ambas manos. No te protegerán mucho pero es mejor que ir con las manos desnudas.");
					break;
				case O_PALA:
					if (!param[2])
						P("Debes utilizar la pala con algo: USAR PALA <algo>");
					else
					{
						if (!Tiene(O_MIGAS) && (C2("horno") || C2("migas")))
						{
							P("Con mucha destreza consigues meter la pala en el horno y acercarte unas migas de pan. Te las metes en el bolsillo.");
							kus->objetos |= ID(O_MIGAS);
							kus->posobjs[O_MIGAS] = 0;
							break;
						}
						P("No puedes usar la pala con esto.");
					}
					break;
				case O_MIGAS:
					P("Muerto de hambre te comes las últimas migas que te quedaban.");
					kus->objetos &= ~ID(O_MIGAS);
					break;
			}
			if (objs[i].defensa)
				Responde(kus->cl, kyrhos->cl, "Tu defensa aumenta %u punto%s.", objs[i].defensa, objs[i].defensa > 1 ? "s" : "");
			if (objs[i].ataque)
				Responde(kus->cl, kyrhos->cl, "Tu ataque aumenta %u punto%s.", objs[i].ataque, objs[i].ataque > 1 ? "s" : "");
			if (objs[i].vida)
				Responde(kus->cl, kyrhos->cl, "Tu vida aumenta %u punto%s.", objs[i].vida, objs[i].vida > 1 ? "s" : "");
			if (objs[i].vestir)
				kus->usando |= ID(i);
			kus->defensa += objs[i].defensa;
			kus->ataque += objs[i].ataque;
			kus->vida += objs[i].vida;
		}
	}
	else if (C0("INFO"))
	{
		Responde(kus->cl, kyrhos->cl, "Defensa: %u puntos", kus->defensa);
		Responde(kus->cl, kyrhos->cl, "Ataque: %u puntos", kus->ataque);
		Responde(kus->cl, kyrhos->cl, "Experiencia: %u puntos", (u_int)kus->experiencia);
		Responde(kus->cl, kyrhos->cl, "Vida: %u/%u puntos", kus->vida, ((u_int)kus->experiencia)*P_EXP);
	}
	else if (C0("INVENTARIO") || C0("I"))
	{
		int i;
		Responde(kus->cl, kyrhos->cl, "%u monedas de oro", kus->oro);
		for (i = 0; i < O_OBJS; i++)
		{
			if (Tiene(i))
				Responde(kus->cl, kyrhos->cl, "- %s %s", objs[i].nombre, Usa(i) ? "*" : "");
		}
		if (kus->objetos)
			P("(* lo estás usando)");
	}
	else if (C0("DESVESTIR"))
	{
		int i;
		for (i = 0; i < O_OBJS; i++)
		{
			if (Tiene(i) && C1(objs[i].nombre))
				break;
		}
		if (i == O_OBJS)
			P("No tienes este objeto.");
		else if (!Usa(i))
			Responde(kus->cl, kyrhos->cl, "No estás usando %s %s.", Articulo(objs[i].art, 1), objs[i].nombre);
		else
		{
			switch(i)
			{
				case O_GUANTES:
					P("Prefieres no usar estos guantes tan roñosos. A saber por qué manos han pasado.");
					break;
			}
			if (objs[i].defensa)
				Responde(kus->cl, kyrhos->cl, "Tu defensa decrece %u punto%s.", objs[i].defensa, objs[i].defensa > 1 ? "s" : "");
			if (objs[i].ataque)
				Responde(kus->cl, kyrhos->cl, "Tu ataque decrece %u punto%s.", objs[i].ataque, objs[i].ataque > 1 ? "s" : "");
			if (objs[i].vida)
				Responde(kus->cl, kyrhos->cl, "Tu vida decrece %u punto%s.", objs[i].vida, objs[i].vida > 1 ? "s" : "");
			kus->usando &= ~ID(i);
			kus->defensa -= objs[i].defensa;
			kus->ataque -= objs[i].ataque;
			kus->vida -= objs[i].vida;
		}
	}
	else if (C0("DEJAR"))
	{
		int i;
		for (i = 0; i < O_OBJS; i++)
		{
			if (Tiene(i) && C1(objs[i].nombre))
				break;
		}
		if (i == O_OBJS)
			P("No tienes este objeto.");
		else
		{
			if (Usa(i))
			{
				ircsprintf(buf, "DESVESTIR %s", param[1]);
				ProcesaKyrhos(cl, buf);
			}
			kus->objetos &= ~ID(i);
			kus->posobjs[i] = kus->pos->xy;
		}
	}
	else if (C0("LUCHAR") || C0("ATACAR"))
	{
		int i;
		for (i = 0; i < E_ENEMS; i++)
		{
			if (C1(enems[i].nombre))
				break;
		}
		if (i == E_ENEMS || kus->pos->xy != enems[i].pos)
			P("Estás muy cansado. Prefieres dejarlo estar.");
		else if (Muerto(i))
			P("Deja a los muertos en paz.");
		else
		{
			Responde(kus->cl, kyrhos->cl, "Cargado de valor empiezas un feroz ataque con %s %s.", Articulo(enems[i].art, 1), enems[i].nombre);
			kus->klu = EmpiezaAtaque(kus, i);
		}
	}
	else if (C0("RENDIR"))
	{
		if (!kus->klu)
			P("Ahora no estás luchando.");
		else
		{
			DetieneLucha(kus);
			P("Muerto de miedo decides rendirte e irte por patas.");
		}
	}
	else if (C0("SALIDAS"))
		Salidas(kus);
	return 0;
}
int KyrhosSockClose()
{
	KyrhosUser *kus, *sig;
	for (kus = kusers; kus; kus = sig)
	{
		sig = kus;
		if (kus->klu)
			DetieneLucha(kus);
		Free(kus);
	}
	return 0;
}
int KyrhosEOS()
{
	kyrhos->cl = CreaBot(kyrhos->nick ? kyrhos->nick : "Kyrhos", gameserv->hmod->ident, gameserv->hmod->host, "kdBq", "La sombra de Kyrhos");
	return 0;
}
int KyrhosSQL()
{
	if (!SQLEsTabla(GS_KYRHOS))
	{
		SQLQuery("CREATE TABLE IF NOT EXISTS %s%s ( "
			"item varchar(255), "
			"pos int, "
			"KEY item (item) "
			");", PREFIJO, GS_KYRHOS);
		if (sql->_errno)
			Alerta(FADV, "Ha sido imposible crear la tabla '%s%s'.", PREFIJO, GS_KYRHOS);
	}
	return 0;
}
int KyrhosDescarga()
{
	ircfree(kyrhos->nick);
	DesconectaBot(kyrhos->cl, "La sombra de Kyrhos");
	kyrhos->cl = NULL;
	BorraSenyal(SIGN_EOS, KyrhosEOS);
	BorraSenyal(SIGN_SQL, KyrhosSQL);
	BorraSenyal(SIGN_PMSG, KyrhosPMsg);
	BorraSenyal(SIGN_SOCKCLOSE, KyrhosSockClose);
	return 0;
}
int KyrhosParseaConf(Conf *cnf)
{
	int p;
	if (!kyrhos)
		kyrhos = BMalloc(Kyrhos);
	for (p = 0; p < cnf->secciones; p++)
	{
		if (!strcmp(cnf->seccion[p]->item, "nick"))
			ircstrdup(kyrhos->nick, cnf->seccion[p]->data);
	}
	InsertaSenyal(SIGN_EOS, KyrhosEOS);
	InsertaSenyal(SIGN_SQL, KyrhosSQL);
	InsertaSenyal(SIGN_PMSG, KyrhosPMsg);
	InsertaSenyal(SIGN_SOCKCLOSE, KyrhosSockClose);
	return 0;
}