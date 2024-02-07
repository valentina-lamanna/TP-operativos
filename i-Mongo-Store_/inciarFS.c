/*
 * inciarFS.c
 *
 *  Created on: May 25, 2021
 *      Author: utnso
 */
#include "iniciarFS.h"

void iniciar_FS(){


	log_info(logger,"Iniciando mongo en: %s \n",punto_montaje);

	directorio = punto_montaje;
	superBloquePath= string_new();
	blocksPath= string_new();
	filesPath= string_new();
	bitacoraPath= string_new();
	string_append(&superBloquePath,punto_montaje);
	string_append(&superBloquePath,"/SuperBloque.ims");


	string_append(&blocksPath,punto_montaje);
	string_append(&blocksPath, "/Blocks.ims");
	string_append(&filesPath,punto_montaje);
	string_append(&filesPath,"/Files");

	string_append(&bitacoraPath,punto_montaje);
	string_append(&bitacoraPath,"/Bitacora");


	if (!isDir(punto_montaje)){

		crear_directorios(punto_montaje); // crear_directorios no devuelve nada
		superBloquePath = crear_superbloque(punto_montaje);
		blocksPath = crear_blocks(superBloquePath);


		log_info(logger,"Filesystem I-MONGO-STORE iniciado correctamente, actualmente vacio \n");

	} else {
		traer_superbloque_existente();
		log_info(logger, "I-MONGO-STORE ya existente en %s \n",punto_montaje);
	}
}

bool isDir(const char* name){
    DIR* directory = opendir(name);

    if(directory != NULL){
     closedir(directory);
     return true;
    } else {
    	return false;
    }
}


void crear_directorios(char* path){

	mkdir(path,0777); /// esto crea un directorio no un archivos
					  /// con el 777 se tiene permiso a leer, escribir y ejecutar

	mkdir(filesPath,0777);
	mkdir(bitacoraPath, 0777);

	log_info(logger,"Se crearon los directorios: Files, Bitacora. Y los archivos: SuperBloque, Blocks \n");
}

void traer_superbloque_existente() {

	//------------------------VARIABLES
	double tamEnBytes= ceil((double)cantidad_clusters/8.0);
	//int tamEnBits = (int)tamEnBytes * 8; //USAR BYTES

	int tamDelSuperBloque = 2 * sizeof(uint32_t)+ tamEnBytes;

	punteroAlSBdeMMAP= calloc(1, tamDelSuperBloque);

	int fd = open(superBloquePath,  O_RDWR, 0777);

	//---------------------------SUPER BLOQUE
	punteroAlSBdeMMAP = mmap(0,tamDelSuperBloque, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);


	char* bitmap = obtenerBitArray();
	bitmapFS =  bitarray_create_with_mode(bitmap, (int)tamEnBytes, LSB_FIRST);


	msync(punteroAlSBdeMMAP, tamDelSuperBloque, MS_SYNC);

	//-------------------------BLOCKS
	block_copia =malloc(tam_clusters * cantidad_clusters);// esto es la copia del memcpy no el mmap

	int fd2;
	fd2 = open(blocksPath, O_RDWR , 0777);

	block_file_final = mmap(NULL, tam_clusters * cantidad_clusters, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0);

	memcpy( block_copia,block_file_final, tam_clusters * cantidad_clusters);

	sincronizarMMAp();
}

void setearBitarrayEn0(t_bitarray* ba, int tam){

	    for(int i = 0 ; i < tam; i++)
	    {
	    	bitarray_clean_bit(ba,i);
	    }

	}
void showbitarr(t_bitarray* ba ,int tam){

    for(int i = 0 ; i < tam; i++)
    {
    	log_info(logger,"BIT %d: %d -----",i,bitarray_test_bit(ba,i));
    }
}

