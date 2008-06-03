/*
 * $Id: md5.h,v 1.7 2007/01/09 19:08:30 Trocotronic Exp $ 
 */
 
#define MDInit MD5_Init
#define MDUpdate MD5_Update
#define MDFinal MD5_Final
#if !defined(_MD5_H)
#define _MD5_H

#endif

extern char *MDString(char *, unsigned int);
