#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Flight.h"
#include "fileHelper.h"

#define DEFAULT_YEAR 2021
#define ASCII_A 65

void	initFlight(Flight* pFlight, const AirportManager* pManager)
{
	Airport* pPortOr = setAiportToFlight(pManager, "Enter name of origin airport:");
	pFlight->nameSource = _strdup(pPortOr->name);
	int same;
	Airport* pPortDes;
	do {
		pPortDes = setAiportToFlight(pManager, "Enter name of destination airport:");
		same = isSameAirport(pPortOr, pPortDes);
		if (same)
			printf("Same origin and destination airport\n");
	} while (same);
	pFlight->nameDest = _strdup(pPortDes->name);
	initPlane(&pFlight->thePlane);
	getCorrectDate(&pFlight->date);
}

int		isFlightFromSourceName(const Flight* pFlight, const char* nameSource)
{
	if (strcmp(pFlight->nameSource, nameSource) == 0)
		return 1;
		
	return 0;
}


int		isFlightToDestName(const Flight* pFlight, const char* nameDest)
{
	if (strcmp(pFlight->nameDest, nameDest) == 0)
		return 1;

	return 0;


}

int		isPlaneCodeInFlight(const Flight* pFlight, const char*  code)
{
	if (strcmp(pFlight->thePlane.code, code) == 0)
		return 1;
	return 0;
}

int		isPlaneTypeInFlight(const Flight* pFlight, ePlaneType type)
{
	if (pFlight->thePlane.type == type)
		return 1;
	return 0;
}


void	printFlight(const Flight* pFlight)
{
	printf("Flight From %s To %s\t",pFlight->nameSource, pFlight->nameDest);
	printDate(&pFlight->date);
	printPlane(&pFlight->thePlane);
}

void	printFlightV(const void* val)
{
	const Flight* pFlight = *(const Flight**)val;
	printFlight(pFlight);
}


Airport* setAiportToFlight(const AirportManager* pManager, const char* msg)
{
	char name[MAX_STR_LEN];
	Airport* port;
	do
	{
		printf("%s\t", msg);
		myGets(name, MAX_STR_LEN,stdin);
		port = findAirportByName(pManager, name);
		if (port == NULL)
			printf("No airport with this name - try again\n");
	} while(port == NULL);

	return port;
}

void	freeFlight(Flight* pFlight)
{
	free(pFlight->nameSource);
	free(pFlight->nameDest);
	free(pFlight);
}


int saveFlightToFile(const Flight* pF, FILE* fp)
{
	if (!writeStringToFile(pF->nameSource, fp, "Error write flight source name\n"))
		return 0;

	if (!writeStringToFile(pF->nameDest, fp, "Error write flight destination name\n"))
		return 0;

	if (!savePlaneToFile(&pF->thePlane,fp))
		return 0;

	if (!saveDateToFile(&pF->date,fp))
		return 0;

	return 1;
}

//function saves flight to compressed file by given assumptions
int saveFlightToCompressedFile(Flight* pF, FILE* fp)
{
	BYTE data[6] = { 0 };

	int srcNameLength = (int)strlen(pF->nameSource);
	int destNameLength = (int)strlen(pF->nameDest);
	int planecode0 = ((int)pF->thePlane.code[0]) - ASCII_A;
	int planecode1 = ((int)pF->thePlane.code[1]) - ASCII_A;
	int planecode2 = ((int)pF->thePlane.code[2]) - ASCII_A;
	int planecode3 = ((int)pF->thePlane.code[3]) - ASCII_A;
	data[0] = srcNameLength << 3 | (destNameLength >> 2);
	data[1] = ((destNameLength & 0x3) << 6) | (pF->thePlane.type << 4);
	data[1] = data[1] | pF->date.month;
	data[2] = (planecode0 << 3) | (planecode1 >> 2);
	data[3] = ((planecode1 & 0x3) << 6) | (planecode2 << 1);
	data[3] = data[3] | (planecode3 >> 4);
	int regYear = pF->date.year - 2021;
	data[4] = ((planecode3 & 0xf) << 4) | (regYear & 0xf);
	data[5] = pF->date.day & 0x1f;

	if (fwrite(&data, sizeof(BYTE), 6, fp) != 6)
		return 0;
	if (fwrite(pF->nameSource, sizeof(char), srcNameLength, fp) != srcNameLength)
		return 0;
	if (fwrite(pF->nameDest, sizeof(char), destNameLength, fp) != destNameLength)
		return 0;

	return 1;
}


