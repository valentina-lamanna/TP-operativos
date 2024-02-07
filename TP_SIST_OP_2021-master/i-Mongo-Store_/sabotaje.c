/*
 * sabotaje.c
 *
 *  Created on: Jun 25, 2021
 *      Author: utnso
 */

#include "sabotaje.h"
#include "iniciarFS.h"

void esperarSabotaje(){

		pthread_t sabotaje;
		pthread_create(&sabotaje,NULL, (void*)sabotear, NULL);
		pthread_detach(sabotaje);
		contadorDeSabotajes =-1;

}

void sabotear(){

	while(1){
		sleep(1);
		signal(SIGUSR1,avisarADiscordiadorSab);

	}

}

void avisarADiscordiadorSab(){

	log_info(logger, "Se obtuvo una señal para arreglar un sabotaje \n");


	int tamDePocisiones = size_char_asterisco(posicionesDeSabotaje);

	if(contadorDeSabotajes < tamDePocisiones){
		contadorDeSabotajes ++;
	}else{contadorDeSabotajes =-1; }
	
	
	
	char* posicionSabotaje =posicionesDeSabotaje[contadorDeSabotajes];

	enviar_respuesta_a_paquete(posicionSabotaje, socket_sabotajes, SABOTAJE);
	log_info(logger, "Se le mando a Discordiador de la posicion del sabotaje \n");

	char* rta = recibirUnPaqueteDeSabotajes(socket_sabotajes); // se blockea la ejecucion hasta que por este socket se reciba un mensaje


	if(strcmp(rta, "FSCK") != 0){
		log_error(logger, "El tripulante me mando un codigo incorrecto, por las dudas no resuelvo nada \n");
		free(rta);
		return;
	}

	log_info(logger, "El tripulante ya esta posicionado, comienza el protocolo de File System Catastrophe Killer\n");
	buscarElError();

	enviar_respuesta_a_paquete("listo", socket_sabotajes, SABOTAJE);

}


void buscarElError(){

	//SUPERBLOQUE------------------------------------------------------------------------------------------------

	if(cantBlocksSB()){//check
		//lo arregle adentro
		log_info(logger, "Se encontro el problema y se soluciono. Este era en el Super Bloque en la cantidad de blocks. \n");
		return;
	}

	//FILES-----------------------------------------------------------------------------------------------------
	else if(blockCountFiles()){ // check
		log_info(logger, "Se encontro el problema y se soluciono. Este era en los Files, habia una diferencia en la cantidad de bloques. \n");
		return;
	}


	else if(verificarSizeFiles()){ //check

			log_info(logger, "Se encontro el problema y se soluciono. Este era en los Files, habia una diferencia en el size. \n");
			return;
	}

	else if(validarBlocksFile()){ //md5 //check
			log_info(logger, "Se encontro el problema y se soluciono. Este era en los Files, habia una diferencia en el MD5. \n");
			return;
	}


	//-----------SB -BITMAP
	else if(bitmapSB()){
			//se arregla adentro
			log_info(logger, "Se encontro el problema y se soluciono. Este era en el Super Bloque en el bitmap. \n");
			return;
	}



}


//SUPERBLOQUE--------------------------------------------------------------------------------------------
//TAMAÑO DE FILES----------------------------------------------------------------------------------------
bool cantBlocksSB(){

	uint32_t blocks;
	blocks = obtenerBlocksDelSB();

	if(blocks != cantidad_clusters){

		int tamSB= 2 * sizeof(uint32_t)+ (int)ceil((double)cantidad_clusters/8.0);
		pthread_mutex_lock(&superBloqueSem);
		memcpy(punteroAlSBdeMMAP + sizeof(uint32_t),&cantidad_clusters, sizeof(uint32_t));
		pthread_mutex_unlock(&superBloqueSem);
		return 1;
	}
	log_info(logger, "el problema no fue cantidad de blocks en el SB");
	return 0;
}

uint32_t obtenerBlocksDelSB(){
	pthread_mutex_lock(&superBloqueSem);


		uint32_t blocks ;
		memcpy(&blocks, punteroAlSBdeMMAP + sizeof(uint32_t),sizeof(uint32_t));

		pthread_mutex_unlock(&superBloqueSem);

		return blocks;
}

char* obtenerBitArray(){

	pthread_mutex_lock(&superBloqueSem);

	double tam =ceil((double)cantidad_clusters/8.0);
	char* bitmap= calloc(1,(int)tam);

	memcpy(bitmap, punteroAlSBdeMMAP+2*sizeof(uint32_t),(int)tam);

	pthread_mutex_unlock(&superBloqueSem);

	return bitmap;

}

