/*
 * $Id: ssl.h,v 1.3 2004-10-01 19:06:11 Trocotronic Exp $ 
 */

/*
 * Todas las rutinas se han cogido de UnrealIRCd 
 */
 
extern	SSL_CTX *ctx_server;
extern	SSL_CTX *ctx_client;

struct _sock; /* tiene que definirse más abajo! */

extern void init_ssl();
extern	int ircd_SSL_read(struct _sock *, void *, int);
extern	int ircd_SSL_write(struct _sock *, const void *, int);
extern	int ircd_SSL_accept(struct _sock *, int);
extern	int ircd_SSL_connect(struct _sock *);
extern	int SSL_smart_shutdown(SSL *);
extern	int ircd_SSL_client_handshake(struct _sock *);
extern void SSL_set_nonblocking(SSL *);

#define SSLFLAG_FAILIFNOCERT 	0x1
#define SSLFLAG_VERIFYCERT 	0x2
#define SSLFLAG_DONOTACCEPTSELFSIGNED 0x4
