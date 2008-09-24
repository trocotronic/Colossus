/*
 * $Id: ssl.h,v 1.4 2005/06/29 21:13:46 Trocotronic Exp $ 
 */

/*
 * Todas las rutinas se han cogido de UnrealIRCd 
 */
 
extern	SSL_CTX *ctx_server;
extern	SSL_CTX *ctx_client;

struct _sock; /* tiene que definirse más abajo! */

extern void SSLInit();
extern	int SSLSockRead(struct _sock *, void *, int);
extern	int SSLSockWrite(struct _sock *, const void *, int);
extern	int SSLAccept(struct _sock *, int);
extern	int SSLConnect(struct _sock *);
extern	int SSLShutDown(SSL *);
extern	int SSLClienteConexion(struct _sock *);
extern void SSLSockNoBlock(SSL *);

#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