char* crear_superbloque(char* path){

	double tamEnBytes= ceil((double)cantidad_clusters/8.0);
	int tamEnBits = (int)tamEnBytes * 8;

	int tamDelSuperBloque = 2 * sizeof(uint32_t)+ tamEnBytes;

	punteroAlSBdeMMAP= calloc(1,tamDelSuperBloque);
	int fd;

	fd = open(superBloquePath, O_CREAT| O_RDWR, 0777);
	if (fd < 0){
		  printf("fallo abrir/crear el archivo");
		  exit(EXIT_FAILURE);
	}

	ftruncate(fd, tamDelSuperBloque);// tiene que estar abierto para escritura // le cambia el tamaño y pisa to-do lo anterior

	punteroAlSBdeMMAP = mmap(NULL,tamDelSuperBloque, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // map file

	 if(punteroAlSBdeMMAP == MAP_FAILED) // check mapping successful
		  {
			perror("Error  mapping \n");
			exit(1);
		  }
	 void* aux= malloc(4);
	 aux= realloc(aux, sizeof(uint32_t));

	memcpy(aux, &tam_clusters, sizeof(uint32_t));
	memcpy(punteroAlSBdeMMAP,aux, sizeof(uint32_t));
	memcpy(aux, &cantidad_clusters, sizeof(uint32_t));
	memcpy(punteroAlSBdeMMAP + sizeof(uint32_t),aux, sizeof(uint32_t));

	bitmapFS =  bitarray_create_with_mode(punteroAlSBdeMMAP +2* sizeof(uint32_t), (int)tamEnBytes, LSB_FIRST);
	setearBitarrayEn0(bitmapFS, tamEnBits);


	msync(bitmapFS->bitarray, (int)tamEnBytes, MS_SYNC);//sincroniza solo del bitmap
	msync(punteroAlSBdeMMAP, tamDelSuperBloque, MS_SYNC);//sincroniza to-do el archivo

	log_info(logger,"Se creo SuperBloque en la ruta: %s \n",superBloquePath);

	return superBloquePath;

}

char* crear_blocks(char* path){

	block_copia =calloc(1, tam_clusters * cantidad_clusters);// esto es la copia del memcpy no el mmap


	create_file_with_size(blocksPath, tam_clusters * cantidad_clusters);

	sincronizarMMAp();

	log_info(logger,"Se crearon el file de tamaño : %d bytes, y ya se empezo a sincronizar \n",tam_clusters * cantidad_clusters);

	return blocksPath;

}

void crearFile(char* tarea,char* llenado){// tarea = /Oxigeno.ims

	char* realPath =string_new();
	string_append(&realPath, filesPath);
	string_append(&realPath, "/");
	string_append(&realPath,tarea);
	string_append(&realPath,".ims");


	FILE* f = fopen(realPath,"wb");

	t_config* metadata_config = config_create(realPath);

	char* blocks= string_new();
	string_append(&blocks, "[");
	string_append(&blocks, "]"); // "[]"
	char* MD5vacio = "";

	config_set_value(metadata_config,"SIZE",string_itoa(0));// antes decia tam_clusters
	config_set_value(metadata_config,"BLOCK_COUNT",string_itoa(0));
	config_set_value(metadata_config,"BLOCKS", blocks);//blocks
	config_set_value(metadata_config,"CARACTER_LLENADO",llenado);
	config_set_value(metadata_config,"MD5_ARCHIVO",MD5vacio);

	int result = config_save(metadata_config);


	if (result != 1){
		log_error(logger,"El resultado al usar config_save genero un -1 \n");
	}

	fclose(f);
	config_destroy(metadata_config);
	log_info(logger,"Se creo el archivo de recursos %s en: %s \n",tarea, realPath);
	free(realPath);
	free(blocks);
}



void crearBitacora(int tripulante){

	char* realPath =string_new();
	string_append(&realPath,bitacoraPath);
	string_append(&realPath,"/Tripulante");
	string_append(&realPath,string_itoa(tripulante));
	string_append(&realPath,".ims");
	FILE* f = fopen(realPath,"wb");


	t_config* bitacora_config = config_create(realPath);

	config_set_value(bitacora_config,"SIZE", string_itoa(0));

	char* charBlocks=string_new();
	string_append(&charBlocks, "[");
	string_append(&charBlocks, "]" );

	config_set_value(bitacora_config,"BLOCKS",charBlocks);

	int result = config_save(bitacora_config);

	if (result != 1){
		log_error(logger,"El resultado al usar config_save genero un -1 \n");
	}
	fclose(f);
	config_destroy(bitacora_config);
	log_info(logger,"Se creo el archivo de bitacora del tripulante de TID  %d en: %s \n",tripulante, realPath);
	free(realPath);
	free(charBlocks);

}


/*INICIO-----------------------------BITAMAP-----------------------------------------*/



int proximoBlockLibre(){
	int i = -1;
	int bit = 1;

	while(bit == 1){

		i++;
		bit = bitarray_test_bit(bitmapFS, i);
	}

	return i;
}

bool bitmapNoCompleto(){
		t_bitarray* bitarray = bitmapFS;

		int i = 0;

		bool vacio;

		while ( i <= (cantidad_clusters-1) ){

			if (bitarray_test_bit(bitarray,i) == true){
				vacio = false;
			}else {
				vacio = true;
				return vacio;
				break;
			}
			i++;
		}
		return vacio;
		bitarray_destroy(bitarray);

}

void liberarBitmap(int index){
	pthread_mutex_lock(&superBloqueSem);

	bitarray_clean_bit(bitmapFS,(off_t)index);
	double tam =ceil((double)cantidad_clusters/8.0);

	msync(bitmapFS->bitarray, (int)tam, MS_SYNC);
	memcpy(punteroAlSBdeMMAP + 2*sizeof(uint32_t), bitmapFS->bitarray, tam);

	log_info(logger, "Se libero el bitmap %d \n", index);

	pthread_mutex_unlock(&superBloqueSem);
}


void ocuparBitmap(int index){
	pthread_mutex_lock(&superBloqueSem);

	bitarray_set_bit(bitmapFS, (off_t)index);
	double tam =ceil((double)cantidad_clusters/8.0);

	msync(bitmapFS->bitarray, (int)tam, MS_SYNC);

	memcpy(punteroAlSBdeMMAP + 2*sizeof(uint32_t), bitmapFS->bitarray, tam);

	log_info(logger, "Se ocupo el bitmap %d  con valor %d\n", index, bitarray_test_bit(bitmapFS,(off_t)index));


	pthread_mutex_unlock(&superBloqueSem);
}

/*FIN--------------------------------BITAMAP-----------------------------------------*/

/*INICIO-----------------------------BLOCKS-----------------------------------------*/

void create_file_with_size(char* path,int size) {

			int fd;
		       fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0777);
		       if (fd < 0)
		       {

		          exit(EXIT_FAILURE);
		       }
		       ftruncate(fd, size);
		       block_file_final = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);


		       if(block_file_final == MAP_FAILED) //check mapping successful
		       	  {
		       		perror("Error  mapping \n");
		       		exit(1);
		       	  }


}


