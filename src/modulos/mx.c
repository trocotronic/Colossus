/*
 * $Id: mx.c,v 1.2 2004-09-11 16:08:05 Trocotronic Exp $ 
 */

#include <windows.h>
#include <windns.h>
__declspec(dllexport) char *MX(char *dominio)
{
	PDNS_RECORD record;
	if (!dominio)
		return NULL;
	if (!DnsQuery_A(dominio, DNS_TYPE_MX, DNS_QUERY_STANDARD, NULL, &record, NULL))
		return (char *)record->Data.MX.pNameExchange;
	return NULL;
}
