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
