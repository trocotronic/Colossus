/*
 * $Id: ssl.c,v 1.7 2005-06-29 21:13:58 Trocotronic Exp $ 
 */
 
#include "struct.h"
#ifdef USA_SSL
#include "ircd.h"
#include "ssl.h"
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#define IDC_PASS 1108
extern HINSTANCE hInst;
extern HWND hwMain;
#endif

#define SAFE_SSL_READ 1
#define SAFE_SSL_WRITE 2
#define SAFE_SSL_ACCEPT 3
#define SAFE_SSL_CONNECT 4

static int SSLFatalError(int, int, Sock *);

SSL_CTX *ctx_server;
SSL_CTX *ctx_client;

char *SSLKeyPasswd;

typedef struct {
	int *size;
	char **buffer;
} StreamIO;

#ifdef _WIN32
static StreamIO *streamp;
LRESULT SSLPassDLG(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	StreamIO *stream = NULL;
	switch (Message) 
	{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			stream = (StreamIO *)streamp;
			if (LOWORD(wParam) == IDCANCEL) 
			{
				*stream->buffer = NULL;
				EndDialog(hDlg, TRUE);
			}
			else if (LOWORD(wParam) == IDOK) 
			{
				GetDlgItemText(hDlg, IDC_PASS, *stream->buffer, *stream->size);
				EndDialog(hDlg, TRUE);
			}
			return FALSE;
		case WM_CLOSE:
			if (stream)
				*stream->buffer = NULL;
			EndDialog(hDlg, TRUE);
		default:
			return FALSE;
	}
}
#endif
char *ErrorSSL(int err)
{
	char *ssl_errstr = NULL;
	switch(err) 
	{
		case SSL_ERROR_NONE:
			ssl_errstr = "SSL: Sin error";
			break;
		case SSL_ERROR_SSL:
			ssl_errstr = "Error interno de OpenSSL o error de protocolo";
			break;
		case SSL_ERROR_WANT_READ:
			ssl_errstr = "Funciones OpenSSL han pedido read()";
			break;
		case SSL_ERROR_WANT_WRITE:
			ssl_errstr = "Funciones OpenSSL han pedido write()";
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			ssl_errstr = "OpenSSL ha pedido una resolución X509 que no ha llegado";
			break;
		case SSL_ERROR_SYSCALL:
			ssl_errstr = "Error de sistema";
			break;
		case SSL_ERROR_ZERO_RETURN:
			ssl_errstr = "Error de conexión";
			break;
		case SSL_ERROR_WANT_CONNECT:
			ssl_errstr = "Funciones OpenSSL han pedido connect()";
			break;
		default:
			ssl_errstr = "Error OpenSSL desconocido";
	}
	return ssl_errstr;
}
int  SSLPemPasswd(char *buf, int size, int rwflag, void *password)
{
	char *pass;
	static int before = 0;
	static char beforebuf[1024];
#ifdef _WIN32
	StreamIO stream;
	char passbuf[512];	
	int passsize = 512;
#endif
	bzero(buf, size);
	if (before)
	{
		strncpy(buf, (char *)beforebuf, size);
		return (strlen(buf));
	}
#ifndef _WIN32
	pass = getpass("Contraseña para la clave privada SSL: ");
#else
	pass = passbuf;
	stream.buffer = &pass;
	stream.size = &passsize;
	streamp = &stream;
	DialogBoxParam(hInst, "SSLPass", hwMain, (DLGPROC)SSLPassDLG, (LPARAM)NULL); 
#endif
	if (pass)
	{
		bzero(buf, size);
		bzero(beforebuf, sizeof(beforebuf));
		strncpy(buf, (char *)pass, size);
		strncpy(beforebuf, (char *)pass, sizeof(beforebuf));
		before = 1;
		SSLKeyPasswd = beforebuf;
		return (strlen(buf));
	}
	return 0;
}
static int SSLComprueba(int preverify_ok, X509_STORE_CTX *ctx)
{
	int verify_err = 0;

	verify_err = X509_STORE_CTX_get_error(ctx);
	if (preverify_ok)
		return 1;
	if (conf_ssl->opts & SSLFLAG_VERIFYCERT)
	{
		if (verify_err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
			if (!(conf_ssl->opts & SSLFLAG_DONOTACCEPTSELFSIGNED))
			{
				return 1;
			}
		return preverify_ok;
	}
	else
		return 1;
}
void CTXInitServer()
{
	ctx_server = SSL_CTX_new(SSLv23_server_method());
	if (!ctx_server)
	{
		Alerta(FERR, "Falla al hacer SSL_CTX_new server");
		CierraColossus(2);
	}
	SSL_CTX_set_default_passwd_cb(ctx_server, SSLPemPasswd);
	SSL_CTX_set_options(ctx_server, SSL_OP_NO_SSLv2);
	SSL_CTX_set_verify(ctx_server, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE
			| (conf_ssl->opts & SSLFLAG_FAILIFNOCERT ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0), SSLComprueba);
	SSL_CTX_set_session_cache_mode(ctx_server, SSL_SESS_CACHE_OFF);
	if (SSL_CTX_use_certificate_file(ctx_server, SSL_SERVER_CERT_PEM, SSL_FILETYPE_PEM) <= 0)
	{
		Alerta(FERR, "Falla al cargar el certificado SSL %s", SSL_SERVER_CERT_PEM);
		CierraColossus(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx_server, SSL_SERVER_KEY_PEM, SSL_FILETYPE_PEM) <= 0)
	{
		Alerta(FERR, "Falla al cargar la clave privada SSL %s", SSL_SERVER_KEY_PEM);
		CierraColossus(4);
	}
	if (!SSL_CTX_check_private_key(ctx_server))
	{
		Alerta(FERR, "Falla al comprobar la clave privada SSL");
		CierraColossus(5);
	}
	if (conf_ssl->trusted_ca_file)
	{
		if (!SSL_CTX_load_verify_locations(ctx_server, conf_ssl->trusted_ca_file, NULL))
		{
			Alerta(FERR, "Falla al cargar los certificados seguros desde %s", conf_ssl->trusted_ca_file);
			CierraColossus(6);
		}
	}
}
void CTXInitCliente()
{
	ctx_client = SSL_CTX_new(SSLv3_client_method());
	if (!ctx_client)
	{
		Alerta(FERR, "Falla al hacer SSL_CTX_new client");
		CierraColossus(2);
	}
	SSL_CTX_set_default_passwd_cb(ctx_client, SSLPemPasswd);
	SSL_CTX_set_session_cache_mode(ctx_client, SSL_SESS_CACHE_OFF);
	if (SSL_CTX_use_certificate_file(ctx_client, SSL_SERVER_CERT_PEM, SSL_FILETYPE_PEM) <= 0)
	{
		Alerta(FERR, "Falla al cargar el certificado %s (client)", SSL_SERVER_CERT_PEM);
		CierraColossus(3);
	}
	if (SSL_CTX_use_PrivateKey_file(ctx_client, SSL_SERVER_KEY_PEM, SSL_FILETYPE_PEM) <= 0)
	{
		Alerta(FERR, "Falla alcargar la clave privada SSL %s (client)", SSL_SERVER_KEY_PEM);
		CierraColossus(4);
	}

	if (!SSL_CTX_check_private_key(ctx_client))
	{
		Alerta(FERR, "Falla al comprobar la clave privada SSL (client)");
		CierraColossus(5);
	}
}
void SSLInit(void)
{
	static int cargado = 0;
	if (!conf_ssl) /* no hay configuración, así que no hay "soporte" */
		return;
	if (cargado)
		return;
	cargado = 1;
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	if (conf_ssl->usa_egd) 
	{
#if OPENSSL_VERSION_NUMBER >= 0x000907000
		if (!conf_ssl->egd_path)
			RAND_status();
		else

#else
		if (conf_ssl->egd_path) 
#endif
			RAND_egd(conf_ssl->egd_path);		
	}
	CTXInitServer();
	CTXInitCliente();
}
void	SSLSockNoBlock(SSL *s)
{
	BIO_set_nbio(SSL_get_rbio(s),1);  
	BIO_set_nbio(SSL_get_wbio(s),1);  
}
char *SSLCifrado(SSL *ssl)
{
	static char buf[400];
	int bits;
	SSL_CIPHER *c; 
	buf[0] = '\0';
	strcpy(buf, SSL_get_version(ssl));
	strcat(buf, "-");
	strcat(buf, SSL_get_cipher(ssl));
	c = SSL_get_current_cipher(ssl);
	SSL_CIPHER_get_bits(c, &bits);
	strcat(buf, "-");
	strcat(buf, (char *)my_itoa(bits));
	strcat(buf, "bits");
	return (buf);
}
int SSLSockRead(Sock *sck, void *buf, int sz)
{
	int len, ssl_err;
	if (EsCerr(sck))
		return -1;
	len = SSL_read((SSL *)sck->ssl, buf, sz);
	if (len <= 0)
	{
		switch ((ssl_err = SSL_get_error((SSL *)sck->ssl, len)))
		{
			case SSL_ERROR_SYSCALL:
				if (ERRNO == P_EWOULDBLOCK || ERRNO == P_EAGAIN || ERRNO == P_EINTR) 
				{
           		case SSL_ERROR_WANT_READ:
					SET_ERRNO(P_EWOULDBLOCK);
					return -1;
				}
			case SSL_ERROR_SSL:
				if (ERRNO == EAGAIN)
                   			return -1;
			default:
				return SSLFatalError(ssl_err, SAFE_SSL_READ, sck);        
		}
	}
	return len;
}
int SSLSockWrite(Sock *sck, const void *buf, int sz)
{
	int len, ssl_err;
	len = SSL_write((SSL *)sck->ssl, buf, sz);
	if (len <= 0)
	{
		switch ((ssl_err = SSL_get_error((SSL *)sck->ssl, len)))
		{
			case SSL_ERROR_SYSCALL:
				if (ERRNO == P_EWOULDBLOCK || ERRNO == P_EAGAIN || ERRNO == P_EINTR)
					SET_ERRNO(P_EWOULDBLOCK);
				return -1;
			case SSL_ERROR_WANT_WRITE:
				SET_ERRNO(P_EWOULDBLOCK);
				return -1;
			case SSL_ERROR_SSL:
				if (ERRNO == EAGAIN)
					return -1;
			default:
				return SSLFatalError(ssl_err, SAFE_SSL_WRITE, sck);
		}
	}
	return len;
}
int SSLClienteConexion(Sock *sck)
{
	sck->ssl = SSL_new(ctx_client);
	if (!sck->ssl)
		return FALSE;
	SSL_set_fd(sck->ssl, sck->pres);
	SSL_set_connect_state(sck->ssl);
	SSLSockNoBlock(sck->ssl);
	if (SockIrcd == sck && conf_ssl->cifrados)
	{
		if (SSL_set_cipher_list((SSL *)sck->ssl, conf_ssl->cifrados) == 0)
			return -2;
	}
	sck->opts |= OPT_SSL;
	switch (SSLConnect(sck))
	{
		case -1: 
			return -1;
		case 0:
			SetSSLConnectHandshake(sck);
			return 0;
		case 1: 
			return 1;
		default:
			return -1;		
	}
}
int SSLAccept(Sock *sck, int fd) 
{
	int ssl_err;
	if ((ssl_err = SSL_accept((SSL *)sck->ssl)) <= 0)
	{
		switch ((ssl_err = SSL_get_error((SSL *)sck->ssl, ssl_err)))
		{
			case SSL_ERROR_SYSCALL:
				if (ERRNO == P_EINTR || ERRNO == P_EWOULDBLOCK || ERRNO == P_EAGAIN)
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				    return 1;
			default:
				return SSLFatalError(ssl_err, SAFE_SSL_ACCEPT, sck);
		}
		return -1;
	}
	return 1;
}
int SSLConnect(Sock *sck)
{
	int ssl_err;
	if ((ssl_err = SSL_connect((SSL *)sck->ssl)) <= 0) 
	{
		switch ((ssl_err = SSL_get_error((SSL *)sck->ssl, ssl_err)))
		{
			case SSL_ERROR_SYSCALL:
				if (ERRNO == P_EINTR || ERRNO == P_EWOULDBLOCK || ERRNO == P_EAGAIN)
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				    return 0;
			default:
				return SSLFatalError(ssl_err, SAFE_SSL_CONNECT, sck);
		}
		return -1;
	}
	return 0;
}
int SSLShutDown(SSL *ssl) 
{
	char i;
	int rc = 0;
	for (i = 0; i < 4; i++) 
	{
		if ((rc = SSL_shutdown(ssl)))
	    		break;
	}
	return rc;
}
static int SSLFatalError(int ssl_error, int where, Sock *sck)
{
	int errtmp = ERRNO;
	char *ssl_errstr, *ssl_func;
	switch (where) 
	{
		case SAFE_SSL_READ:
			ssl_func = "SSL_read()";
			break;
		case SAFE_SSL_WRITE:
			ssl_func = "SSL_write()";
			break;
		case SAFE_SSL_ACCEPT:
			ssl_func = "SSL_accept()";
			break;
		case SAFE_SSL_CONNECT:
			ssl_func = "SSL_connect()";
			break;
		default:
			ssl_func = "undefined SSL func";
	}
	switch (ssl_error) 
	{
		case SSL_ERROR_NONE:
			ssl_errstr = "SSL: Sin error";
			break;
		case SSL_ERROR_SSL:
			ssl_errstr = "Error interno de OpenSSL o error de protocolo";
			break;
		case SSL_ERROR_WANT_READ:
			ssl_errstr = "Funciones OpenSSL han pedido read()";
			break;
		case SSL_ERROR_WANT_WRITE:
			ssl_errstr = "Funciones OpenSSL han pedido write()";
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			ssl_errstr = "OpenSSL ha pedido una resolución X509 que no ha llegado";
			break;
		case SSL_ERROR_SYSCALL:
			ssl_errstr = "Error de sistema";
			break;
		case SSL_ERROR_ZERO_RETURN:
			ssl_errstr = "Error de conexión";
			break;
		case SSL_ERROR_WANT_CONNECT:
			ssl_errstr = "Funciones OpenSSL han pedido connect()";
			break;
		default:
			ssl_errstr = "Error OpenSSL desconocido";
	}
#ifdef DEBUG
    	Debug("Falla usando %s: %s", ssl_func, ssl_errstr);
#endif
	if (errtmp)
		SET_ERRNO(errtmp);
	else
		SET_ERRNO(P_EIO);
	return -1;
}
#endif