//BITMAP----------------------------------------------------------------------------------------
t_bitarray* volverAAbrir(){
	double tamEnBytes= ceil((double)cantidad_clusters/8.0);
	int tamDelSuperBloque = 2 * sizeof(uint32_t)+ tamEnBytes;

	void* puntero;
	int fd = open(superBloquePath,  O_RDWR, 0777);
	puntero = mmap(0,tamDelSuperBloque, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	char* bitmap = obtenerBitArray();
	t_bitarray* bitarray;
	bitarray =  bitarray_create_with_mode(bitmap, (int)tamEnBytes, LSB_FIRST);

	msync(punteroAlSBdeMMAP, tamDelSuperBloque, MS_SYNC);

	return bitarray;

}

char* devolverTripPath(char* id ){
	char* realPath =string_new();
	string_append(&realPath,bitacoraPath);
	string_append(&realPath,"/Tripulante");
	string_append(&realPath,id);
	string_append(&realPath,".ims");

	return realPath;


}
bool bitmapSB(){

	for(int i=0; i < cantidad_clusters ; i++){

		int valorBitmap =bitarray_test_bit(bitmapFS,i); //0 vacio , 1 ocupado //"0" "1"

		char* block = leerBlock(i); // funcion de inciarFS

		bool vacio = strcmp(block, ""); // 0 iguales

			if(valorBitmap == 0){
					if(vacio == 0){//realmente esta vacio

					}if(vacio == 1){// false que esta vacio --> esta lleno

						ocuparBitmap(i);


						pthread_mutex_lock(&superBloqueSem);
						double tamEnBytes= ceil((double)cantidad_clusters/8.0);

						memcpy(punteroAlSBdeMMAP + 2*sizeof(uint32_t), bitmapFS->bitarray, tamEnBytes);


						pthread_mutex_unlock(&superBloqueSem);
						return 1;
					}

			}
			else{
				if(vacio == 1){//realmente esta lleno

				}if(vacio == 0){// realmente esta vacio

					liberarBitmap(i);// aunque no se libere es simbolico

					pthread_mutex_lock(&superBloqueSem);
					double tamEnBytes= ceil((double)cantidad_clusters/8.0);

					memcpy(punteroAlSBdeMMAP + 2*sizeof(uint32_t), bitmapFS->bitarray, tamEnBytes);

					pthread_mutex_unlock(&superBloqueSem);
					return 1;
				}
			}


		}

		log_info(logger, "el problema no fue el bitmap en el SB");
		return 0;
}



//FILE-------------------------------------------------------------------------------------------------
//SIZE--------------------------------------------------------------------------------------------------
bool verificarSizeFiles(){

	char* tarea;

	int size = list_size(listaTareas);

	for(int i = 0 ; i < size; i ++){
		
		pthread_mutex_lock(&listaTareasSem);
		tarea = list_get(listaTareas,i);
		pthread_mutex_unlock(&listaTareasSem);

		char*path =devolverFilePath(tarea);
		int tamRealINT = tamReal(path, tarea);

		t_file file= leer_files(path);

		if(tamRealINT != file.size){

			actualizarFile(tarea, "SIZE",tamRealINT);
			free(file.blocks);
			return 1;
		}

		free(file.blocks);

	}

	log_info(logger, "el problema no size files");
	return 0;
}



char* devolverFilePath(char* tarea){
	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");


	return realPath;


}
int tamReal(char* realPath, char* tarea){
	t_file file= leer_files(realPath);
	int tamReal = 0;

		int block;

		int posicion;

		for(int j = 0; j < file.blockCount; j++){
			block = file.blocks[j];
			for(int i=0 ; i< tam_clusters ; i++){
				posicion = block * tam_clusters + i;
				char lectura;
				memcpy(&lectura, (block_copia +posicion) , sizeof(char));

				if(lectura == file.caracterLlenado[0]){
					tamReal ++;
				}

			}
		}


	free(file.blocks);
	return tamReal;

}

//BLOCK COUNT FILE----------------------------------------------------------------------------------------
int size_int_asterisco(int* data){
	int i=0;
	while(data[i]!= NULL){
		i++;
	}
	return i;
}
bool blockCountFiles(){
	char* tarea;

	int size = list_size(listaTareas);
	for(int i = 0 ; i < size; i ++){
		
		pthread_mutex_lock(&listaTareasSem);
		tarea = list_get(listaTareas,i);
		pthread_mutex_unlock(&listaTareasSem);
		
		

		t_file file =obetnerDatosDelFile(tarea);
		int cantBlocks = size_int_asterisco(file.blocks);
		if(cantBlocks != file.blockCount){

			actualizarFile(tarea, "BLOCK_COUNT", cantBlocks);
			return 1;
		}
		free(file.blocks);
	}
	log_info(logger, "el problema no fue block count");

	return 0;
}
t_file obetnerDatosDelFile(char* tarea){
	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");
	
	t_file file= leer_files(realPath);


	free(realPath);
	return file;
	
}


// VALIDAR BLOCKS FILE---MD5----------------------------------------------------------------------------------
bool validarBlocksFile(){

	char* tarea;

	int size = list_size(listaTareas);
	for(int i = 0 ; i < size; i ++){

		pthread_mutex_lock(&listaTareasSem);
		tarea = list_get(listaTareas,i);
		pthread_mutex_unlock(&listaTareasSem);


		bool error =comparcionMD5(tarea) ;
		if(error == 1){
			return error;
		}

	}
	log_info(logger, "el problema no fue el md5");
	return 0;
}



bool comparcionMD5(char* tarea)
{


	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");
	
	t_file file= leer_files(realPath);
	char* md5AReal = file.md5Archivo; // md5  que esta en el file.ims
	int* blocksAChequear= file.blocks;
	
	char* md5AChequear =obtenerMD5(realPath, tarea, file.size); // el md5 que real , xq se calcula en el momento/


	if (strcmp(md5AChequear, md5AReal) ==0){
		free(file.blocks);
		return 0;
	}else{
		arreglarOrdenBlocks(blocksAChequear, file.caracterLlenado[0], tarea);
		free(file.blocks);
		return 1;
	}


	free(realPath);
}

void arreglarOrdenBlocks(int* blocksAChequear, char caracter, char* tarea){

	int cuantosBloquesHay = size_int_asterisco(blocksAChequear);
	int block, posicion, blockIncompleto = 0, falsoUltimoBlock = 0, aux = 0;

	for(int j = 0; j < cuantosBloquesHay; j++){
		block = blocksAChequear[j];
		for(int i=0 ; i< tam_clusters ; i++){
			posicion = block * tam_clusters + i;
			char lectura;
			memcpy(&lectura, (block_copia +posicion) , sizeof(char));

			if(lectura != caracter){

				blockIncompleto = block;
				falsoUltimoBlock = blocksAChequear[cuantosBloquesHay - 1];

				aux = blockIncompleto;
				blocksAChequear[j] = blocksAChequear[cuantosBloquesHay - 1];
				blocksAChequear[cuantosBloquesHay - 1] = aux;

				t_list* listaBlocks = int_asterisco_to_tlist(blocksAChequear);
				char* blocks_actualizado = list_to_string_array(listaBlocks);

				actualizarFile2(tarea, "BLOCKS", blocks_actualizado);

				return;
			}

		}
	}
}

t_list* int_asterisco_to_tlist(int* intAsterisco){

	int size = size_int_asterisco(intAsterisco);
	t_list* lista = list_create();

	for(int i=0; i<size; i++){

		list_add(lista,intAsterisco[i]);
	}
	return lista;
}

// cambios en los files --------------------------------------------------------------------------------
void actualizarFile(char* tarea, char* key , int value){
	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");
	
	t_config* config_metadata = config_create(realPath);
	config_set_value(config_metadata, key ,string_itoa(value));
	config_save(config_metadata);
	config_destroy(config_metadata);

	free(realPath);
}
void actualizarFile2(char* tarea, char* key , char* value){
	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");

	t_config* config_metadata = config_create(realPath);
	config_set_value(config_metadata, key ,value);
	config_save(config_metadata);
	config_destroy(config_metadata);

	free(realPath);
}
void cambiarMD5(char* tarea,char* md5){
	char* realPath =string_new();
		string_append(&realPath, filesPath);
		string_append(&realPath, "/");
		string_append(&realPath,tarea);
		string_append(&realPath,".ims");

		t_config* config_metadata = config_create(realPath);
		config_set_value(config_metadata, "MD5_ARCHIVO" ,md5);
		config_save(config_metadata);
		config_destroy(config_metadata);

		free(realPath);

}
