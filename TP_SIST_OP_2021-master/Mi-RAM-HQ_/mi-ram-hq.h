/*
 * mi-ram-hq.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef MI_RAM_HQ_H_
#define MI_RAM_HQ_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include<pthread.h>


#include "utils-mi-ram-hq.h"

// memoria es un void* pero la estructura para segmentacion y paginacion son listas

t_config* iniciar_config(void);
void leerConfig(void);
void inicializarMemoriaSegunEsquema(void);


#endif /* MI_RAM_HQ_H_ */
