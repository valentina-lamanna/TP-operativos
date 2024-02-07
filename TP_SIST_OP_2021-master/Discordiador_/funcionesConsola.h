/*
 * funsionesConsola.h
 *
 *  Created on: May 24, 2021
 *      Author: utnso
 */

#ifndef FUNCIONESCONSOLA_H_
#define FUNCIONESCONSOLA_H_

//#include "utils-main.h"
#include "planificacion.h"

Patota* crearPatota(uint32_t, int, char*);
void crearHiloTripulante(Patota*, uint32_t, char*, int);
void accionesCrearTripulante(Tripulante*, int);
void borrarDePatota(uint32_t, Patota);
int encontrarIndexEnPatota(uint32_t, Patota*);
char** asignarDireccDeMemoria(char**, int);



#endif /* FUNCIONESCONSOLA_H_ */
