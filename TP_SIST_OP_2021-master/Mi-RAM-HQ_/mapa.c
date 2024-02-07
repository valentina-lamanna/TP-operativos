/*
 * mapa.c
 *
 *  Created on: May 23, 2021
 *      Author: utnso
 */

#include "mapa.h"

void ASSERT_CREATE(char id, int err){
    if(err) {
        nivel_destruir(mapa);
        nivel_gui_terminar();
        fprintf(stderr, "Error al crear '%c': %s\n", id, nivel_gui_string_error(err));
    }
}

void dibujarMapa(){

	int inicio_mapa = nivel_gui_inicializar();

	nivel_gui_get_area_nivel(&cols, &rows);

	if(inicio_mapa == NGUI_ALREADY_INIT){
		log_warning(logger, "Ojo, el mapa ya fue inicializado");
	}

	mapa = nivel_crear("mapa");
	nivel_gui_dibujar(mapa);
}

void actualizarMapa()
{
	pthread_mutex_lock(&mutexMapa);
    nivel_gui_dibujar(mapa);
    pthread_mutex_unlock(&mutexMapa);
}

void dibujarTripulante(uint32_t TID, uint32_t posX, uint32_t posY)
{
	char tid = uintToChar(TID);
	int posx = (int) posX;
	int posy = (int) posY;

	err = personaje_crear(mapa, tid, posx , posy);

	ASSERT_CREATE(tid,err);
	actualizarMapa();

}

void actualizarUbicacionTripEnMapa(uint32_t TID, uint32_t posActualizadaX, uint32_t posActualizadaY)
{
	char tid = uintToChar(TID);
	int posActx = (int) posActualizadaX;
	int posActy = (int) posActualizadaY;

	err = item_mover(mapa, tid, posActx , posActy);

	if(err == NGUI_ITEM_NOT_FOUND){
		log_warning(logger, "No existe el item %c en el mapa", tid);
	}
	else if(err == NGUI_ITEM_INVALID_POSITION){
		log_warning(logger, "La posicion %d|%d del item %c es invalida en el mapa", posActx, posActy, tid);
	}

	actualizarMapa();
}

void eliminarTripDelMapa(uint32_t TID)
{
	char tid = uintToChar(TID);

	item_borrar(mapa, tid);
	actualizarMapa();
}