int sobranteBlock(int size){
	int modulo= size % tam_clusters;
	int espacioLibre = tam_clusters - modulo;

	return espacioLibre;
}



char* leerBlock(int nroBlock){
	int posicion = nroBlock * tam_clusters;
	char* lectura=calloc(1,tam_clusters);
	pthread_mutex_lock(&blockSem);
	memcpy(lectura, (block_copia +posicion) , tam_clusters);
	pthread_mutex_unlock(&blockSem);


	return lectura;
}

char* leerUltimoBlock(int nroBlock, int tamDeLectura){
	int posicion = nroBlock * tam_clusters;
	void* lectura=calloc(1,tamDeLectura);
	pthread_mutex_lock(&blockSem);
	memcpy(lectura, (block_copia +posicion) , tamDeLectura);
	pthread_mutex_unlock(&blockSem);

	return (char*)lectura;
}


void escrbirblock(int bloque, char caracter, int posicion){
	int posicionReal = tam_clusters * bloque + posicion;

	pthread_mutex_lock(&blockSem);
	memcpy((block_copia +posicionReal), &(caracter),  sizeof(char));
	pthread_mutex_unlock(&blockSem);
}

void borrarBlock(int bloque,int posicion){
	int posicionReal = tam_clusters * bloque + posicion;
	pthread_mutex_lock(&blockSem);
	memcpy((block_copia +posicionReal), "",  sizeof(char));
	pthread_mutex_unlock(&blockSem);
}

void sincronizarMMAp(){
	pthread_t sincronizar;
	pthread_create(&sincronizar,NULL, (void*)sincronizacion, NULL);
	pthread_detach(sincronizar);
}
void sincronizacion(){


	int tamContenido= cantidad_clusters* tam_clusters;
	int contador = 1;

	while(1){


		pthread_mutex_lock(&sincronizadorSem);

		memcpy(block_file_final, block_copia, tamContenido);

		msync(block_file_final, tamContenido, MS_SYNC);

		log_info(logger, "Se sincronizo el archivo de blocks  por vez  : %d  \n ", contador );
		contador ++;

		pthread_mutex_unlock(&sincronizadorSem);


	sleep(tiempo_sincronizacion);
	}
}

