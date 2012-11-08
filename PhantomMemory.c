#include "PhantomMemory.h"

/**
 * Create a sharead memory area
 * @return 0 if success
 */
int CreatePhantomMemory()
{
	hFile = CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeMem,(LPCWSTR)"PhantomMemory");//era 0 sizeMem*4
	if (hFile == NULL)
	{
		printf("ERROR: Unable to create a fileMapping.\n");
		return 1;
	}
	hView = MapViewOfFile(hFile,FILE_MAP_ALL_ACCESS,0,0,0);
	if (hView == NULL)
	{
		printf("ERROR: Unable to map a viewOfFile.\n");
		return 2;
	}
	#ifdef DEBUG
	cout<<"SUCCESS: Shared memory has been successfully created.\n";
	#endif 
	return 0;
}

/**
 * Open a previously created shared memory area
 * @return 0 if success
 */ 
int OpenPhantomMemory(void)
{
	hFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,(LPCWSTR)"PhantomMemory");
	if (hFile == NULL)
	{
		printf("ERROR: Unable to open the FileMapping.\n");
		return 3;
	} 
	hView = MapViewOfFile(hFile,FILE_MAP_ALL_ACCESS,0,0,0);
	if (hView == NULL)
	{
		printf("ERROR: Unable to open the FileMapping.\n");
		return 4;
	}
	#ifdef DEBUG
	cout<<"SUCCESS: Shared memory has been successfully opened.\n";
	#endif
	hookOped = (OpeData*)(hView);
	hookSurface = (float*)(hookOped+1);
	return 0;
}

/**
 * Close a previously opened shared memory area
 * @return 0 if success
 */ 
int ClosePhantomMemory(void)
{
	if (!UnmapViewOfFile(hView))
	{   
		printf("ERROR: Could not unmap viewOfFile.\n");
		return 5;
	}
	CloseHandle(hFile);
	return 0;
}

/**
 * Write an OpeData struct in the shared memory area
 * @param elemento the OpeData struct to be written
 */ 
void WriteOpeData(OpeData *elemento)
{
	hookOped->nForce = elemento->nForce;
	hookOped->velocita = elemento->velocita;
	hookOped->kappa = elemento->kappa;
	hookOped->depth = elemento->depth;
	hookOped->statFr = elemento->statFr;
	hookOped->dynFr = elemento->dynFr;
	hookOped->noiseFile = elemento->noiseFile;
}

/**
 * Write the surface value in the shared memory area
 * @param elemento the value to be written
 */ 
void WriteSurface(float *elemento)
{
	*hookSurface = *elemento;
}

/**
 * Read the OpeData struct from shared memory
 */ 
OpeData ReadOpeData()
{
	return *hookOped;
}

/**
 * Read the surface value from shared memory
 */ 
float ReadSurface()
{
	return *hookSurface;
}
