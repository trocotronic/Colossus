/*
 * $Id: core.c,v 1.1 2007-01-18 12:46:27 Trocotronic Exp $ 
 */

#include "struct.h"
#include "ircd.h"
#include "modulos.h"
#include "socksint.h"

char *ExMalloc(size_t size, int bz, char *file, long line)
{
	void *x;
	x = malloc(size);
#ifdef DEBUG
	Debug("Dando direccion %X (%s:%i)", x, file, line);
#endif
	if (!x)
	{
		Alerta(FERR,  "[%s:%i] Te has quedado sin memoria", file, line);
		exit(-1);
	}
	if (bz)
		memset(x, 0, size);
	return x;
}

#ifdef _WIN32
typedef enum _STORAGE_QUERY_TYPE {
    	PropertyStandardQuery = 0, 
    	PropertyExistsQuery, 
   	PropertyMaskQuery, 
    	PropertyQueryMaxDefined 
} STORAGE_QUERY_TYPE;
typedef enum _STORAGE_PROPERTY_ID {
    	StorageDeviceProperty = 0,
    	StorageAdapterProperty
} STORAGE_PROPERTY_ID;
typedef struct _STORAGE_PROPERTY_QUERY 
{
    	STORAGE_PROPERTY_ID PropertyId;
    	STORAGE_QUERY_TYPE QueryType;
    	UCHAR AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY;
typedef struct _STORAGE_DEVICE_DESCRIPTOR 
{
   	ULONG Version;
   	ULONG Size;
   	UCHAR DeviceType;
   	UCHAR DeviceTypeModifier;
	BOOLEAN RemovableMedia;
	BOOLEAN CommandQueueing;
    	ULONG VendorIdOffset;
    	ULONG ProductIdOffset;
	ULONG ProductRevisionOffset;
    	ULONG SerialNumberOffset;
    	STORAGE_BUS_TYPE BusType;
    	ULONG RawPropertiesLength;
    	UCHAR RawDeviceProperties[1];

} STORAGE_DEVICE_DESCRIPTOR;
#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
char *flipAndCodeBytes (char *str)
{
	static char flipped[1000];
	int i = 0;
	int j = 0;
	int k = 0;
	int num = strlen (str);
	strlcpy(flipped, "", sizeof(flipped));
	for (i = 0; i < num; i += 4)
	{
		for (j = 1; j >= 0; j--)
		{
			int sum = 0;
			for (k = 0; k < 2; k++)
			{
				sum *= 16;
				switch (str[i + j * 2 + k])
				{
					case '0': 
						sum += 0; 
						break;
					case '1': 
						sum += 1; 
						break;
					case '2': 
						sum += 2; 
						break;
					case '3': 
						sum += 3; 
						break;
					case '4': 
						sum += 4; 
						break;
					case '5': 
						sum += 5; 
						break;
					case '6': 
						sum += 6; 
						break;
					case '7': 
						sum += 7; 
						break;
					case '8': 
						sum += 8; 
						break;
					case '9': 
						sum += 9; 
						break;
					case 'A': 
					case 'a': 
						sum += 10; 
						break;
					case 'B':
					case 'b': 
						sum += 11; 
						break;
					case 'C':
					case 'c': 
						sum += 12; 
						break;
					case 'D':
					case 'd': 
						sum += 13; 
						break;
					case 'E':
					case 'e': 
						sum += 14; 
						break;
					case 'F':
					case 'f': 
						sum += 15; 
						break;
				}
			}
			if (isalnum((char)sum)) 
			{
				char sub[2];
				sub[0] = (char) sum;
				sub[1] = 0;
				strlcat(flipped, sub, sizeof(flipped));
			}
		}
	}
	return flipped;
}
#endif
void CpuId()
{
#ifdef _WIN32
	/* todas estas rutinas y estructuras se han sacado de http://www.winsim.com/diskid32/diskid32.cpp */
	HANDLE hPhysicalDriveIOCTL = 0;
	bzero(sgn.cpuid, sizeof(sgn.cpuid));
	hPhysicalDriveIOCTL = CreateFile("\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhysicalDriveIOCTL != INVALID_HANDLE_VALUE)
	{
		STORAGE_PROPERTY_QUERY query;
         	DWORD cbBytesReturned = 0;
		char buffer[10000];
		memset ((void *)&query, 0, sizeof (query));
		query.PropertyId = StorageDeviceProperty;
		query.QueryType = PropertyStandardQuery;
		memset(buffer, 0, sizeof(buffer));
		if (DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &buffer, sizeof(buffer), &cbBytesReturned, NULL))
		{         
			STORAGE_DEVICE_DESCRIPTOR *descrip = (STORAGE_DEVICE_DESCRIPTOR *)&buffer;
			char serialNumber[1000], modelNumber[1000];
			strlcpy(serialNumber, flipAndCodeBytes(&buffer[descrip->SerialNumberOffset]), sizeof(serialNumber));
			strlcpy(modelNumber, &buffer[descrip->ProductIdOffset], sizeof(modelNumber));
			if (isalnum(serialNumber[0]) || isalnum(serialNumber[19]))
				strlcpy(sgn.cpuid, serialNumber, sizeof(sgn.cpuid));
         	}
	}
#else
	int r, s;
	struct ifreq ifr;
	char *hwaddr;
	strlcpy(ifr.ifr_name, "eth0", sizeof(ifr.ifr_name));
	bzero(sgn.cpuid, sizeof(sgn.cpuid));
	if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) > 0)
	{
		if ((r = ioctl( s, SIOCGIFHWADDR, &ifr )) >= 0)
		{
			hwaddr = ifr.ifr_hwaddr.sa_data;
			snprintf(sgn.cpuid, 13, "%02X%02X%02X%02X%02X%02X", 
				hwaddr[5] & 0xFF, hwaddr[3] & 0xFF,
				hwaddr[1] & 0xFF, hwaddr[4] & 0xFF,
				hwaddr[6] & 0xFF, hwaddr[2] & 0xFF);
		}
		close(s);
	}
#endif
}
void EscribePid()
{
#ifdef PID
	int  fd;
	char buff[20];
	if ((fd = open(PID, O_CREAT | O_WRONLY, 0600)) >= 0)
	{
		bzero(buff, sizeof(buff));
		sprintf(buff, "%5i\n", (int)getpid());
		if (write(fd, buff, strlen(buff)) == -1)
			Debug("No se puede escribir el archivo pid %s", PID);
		close(fd);
		return;
	}
#ifdef DEBUG
	else
		Debug("No se puede abrir el archivo pid %s", PID);
#endif
#endif
}
#ifndef _WIN32
int LeePid()
{
	int  fd;
	char buff[20];
	if ((fd = open("colossus.pid.bak", O_RDONLY, 0600)) >= 0)
	{
		bzero(buff, sizeof(buff));
		if (read(fd, buff, sizeof(buff)) == -1)
		{
			Debug("No se puede leer el archivo pid %s (%i)", "colossus.pid.bak", errno);
			return -1;
		}
		close(fd);
		return atoi(buff);
	}
#ifdef DEBUG
	else
		Debug("No se puede abrir el archivo pid %s (%i)", "colossus.pid.bak", errno);
#endif
	return -1;
}
#endif
void BorraTemporales()
{
	Modulo *mod;
	for (mod = modulos; mod; mod = mod->sig)
	{
		//Alerta(FOK,"eliminando %s",mod->tmparchivo);
		unlink(mod->tmparchivo);
	}
}