/*
 mmap --> guardar consistentemente (refleja el FS de la compu)
 copia---> copia que manejamos --> yo trabajo aca
 */


/*FIN--------------------------------BLOCKS-----------------------------------------*/

/*INCIO---------------------------- -FILES-----------------------------------------*/
void escribirFile(char* tarea, int cantDeVeces){

	char* realPath =string_new();
		string_append(&realPath, filesPath);
		string_append(&realPath, "/");
		string_append(&realPath,tarea);
		string_append(&realPath,".ims");

	if(strcmp(tarea, "OXIGENO") == 0){
		pthread_mutex_lock(&oxigenoSem);

	}
	else if(strcmp(tarea, "COMIDA") == 0){
		pthread_mutex_lock(&comidaSem);
	}
	else if(strcmp(tarea, "BASURA") == 0){
		pthread_mutex_lock(&basuraSem);
	}



	pthread_mutex_lock(&sincronizadorSem);// por si se esta mapeando a memoria

	t_file file= leer_files(realPath);
		int cantidad = file.blockCount;
		int ultimoBlock;
		int espacioLibre = sobranteBlock(file.size);

	if(file.size == 0 || espacioLibre == tam_clusters){
		pthread_mutex_lock(&bitmapS);
		ultimoBlock= proximoBlockLibre();
		ocuparBitmap(ultimoBlock);
		pthread_mutex_unlock(&bitmapS);

		agregarOtroBlockFile(ultimoBlock, realPath);
		log_info(logger, "Se agrego un nuevo block %d para la tarea %s \n", ultimoBlock, tarea);
		espacioLibre= tam_clusters;
	}
	else {
		ultimoBlock = file.blocks[cantidad-1];
		log_info(logger, "El ultimo block para la tarea %s es el %d \n", tarea, ultimoBlock);
	}


	for(int i =0 ; i < cantDeVeces; i ++){    //------------------> ir escribirendo revisando si me quede sin espacio

		if(espacioLibre > 0 ){
			escrbirblock(ultimoBlock,file.caracterLlenado[0], tam_clusters - espacioLibre);
			espacioLibre --;
		}
		else if (espacioLibre == 0){

			if(bitmapNoCompleto()){

				pthread_mutex_lock(&bitmapS);

				ultimoBlock= proximoBlockLibre();
				ocuparBitmap(ultimoBlock);

				pthread_mutex_unlock(&bitmapS);

				agregarOtroBlockFile(ultimoBlock, realPath);
				log_info(logger, "Se agrego un nuevo block %d a la tarea %s \n", ultimoBlock, tarea);

				espacioLibre= tam_clusters;
				escrbirblock( ultimoBlock,file.caracterLlenado[0], tam_clusters - espacioLibre);
				espacioLibre --;
			} else{ log_info(logger, "no hay mas bloques disponibles \n");}

		}
	}

	log_info(logger, "Se escribieron %d veces el caracter de la tarea %s \n", cantDeVeces, tarea);
	int nuevoSize = file.size + sizeof(char)* cantDeVeces;
	cambiarSizeFile(nuevoSize, realPath);

	char* MD5 = obtenerMD5(realPath,tarea, nuevoSize);
	settearMD5(realPath,MD5);
	
	pthread_mutex_unlock(&sincronizadorSem);// por si se esta mapeando a memoria
	
	if(strcmp(tarea, "OXIGENO") == 0){
			pthread_mutex_unlock(&oxigenoSem);
		}
		else if(strcmp(tarea, "COMIDA") == 0){
			pthread_mutex_unlock(&comidaSem);
		}
		else if(strcmp(tarea, "BASURA") == 0){
			pthread_mutex_unlock(&basuraSem);
		}

	free(realPath);
	free(file.blocks);
}

