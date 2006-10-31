/*
 * $Id: zlib.c,v 1.14 2006-10-31 23:49:11 Trocotronic Exp $ 
 */
 
#include "struct.h"
#ifdef USA_ZLIB
#include "zip.h"
#define ZIP_BUFFER_SIZE	ZIP_MAXIMUM + BUFSIZE
#define UNZIP_BUFFER_SIZE	6 * ZIP_BUFFER_SIZE

static char unzipbuf[UNZIP_BUFFER_SIZE];
static char zipbuf[ZIP_BUFFER_SIZE];

int ZLibInit(Sock *sck, int nivel)
{
	sck->zlib = BMalloc(Zlib);
	sck->zlib->in = BMalloc(z_stream);
	sck->zlib->out = BMalloc(z_stream);
	sck->zlib->in->data_type = sck->zlib->out->data_type = Z_ASCII;
	if (inflateInit(sck->zlib->in) != Z_OK)
		return -1;
	if (deflateInit(sck->zlib->out, nivel) != Z_OK)
		return -2;
	return 0;
}
void ZLibLibera(Sock *sck)
{
	UnsetZlib(sck);
	if (sck->zlib)
	{
		if (sck->zlib->in)
		{
			inflateEnd(sck->zlib->in);
			Free(sck->zlib->in);
		}
		if (sck->zlib->out)
		{
			deflateEnd(sck->zlib->out);
			Free(sck->zlib->out);
		}
		Free(sck->zlib);
		sck->zlib = NULL;
	}
}
/*
 * Comprime un mensaje de cola
 * Devuelve el mensaje comprimido y guarda en len su longitud
 */
char *ZLibComprime(Sock *sck, char *mensaje, int *len, int flush)
{
	z_stream *zout = sck->zlib->out;
	int e;
	if (mensaje)
	{
		memcpy((void *)(sck->zlib->outbuf + sck->zlib->outcount), (void *)mensaje, *len);
		sck->zlib->outcount += *len;
	}
	*len = 0;
	if (!flush && (sck->zlib->outcount < ZIP_MINIMUM))
		return NULL;
	zout->next_in = (Bytef *) sck->zlib->outbuf;
	zout->avail_in = sck->zlib->outcount;
	zout->next_out = (Bytef *) zipbuf;
	zout->avail_out = ZIP_BUFFER_SIZE;
	switch ((e = deflate(zout, Z_PARTIAL_FLUSH)))
	{
		case Z_OK:
			if (zout->avail_in)
				Info("deflate() no se puede procesar todos los datos");
        		sck->zlib->outcount = 0;
			*len = ZIP_BUFFER_SIZE - zout->avail_out;
			return zipbuf;
		default:
			Info("deflate() error(%i): %s", e, zout->msg ? zout->msg : "?");
			*len = -1;
			break;
	}
	*len = -1;
	return NULL;
}
char *ZLibDescomprime(Sock *sck, char *mensaje, int *len)
{
	z_stream *zin = sck->zlib->in;
	int e;
	char *p;
	if (sck->zlib->incount)
	{
		memcpy((void *)unzipbuf, (void *)sck->zlib->inbuf, sck->zlib->incount);
		zin->avail_out = UNZIP_BUFFER_SIZE - sck->zlib->incount;
		zin->next_out = (Bytef *) (unzipbuf + sck->zlib->incount);
		sck->zlib->incount = 0;
	}
	else
	{
		zin->next_in = (Bytef *)mensaje;
		zin->avail_in = *len;
		zin->next_out = (Bytef *)unzipbuf;
		zin->avail_out = UNZIP_BUFFER_SIZE;
	}
	switch ((e = inflate(zin, Z_NO_FLUSH)))
	{
		case Z_OK:
			if (zin->avail_in)
       			{
          			sck->zlib->incount = 0;
				if (!zin->avail_out)
				{
					Info("Hay que incrementar UNZIP_BUFFER_SIZE");
					if (zin->next_out[0] == '\n' || zin->next_out[0] == '\r')
					{
						sck->zlib->inbuf[0] = '\n';
						sck->zlib->incount = 1;
                			}
              				else
					{
						for (p = (char *) zin->next_out; p >= unzipbuf;)
						{
							if (*p == '\r' || *p == '\n')
								break;
							zin->avail_out++;
							p--;
							sck->zlib->incount++;
						}
				                if (p == unzipbuf)
						{
							sck->zlib->incount = 0;
							sck->zlib->inbuf[0] = '\0';
							*len = -1;
							return NULL;
						}
						p++;
						sck->zlib->incount--;
						memcpy((void *)sck->zlib->inbuf, (void *)p, sck->zlib->incount);
					}
				}
         			else
            			{
					*len = -1;
					return NULL;
				}
			}
			*len = UNZIP_BUFFER_SIZE - zin->avail_out;
			return unzipbuf;
		case Z_BUF_ERROR:
			if (zin->avail_out == 0)
			{
				Info("inflate() ha devuelto Z_BUF_ERROR: %s", zin->msg ? zin->msg : "?");
				*len = -1;
			}
			break;
		case Z_DATA_ERROR:
			if (!strncmp("ERROR ", mensaje, 6))
			{
				sck->zlib->primero = 0;
				UnsetZlib(sck);
				return mensaje;
			}
			Info("inflate() error: * Quizás esté linkando a un servidor que no tiene ZLib");
        	default:
			Info("inflate() error(%i): %s", e, zin->msg ? zin->msg : "?");
			*len = -1;
      			break;
	}
	*len = -1;
	return NULL;
}
#endif
