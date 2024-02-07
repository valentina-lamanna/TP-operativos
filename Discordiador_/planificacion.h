/*
 * planificacion.h
 *
 *  Created on: May 24, 2021
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include "utils-main.h"

void crear_planificacion(void);
void ejecutarAlgoritmo(char*);
void pasarTripAExec(Tripulante*);
void ejecutarTarea(Tripulante*);
void cambiarDeEstado(t_list*, t_list*, Tripulante*, char);
void actualizarCharEstado(Tripulante*, char);
void eliminarDeEstado(t_list*, Tripulante*);
void agregarAEstado(t_list*, Tripulante*);
void irABlockedIO(Tripulante*);
void manejarDesalojo(Tripulante*);
void desalojar(Tripulante*);
void contarQuantum(Tripulante*);
void ejecutarTareaEnExec(Tripulante*);
void informarYHacerMovimientoDePosiciones(Tripulante*);
void matarTripYCancelarHiloSiCorresponde(Tripulante*);
void expulsarTripulante(Tripulante*, bool);

#endif /* PLANIFICACION_H_ */
