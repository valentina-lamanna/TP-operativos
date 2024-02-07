/*
 * mapa.h
 *
 *  Created on: May 23, 2021
 *      Author: utnso
 */

#ifndef MAPA_H_
#define MAPA_H_

#include "mi-ram-hq.h"



int cols, rows;
int err;

//funciones
void ASSERT_CREATE(char, int);
void dibujarMapa();
void actualizarMapa();
void dibujarTripulante(uint32_t, uint32_t, uint32_t);
void actualizarUbicacionTripEnMapa(uint32_t, uint32_t, uint32_t);
void eliminarTripDelMapa(uint32_t);

//finalizar mapa está creado en el finalizador.c que igual no se usa xd

#endif /* MAPA_H_ */
