/*
 * $Id: zip.c,v 1.2 2004-09-11 16:08:04 Trocotronic Exp $ 
 */

/*
 *   IRC - Internet Relay Chat, ircd/s_zip.c
 *   Copyright (C) 1996  Christophe Kalt
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include "struct.h"
#include <string.h>
#include <stdlib.h>

#ifdef  ZIP_LINKS
#define ZIP_BUFFER_SIZE         (ZIP_MAXIMUM + READBUF_SIZE)

#define UNZIP_BUFFER_SIZE       6 * ZIP_BUFFER_SIZE

/* buffers */
static  char    unzipbuf[UNZIP_BUFFER_SIZE];
static  char    zipbuf[ZIP_BUFFER_SIZE];

int     zip_init(Sock *sck, int compressionlevel)
{
  sck->zip  = (Zip *) Malloc(sizeof(Zip));
  sck->zip->incount = 0;
  sck->zip->outcount = 0;

  sck->zip->in  = (z_stream *) Malloc(sizeof(z_stream));
  bzero(sck->zip->in, sizeof(z_stream)); /* Just to be sure -- Syzop */
  sck->zip->in->total_in = 0;
  sck->zip->in->total_out = 0;
  sck->zip->in->zalloc = NULL;
  sck->zip->in->zfree = NULL;
  sck->zip->in->data_type = Z_ASCII;
  if (inflateInit(sck->zip->in) != Z_OK)
    {
      sck->zip->out = NULL;
      return -1;
    }

  sck->zip->out = (z_stream *) Malloc(sizeof(z_stream));
  bzero(sck->zip->out, sizeof(z_stream)); /* Just to be sure -- Syzop */
  sck->zip->out->total_in = 0;
  sck->zip->out->total_out = 0;
  sck->zip->out->zalloc = NULL;
  sck->zip->out->zfree = NULL;
  sck->zip->out->data_type = Z_ASCII;
  if (deflateInit(sck->zip->out, compressionlevel) != Z_OK)
    return -1;

  return 0;
}

void zip_free(Sock *sck)
{
	if (sck->zip)
	{
		if (sck->zip->in)
			inflateEnd(sck->zip->in);
		Free(sck->zip->in);
		sck->zip->in = NULL;
		if (sck->zip->out) 
			deflateEnd(sck->zip->out);
		Free(sck->zip->out);
		sck->zip->out = NULL;
		Free(sck->zip);
		sck->zip = NULL;
	}
}

char *unzip_packet(Sock *sck, char *buffer, int *length)
{
  z_stream *zin = sck->zip->in;
  int   r;
  char  *p;
  if(sck->zip->incount)
    {
      memcpy((void *)unzipbuf,(void *)sck->zip->inbuf,sck->zip->incount);
      zin->avail_out = UNZIP_BUFFER_SIZE - sck->zip->incount;
      zin->next_out = (Bytef *) (unzipbuf + sck->zip->incount);
      sck->zip->incount = 0;
      sck->zip->inbuf[0] = '\0'; /* again unnecessary but nice for debugger */
    }
  else
    {
      if(!buffer)       /* Sanity test never hurts */
        {
          *length = -1;
          return((char *)NULL);
        }
      zin->next_in = (Bytef *) buffer;
      zin->avail_in = *length;
      zin->next_out = (Bytef *) unzipbuf;
      zin->avail_out = UNZIP_BUFFER_SIZE;
    }

  switch (r = inflate(zin, Z_NO_FLUSH))
    {
    case Z_OK:
      if (zin->avail_in)
        {
          sck->zip->incount = 0;

          if(zin->avail_out == 0)
            {
              Debug("Overflowed unzipbuf increase UNZIP_BUFFER_SIZE");
              if((zin->next_out[0] == '\n') || (zin->next_out[0] == '\r'))
                {
                  sck->zip->inbuf[0] = '\n';
                  sck->zip->incount = 1;
                }
              else
                {
                  for(p = (char *) zin->next_out;p >= unzipbuf;)
                    {
                      if((*p == '\r') || (*p == '\n'))
                        break;
                      zin->avail_out++;
                      p--;
                      sck->zip->incount++;
                    }
                  if(p == unzipbuf)
                    {
                      sck->zip->incount = 0;
                      sck->zip->inbuf[0] = '\0';       /* only for debugger */
                      *length = -1;
                      return((char *)NULL);
                    }
                  p++;
                  sck->zip->incount--;
                  memcpy((void *)sck->zip->inbuf,
                         (void *)p,sck->zip->incount);
                }
            }
          else
            {
              *length = -1;
              return((char *)NULL);
            }
        }

      *length = UNZIP_BUFFER_SIZE - zin->avail_out;
      return unzipbuf;

    case Z_BUF_ERROR:
      if (zin->avail_out == 0)
        {
          Debug("inflate() returned Z_BUF_ERROR: %s",
                      (zin->msg) ? zin->msg : "?");
          *length = -1;
        }
      break;

    case Z_DATA_ERROR: /* the buffer might not be compressed.. */
      if (!strncmp("ERROR ", buffer, 6))
        {
          sck->zip->first = 0;
          return buffer;
        }
        /* no break */

    default: /* error ! */
      /* should probably mark link as dead or something... */
      Debug("inflate() error(%d): %s", r,
                  (zin->msg) ? zin->msg : "?");
      *length = -1; /* report error condition */
      break;
    }
  return((char *)NULL);
}

char *zip_buffer(Sock *sck, char *buffer, int *length, int flush)
{
  z_stream *zout = sck->zip->out;
  int   r;

  if (buffer)
    {
      /* concatenate buffer in sck->zip->outbuf */
      memcpy((void *)(sck->zip->outbuf + sck->zip->outcount), (void *)buffer,
             *length );
      sck->zip->outcount += *length;
    }
  *length = 0;

  if (!flush && (sck->zip->outcount < ZIP_MINIMUM))
    return((char *)NULL);
  zout->next_in = (Bytef *) sck->zip->outbuf;
  zout->avail_in = sck->zip->outcount;
  zout->next_out = (Bytef *) zipbuf;
  zout->avail_out = ZIP_BUFFER_SIZE;
  switch (r = deflate(zout, Z_PARTIAL_FLUSH))
    {
    case Z_OK:
      if (zout->avail_in)
        {
          /* can this occur?? I hope not... */
          Debug("deflate() didn't process all available data!");
        }
      sck->zip->outcount = 0;
      *length = ZIP_BUFFER_SIZE - zout->avail_out;
      return zipbuf;

    default: /* error ! */
      Debug("deflate() error(%d): %s", r, (zout->msg) ? zout->msg : "?");
      *length = -1;
      break;
    }
  return((char *)NULL);
}

#endif  /* ZIP_LINKS */
