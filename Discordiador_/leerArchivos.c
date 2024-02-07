/*
 * leerArchivos.c
 *
 *  Created on: May 23, 2021
 *      Author: utnso
 */

#include "main.h"
#include "fcntl.h"

char* leer_nombreTarea(FILE*);
char* leer_numeros(FILE*);

t_list* leerArchivoDeTareasReturnTareasList(char* pathArchivoTareas) {

	//pathArchivoTareas = "/home/utnso/Desktop/TP-SO/tp-2021-1c-GrupoChicasSistOp2021/tareasPatota.txt"; // esto desp se borra, me lo pasan por param

	FILE* fArchivoTareas = fopen(pathArchivoTareas,"r");

	Tarea* tarea = malloc(sizeof(Tarea) + sizeof(char*));

	t_list* listaTareas;
	listaTareas = list_create();

	char* linea = leer_nombreTarea(fArchivoTareas);

	/*	FORMATO ESPERADO: TAREA PARAMETROS;POSX;POSY;TIEMPO  --> notar: espacio entre tarea y params y ENTER al final del archivo */

	while(strcmp(linea, "FIN") != 0){
		strcpy(&tarea->nombreTarea, &linea);
		tarea->parametros = atoi(leer_numeros(fArchivoTareas));
		tarea->posX = atoi(leer_numeros(fArchivoTareas));
		tarea->posY = atoi(leer_numeros(fArchivoTareas));
		tarea->tiempo = atoi(leer_numeros(fArchivoTareas));

		list_add(listaTareas, tarea);
		tarea = malloc(sizeof(Tarea) + sizeof(char*));
		linea = leer_nombreTarea(fArchivoTareas);
	}

	fclose(fArchivoTareas);

	/*void iteradorDeTareas(Tarea* tarea)
	{
		printf("%s \n",tarea->nombreTarea);
		printf("%d \n",tarea->parametros);
		printf("%d \n",tarea->posX);
		printf("%d \n",tarea->posY);
		printf("%d \n",tarea->tiempo);
	}

	list_iterate(listaTareas, iteradorDeTareas);*/

	return listaTareas;
}

char* leer_nombreTarea(FILE* archivo){

	char* linea = string_new();

	char* a = malloc(2);
	a[1] = '\0';

	if (fread(a,1,1,archivo) != 1){
		return "FIN";
	}

	while (*a!= ' '){
		string_append(&linea, a);
		fread(a,1,1,archivo);
	}

	free(a);

	return linea;
}

char* leer_numeros(FILE* archivo){

	char* linea = string_new();
	char* a = malloc(2);
	a[0] = '0'; // si no lee nada, tiene por default 0
	a[1] = '\0';

	fread(a,1,1,archivo);
	while (*a!= ';'){
		if(strcmp(a,"\n") == 0){
			break;
		}
		string_append(&linea, a);
		fread(a,1,1,archivo);
	}

	free(a);

	return linea;
}

int archivo_obtenerTamanio (char*path){
	FILE* f = fopen(path,"r");
	fseek(f, 0L, SEEK_END);
	int i = ftell(f);
	fclose(f);
	return i;
	/*int fd = open(path, O_RDONLY, 0777);
	struct stat statbuf;
	fstat(fd, &statbuf);
	close(fd);
	return statbuf.st_size;*/
}

char* archivo_leer (char*path){
	FILE*archivo=fopen(path,"r");

	int tamanio= archivo_obtenerTamanio(path);
	char*texto = malloc(tamanio+1);

	fread(texto,tamanio,sizeof(char),archivo);

	fclose(archivo);

	texto[tamanio]='\0';

	return texto;
	// "GENERAR_OXIGENO 12;2;3;5\nCONSUMIR_OXIGENO 120;2;3;1\nGENERAR_COMIDA 4;2;3;1\nCONSUMIR_COMIDA 1;2;3;4\nGENERAR_BASURA 12;2;3;5\nDESCARTAR_BASURA;3;1;7"
}
