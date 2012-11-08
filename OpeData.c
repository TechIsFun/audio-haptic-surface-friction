#include "OpeData.h"

/**
 * Write data values in a OpeData struct
 * @param data		the OpeData struct
 * @param nForce	square root of the product velocity * normal force
 * @param velocita	current velocity
 * @param kappa		stiffness parameter
 * @param depth		surface depth coefficient
 * @param statFr	static friction coefficient
 * @param dynFr		dynamic friction coefficient
 * @param noiseFile	index of the noise file
 */
void setOpeData(OpeData *data, double nForce, double velocita, float kappa, float depth, float statFr, float dynFr, int noiseFile)
{
	data->nForce=nForce;
	data->velocita=velocita;
	data->kappa=kappa;
	data->depth=depth;
	data->statFr=statFr;
	data->dynFr=dynFr;
	data->noiseFile=noiseFile;
}

/**
 * Print out values contained in an OpeData struct
 * @param data the OpeData struct
 */
void printOpeData(OpeData *data)
{
	printf("Forza Normale %f\n",data->nForce);
	printf("Velocita %f\n",data->velocita);
	printf("Kappa %f\n",data->kappa);
	printf("Depth %f\n",data->depth);
	printf("statFr %f\n",data->statFr);
	printf("dynFr %f\n",data->dynFr);
	printf("NoiseFile %d\n\n",data->noiseFile);
}
