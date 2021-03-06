/*
 * $Id: ssl.c,v 1.18 2008/01/21 19:46:46 Trocotronic Exp $
 */

#ifdef _WIN32
#include <io.h>
#endif
#include <sys/stat.h>
#include "struct.h"
#ifdef USA_SSL
#include "ircd.h"
#ifdef _WIN32
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
LRESULT SSLPassDLG(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static StreamIO *stream = NULL;
	switch (Message)
	{
		case WM_INITDIALOG:
			return TRUE;
		case WM_COMMAND:
			stream = (StreamIO*)lParam;
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
			ssl_errstr = "OpenSSL ha pedido una resoluci�n X509 que no ha llegado";
			break;
		case SSL_ERROR_SYSCALL:
			ssl_errstr = "Error de sistema";
			break;
		case SSL_ERROR_ZERO_RETURN:
			ssl_errstr = "Error de conexi�n";
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
		strlcpy(buf, (char *)beforebuf, size);
		return (strlen(buf));
	}
#ifndef _WIN32
	pass = getpass("Contrase�a para la clave privada SSL: ");
#else
	pass = passbuf;
	stream.buffer = &pass;
	stream.size = &passsize;
	DialogBoxParam(hInst, "SSLPass", hwMain, (DLGPROC)SSLPassDLG, (LPARAM)&stream);
#endif
	if (pass)
	{
		bzero(buf, size);
		bzero(beforebuf, sizeof(beforebuf));
		strlcpy(buf, (char *)pass, size);
		strlcpy(beforebuf, (char *)pass, sizeof(beforebuf));
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
	if (conf_ssl && conf_ssl->opts & SSLFLAG_VERIFYCERT)
	{
		if (verify_err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT)
		{
			if (!(conf_ssl->opts & SSLFLAG_DONOTACCEPTSELFSIGNED))
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
	SSL_CTX_set_verify(ctx_server, SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE | (conf_ssl && conf_ssl->opts & SSLFLAG_FAILIFNOCERT ? SSL_VERIFY_FAIL_IF_NO_PEER_CERT : 0), SSLComprueba);
	SSL_CTX_set_session_cache_mode(ctx_server, SSL_SESS_CACHE_OFF);
	if (conf_ssl)
	{
		if (SSL_CTX_use_certificate_chain_file(ctx_server, SSL_SERVER_CERT_PEM) <= 0)
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
	if (conf_ssl)
	{
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
}
void SSLKeysInit()
{
	if (!SQLCogeRegistro(SQL_CONFIG, "PKey", "valor") || !SQLCogeRegistro(SQL_CONFIG, "PBKey", "valor"))
	{
		RSA *pvkey = NULL;
		BIO *bio;
		char *bptr;
		long blen;
		char tmp[BUFSIZE];
		pvkey = RSA_generate_key(2048, 0x10001, NULL, NULL);
		bio = BIO_new(BIO_s_mem());
		//PEM_write_bio_RSAPrivateKey(bio, pvkey, NULL, cpid, strlen(cpid), NULL, cpid);
		PEM_write_bio_RSAPrivateKey(bio, pvkey, NULL, NULL, 0, NULL, NULL);
		blen = BIO_get_mem_data(bio, &bptr);
		strlcpy(tmp, bptr, blen);
		SQLInserta(SQL_CONFIG, "PKey", "valor", Encripta(tmp, cpid));
		BIO_free(bio);
		bio = BIO_new(BIO_s_mem());
		PEM_write_bio_RSA_PUBKEY(bio, pvkey);
		blen = BIO_get_mem_data(bio, &bptr);
		strlcpy(tmp, bptr, blen);
		SQLInserta(SQL_CONFIG, "PBKey", "valor", tmp);
		BIO_free(bio);
	}
}
void SSLInit(void)
{
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	if (conf_ssl)
	{
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
	}
	CTXInitServer();
	CTXInitCliente();
	/*{
		RSA *pvkey = NULL;
		BIO * bio_out = NULL, *in;
		char * bio_mem_ptr;
		long bio_mem_len;
		char *data = "tralari", *cbuf, *dbuf, *ebuf, *c;
		int clen, dlen, len, fd;
		struct stat sb;
		FILE *fp;
		pvkey = RSA_generate_key(2048, 0x10001, NULL, NULL);
		//EVP_PKEY_assign_RSA(pvkey, );
		bio_out = BIO_new(BIO_s_mem());
		PEM_write_bio_RSAPrivateKey(bio_out, pvkey, EVP_des_ede3_cbc(), NULL, 0, NULL, "ble");
		bio_mem_len = BIO_get_mem_data(bio_out, &bio_mem_ptr);
		strlcpy(buf, bio_mem_ptr, bio_mem_len);
		Debug ("***** %s",buf);
		BIO_free(bio_out);
		bio_out = BIO_new(BIO_s_mem());
		PEM_write_bio_RSA_PUBKEY(bio_out, pvkey);
		bio_mem_len = BIO_get_mem_data(bio_out, &bio_mem_ptr);
		strlcpy(tokbuf, bio_mem_ptr, bio_mem_len);
		Debug ("!! %s",tokbuf);
		BIO_free(bio_out);
		in = BIO_new_mem_buf(tokbuf, strlen(tokbuf));
		RSA_free(pvkey);
		pvkey = PEM_read_bio_RSA_PUBKEY(in, NULL,NULL, NULL);
		fd = open("src/modulos/nickserv.dll", O_RDONLY|O_BINARY);
		fstat(fd, &sb);
		clen = RSA_size(pvkey);
		cbuf = (char *)Malloc(clen + 1);
		c = (char *)Malloc(sizeof(char) * (sb.st_size + 1));
		ebuf = c;
		read(fd, c, sb.st_size);
		close(fd);
		fp = fopen("nickserv.dll", "wb");
		dlen = sb.st_size;
		while (dlen > 0)
		{
			clen = RSA_public_encrypt(MIN(clen-11,dlen), c, cbuf, pvkey, RSA_PKCS1_PADDING);
			fwrite(cbuf, 1, clen, fp);
			dlen -= clen-11;
			c += clen-11;
		}
		fclose(fp);
		Free(ebuf);
		BIO_free(in);
		in = BIO_new_mem_buf(buf, strlen(buf));
		RSA_free(pvkey);
		pvkey = PEM_read_bio_RSAPrivateKey(in, NULL,NULL, "ble");
		//dlen = RSA_size(pvkey);
		fd = open("nickserv.dll", O_RDONLY|O_BINARY);
		fstat(fd, &sb);
		c = (char *)Malloc(sizeof(char) * (sb.st_size + 1));
		read(fd, c, sb.st_size);
		close(fd);
		fp = fopen("nickserv.dec.dll", "wb");
		len = sb.st_size;
		while (len > 0)
		{
			dlen = RSA_private_decrypt(MIN(clen,len), c, cbuf, pvkey, RSA_PKCS1_PADDING);
			fwrite(cbuf, 1, dlen, fp);
			len -= clen;
			c += clen;
		}
		fclose(fp);
		RSA_free(pvkey);
		BIO_free(in);
	}*/
}
void	SSLSockNoBlock(SSL *s)
{
	BIO_set_nbio(SSL_get_rbio(s),1);
	BIO_set_nbio(SSL_get_wbio(s),1);
}
char *SSLCifrado(SSL *ssl)
{
	static char buf[400];
	char tmp[128];
	int bits;
	SSL_CIPHER *c;
	buf[0] = '\0';
	strlcpy(buf, SSL_get_version(ssl), sizeof(buf));
	strlcat(buf, "-", sizeof(buf));
	strlcat(buf, SSL_get_cipher(ssl), sizeof(buf));
	c = SSL_get_current_cipher(ssl);
	SSL_CIPHER_get_bits(c, &bits);
	strlcat(buf, "-", sizeof(buf));
	ircsprintf(tmp, "%i", bits);
	strlcat(buf, tmp, sizeof(buf));
	strlcat(buf, "bits", sizeof(buf));
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
	if (!(sck->ssl = SSL_new(ctx_client)))
		return -1;
	SSL_set_fd(sck->ssl, sck->pres);
	SSL_set_connect_state(sck->ssl);
	SSLSockNoBlock(sck->ssl);
	if (SockIrcd == sck && conf_ssl && conf_ssl->cifrados)
	{
		if (SSL_set_cipher_list((SSL *)sck->ssl, conf_ssl->cifrados) == 0)
			return -2;
	}
	SetSSL(sck);
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
			ssl_errstr = "OpenSSL ha pedido una resoluci�n X509 que no ha llegado";
			break;
		case SSL_ERROR_SYSCALL:
			ssl_errstr = "Error de sistema";
			break;
		case SSL_ERROR_ZERO_RETURN:
			ssl_errstr = "Error de conexi�n";
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