char* obtenerMD5(char* realPath, char* tarea, int tam){
	char* nombreFile =string_new();
	string_append(&nombreFile,punto_montaje);
	string_append(&nombreFile,"/");
	string_append(&nombreFile,tarea);
	string_append(&nombreFile, "ExclusivoMD5.txt");
	FILE* archivo = fopen(nombreFile,"w");
	
	char* textoFile = malloc(tam);
	textoFile = obtenerFileCompleto(realPath,tarea);

	fprintf(archivo, "%s" ,textoFile);
	fclose(archivo);

	char* name = string_new();
	string_append(&name,punto_montaje);
	string_append(&name, "/");
	string_append(&name, tarea );
	string_append(&name,"prueba.txt" );
	int archivoMD5 = open( name, O_CREAT | O_TRUNC | O_RDWR, 0777);

	char* str = string_new();
	string_append(&str,"md5sum ");
	string_append(&str, nombreFile );
	string_append(&str, " > ");
	string_append(&str,name);

	system(str);

	char* md5 = calloc(1, 33);

	struct stat sb;
	fstat(archivoMD5, &sb);

	read(archivoMD5, md5 ,32);

	close(archivoMD5);

	remove(nombreFile);
 	remove(name);

 	free(nombreFile);
 	free(name);
 	free(str);
	return  md5;
}

int get_size_by_fd(int fd) {
	    struct stat statbuf;
	    fstat(fd, &statbuf);
	    return statbuf.st_size;
}


char* obtenerFileCompleto(char* realPath, char* tarea){
	t_file file= leer_files(realPath);
	int ultimo= file.blocks[file.blockCount-1];

	char* lectura = string_new();

	int masChico = file.size % tam_clusters;

	for(int i=0 ; i< file.blockCount ; i++){

			if(file.blocks[i] == ultimo){
			string_append(&lectura,leerUltimoBlock(file.blocks[i], masChico));
			}
			else{
			string_append(&lectura, leerBlock(file.blocks[i]));
			}


		}

	free(file.blocks);
	return lectura;

}

char* relacionarTareaConCaracter(char* tarea){

	char* realPath =string_new();
		string_append(&realPath, filesPath);
		string_append(&realPath, "/");
		string_append(&realPath,tarea);
		string_append(&realPath,".ims");

	t_file file= leer_files(realPath);
	char* caracter = file.caracterLlenado;
	free(realPath);
	free(file.blocks);
	return caracter;
}


void borrarDelFile(char* tarea, int veces){

	char* realPath =string_new();
		string_append(&realPath, filesPath);
		string_append(&realPath, "/");
		string_append(&realPath,tarea);
		string_append(&realPath,".ims");

	t_file file= leer_files(realPath);
	int cantidad = (int)ceil((double)file.size / tam_clusters);

	int ultimoBlock = file.blocks[cantidad-1];


	if(file.size >= veces){

	if(strcmp(tarea, "OXIGENO") == 0){
			pthread_mutex_lock(&oxigenoSem);
		}
		else if(strcmp(tarea, "COMIDA") == 0){
			pthread_mutex_lock(&comidaSem);
		}
		else if(strcmp(tarea, "BASURA") == 0){
			pthread_mutex_lock(&basuraSem);
		}

	pthread_mutex_lock(&sincronizadorSem);// por si se esta mapeando a memoria

	int nuevoSize = file.size;
	int masChico = file.size% tam_clusters;
	for(int i=0 ; i< veces; i++){
		borrarBlock(ultimoBlock, masChico -1 );
		nuevoSize -= sizeof(char);
		masChico --;

		int espacioLibre= sobranteBlock(nuevoSize);
		if( espacioLibre == tam_clusters){

			ultimoBlock= borrarBlock_devolverNuevoUltimoBlock(realPath); //borro de la config y  del bit map
			log_info(logger, "Se desocupo un block  %d de la tarea %s \n", ultimoBlock , tarea);

		}

	}

	cambiarSizeFile(nuevoSize, realPath);

	log_info(logger, "Se borraron %d veces el carater de  la tarea %s \n",veces, tarea);
	
	char* MD5 = obtenerMD5(realPath,tarea, nuevoSize);
	settearMD5(realPath,MD5);
	
	pthread_mutex_unlock(&sincronizadorSem);// por si se esta mapeando a memoria
	if(strcmp(tarea, "OXIGENO") == 0){
			pthread_mutex_unlock(&oxigenoSem);
		}
		else if(strcmp(tarea, "COMIDA") == 0){
			pthread_mutex_unlock(&comidaSem);
		}
		else if(strcmp(tarea, "BASURA") == 0){
			pthread_mutex_unlock(&basuraSem);
		}

	}//cierro el if size >= veces
	else{
		log_error(logger, "no se puede borrar %d veces el caracteres porque el size es %d", veces , file.size);
	}
	free(realPath);
	free(file.blocks);

}
int size_char_asterisco(char** data){
	int i=0;
	while(data[i]!= NULL){
		i++;
	}
	return i;
}

