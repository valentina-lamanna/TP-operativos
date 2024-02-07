/*
 * sabotaje.h
 *
 *  Created on: Jun 25, 2021
 *      Author: utnso
 */

#ifndef SABOTAJE_H_
#define SABOTAJE_H_

#include "utils-i-mongo-store.h"
#include "iniciarFS.h"

void esperarSabotaje();
void sabotear();
void avisarADiscordiadorSab();
void buscarElError();

//SUPERBLOQUE--------------------------------------------------------------------------------------------
//TAMAÑO DE FILES----------------------------------------------------------------------------------------
bool cantBlocksSB();
uint32_t obtenerBlocksDelSB();
char* obtenerBitArray();
//BITMAP----------------------------------------------------------------------------------------
bool bitmapSB();



//FILE-------------------------------------------------------------------------------------------------
//SIZE--------------------------------------------------------------------------------------------------
bool verificarSizeFiles();
char* devolverFilePath(char*);
int tamReal(char* , char* );
//BLOCK COUNT FILE----------------------------------------------------------------------------------------
bool blockCountFiles();
t_file obetnerDatosDelFile(char*);
// VALIDAR BLOCKS FILE--------------------------------------------------------------------------------------
int size_int_asterisco(int*);
t_list* int_asterisco_to_tlist(int*);
bool validarBlocksFile();
bool comparcionMD5(char*);
void arreglarOrdenBlocks(int*, char, char*);
// cambios en los files --------------------------------------------------------------------------------
void actualizarFile(char* , char*, int );
void actualizarFile2(char*, char* , char*);
void cambiarMD5(char* ,char* );

#endif /* SABOTAJE_H_ */

