/*
 * i-mongo-store.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef I_MONGO_STORE_H_
#define I_MONGO_STORE_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <stdbool.h>
#include<commons/log.h>
#include<pthread.h>



#include "utils-i-mongo-store.h"

void atenderCliente(int);
t_config* iniciar_config(void);
void leerConfig();
void esperarConexion(void);
//void ponerEnDiccionraioFiles(t_dictionary ,char* ,void*);
void ponerEnListaTareas(t_list* ,char*);
void ponerEnListaBitacora(t_list* ,char* );
//bool estaEnEldiccionarioFiles(t_dictionary* ,char* );
bool estaEnListaBitacoras(t_list* ,char* );



#endif /* I_MONGO_STORE_H_ */