t_file leer_files(char* path){

	t_file file_config ;
	t_config* config_metadata = config_create(path);
	char** arrayDeBlocks;

	file_config.size= config_get_int_value(config_metadata,"SIZE");
	file_config.blockCount=config_get_int_value(config_metadata,"BLOCK_COUNT");
	arrayDeBlocks=  config_get_array_value(config_metadata,"BLOCKS");
	file_config.caracterLlenado =strdup(config_get_string_value(config_metadata,"CARACTER_LLENADO")) ;//necesito un char no el sting
	file_config.md5Archivo =strdup(config_get_string_value(config_metadata,"MD5_ARCHIVO"));// es el tam en bytes del MD5 por defecto


	int sizeArray = size_char_asterisco(arrayDeBlocks);

	file_config.blocks = malloc(cantidad_clusters);

	for(int i = 0; i < sizeArray ;i++){
		file_config.blocks[i] = atoi(arrayDeBlocks[i]);
	}
	file_config.blocks[sizeArray] = NULL;

	config_destroy(config_metadata);
	free(arrayDeBlocks);
	return file_config;

}

void settearMD5(char* realPath, char* MD5){

	t_config* config_metadata = config_create(realPath);
	config_set_value(config_metadata,"MD5_ARCHIVO",MD5);
	config_save(config_metadata);
	config_destroy(config_metadata);

}

void agregarOtroBlockFile(int nuevoBlock, char* realPath){

	t_config* config_metadata = config_create(realPath);
	char** viejoArray = config_get_array_value(config_metadata,"BLOCKS"); // [3] --> [3,ultimoBlock]

	t_list* listaBlocks = chardoble_to_tlist(viejoArray);
	list_add(listaBlocks, nuevoBlock);
	int size = list_size(listaBlocks);
	char* blocks_actualizado = list_to_string_array(listaBlocks);


	config_set_value(config_metadata,"BLOCKS" ,blocks_actualizado);
	config_set_value(config_metadata,"BLOCK_COUNT", string_itoa(size));



	config_save(config_metadata);
	config_destroy(config_metadata);

}

int borrarBlock_devolverNuevoUltimoBlock(char* realPath){

	t_config* config_metadata = config_create(realPath);

	char** viejoArray = config_get_array_value(config_metadata,"BLOCKS");

	t_list* listaBlocks = chardoble_to_tlist(viejoArray);// array to list

	int tam= list_size(listaBlocks);

	void* esElUltimo(int unBlock, int otroBlock){
		return unBlock > otroBlock ? unBlock : otroBlock;
	}


	int blockABorrar = (int)list_get_maximum(listaBlocks, (void*)esElUltimo);

	bool esElBlockQueCalcule(int nroBlock){
		return nroBlock == blockABorrar;
	}

	list_remove_by_condition(listaBlocks, (void*)esElBlockQueCalcule);

	log_info(logger, "la lista de blocks tiene size %d, el block a borrar es el %d", blockABorrar);

	pthread_mutex_lock(&bitmapS);
	liberarBitmap(blockABorrar); //--> por ahora liberar no devuelve nada
	pthread_mutex_unlock(&bitmapS);

	char* blocks_actualizado = list_to_string_array(listaBlocks);
	config_set_value(config_metadata,"BLOCKS" ,blocks_actualizado);

	tam= list_size(listaBlocks);
	int nuevoUltimoBlock = (int)list_get_maximum(listaBlocks, (void*)esElUltimo);//ultimoBlockFuncion(listaBlocks, tam) ;//ultimo block
	log_info(logger, "la lista de blocks ahora tiene size %d, el ultimo block ahora es el %d", nuevoUltimoBlock);

	config_save(config_metadata);
	config_destroy(config_metadata);
	free(viejoArray);

	list_clean(listaBlocks);
	list_destroy(listaBlocks);

	return nuevoUltimoBlock;
}

