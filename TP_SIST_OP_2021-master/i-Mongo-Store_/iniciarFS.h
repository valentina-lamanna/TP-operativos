/*
 * iniciarFS.h
 *
 *  Created on: May 25, 2021
 *      Author: utnso
 */


#ifndef INICIARFS_H_
#define INICIARFS_H_


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#include <math.h> // para la funcion ceil()

#include "utils-i-mongo-store.h"

typedef struct {
	int size;
	int blockCount;
	int* blocks;
	char* caracterLlenado;
	char* md5Archivo;


}t_file;

typedef struct{
	int size;
	int* blocks;
}t_bitacora;


void iniciar_FS();
bool isDir(const char*);
void crear_directorios(char*);
char* crear_superbloque(char*);
void setearBitarrayEn0(t_bitarray*,int);
char* crear_blocks(char*);
void crearFile(char* ,char* );
void crearBitacora(int);
void traer_superbloque_existente();

//-------------------BITMAP
int proximoBlockLibre();
bool bitmapNoCompleto();
void liberarBitmap(int );
void ocuparBitmap(int);
//void crear_bitmap(char*, uint32_t);
//t_bitarray* obtener_bitmap(char*);

//------------------BLOCK
void create_file_with_size(char* ,int );
int sobranteBlock(int);
char* leerBlock(int );
char* leerUltimoBlock(int , int );
void escrbirblock(int , char ,int);
void sincronizarMMAp();
void sincronizacion();

//-------------FILE
void escribirFile(char*,int);
void borrarDelFile(char* tarea, int veces);
int size_char_asterisco(char**);
t_file leer_files(char*); //lee la config
void agregarOtroBlockFile(int , char* );
int borrarBlock_devolverNuevoUltimoBlock(char*);
int ultimoBlock(t_list*, int);
void cambiarSizeFile(int , char* );
char* obtenerFileCompleto(char*,char*);
char* relacionarTareaConCaracter(char*);
char* obtenerMD5(char* , char* ,int);
void settearMD5(char* , char* );
int get_size_by_fd(int);

//-----------BITACORA
char* obtenerBitacora(int);
t_bitacora leerBitacoraConfig(char*);
void escrbirBitacora(int ,char* );
void agregarNuevoBlockBitacora(int , char* );
void cambiarSizeBitacora(int , char*);

//------------EXTRA
char* concatenar(char*, char* );
char** agregar(char**, int, int);
int size_char_doble(char**);
char* convertir_array_string(int* , int );
char* list_to_string_array(t_list*);
t_list* chardoble_to_tlist(char**);
//pthread_mutex_t obtenerSemaforoBitacoras(t_dictionary* , char*);
//pthread_mutex_t obtenerSemaforoFiles(t_dictionary* , char*);


#endif /* INICIARFS_H_ */
