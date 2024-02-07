/*
 * main.h
 *
 *  Created on: May 22, 2021
 *      Author: utnso
 */

#ifndef MAIN_H_
#define MAIN_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<commons/collections/queue.h>
#include<readline/readline.h>
#include<pthread.h>
#include<semaphore.h>

#include "utils-main.h"

t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void terminar_programa();

#endif /* MAIN_H_ */