int ultimoBlockFuncion(t_list* p, int n){
 	t_list* aux = list_duplicate(p);
 	int pos = n -1;

 	if (aux != NULL){
 		for (int i=0; i<pos; i++){
		 aux = aux->head;
		}

 	}

 	int val = aux -> elements_count;
	return val;
}

void cambiarSizeFile(int nuevoSize, char* realPath){
		t_config* config_metadata = config_create(realPath);

		config_set_value(config_metadata, "SIZE", string_itoa(nuevoSize));
		config_save(config_metadata);
		config_destroy(config_metadata);
}



/*FIN--------------------------------FILES-----------------------------------------*/

/*INCIO------------------------------BITACORA-----------------------------------------*/
char* obtenerBitacora(int trip){
	char* realPath =string_new();
	string_append(&realPath,bitacoraPath);
	string_append(&realPath,"/Tripulante");
	string_append(&realPath,string_itoa(trip));
	string_append(&realPath,".ims");
	FILE* f = fopen(realPath,"rb");


	pthread_mutex_lock(&sincronizadorSem);// por si se esta mapeando a memoria

	t_bitacora bita;
	bita= leerBitacoraConfig(realPath);

	int cantBlocks = (int)ceil((double)bita.size / (double)tam_clusters);
	int ultimo= bita.blocks[cantBlocks-1];
	int masChico = bita.size % tam_clusters;

	char* lectura = string_new();

	for(int i=0 ; i< cantBlocks ; i++){

		if(bita.blocks[i] == ultimo){
		string_append(&lectura,leerUltimoBlock(bita.blocks[i], masChico));
		}
		else{
		string_append(&lectura,leerBlock(bita.blocks[i]));
		}


	}


	log_info(logger, "Se mando la bitacora  del tripulante de TID %d \n", trip);


	pthread_mutex_unlock(&sincronizadorSem);// por si se esta mapeando a memoria


	fclose(f);
	free(realPath);
	free(bita.blocks);
	return lectura;

}

t_bitacora leerBitacoraConfig(char* path){

	t_bitacora bitacora_config;
	t_config* config_metadata = config_create(path);

	char** arrayDeBlocks;



		bitacora_config.size = config_get_int_value(config_metadata,"SIZE");
		arrayDeBlocks =  config_get_array_value(config_metadata,"BLOCKS");



		int sizeArray = size_char_asterisco(arrayDeBlocks);

		bitacora_config.blocks = malloc(cantidad_clusters);


		for(int i = 0; i < sizeArray; i++){
			bitacora_config.blocks[i] = atoi(arrayDeBlocks[i]);
		}
		bitacora_config.blocks[sizeArray] = NULL;


		config_destroy(config_metadata);
		free(arrayDeBlocks);
		return bitacora_config;
}

void escrbirBitacora(int tripulante,char* mensaje){
	char* realPath =string_new();
	string_append(&realPath,bitacoraPath);
	string_append(&realPath,"/Tripulante");
	string_append(&realPath,string_itoa(tripulante));
	string_append(&realPath,".ims");
	FILE* f = fopen(realPath,"rb+");


	pthread_mutex_lock(&sincronizadorSem);// por si se esta mapeando a memoria

	t_bitacora bita;
	bita= leerBitacoraConfig(realPath);


	int ultimoBlock;
	int espacioLibre = sobranteBlock(bita.size);
	int cantDeBloques = (int)ceil((double)bita.size / tam_clusters);

	if(bita.size== 0 || espacioLibre == tam_clusters){
		pthread_mutex_lock(&bitmapS);
		ultimoBlock= proximoBlockLibre();
		ocuparBitmap(ultimoBlock);
		pthread_mutex_unlock(&bitmapS);

		agregarNuevoBlockBitacora(ultimoBlock, realPath);
		espacioLibre = tam_clusters;
		log_info(logger, "Se agrego un nuevo block %d para el tripulante de TID %d \n", ultimoBlock,tripulante);
	}
	else {
		ultimoBlock = bita.blocks[cantDeBloques -1];

	}


		for(int i =0 ; i< strlen(mensaje); i ++){    //------------------> ir escribirendo revisando si me quede sin espacio

			if(espacioLibre > 0 ){
				escrbirblock( ultimoBlock,mensaje[i], tam_clusters - espacioLibre);
				espacioLibre --;
			}
			else if (espacioLibre == 0){//-------------> si estaba al fianl le asigno un nuevo block
				if(bitmapNoCompleto()){

					pthread_mutex_lock(&bitmapS);
					ultimoBlock= proximoBlockLibre();
					ocuparBitmap(ultimoBlock);
					pthread_mutex_unlock(&bitmapS);

					agregarNuevoBlockBitacora(ultimoBlock, realPath);

					espacioLibre= tam_clusters;
					log_info(logger, "Se agrego un nuevo block %d al trupulante  de TID %d \n", ultimoBlock, tripulante);
					escrbirblock( ultimoBlock,mensaje[i], tam_clusters - espacioLibre);
					espacioLibre --;
				} else{
					log_info(logger, "no hay mas blocks dispoblibles \n");
				}
			}
		}




		int nuevoSize = bita.size + sizeof(char) * strlen(mensaje);
		cambiarSizeBitacora(nuevoSize, realPath);
		log_info(logger, "Se escribio la bitacora %s del tripulante con TID %d \n", realPath, tripulante);

		pthread_mutex_unlock(&sincronizadorSem);// por si se esta mapeando a memoria
		free(realPath);
		free(bita.blocks);
	fclose(f);
}