int loadFlightFromFile(Flight* pF, const AirportManager* pManager, FILE* fp)
{

	pF->nameSource = readStringFromFile(fp, "Error reading source name\n");
	if (!pF->nameSource)
		return 0;

	if (findAirportByName(pManager, pF->nameSource) == NULL)
	{
		printf("Airport %s not in manager\n", pF->nameSource);
		free(pF->nameSource);
		return 0;
	}

	pF->nameDest = readStringFromFile(fp, "Error reading destination name\n");
	if (!pF->nameDest)
	{
		free(pF->nameSource);
		return 0;
	}

	if (findAirportByName(pManager, pF->nameDest) == NULL)
	{
		printf("Airport %s not in manager\n", pF->nameDest);
		free(pF->nameSource);
		free(pF->nameDest);
		return 0;
	}

	if (!loadPlaneFromFile(&pF->thePlane, fp))
	{
		free(pF->nameSource);
		free(pF->nameDest);
		return 0;
	}


	if (!loadDateFromFile(&pF->date, fp))
	{
		free(pF->nameSource);
		free(pF->nameDest);
		return 0;
	}

	return 1;
}

//funciton loads flight from compressed file by asamptions given
int loadFlightFromCompressedFile(Flight* pF, const AirportManager* pManager, FILE* fp)
{
	BYTE data[6];
	if (fread(&data, sizeof(BYTE), 6, fp) != 6)
		return 0;

	pF->thePlane.type = (ePlaneType)((data[1] >> 4) & 0x7); //read plane type
	//read plane code
	pF->thePlane.code[0] = (char)(ASCII_A + ((data[2] >> 3) & 0x1f)); 
	pF->thePlane.code[1] = (char)(ASCII_A + (((data[2] & 0x7) << 2) | ((data[3] >> 6) & 0x3)));
	pF->thePlane.code[2] = (char)(ASCII_A + ((data[3] >> 1) & 0x1f));
	pF->thePlane.code[3] = (char)(ASCII_A + (((data[3] & 0x1) << 4) | ((data[4] >> 4) & 0xf)));
	pF->thePlane.code[4] = '\0';

	//read date
	pF->date.day = data[5] & 0x1f;
	pF->date.month = data[1] & 0xf;
	pF->date.year = DEFAULT_YEAR + (data[4] & 0xf);

	int srcNameLength = ((data[0] >> 3) & 0x1f);  //read src Name Length
	int destNameLength = ((data[0] & 0x7) << 2) | ((data[1] >> 6) & 0x3); //read dest name length
	pF->nameSource = (char*)calloc(srcNameLength + 1, sizeof(char));
	if (!pF->nameSource)
		return 0;

	if (fread(pF->nameSource, sizeof(char), srcNameLength, fp) != srcNameLength) // read airport src name
	{
		free(pF->nameSource);
		return 0;
	}
	strcat(pF->nameSource, "\0");

	pF->nameDest = (char*)calloc(destNameLength + 1, sizeof(char));
	if (!pF->nameDest)
	{
		free(pF->nameSource);
		return 0;
	}
	if (fread(pF->nameDest, sizeof(char), destNameLength, fp) != destNameLength) // read airport dest name
	{
		free(pF->nameSource);
		free(pF->nameDest);
		return 0;
	}
	strcat(pF->nameDest, "\0");
	return 1;
}

int	compareFlightBySourceName(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->nameSource, pFlight2->nameSource);
}

int	compareFlightByDestName(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->nameDest, pFlight2->nameDest);
}

int	compareFlightByPlaneCode(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;
	return strcmp(pFlight1->thePlane.code, pFlight2->thePlane.code);
}

int		compareFlightByDate(const void* flight1, const void* flight2)
{
	const Flight* pFlight1 = *(const Flight**)flight1;
	const Flight* pFlight2 = *(const Flight**)flight2;


	return compareDate(&pFlight1->date, &pFlight2->date);
	

	return 0;
}

