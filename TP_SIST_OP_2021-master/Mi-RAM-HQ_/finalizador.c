#include "mi-ram-hq.h"

//------------------HEADER-------------------//
//void liberarTodosLosSegmentos();
void liberarMemoria();
void terminarPrograma(t_log*);
void expulsarTripulante(TCB*, uint32_t);



//-------FUNCIONES LIBERACION DE MEMORIA-------//
/*
void liberarTodosLosSegmentos(){
	list_destroy_and_destroy_elements(memoriaListSeg);
}
*/


void liberarMemoria(){
	free(memoria);
}

void liberarSemaforos(){
	pthread_mutex_destroy(&mutex_segmentos);
	pthread_mutex_destroy(&mutex_memoria);

}

void finalizarMapa()
{
	nivel_destruir(mapa);
	nivel_gui_terminar();
}

//deberiamos terminar el programa?
void terminarPrograma(t_log* logger){
	//liberar_logger(logger);
	liberarMemoria();
	//liberarTodosLosSegmentos();
	liberarSemaforos();
	finalizarMapa();
}


//-------------FUNCION DEL SWITCH-------------//
//al final este archivo nunca se usa

void expulsarTripulante(TCB* tripulante, uint32_t direccTCB){
	if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		Segmento* segmentoDelTCB = list_get(memoriaListSeg,direccTCB);

		segmentoDelTCB->ocupado = 0;
	}else if(strcmp(esquemaMemoria,"PAGINACION") == 0){
		//swap(tripulante, nmro pagina);
	}

	//eliminarTripDelMapa(tripulante->TID);
}