void agregarNuevoBlockBitacora(int nuevoBlock, char* path){

	t_config* config_metadata = config_create(path);
	char** viejoArray = config_get_array_value(config_metadata,"BLOCKS"); // [3] --> [3,ultimoBlock]

	t_list* listaDeBloques = chardoble_to_tlist(viejoArray);

	list_add(listaDeBloques, nuevoBlock);

	char* blocks_actualizado = list_to_string_array(listaDeBloques);

	config_set_value(config_metadata,"BLOCKS" ,blocks_actualizado);
	config_save(config_metadata);

	config_destroy(config_metadata);
	free(viejoArray);

	list_clean(listaDeBloques);
	list_destroy(listaDeBloques);

	free(blocks_actualizado);
}

void cambiarSizeBitacora(int nuevoSize, char* realPath){
	t_config* config_metadata = config_create(realPath);
	config_set_value(config_metadata, "SIZE", string_itoa(nuevoSize));
	config_save(config_metadata);
	config_destroy(config_metadata);
}

/*FIN--------------------------------BITACORA-----------------------------------------*/
//EXTRAS-----------------------------------
char* concatenar(char* str1, char* str2) {
	char* new_str;
	if ((new_str = malloc(strlen(str1) + strlen(str2) + 1)) != NULL) {
		new_str[0] = '\0';
		strcat(new_str, str1);
		strcat(new_str, str2);
	} else {
		log_error(logger, "error al concatenar \n");
	}

	return new_str;
}

int size_char_doble(char** array){

	int i = 0;
	int size = 0;
	int a;
	while(array[i] != NULL){
		a=array[i];
		i++;
		size++;
	}

	return size;

}

char* convertir_array_string(int* nuevoArray, int tam){
	char* retorno =string_new();
	string_append(&retorno,"[");
	string_append(&retorno,string_itoa(nuevoArray[0]));

	for(int i=1; i< tam; i++){

		string_append(&retorno,",");
		string_append(&retorno,string_itoa(nuevoArray[i]));

	}

	string_append(&retorno, "]");
	return retorno;

	//Ejemplo:
	// char* array_string = "[1,2,3,4]"
	// string_get_value_as_array(array_string) => ["1","2","3","4"]
	// char**  string_get_string_as_array(char* text);
}

t_list* chardoble_to_tlist(char** chardoble){

	int size_chardoble = size_char_doble(chardoble);
	t_list* lista = list_create();

	for(int i=0; i<size_chardoble; i++){

		list_add(lista,atoi(chardoble[i]));
	}
	return lista;
}

char* list_to_string_array(t_list* lista){

	char* format = string_new();
	string_append(&format,"[");

	if (lista->elements_count == 1){

		string_append(&format,string_itoa(lista->head->data));

	} else if (lista->elements_count == 0){

		log_info(logger,"La tabla de indices solamente tenia un cluster, ahora queda vacia");

	} else {

			while (lista->head->next != NULL){
				string_append(&format,string_itoa(lista->head->data));
				string_append(&format,",");
				lista->head = lista->head->next;

				if (lista->head->next == NULL){
					string_append(&format,string_itoa(lista->head->data));
				}
			}
	}

	string_append(&format,"]");

	return format;
}

