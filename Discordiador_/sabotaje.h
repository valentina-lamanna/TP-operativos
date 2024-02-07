/*
 * sabotaje.h
 *
 *  Created on: Jun 17, 2021
 *      Author: utnso
 */

#ifndef SABOTAJE_H_
#define SABOTAJE_H_

#include "utils-main.h"

void esperarARecibirSabotajes(void);
void recibirSenialDeSabotaje(void);
void manejarSabotaje(Tripulante*);
void moverAColaDeBlockEmergencia(void);
void finalizarSabotaje(void);
Tripulante* calcularTripMasCercanoA(uint32_t, uint32_t);
uint32_t calcularCantidadDeMovimientos(uint32_t, uint32_t, uint32_t, uint32_t);
void mandarMensajeSabotaje(char*);
void moverEInformar(Tripulante*, uint32_t, uint32_t);

//----------VARIABLES "GLOBALES" DE SABOTAJES----------//
t_list* execOrdenado;
t_list* readyOrdenado;

uint32_t posXsabotaje;
uint32_t posYsabotaje;

int socketSabotaje;
int cantTripsEnReadyYExec;

bool sabotajeResuelto;
//-----------------------------------------------------//

#endif /* SABOTAJE_H_ */
