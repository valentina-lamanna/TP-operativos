#include "mi-ram-hq.h"
#include "commons/memory.h"


//------------------HEADER-------------------//
uint32_t guardarEnMemoria(void*, uint32_t, tipoDeContenido, uint32_t);
bool hayTamSuficienteEnMemoria(uint32_t); //revisar logica
void* buscarEnMemoriaSegunEstrucAdmin(void*);
void expulsarTripEnPaginacion(Patota*, uint32_t);
TCB* encontrarOActualizarTCB(Patota*, uint32_t, TCB*, bool);
char* encontrarTareasSegunEstrucAdmin(Patota*, uint32_t);

void guardar_contenido_en_memoria(uint32_t, void*, uint32_t);
void* obtener_de_memoria(uint32_t, uint32_t);
void guardar_contenido_en_memoria_aux(void*, uint32_t, void*, uint32_t);
void guardarContenidoEnMemoriaVirtual(uint32_t, void*, uint32_t);
void* obtenerDeMemoriaVirtual(uint32_t, uint32_t);

//-------------FUNCIONES SIGNALS----------//
void escucharSenialParaCompactar(void); // se queda escuchando siempre a ver cuando tiral la senial
void esperarSenialParaCompactar(void); // pasa al inicio de RAM

//----------------SEGMENTACION---------------//
void agregarSegmento(Segmento*);
void eliminarSegmento(Segmento*);
uint32_t cantMemoriaLibreEnSegmentos(uint32_t);
Segmento* encontrar_segmento_segun_algor(uint32_t);
uint32_t tamanioSegmento(Segmento*);
bool esDeMenorTamanio(void*, void*);
Segmento* recorrer_first_fit(uint32_t);
Segmento* recorrer_best_fit(uint32_t);
uint32_t guardarSegmento(void*, uint32_t, tipoDeContenido, bool, uint32_t);
bool esSegLibre(void*);
bool esSegOcupado(void*);
bool estanContiguosYOcupados(Segmento*, Segmento*);
void compactar(void);
Segmento* recorrerListaSegmentos(t_list*, uint32_t , uint32_t);
Segmento* devolverSegmentoPCB(uint32_t);

//----------------PAGINACION----------------//

//---ram---//
uint32_t guardarPagina(void*, uint32_t, uint32_t);
uint32_t popularPaginasEnMemoriaRAM(uint32_t, uint32_t, uint32_t);
bool hayMemoriaPag(uint32_t);
void actualizarUltimaReferencia(Pagina*);
void activarBitUso(Pagina*);
void desactivarBitUso(Pagina*);
bool noEstaEnRam(Pagina*);
void agregarPaginaALaListaDeLaPatota(Patota*, Pagina*);
void eliminarPaginaDeLaListaDeLaPatota(Patota*, Pagina*);
Pagina* buscarPaginaAReemplazarSegunAlgoritmo();
Pagina* seleccionoVictimaPorLRU();
Pagina* seleccionoVictimaPorCLOCK();
bool frameLibre(Frame*);
uint32_t cantFramesVacios();

uint32_t obtenerIndexEnListaDeIdTrips(t_list*, uint32_t);
PaginasPatota* obtenerInfoPaginasPatota(uint32_t);
void agregarInfoPaginasPatota(PaginasPatota*);

//----virtual----//
void expulsarTripEnPaginacion(Patota*, uint32_t);
void liberarMarco(uint32_t);
Frame* ubicarFramePorNroDeFrame(uint32_t);
Pagina* ubicarPaginaPorNroDePagina(uint32_t, uint32_t);
Pagina* buscarEnMemoriaVirtual(uint32_t);
bool paginaPresente(Pagina*);

//---mmap---//
void* leerMMAP(uint32_t, uint32_t);
void escribirMMAP(void* ,uint32_t, uint32_t);

//void swap(Pagina*, Pagina*);
void swap(uint32_t);
void actualizarFramesRAMConElCambio(void);

//----manejo estructuras admin ram y virtual----//
void agregarPaginaARam(Pagina*);
void sacarPagina(Pagina*);
uint32_t agregarEnMemVirtual(Pagina*);
void sacarDeMemVirtual(Pagina*);

//----------------------FUNCIONES-------------------------//

uint32_t guardarEnMemoria(void* contenidoAGuardar, uint32_t tamContenido, tipoDeContenido tipoDeContenidoParaSegmentos, uint32_t PID) {
	 //no va con puntero porque es del codigo, no real y dinamica.
	uint32_t direccLogica = -1;

	if(tamContenido > tamMemoria){
		log_error(logger, "No ha sido posible guardar el mensaje, ya que es mas grande que la memoria");
	} else {
		if(strcmp(esquemaMemoria,"SEGMENTACION") == 0) {
			direccLogica = guardarSegmento(contenidoAGuardar, tamContenido, tipoDeContenidoParaSegmentos, true, PID);

		} else if(strcmp(esquemaMemoria,"PAGINACION") == 0) {
			direccLogica = guardarPagina(contenidoAGuardar, tamContenido, PID);
		}
	}

	return direccLogica;
}

bool hayTamSuficienteEnMemoria(uint32_t tamDeseado)
{
	bool haySuficiente = false;

	if(strcmp(esquemaMemoria,"PAGINACION") == 0)
	{
		double cantPaginasAUsar = ceil((double)tamDeseado / tamPagina); // el ceil redondea para arriba, o sea si da 7,54353 te dice 8

		if(hayMemoriaPag((int)cantPaginasAUsar)){
			haySuficiente = true;
		}
		else {
			uint32_t cantVacios = cantFramesVacios();
			int espacioLibreEnRAM = cantVacios * tamPagina;
			int espacioLibreEnSwap = tamSwap - list_size(listaPagsVirtual) * tamPagina;

			log_info(logger, "El tam que queda disponible en memoria paginada, sumando RAM + SWAP, es de %d bytes\n", (espacioLibreEnRAM + espacioLibreEnSwap));

			haySuficiente = (espacioLibreEnRAM + espacioLibreEnSwap) >= tamDeseado;
		}
	}
	else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){

		uint32_t tamLibreTotal = cantMemoriaLibreEnSegmentos(tamDeseado); //va a ser la suma de todos los huecos!
		log_info(logger, "El tam que queda disponible en memoria segmentada es de %d bytes\n", tamLibreTotal);
		haySuficiente = tamLibreTotal >= tamDeseado;
	}

	//list size y comparar contra malloc.

	return haySuficiente;
}

void* buscarEnMemoriaSegunEstrucAdmin(void* info)
{
	if(strcmp(esquemaMemoria,"PAGINACION") == 0)
	{
		Frame* marcoABuscar = (Frame*)info;

		return obtener_de_memoria(marcoABuscar->direccionFisica, tamPagina);
	}
	else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0)
	{
		Segmento* segABuscar = (Segmento*)info;

		return obtener_de_memoria(segABuscar->base, segABuscar->tamanio);
	}
}



void agregarSegmento(Segmento* segment){
	pthread_mutex_lock(&mutex_segmentos);
	list_add(memoriaListSeg, segment);
	pthread_mutex_unlock(&mutex_segmentos);
}

void eliminarSegmento(Segmento* segAEliminar){

	bool esElMismoSegmento(void* segCualquiera) {
		Segmento* segCasteado = segCualquiera;
		return segCasteado == segAEliminar;
	}
	pthread_mutex_lock(&mutex_segmentos);
	list_remove_by_condition(memoriaListSeg, esElMismoSegmento);
	pthread_mutex_unlock(&mutex_segmentos);

}

void marcarSegmentoComoLibre(Segmento* segment){
	pthread_mutex_lock(&mutex_segmentos);
	segment->ocupado = false;
	segment->PID = 0;
	pthread_mutex_unlock(&mutex_segmentos);
}

uint32_t cantMemoriaLibreEnSegmentos(uint32_t tamDeseado)
{
	uint32_t tamLibreTotal = 0; //va a ser la suma de todos los huecos!

	void acumularTamLibres(void* huecoDeLaLista) {
		Segmento* huecoTipado = huecoDeLaLista;
		if(!huecoTipado->ocupado){
			uint32_t tam = huecoTipado->tamanio;
			tamLibreTotal += tam;
		}
	}

	pthread_mutex_lock(&mutex_segmentos);
	list_iterate(memoriaListSeg, acumularTamLibres);
	pthread_mutex_unlock(&mutex_segmentos);

	return tamLibreTotal;
}

// ---------------------------- FUNCIONES SEGMENTACION ----------------------------------//

Segmento* encontrar_segmento_segun_algor(uint32_t tamContenido) {
	Segmento* segmentoElegido = NULL;

	if(strcmp(criterioSeleccion,"FF") == 0){
		segmentoElegido = recorrer_first_fit(tamContenido);

	} else if(strcmp(criterioSeleccion,"BF") == 0){
		segmentoElegido = recorrer_best_fit(tamContenido);
	}

	return segmentoElegido;
}

uint32_t tamanioSegmento(Segmento* unSegmento){
	uint32_t tamSeg = unSegmento->tamanio;  // o sea, del 0 al 9 el tam es 10
	return tamSeg;
}

Segmento* recorrer_first_fit(uint32_t tamContenido) {

	log_info(logger, "Se buscara un segmento con un tamaño de %d segun FF", tamContenido);

	bool esSegmentoUtil(Segmento* unSegmento) {
		uint32_t tamSegmento = tamanioSegmento(unSegmento);
		return (unSegmento -> ocupado) == 0 && tamSegmento >= tamContenido;
	}

	Segmento* segmentoElegido = list_find(memoriaListSeg, (void*)esSegmentoUtil); // list_find retorna el primer valor encontrado

	if(segmentoElegido != NULL){
		log_info(logger, "El segmento util va de %d a %d", segmentoElegido->base, (segmentoElegido->base + segmentoElegido->tamanio - 1));
	}
	else {
		log_info(logger, "No se encontro ningun seg libre!");
	}

	return segmentoElegido; // NULL si no lo encontro
}

bool esDeMenorTamanio(void* unSegmento, void* otroSegmento){
	Segmento* seg1 = unSegmento;
	Segmento* seg2 = otroSegmento;

	return seg1->tamanio < seg2->tamanio;
}

Segmento* recorrer_best_fit(uint32_t tamContenido) {
	// list_find retorna el primer valor encontrado, entonces primero creamos una lista de huecos ordenada con el best fit primero,
	// y despues hacemos list_find

	log_info(logger, "Se buscara un segmento con un tamaño de %d segun BF", tamContenido);

	t_list* listaOrdenada = list_sorted(memoriaListSeg, (void*)esDeMenorTamanio);

	bool esSegmentoUtil(Segmento* unSegmento) {
		uint32_t tamSegmento = tamanioSegmento(unSegmento);
		return (unSegmento -> ocupado) == 0 && tamSegmento >= tamContenido;
	}

	Segmento* segmentoElegido = list_find(listaOrdenada, (void*)esSegmentoUtil);

	if(segmentoElegido != NULL){
		log_info(logger, "El segmento adecuado va de %d a %d", segmentoElegido->base, (segmentoElegido->base + segmentoElegido->tamanio - 1));
	}
	else {
		log_info(logger, "No se encontro ningun seg libre!");
	}

	return segmentoElegido; // NULL si no lo encontro
}

uint32_t guardarSegmento(void* contenido, uint32_t tamContenido, tipoDeContenido tipoDeContenido, bool noSeCompacto, uint32_t PID)
{
	Segmento* segALlenar;
	Segmento* pedazoSegALlenarLleno = malloc(sizeof(Segmento));
	Segmento* pedazoSegALlenarVacio = malloc(sizeof(Segmento));
	uint32_t direcc_logica = -1;

	segALlenar = encontrar_segmento_segun_algor(tamContenido);

	if(segALlenar == NULL && noSeCompacto){
		compactar();
		//en compactacion volver a llamar a segmento a ver si entró
		return guardarSegmento(contenido, tamContenido, tipoDeContenido, false, PID); // unico caso en que llamo a la funcion con noSeCompacto = false
	}
	else if(segALlenar == NULL && !noSeCompacto){
		log_error(logger, "Aun despues de compactar, no hay lugar en memoria para guardar el contenido");
		return direcc_logica;
	}

	int baseSegALlenar = segALlenar->base;
	int tamanioSegALlenar = segALlenar->tamanio;

	pedazoSegALlenarLleno->base = baseSegALlenar;
	pedazoSegALlenarLleno->tamanio = tamContenido;
	pedazoSegALlenarLleno->ocupado = 1;
	pedazoSegALlenarLleno->contenido = tipoDeContenido;
	pedazoSegALlenarLleno->PID = PID;
	agregarSegmento(pedazoSegALlenarLleno);
	// si justo ocupa un hueco entero sin dejar nada libre, entonces no existe mas el hueco

	guardar_contenido_en_memoria(pedazoSegALlenarLleno->base, contenido, tamContenido);

	int baseAnterior = pedazoSegALlenarLleno->base;

	if(pedazoSegALlenarLleno->tamanio < segALlenar->tamanio){
		// ahora el hueco sigue existiendo pero es mas chico, actualizo el tam
		pedazoSegALlenarVacio->base = baseAnterior + tamContenido;
		pedazoSegALlenarVacio->tamanio = tamanioSegALlenar - tamContenido;
		pedazoSegALlenarVacio->ocupado = 0;
		pedazoSegALlenarVacio->PID = 0;
		agregarSegmento(pedazoSegALlenarVacio);
		log_info(logger, "Se actualiza el tam de un hueco para que vaya desde la direcc %d a la %d", pedazoSegALlenarVacio->base, (pedazoSegALlenarVacio->base + pedazoSegALlenarVacio->tamanio - 1));
	}

	eliminarSegmento(segALlenar);

	direcc_logica = pedazoSegALlenarLleno->base;
	log_info(logger, "La direccion logica del comienzo del segmento guardado es %d", direcc_logica);

	//free(segALlenar);

	return direcc_logica;
}

bool esSegLibre(void* unSeg){
		Segmento* seg = unSeg;
		return !(seg -> ocupado);
}

bool esSegOcupado(void* unSeg){
		Segmento* seg = unSeg;
		return (seg -> ocupado);
}

bool estanContiguosYOcupados(Segmento* unSegmento, Segmento* otroSegmento){
	return esSegOcupado(unSegmento) && esSegOcupado(otroSegmento) &&
			(unSegmento->tamanio == (otroSegmento->base + 1) ||
			otroSegmento->tamanio == (unSegmento->base + 1));
}

void compactar(void)
{
	uint32_t ultimoLugarOcupadoEnMemoria = 0; // o sea, el limite del ultimo segmento ocupado
	void* memoriaAux = malloc(tamMemoria); // VOY A PONER EL RESULTADO DE LA COMPACTACION ACA

	log_info(logger, "Comienza el proceso de compactacion");

	//pthread_mutex_lock(&mutex_compactacion); // mientras compacto, que nadie guarde nada!!
	pthread_mutex_lock(&mutex_segmentos);
	pthread_mutex_lock(&mutex_memoria);

	void actualizarSegmentosParaQueTodosEstenContiguos(Segmento* unSegmento){
		if(unSegmento->ocupado){
			log_info(logger, "Se actualizara un segmento para compactarlo con su anterior");
			pthread_mutex_unlock(&mutex_memoria);
			void* contenidoSegmento = buscarEnMemoriaSegunEstrucAdmin(unSegmento); // antes de actualizar, me guardo su contenido
			pthread_mutex_lock(&mutex_memoria);
			//unSegmento -> base = segmentoAnterior -> limite + 1; // los obligo a ser contiguos
			//unSegmento -> limite = unSegmento -> base + tamSegmento;
			unSegmento -> base = ultimoLugarOcupadoEnMemoria;
			guardar_contenido_en_memoria_aux(memoriaAux, unSegmento -> base, contenidoSegmento, unSegmento->tamanio);
			//guardar_contenido_en_memoria(unSegmento -> base, contenidoSegmento, tamSegmento);
			ultimoLugarOcupadoEnMemoria += unSegmento -> tamanio;
			if(unSegmento->contenido == PCBenum){
				Patota* patota = buscarPatotaPorId(listaPatotas, unSegmento->PID);
				if(patota == NULL){
					log_error(logger, "Estaba por actualizar el puntero PCB pero no econtre la patota");
				}
				else{
					int base = unSegmento -> base;
					log_info(logger, "La direcc del PCB de la patota %d cambio a %d", patota->PID, base);
					patota->direccPCB = base;
				}
			}
			free(contenidoSegmento);
		}
	}

	t_list* listaDeSegmentosQueAntesEstabanLibres = list_filter(memoriaListSeg, (void*)esSegLibre); // el filter retorna una NUEVA lista, no estoy modificando la original

	list_iterate(memoriaListSeg, actualizarSegmentosParaQueTodosEstenContiguos);

	pthread_mutex_unlock(&mutex_memoria);
	pthread_mutex_unlock(&mutex_segmentos);
	guardar_contenido_en_memoria(0, memoriaAux, tamMemoria); // piso lo que habia en la memoria con esto nuevo!!

	free(memoriaAux); // para no hacer mallocs de mas, libero la auxiliar

	//pthread_mutex_unlock(&mutex_compactacion);

	// elimino todos los segmentos que antes estaban libres porque ahora como quedan todos contiguos, va a haber un unico segmento libre que vaya
	// desde el ultimo segmento escrito, hasta que se termine la memoria

	list_iterate(listaDeSegmentosQueAntesEstabanLibres, (void*)eliminarSegmento);

	Segmento* unicoSegmentoLibre = malloc(sizeof(Segmento));
	unicoSegmentoLibre->base = ultimoLugarOcupadoEnMemoria;
	unicoSegmentoLibre->tamanio = tamMemoria - ultimoLugarOcupadoEnMemoria;
	unicoSegmentoLibre->ocupado = false;
	unicoSegmentoLibre->PID = 0;

	agregarSegmento(unicoSegmentoLibre);

	/*void recorrerPatotas(Patota* patota){
		void actualizarDireccPCB(TCB* unTCB){
			int direccPCB = patota->direccPCB;
			unTCB->punteroPCB = direccPCB;
			encontrarOActualizarTCB(patota, unTCB->TID, unTCB, true);
		}
		list_iterate(patota->listaDeLosTCB, (void*)actualizarDireccPCB);
	}

	pthread_mutex_lock(&mutex_patotas);
	list_iterate(listaPatotas, (void*)recorrerPatotas);
	pthread_mutex_unlock(&mutex_patotas);*/
}


Segmento* recorrerListaSegmentos(t_list* listaSeg, uint32_t PID, uint32_t identificador){
	//si el identificador es 0  busca por tareas, si es otro valor busca por trip

	Segmento* segBuscado;

	bool buscarElSegmentoDeTareas(Segmento* elmentoDeListaSeg){
		return elmentoDeListaSeg->contenido == TAREAenum && elmentoDeListaSeg->PID == PID && identificador == 0 && elmentoDeListaSeg->ocupado;
	}

	bool buscarElSegmentoDelTripulante(Segmento* elmentoDeListaSeg){
		if(elmentoDeListaSeg->contenido == TCBenum && elmentoDeListaSeg->PID == PID && elmentoDeListaSeg->ocupado){
			TCB* contenidoSeg = buscarEnMemoriaSegunEstrucAdmin(elmentoDeListaSeg); //TODO: se pierden muchos bytes aca segun valgrind
			return contenidoSeg->TID == identificador;
		}
		return false;
	}

	if(identificador == 0){
		segBuscado = list_find(listaSeg, (void*)buscarElSegmentoDeTareas);
		if(segBuscado != NULL){
			log_info(logger, "Encontré el segmento de tareas");
		}
	}
	else {
		segBuscado = list_find(listaSeg, (void*)buscarElSegmentoDelTripulante);
		if(segBuscado != NULL){
			log_info(logger, "Encontré el segmento del tripulante con id %d", identificador);
		}
	}

	return segBuscado;
}

void escucharSenialParaCompactar(){
	while(1) // se queda escuchando infinitamente y cuando recibe la senial, compacta
	{
		sleep(1);
		signal(SIGUSR1,compactar);
	}
}

void esperarSenialParaCompactar(){
	pthread_t hiloCompactacion;
	pthread_create(&hiloCompactacion,NULL, (void*)escucharSenialParaCompactar, NULL);
	pthread_detach(hiloCompactacion);
}

Segmento* devolverSegmentoPCB(uint32_t patotaID){

	Segmento* segBuscado;

	bool buscarPCB(Segmento* elmentoDeListaSeg){
		return elmentoDeListaSeg->contenido == PCBenum && elmentoDeListaSeg->PID == patotaID;
	}

	segBuscado = list_find(memoriaListSeg, (void*)buscarPCB);
	//segBuscado = list_find(listaSegPatota, (void*)buscarPCB);

	log_info(logger, "Encontré el segmento del PCB");

	return segBuscado;
}

TCB* encontrarOActualizarTCB(Patota* patota, uint32_t tripulanteID, TCB* unTCB, bool hayQueActualizarEnMemoria){

	if(strcmp(esquemaMemoria,"PAGINACION") == 0)
	{
		PaginasPatota* infoPagsPatota = obtenerInfoPaginasPatota(patota->PID);
		if(infoPagsPatota == NULL){
			log_error(logger, "No se encontro la info de la patota %d", patota->PID);
		}

		uint32_t tamTodasLasTareas = infoPagsPatota->longitudTareas;

		uint32_t indexEnListaDeIDTrips = obtenerIndexEnListaDeIdTrips(infoPagsPatota->idsTripulantes, tripulanteID);

		uint32_t offsetDondeEmpiezaElTrip = tamTodasLasTareas + tamPCB + (indexEnListaDeIDTrips * tamTCB);


		//-------tareas1-PCB1-tripulante1-tripulante2------

		//------tareas2-PCB2-tripulante3-tripulante4--

		//-------------------------------53
		//

		uint32_t numDePrimeraPag = ceil(offsetDondeEmpiezaElTrip / tamPagina); //redondea para abajo

		uint32_t numDeUltimaPag = ceil((offsetDondeEmpiezaElTrip + tamTCB) / tamPagina); //redondea para abajo

		t_list* paginas = list_create();

		if(numDePrimeraPag == numDeUltimaPag){
			log_info(logger, "El tripulante %d se encuentra en la pagina %d de la patota %d", tripulanteID, numDePrimeraPag, patota->PID);
			list_add(paginas, ubicarPaginaPorNroDePagina(numDePrimeraPag, patota->PID));
		}
		else{
			log_info(logger, "El tripulante %d se encuentra desde la pagina %d hasta la %d de la patota %d", tripulanteID, numDePrimeraPag, numDeUltimaPag, patota->PID);
			uint32_t nroPag = numDePrimeraPag;
			while(nroPag <= numDeUltimaPag){
				list_add(paginas, ubicarPaginaPorNroDePagina(nroPag, patota->PID));
				nroPag ++;
			}
		}

		t_list* paginasQueNoEstanEnRam = list_create();

		paginasQueNoEstanEnRam = list_filter(paginas, (void*)noEstaEnRam);

		if(!list_is_empty(paginasQueNoEstanEnRam)){
			void hacerSwappeo(Pagina* pagATraer){
				swap(pagATraer->indexEnMemVirtual);
			}

			list_iterate(paginasQueNoEstanEnRam, (void*)hacerSwappeo);
			//listaDeLasPagsDelTCB = paginasDelTripulante(patota, tripulanteID); // ahora si todas van a estar en ram
		}

		t_list* listaFramesQueTienenAlTCB = list_create();

		void armarListaDeFrames(Pagina* pag){

			bool esElFrame(Frame* frame){
				return frame->nroFrame == pag->nroFrame;
			}

			Frame* elFrame = list_find(listaFramesTodos, (void*)esElFrame);

			list_add(listaFramesQueTienenAlTCB, elFrame);

			actualizarUltimaReferencia(pag); //hay que tenerlo actualizado para LRU
			activarBitUso(pag);
		}

		list_iterate(paginas, (void*)armarListaDeFrames);
		list_destroy(paginas);

		void* contenidoFrames = malloc(list_size(listaFramesQueTienenAlTCB) * tamPagina);
		int offset = 0;

		void traerContenidoDeFrames(Frame* frame){
			void* contenido = buscarEnMemoriaSegunEstrucAdmin(frame);
			memcpy(contenidoFrames + offset, contenido, tamPagina);
			offset += tamPagina;
			free(contenido);
		}

		list_iterate(listaFramesQueTienenAlTCB, (void*)traerContenidoDeFrames);

		int offsetDelTrip = offsetDondeEmpiezaElTrip % tamPagina;

		//Frame* primerFrameDelTrip = list_get(listaFramesQueTienenAlTCB, 0);
		//void* tcbSerializado = obtener_de_memoria(primerFrameDelTrip->direccionFisica + offsetDelTrip, tamTCB);

		void* tcbSerializado = malloc(tamTCB);

		memcpy(tcbSerializado, contenidoFrames + offsetDelTrip, tamTCB);

		TCB* tcbDeserializado = deserializarTCB(tcbSerializado);

		if(!hayQueActualizarEnMemoria){
			return tcbDeserializado;
		}
		else if(hayQueActualizarEnMemoria && unTCB != NULL){

			tcbDeserializado = unTCB;
			void* tripSerializadoo = malloc(tamTCB);
			tripSerializadoo = serializarTCB(tcbDeserializado);
			memcpy(contenidoFrames + offsetDelTrip, tripSerializadoo, tamTCB);

			uint32_t offsetDeFrames = 0;
			void guardarContenidoEnFrames(Frame* frame){
				guardar_contenido_en_memoria(frame->direccionFisica, contenidoFrames + offsetDeFrames, tamPagina);
				offsetDeFrames += tamPagina;
			}

			list_iterate(listaFramesQueTienenAlTCB, (void*)guardarContenidoEnFrames);
			free(tripSerializadoo);
		}

		return NULL;
	}

	else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0)
	{
		Segmento* segmentoDelTrip = recorrerListaSegmentos(patota->tablaSegmentos, patota->PID, tripulanteID);
		if(segmentoDelTrip != NULL && segmentoDelTrip->ocupado){
			TCB* trip = buscarEnMemoriaSegunEstrucAdmin(segmentoDelTrip);

			if(!hayQueActualizarEnMemoria){
				return trip;
			}
			else if(hayQueActualizarEnMemoria && unTCB != NULL){
				trip = unTCB;
				guardar_contenido_en_memoria(segmentoDelTrip->base, trip, tamTCB);
			}

			free(trip);
		}
		else{
			log_warning(logger, "El tripulante %d ya habia sido expulsado asi que su segmento fue marcado como libre");
		}

		return NULL;
	}
}

char* encontrarTareasSegunEstrucAdmin(Patota* patota, uint32_t identficadorTarea){

	if(strcmp(esquemaMemoria,"PAGINACION") == 0)
	{
		PaginasPatota* infoPagsPatota = obtenerInfoPaginasPatota(patota->PID);
		if(infoPagsPatota == NULL){
			log_error(logger, "No se encontro la info de la patota %d", patota->PID);
		}

		if(identficadorTarea >= list_size(infoPagsPatota->infoTareas)){
			return "FIN"; // ya no hay mas tareas!
		}

		TareasPatota* infoTareas = list_get(infoPagsPatota->infoTareas, identficadorTarea);

		if((identficadorTarea + 1) == list_size(infoPagsPatota->infoTareas)){ // la ultima
			infoTareas->longitud --; // no tiene el /n la ultima
		}

		uint32_t numDePrimeraPag = ceil(infoTareas->offset / tamPagina); //redondea para abajo

		uint32_t numDeUltimaPag = ceil((infoTareas->offset + infoTareas->longitud) / tamPagina); //redondea para abajo

		t_list* paginas = list_create();

		if(numDePrimeraPag == numDeUltimaPag){
			list_add(paginas, ubicarPaginaPorNroDePagina(numDePrimeraPag, patota->PID));
		}
		else{
			uint32_t nroPag = numDePrimeraPag;
			while(nroPag <= numDeUltimaPag){
				list_add(paginas, ubicarPaginaPorNroDePagina(nroPag, patota->PID));
				nroPag ++;
			}
		}

		t_list* paginasQueNoEstanEnRam = list_create();

		paginasQueNoEstanEnRam = list_filter(paginas, (void*)noEstaEnRam);

		if(!list_is_empty(paginasQueNoEstanEnRam)){
			void hacerSwappeo(Pagina* pagATraer){
				swap(pagATraer->indexEnMemVirtual);
			}

			list_iterate(paginasQueNoEstanEnRam, (void*)hacerSwappeo);
		}

		//pthread_mutex_lock(&mutex_pags_ram); // es el mismo mutex que usa swap!

		t_list* listaFramesQueTieneLaTarea = list_create();

		void armarListaDeFrames(Pagina* pag){

			bool esElFrame(Frame* frame){
				return frame->nroFrame == pag->nroFrame;
			}

			Frame* elFrame = list_find(listaFramesTodos, (void*)esElFrame);

			list_add(listaFramesQueTieneLaTarea, elFrame);

			actualizarUltimaReferencia(pag); //hay que tenerlo actualizado para LRU
			activarBitUso(pag);
		}

		pthread_mutex_lock(&mutex_frames_ram);
		list_iterate(paginas, (void*)armarListaDeFrames);

		list_destroy(paginas);

		void* contenidoFrames = malloc(list_size(listaFramesQueTieneLaTarea) * tamPagina);
		int offset = 0;

		void traerContenidoDeFrames(void* frame){
			void* contenido = buscarEnMemoriaSegunEstrucAdmin(frame);
			memcpy(contenidoFrames + offset, contenido, tamPagina);
			offset += tamPagina;
			free(contenido);
		}

		list_iterate(listaFramesQueTieneLaTarea, (void*)traerContenidoDeFrames);
		pthread_mutex_unlock(&mutex_frames_ram);

		//pthread_mutex_unlock(&mutex_pags_ram);

		int offsetDentroDePag = infoTareas->offset % tamPagina;

		void* tarea = malloc(infoTareas->longitud);

		memcpy(tarea, contenidoFrames + offsetDentroDePag, infoTareas->longitud);

		return tarea;
	}
	else if(strcmp(esquemaMemoria,"SEGMENTACION") == 0)
	{
		Segmento* segmentoDeTareas = recorrerListaSegmentos(patota->tablaSegmentos, patota->PID, 0);
		if(segmentoDeTareas != NULL){
			return buscarEnMemoriaSegunEstrucAdmin(segmentoDeTareas);
		}
		else{
			log_error(logger, "No se encontro el segmento de tareas de la patota %d", patota->PID);
			return NULL;
		}
	}
	return NULL;
}


//---------------------------FUNCIONES PAGINACION----------------------------//

uint32_t guardarPagina(void* contenido, uint32_t tamContenido, uint32_t PID)
{
	//bool noSeHizoSwap = true;
	//uint32_t cantPaginasAUsar = (tamContenido + 32) / 64;
	uint32_t cantPaginasAUsar = (uint32_t)ceil((double)tamContenido / tamPagina); //ceil redondea para arriba siempre
	uint32_t direccFisicaComienzo = -1;
	uint32_t direccEnVirtual = -1; // solo se usa en el caso que t o d o el contenido vaya a virtual
	uint32_t cantidadFramesVacios = cantFramesVacios();

	if(hayMemoriaPag(cantPaginasAUsar)){ // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

		direccFisicaComienzo = popularPaginasEnMemoriaRAM(cantPaginasAUsar, PID, tamContenido); // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

		guardar_contenido_en_memoria(direccFisicaComienzo,contenido,tamContenido); // ACA SE GUARDA EN EL VOID* MEMORIA

		/*
		log_info(logger, "Si reconstruyo lo que guarde recien, trayendo pag por pag: ");

		void* contenidoFrames = malloc(cantPaginasAUsar * tamPagina);
		int offset = 0;
		int direcc = direccFisicaComienzo;
		for(int i = 0; i < cantPaginasAUsar; i++){
			void* contenido = obtener_de_memoria(direcc, tamPagina);
			memcpy(contenidoFrames + offset, contenido, tamPagina);
			offset += tamPagina;
			free(contenido);
		}

		char* mem = mem_hexstring(contenido, tamPagina * cantPaginasAUsar); // hay un par de bytes de sobra ahi
		log_info(logger, "%s", mem);
		*/
	}

	else if(cantidadFramesVacios > 0){ //O SEA, NO HAY LUGAR EN RAM PARA TODAS PERO PARA ALGUNAS SI
		/*todo una restruccion en swap que no deje elegir como victima las
		paginas recien agregadas a ram?*/
		uint32_t cantidadDePaginasAGuardarEnRam = cantidadFramesVacios;
		uint32_t cantidadDePaginasASwappear = cantPaginasAUsar - cantidadFramesVacios;

		for(int i = 0; i < cantidadDePaginasASwappear; i++){
			swap(-1); // esto significa: liberame un frame
		}

		if(hayMemoriaPag(cantPaginasAUsar)){ // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

			direccFisicaComienzo = popularPaginasEnMemoriaRAM(cantPaginasAUsar, PID, tamContenido); // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

			guardar_contenido_en_memoria(direccFisicaComienzo,contenido,tamContenido); // ACA SE GUARDA EN EL VOID* MEMORIA
		/* todo esto esta en los otros casos pero en este mno?
		void* contenidoFrames = malloc(cantPaginasAUsar * tamPagina);
				int offset = 0;
				uint32_t direcc = direccFisicaComienzo;
				for(int i = 0; i < cantPaginasAUsar; i++){
					void* contenido = obtener_de_memoria(direcc, tamPagina);
					memcpy(contenidoFrames + offset, contenido + offset, tamPagina);
					offset += tamPagina;
					direcc += tamPagina;
					free(contenido);
				}

				char* mem = mem_hexstring(contenido, tamPagina * cantPaginasAUsar); // hay un par de bytes de sobra ahi
				log_info(logger, "%s", mem);
		*/
		}
		else {
			direccFisicaComienzo = -1;
			log_error(logger, "Algo salio mal en el swap, no se liberaron la cantidad de marcos necesarios");
		}

		free(contenido);

	} else { //NO HAY NI UN FRAME LIBRE, hay que swappear por cantPAginasAUsar


		for(int i = 0; i < cantPaginasAUsar; i++){
			swap(-1); // esto significa: liberame un frame
		}

		if(hayMemoriaPag(cantPaginasAUsar)){ // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

				direccFisicaComienzo = popularPaginasEnMemoriaRAM(cantPaginasAUsar, PID, tamContenido); // SE PUEDE GUARDAR T O D O EN MEMORIA RAM

				guardar_contenido_en_memoria(direccFisicaComienzo,contenido,tamContenido); // ACA SE GUARDA EN EL VOID* MEMORIA
		}
		else {
			direccFisicaComienzo = -1;
			log_error(logger, "Algo salio mal en el swap, no se liberaron la cantidad de marcos necesarios");
		}

		/*
		log_info(logger, "Si reconstruyo lo que guarde recien POST SWAP, trayendo pag por pag: ");

		void* contenidoFrames = malloc(cantPaginasAUsar * tamPagina);
		int offset = 0;
		uint32_t direcc = direccFisicaComienzo;
		for(int i = 0; i < cantPaginasAUsar; i++){
			void* contenido = obtener_de_memoria(direcc, tamPagina);
			memcpy(contenidoFrames + offset, contenido + offset, tamPagina);
			offset += tamPagina;
			direcc += tamPagina;
			free(contenido);
		}

		char* mem = mem_hexstring(contenido, tamPagina * cantPaginasAUsar); // hay un par de bytes de sobra ahi
		log_info(logger, "%s", mem);
		*/
	}

	return direccFisicaComienzo;
}

uint32_t popularPaginasEnMemoriaRAM(uint32_t cantPaginas, uint32_t PID, uint32_t tam) {
	t_list* listaVacios = list_filter(listaFramesTodos, (void*)frameLibre);
	t_list* marcosAUsar = list_take_and_remove(listaVacios, cantPaginas);
	int i = 0;
	Frame* marcoAUsar = NULL;
	uint32_t direccFisicaComienzo = -1;

	void crearPaginaYAsignarAMarco(Frame* marco){
		Pagina* paginaALlenar = malloc(sizeof(Pagina));
		marcoAUsar = marco;
		//paginaALlenar->nroFrame = marcoAUsar->nroFrame;
		memcpy(&(paginaALlenar->nroFrame), &(marcoAUsar->nroFrame), sizeof(uint32_t));
		paginaALlenar->nroPagina = i;
		paginaALlenar->estaEnRAM = 1;
		paginaALlenar->bitDeUso = 1;
		paginaALlenar->ultimaReferencia = timestamp(); // para LRU
		paginaALlenar->direccionFisica = paginaALlenar->nroFrame * tamPagina;
		paginaALlenar->PID = PID;
		paginaALlenar->indexEnMemVirtual = -1;
		paginaALlenar->bytesOcupados = (i+1) == cantPaginas ? tam - (i * tamPagina) : tamPagina; // si la primera pag (nroPag=0) == cantPaginasAUsar, entonces es la ultima pag ocupada que tiene fragmentacion interna, sino esta toda ocupada
		//marcoAUsar->nroPagina = paginaALlenar->nroPagina; // esto significa que apuntan a lo mismo
		memcpy(&(marcoAUsar->nroPagina), &(paginaALlenar->nroPagina), sizeof(uint32_t)); // esto "duplica" el contenido. tipo copy paste
		marcoAUsar->ocupado = 1;
		marcoAUsar->PID = PID;

		if(i == 0){ // o sea, solo el primero
			direccFisicaComienzo = marcoAUsar->direccionFisica;
		}

		Patota* patota = buscarPatotaPorId(listaPatotas, PID);
		agregarPaginaALaListaDeLaPatota(patota, paginaALlenar);

		agregarPaginaARam(paginaALlenar);
		i++;

		log_info(logger, "Se guardara el contenido en el marco %d, pagina %d", marcoAUsar->nroFrame, paginaALlenar->nroPagina);
	}

	list_iterate(marcosAUsar, (void*)crearPaginaYAsignarAMarco);

	return direccFisicaComienzo;
}

void agregarPaginaALaListaDeLaPatota(Patota* patota, Pagina* pag){
	pthread_mutex_lock(&patota->mutex_tablas);
	list_add(patota->tablaDePaginas, pag);
	pthread_mutex_unlock(&patota->mutex_tablas);
}

void eliminarPaginaDeLaListaDeLaPatota(Patota* patota, Pagina* pag){
	pthread_mutex_lock(&patota->mutex_tablas);

	bool esLaPag(Pagina* pagina){
		return pag->nroPagina == pagina->nroPagina;
	}

	list_remove_by_condition(patota->tablaDePaginas, (void*)esLaPag);
	pthread_mutex_unlock(&patota->mutex_tablas);
}

bool hayMemoriaPag(uint32_t cantPaginasAUsar)
{
	uint32_t cantMarcosVacios = cantFramesVacios();

	return cantMarcosVacios >= cantPaginasAUsar;
}

bool frameLibre(Frame* marco){
	return marco->ocupado == 0;
}

uint32_t cantFramesVacios(){
	pthread_mutex_lock(&mutex_frames_ram);
	t_list* listaVacios = list_filter(listaFramesTodos, (void*)frameLibre);
	uint32_t cant = list_size(listaVacios);
	pthread_mutex_unlock(&mutex_frames_ram);
	return cant;
}

void actualizarUltimaReferencia(Pagina* pagina) {
	pagina->ultimaReferencia = timestamp();
}

void swap(uint32_t indexPagAPonerEnRam){

	uint32_t cantidadFramesVacios = cantFramesVacios();
	log_info(logger, "EMPIEZA SWAP");

	pthread_mutex_lock(&mutex_pags_ram);

	if(cantidadFramesVacios != 0 && indexPagAPonerEnRam != -1){
		Frame* marcoALlenar = list_find(listaFramesTodos, (void*)frameLibre);
		Pagina* pagAPonerEnRam = buscarEnMemoriaVirtual(indexPagAPonerEnRam);

		uint32_t nroFrame = marcoALlenar->nroFrame;
		uint32_t nroPagina = pagAPonerEnRam->nroPagina;
		uint32_t PID = pagAPonerEnRam->PID;

		pagAPonerEnRam->estaEnRAM = 1;
		pagAPonerEnRam->bitDeUso = 1;
		pagAPonerEnRam->ultimaReferencia = timestamp(); // para LRU
		pagAPonerEnRam->direccionFisica = nroFrame * tamPagina;
		pagAPonerEnRam->indexEnMemVirtual = -1;
		pagAPonerEnRam->nroFrame = nroFrame;

		pthread_mutex_lock(&mutex_frames_ram);
		marcoALlenar->nroPagina = nroPagina;
		marcoALlenar->ocupado = 1;
		marcoALlenar->PID = PID;
		pthread_mutex_unlock(&mutex_frames_ram);

		void* contenidoPagAPonerEnRam = obtenerDeMemoriaVirtual(indexPagAPonerEnRam * tamPagina, tamPagina);

		guardar_contenido_en_memoria(marcoALlenar->direccionFisica,contenidoPagAPonerEnRam, tamPagina); //guardo en el void ram

		free(contenidoPagAPonerEnRam);

		if(list_size(listaPagsVirtual) == 1){ //esto no lo hago siempre xq desp me quedan pags con indice 4 pero solo hay 2 xq saque
			sacarDeMemVirtual(pagAPonerEnRam); //saco de la struct amdin virtual
		}

		log_info(logger, "[SWAP] Se guardo la pagina %d de la patota %d buscada en memoria virtual en la memoria RAM en el marco %d", pagAPonerEnRam->nroPagina, pagAPonerEnRam->PID, marcoALlenar->nroFrame);

	}
	else{
		if(indexPagAPonerEnRam != -1){//elijo victima, saco de ram y pongo en virtual, saco de virutal y pongo en ram
			Pagina* paginaVictima = buscarPaginaAReemplazarSegunAlgoritmo();

			pthread_mutex_lock(&mutex_paginas_memVirtual);
			Pagina* pagDeVirtualARam = list_replace(listaPagsVirtual, indexPagAPonerEnRam, paginaVictima); //reemplazo en virtual pq el replace pone la pag victima en virtual y devuelve la pag a poner en ram
			pthread_mutex_unlock(&mutex_paginas_memVirtual);

			//Pagina* pagAPonerEnRam = buscarEnMemoriaVirtual(indexPagAPonerEnRam); // no hace falta porque es la misma que pagDeVirtualARam

			//asigno a auxiliar valores para ponerla en virtual
			//guardo direccion fisica en mem virtual que voy a pisar
			uint32_t dirFisicaAGuardarEnMemVirtual = pagDeVirtualARam->direccionFisica;
			//guardo direccion fisica en mem ppal que voy a pisar
			uint32_t dirFisicaAGuardarNuevaPag = paginaVictima->direccionFisica;

			//asigno a pagAPonerEnRam los valores para ponerla en ram
			uint32_t frameEnRam = paginaVictima -> nroFrame;

			pagDeVirtualARam->bitDeUso = 1;
			pagDeVirtualARam->ultimaReferencia = timestamp(); // para LRU
			pagDeVirtualARam->estaEnRAM = 1;
			pagDeVirtualARam->nroFrame = frameEnRam;
			pagDeVirtualARam->direccionFisica = dirFisicaAGuardarNuevaPag;
			pagDeVirtualARam->indexEnMemVirtual = -1;

			paginaVictima->estaEnRAM = 0;
			paginaVictima->nroFrame = -1;
			paginaVictima->direccionFisica = dirFisicaAGuardarEnMemVirtual;
			paginaVictima->indexEnMemVirtual = indexPagAPonerEnRam;

			//TODO: chequear si este mutex está bien pq en realidad si lo pongo acá no voy a tener el puntero para cualdo haga lo de los algoritmos, pero si lo hago mas arriba de donde saco el puntero?
			//pthread_mutex_lock(&mutex_clock);
			//punteroParaClock = frameEnRam + 1; // mantenemos actualizado esto en la funcion de clock ya
			//pthread_mutex_unlock(&mutex_clock);

			void* contenidoPagVictima = malloc(tamPagina);
			contenidoPagVictima = obtener_de_memoria(dirFisicaAGuardarNuevaPag, tamPagina);

			void* contenidoPagAPonerEnRam = malloc(tamPagina);
			contenidoPagAPonerEnRam = obtenerDeMemoriaVirtual(dirFisicaAGuardarEnMemVirtual, tamPagina);

			guardar_contenido_en_memoria(dirFisicaAGuardarNuevaPag,contenidoPagAPonerEnRam, tamPagina);
			guardarContenidoEnMemoriaVirtual(dirFisicaAGuardarEnMemVirtual,contenidoPagVictima, tamPagina);

			free(contenidoPagVictima);
			free(contenidoPagAPonerEnRam);

			//dump(); // no descomentarlo xq tiene otro lock de mutex_pags_ram que me bloquea t o d o

			actualizarFramesRAMConElCambio();

			log_info(logger, "[SWAP] Se reemplazo la pagina %d de la patota %d en memoria RAM por la pagina %d de la patota %d", paginaVictima->nroPagina, paginaVictima->PID, pagDeVirtualARam->nroPagina, pagDeVirtualARam->PID);
		}
		else {//llamo a swap para liberar ram pero sin tener algo que virtual para giardar en ram
			Pagina* paginaVictima = buscarPaginaAReemplazarSegunAlgoritmo();
			uint32_t frameEnRam = paginaVictima -> nroFrame;
			uint32_t dirFisicaLiberada = paginaVictima->direccionFisica;

			liberarMarco(frameEnRam);

			uint32_t indiceEnElQueSeAgrego = agregarEnMemVirtual(paginaVictima);
			uint32_t dirFisicaEnMemVirtual = indiceEnElQueSeAgrego * tamPagina;

			paginaVictima->estaEnRAM = 0;
			paginaVictima->nroFrame = -1;
			paginaVictima->indexEnMemVirtual = indiceEnElQueSeAgrego;
			paginaVictima->direccionFisica = dirFisicaEnMemVirtual;

			//TODO: chequear si este mutex está bien pq en realidad si lo pongo acá no voy a tener el puntero para cualdo haga lo de los algoritmos, pero si lo hago mas arriba de donde saco el puntero?
			//pthread_mutex_lock(&mutex_clock);
			//punteroParaClock = frameEnRam + 1; // mantenemos actualizado esto en la funcion de clock ya
			//pthread_mutex_unlock(&mutex_clock);

			void* contenidoPagVictima = malloc(tamPagina);
			contenidoPagVictima = obtener_de_memoria(dirFisicaLiberada, tamPagina);

			guardarContenidoEnMemoriaVirtual(dirFisicaEnMemVirtual,contenidoPagVictima, tamPagina);

			free(contenidoPagVictima);

			//actualizarFramesRAMConElCambio(); // nada que actualizar, ya se libero

			log_info(logger, "[SWAP] Se movio a swap la pagina %d de la patota %d", paginaVictima->nroPagina, paginaVictima->PID);
		}
	}

	pthread_mutex_unlock(&mutex_pags_ram);
}

void actualizarFramesRAMConElCambio(void){
	pthread_mutex_lock(&mutex_frames_ram);
	t_list* listaPagsPresentesEnRam = list_filter(listaPaginasGeneral, (void*)paginaPresente);

	void actualizarLosMarcosCorrespondientes(Pagina* pagina){
		uint32_t numDePag = pagina->nroPagina;
		uint32_t PID = pagina->PID;
		Frame* unFrame = ubicarFramePorNroDeFrame(pagina->nroFrame);
		if(unFrame != NULL){
			unFrame->nroPagina = numDePag;
			unFrame->PID = PID;
		}
	}

	list_iterate(listaPagsPresentesEnRam, (void*)actualizarLosMarcosCorrespondientes);
	pthread_mutex_unlock(&mutex_frames_ram);
}


uint32_t agregarEnMemVirtual(Pagina* paginaAAgregar)
{
	int indexEnElQueAgrego = -1;
	pthread_mutex_lock(&mutex_paginas_memVirtual);
	indexEnElQueAgrego = list_add(listaPagsVirtual, paginaAAgregar);
	pthread_mutex_unlock(&mutex_paginas_memVirtual);

	return indexEnElQueAgrego;
}

void sacarDeMemVirtual(Pagina* pag){

	pthread_mutex_lock(&mutex_paginas_memVirtual);
	//list_remove(listaPagsVirtual, indice); // lo saque asi no falla si no existiera

	bool esLaPag(Pagina* pagina){
		return pagina->nroPagina == pag->nroPagina && pagina->PID == pag->PID;
	}

	list_remove_by_condition(listaPagsVirtual, (void*)esLaPag);

	pthread_mutex_unlock(&mutex_paginas_memVirtual);
}


Pagina* buscarEnMemoriaVirtual(uint32_t index)
{
	return list_get(listaPagsVirtual, index);
}

//sirve esta funcoin??
void activarBitUso(Pagina* pagina) {
	pagina->bitDeUso = true;
}

void desactivarBitUso(Pagina* pagina) {
	pagina->bitDeUso = false;
}

bool noEstaEnRam(Pagina* pagina){
	return !pagina->estaEnRAM;
}

Pagina* buscarPaginaAReemplazarSegunAlgoritmo() // como la listaPaginasGeneral es variable global, no hace falta pasarla
{
	Pagina* paginaVictima;

	if(strcmp(algoritmoReemplazo,"LRU") == 0){
		paginaVictima = seleccionoVictimaPorLRU();

	} else if(strcmp(algoritmoReemplazo,"CLOCK") == 0){
		paginaVictima = seleccionoVictimaPorCLOCK();
	}

	return paginaVictima;
}

bool paginaPresente(Pagina* pagina)
{
	return pagina->estaEnRAM;
}

Pagina* seleccionoVictimaPorLRU()
{
	t_list* memoriaRamSoloConPagsPresentes = list_filter(listaPaginasGeneral, (void*)paginaPresente);

	bool fueReferenciadaAntes(Pagina* unaPagina, Pagina* otraPagina) {
		return (unaPagina->ultimaReferencia) < (otraPagina->ultimaReferencia) ;
	}

	t_list* memoriaRamOrdenada = list_sorted(memoriaRamSoloConPagsPresentes, (void*)fueReferenciadaAntes);

	Pagina* paginaVictima = list_get(memoriaRamOrdenada, 0);

	if(paginaVictima == NULL) {
		log_error(logger, "No hay página victima obtenida por algoritmo LRU.");
	}

	return paginaVictima;
}


Pagina* seleccionoVictimaPorCLOCK()
{
	uint32_t cantidadFrames = tamMemoria / tamPagina;

	if(punteroParaClock >= cantidadFrames){ // o sea, si hay 4 frames (0,1,2,3) y apuntas al 4 o mas, ya diste la vuelta, sos el 0 de nuevo
		pthread_mutex_lock(&mutex_clock);
		punteroParaClock = 0;
		pthread_mutex_unlock(&mutex_clock);
	}

	Pagina* paginaVictima = NULL;
	uint32_t primerLugarDelPuntero = punteroParaClock;

	log_info(logger, "El puntero en primer lugar se encuentra en el frame n° %d", primerLugarDelPuntero);

	//FUNCIONES QUE VAMOS A USAR
	//paginaPresente esta afuera

	bool noTieneBitDeUso(Pagina* pagina) //es decir lo tiene en 0
	{
		return pagina->bitDeUso == 0;
	}

	bool mismoNumeroDePagina(Pagina* pag1, Pagina* pag2)
	{
		return pag1->nroPagina == pag2->nroPagina;
	}


	//filtro sólo las que estén en ram
	t_list* memoriaRamSoloConPagsPresentes = list_create();
	memoriaRamSoloConPagsPresentes = list_filter(listaPaginasGeneral, (void*)paginaPresente);

	Frame* frameDondeEstaElPuntero = ubicarFramePorNroDeFrame(primerLugarDelPuntero);

	bool laPagDelFrame(Pagina* pag){
		return frameDondeEstaElPuntero->nroPagina == pag->nroPagina;
	}

	Pagina* paginaQueEstaEnElFrameDelPuntero = list_find(memoriaRamSoloConPagsPresentes, (void*)laPagDelFrame);


	//DESARROLLO DE LA LOGICA DE CLOCK
	if(paginaQueEstaEnElFrameDelPuntero->bitDeUso == 0) //si donde esta el puntero tiene bitDeUso en 0, devuelvo esa pagina como victima
	{
		paginaVictima = paginaQueEstaEnElFrameDelPuntero;
		pthread_mutex_lock(&mutex_clock);
		punteroParaClock++;
		pthread_mutex_unlock(&mutex_clock);

		log_info(logger, "Se eligió como victima donde estaba el puntero (es decir página n° %d) porque estaba con bit de uso en %d", paginaVictima->nroPagina, paginaVictima->bitDeUso);
		//(en este caso bit de uso deberia decir 0)
	}

	else
	{
		t_list* listaConBitUsoEn0 = list_create();
		listaConBitUsoEn0 =	list_filter(memoriaRamSoloConPagsPresentes, (void*)noTieneBitDeUso);

		t_list* listaConBitUsoEn0OrdenadasPorNroFrame = list_create(); // xq como el puntero recorre frames, necesito ordenarlas en ese orden

		bool ordenPorNroFrame(Pagina* pag1, Pagina* pag2){
			return pag1->nroFrame < pag2->nroFrame;
		}

		listaConBitUsoEn0OrdenadasPorNroFrame = list_sorted(listaConBitUsoEn0, (void*)ordenPorNroFrame);

		if(list_size(listaConBitUsoEn0) != 0) //si algunos no tienen bit de uso
		{
			bool iterarAPartirDelPuntero(Pagina* unaPag){
				if(unaPag->nroFrame == punteroParaClock){ // o sea, las primeras iteraciones no hacen nada hasta que llega al frame apuntado
					log_info(logger, "Puntero de clock en frame %d", punteroParaClock);
					if(noTieneBitDeUso(unaPag)){
						log_info(logger, "Encontre mi pagina victima aca (pag %d del PID %d), porque tenia bit de uso en 0", unaPag->nroPagina, unaPag->PID);

						pthread_mutex_lock(&mutex_clock);
						punteroParaClock++;
						pthread_mutex_unlock(&mutex_clock);

						return true; // cortame la iteracion
					}
					else {
						log_info(logger, "Esta pagina no sera la victima aun (pag %d del PID %d), le cambio el bit de uso a 0", unaPag->nroPagina, unaPag->PID);
						desactivarBitUso(unaPag);

						pthread_mutex_lock(&mutex_clock);
						punteroParaClock++; // a partir de aca, siempre va a ser verdadero en las proximas iteraciones que unaPag->nroFrame == punteroParaClock
						pthread_mutex_unlock(&mutex_clock);

						return false;
					}
				}
				return false;
			}

			//list_iterate(listaConBitUsoEn0OrdenadasPorNroFrame, (void*)iterarAPartirDelPuntero);
			paginaVictima = list_find(listaConBitUsoEn0OrdenadasPorNroFrame, (void*)iterarAPartirDelPuntero);

			if(paginaVictima == NULL){ // o sea, desde donde estaba el puntero, hasta el final de la RAM, no habia ninguna con bitDeUso = 0, estaba antes del puntero
				// esto esta bien si empezamos a contar desde el principio :)
				paginaVictima = list_find(listaConBitUsoEn0OrdenadasPorNroFrame, (void*)noTieneBitDeUso);//busco la primer coincidencia dentro de las que no tienen bitDeUso y devuelvo esa pagina

				pthread_mutex_lock(&mutex_clock);
				punteroParaClock = paginaVictima->nroFrame + 1;
				pthread_mutex_unlock(&mutex_clock);
			}
			log_info(logger, "Se eligió como victima a la página n° %d del PID %d que tenía el bit de uso en %d", paginaVictima->nroPagina, paginaVictima->PID, paginaVictima->bitDeUso);
			log_info(logger, "Puntero de clock queda en el frame %d", punteroParaClock);
			// (en este caso bit de uso deberia decir 0)
		}

		else //o sea si todos tienen bit de uso
		{
			paginaVictima = paginaQueEstaEnElFrameDelPuntero;//como el puntero nunca se movió es como si hubieras dado la vuelta

			pthread_mutex_lock(&mutex_clock);
			punteroParaClock++;
			pthread_mutex_unlock(&mutex_clock);
			/*while(punteroParaClock != primerLugarDelPuntero)
			{
				paginaQueEstaEnElFrameDelPuntero->bitDeUso = 0;
				punteroParaClock++;
			}*/
			list_iterate(memoriaRamSoloConPagsPresentes, (void*)desactivarBitUso);

			log_info(logger, "Todos tenian bit de uso en 1, asi que se dio la vuelta (poniendo a todos en 0) y se eligió como victima a la página n° %d del PID %d donde estaba el puntero originalmente", paginaVictima->nroPagina, paginaVictima->PID);
			log_info(logger, "Puntero de clock queda en el frame %d", punteroParaClock);
		}

		list_destroy(listaConBitUsoEn0);
		list_destroy(listaConBitUsoEn0OrdenadasPorNroFrame);
	}

	list_destroy(memoriaRamSoloConPagsPresentes);

	return paginaVictima;
}


void sacarPagina(Pagina* pagASacar){

	bool esLaPagina(Pagina* paginaCualquiera) {
		return paginaCualquiera->nroPagina == pagASacar->nroPagina;
	}

	pthread_mutex_lock(&mutex_pags_ram);
	list_remove_by_condition(listaPaginasGeneral, (void*)esLaPagina);
	pthread_mutex_unlock(&mutex_pags_ram);
}

void agregarPaginaARam(Pagina* paginaAAgregar){
	pthread_mutex_lock(&mutex_pags_ram);
	list_add(listaPaginasGeneral, paginaAAgregar);
	pthread_mutex_unlock(&mutex_pags_ram);
}


void expulsarTripEnPaginacion(Patota* patota, uint32_t tripulanteID){
	PaginasPatota* infoPagsPatota = obtenerInfoPaginasPatota(patota->PID);

	if(infoPagsPatota == NULL){
		log_error(logger, "No se encontro la info de la patota %d", patota->PID);
	}

	uint32_t tamTodasLasTareas = infoPagsPatota->longitudTareas;

	uint32_t indexEnListaDeIDTrips = obtenerIndexEnListaDeIdTrips(infoPagsPatota->idsTripulantes, tripulanteID);

	uint32_t offsetDondeEmpiezaElTrip = tamTodasLasTareas + tamPCB + (indexEnListaDeIDTrips * tamTCB);

	uint32_t numDePrimeraPag = ceil(offsetDondeEmpiezaElTrip / tamPagina); //redondea para abajo

	uint32_t numDeUltimaPag = ceil((offsetDondeEmpiezaElTrip + tamTCB) / tamPagina); //redondea para abajo

	uint32_t bytesEnTotalALiberar = tamTCB;

	t_list* paginasDelTrip = list_create();

	if(numDePrimeraPag == numDeUltimaPag){
		Pagina* unicaPag = ubicarPaginaPorNroDePagina(numDePrimeraPag, patota->PID);
		list_add(paginasDelTrip, unicaPag);
		unicaPag->bytesOcupados -= bytesEnTotalALiberar;
		log_info(logger, "Se liberaron %d bytes de la pagina %d del proceso %d", bytesEnTotalALiberar, numDePrimeraPag, patota->PID);
	}
	else{ // para simplificar, segun las pruebas, lo maximo que ocuparia un TCB serian 2 paginas
		if(numDeUltimaPag - numDePrimeraPag == 1){
			Pagina* primeraPag = ubicarPaginaPorNroDePagina(numDePrimeraPag, patota->PID);
			list_add(paginasDelTrip, primeraPag);
			int offsetEnEstaPagina = offsetDondeEmpiezaElTrip % tamPagina;
			uint32_t bytesEnEstaPag = tamPagina - offsetEnEstaPagina;
			primeraPag->bytesOcupados -= bytesEnEstaPag;
			bytesEnTotalALiberar -= bytesEnEstaPag;

			Pagina* segundaPag = ubicarPaginaPorNroDePagina(numDeUltimaPag, patota->PID);
			list_add(paginasDelTrip, segundaPag);
			segundaPag->bytesOcupados -= bytesEnTotalALiberar;
			log_info(logger, "Se liberaron %d bytes de la pagina %d y %d de la pagina %d del proceso %d", bytesEnEstaPag, numDePrimeraPag, bytesEnTotalALiberar, numDeUltimaPag, patota->PID);
		}
		else{
			log_warning(logger, "No consideramos el caso que un TCB ocupe mas de 2 paginas");
		}
	}

	void siQuedaronVaciasBorrarlas(Pagina* pagi){
		if(pagi->bytesOcupados == 0){
			eliminarPaginaDeLaListaDeLaPatota(patota, pagi); // de la lista de la patota
			if(pagi->estaEnRAM){
				liberarMarco(pagi->nroFrame);
			}
		}
	}

	list_iterate(paginasDelTrip, (void*)siQuedaronVaciasBorrarlas);
	list_destroy(paginasDelTrip);

	patota->cantTripulantes --;

	if(patota->cantTripulantes == 0){
		void vaciarPaginas(Pagina* paginaPatota){
			paginaPatota->bytesOcupados = 0;
		}
		list_iterate(patota->tablaDePaginas, (void*)vaciarPaginas);
		list_iterate(patota->tablaDePaginas, (void*)siQuedaronVaciasBorrarlas);
		eliminarPatotaDeListaDePatotas(patota);
		log_info(logger, "Se elimino la patota PID %d ya que se quedo sin tripulantes", patota->PID);
		free(infoPagsPatota);
		free(patota);

		void eliminarLoQueQuedo(Pagina* pag){
			sacarDeMemVirtual(pag);
			sacarPagina(pag); // de la lista general
			free(pag);
		}

		if(list_is_empty(listaPatotas)){
			list_clean_and_destroy_elements(listaPagsVirtual, (void*)eliminarLoQueQuedo);
		}
	}
}


void liberarMarco(uint32_t numeroDeMarco) {
	pthread_mutex_lock(&mutex_frames_ram);
	Frame* frame = ubicarFramePorNroDeFrame(numeroDeMarco);
	frame->ocupado = 0;
	pthread_mutex_unlock(&mutex_frames_ram);
}

Frame* ubicarFramePorNroDeFrame(uint32_t numDeFrame){
	bool elFrame(Frame* frame){
		return frame->nroFrame == numDeFrame;
	}

	Frame* frame = list_find(listaFramesTodos, (void*)elFrame);
	return frame;
}

Pagina* ubicarPaginaPorNroDePagina(uint32_t numDePag, uint32_t PID){
	bool laPag(Pagina* pag){
		return pag->nroPagina == numDePag && pag->PID == PID;
	}

	Pagina* pagina = list_find(listaPaginasGeneral, (void*)laPag);
	return pagina;
}

void agregarInfoPaginasPatota(PaginasPatota* info){
	pthread_mutex_lock(&mutex_info_paginas);
	list_add(listaInfoPaginasPatota, info);
	pthread_mutex_unlock(&mutex_info_paginas);
}

PaginasPatota* obtenerInfoPaginasPatota(uint32_t PID){

	bool infoDeEstaPatota(PaginasPatota* info){
		return info->PID == PID;
	}

	PaginasPatota* laInfo = list_find(listaInfoPaginasPatota, (void*)infoDeEstaPatota);

	return laInfo;
}

uint32_t obtenerIndexEnListaDeIdTrips(t_list* lista, uint32_t tripId){
	for(int i = 0; i < list_size(lista); i++){
		uint32_t unId = list_get(lista, i);
		if(unId == tripId){
			return i;
		}
	}
}


void guardar_contenido_en_memoria(uint32_t offset, void* contenido, uint32_t tamanio){

	pthread_mutex_lock(&mutex_memoria);
	memcpy(memoria + offset, contenido, tamanio);
	pthread_mutex_unlock(&mutex_memoria);

	log_info(logger, "Se guardo un contenido a memoria desde la posicion %d hasta la %d.", offset, offset + tamanio - 1);
}

void* obtener_de_memoria(uint32_t offset, uint32_t tamanio){

	void* contenido = malloc(tamanio);

	pthread_mutex_lock(&mutex_memoria);

	memcpy(contenido, memoria + offset, tamanio);

	pthread_mutex_unlock(&mutex_memoria);

	log_info(logger, "Se obtuvo un contenido de memoria desde la posicion %d hasta la %d.", offset, offset + tamanio - 1);

	return contenido;
}

void guardar_contenido_en_memoria_aux(void* memoria_aux, uint32_t offset, void* contenido, uint32_t tamanio){

	memcpy(memoria_aux + offset, contenido, tamanio);

	//log_info(logger, "Se guardo un contenido en una variable auxiliar desde la posicion %d hasta la %d.", offset, offset + tamanio - 1);
}

void guardarContenidoEnMemoriaVirtual(uint32_t offset, void* contenido, uint32_t tamanio){

	pthread_mutex_lock(&mutex_memoriaVirtual);

	escribirMMAP(contenido, tamanio, offset);

	pthread_mutex_unlock(&mutex_memoriaVirtual);

	log_info(logger, "Se guardo un contenido a memoria virtual desde la posicion %d hasta la %d.", offset, offset + tamanio - 1);
}

void* obtenerDeMemoriaVirtual(uint32_t offset, uint32_t tamanio){

	pthread_mutex_lock(&mutex_memoriaVirtual);

	void* contenido = malloc(tamanio);

	contenido = leerMMAP(offset, tamanio);

	log_info(logger, "Se obtuvo un contenido de memoria virtual desde la posicion %d hasta la %d.", offset, offset + tamanio - 1);

	pthread_mutex_unlock(&mutex_memoriaVirtual);

	return contenido;
}

//MMAP

void escribirMMAP(void* contenido ,uint32_t tamanio, uint32_t offset){

	void* memVirtual;
	int fd = 0;
	struct stat fileInfo;
	size_t fileSizeNew;

	if ((fd = open(pathSwap, O_RDWR | O_CREAT, (mode_t)0664 )) == -1) {
		perror("open");
		exit(1);
	}
	if (stat(pathSwap, &fileInfo) == -1) {
		perror("stat");
		exit(1);
	}

	fileSizeNew = fileInfo.st_size + tamanio;

	if (ftruncate(fd, fileSizeNew) == -1) {
		close(fd);
		perror("Error resizing the file");
		exit(1);
	}

	memVirtual = mmap(0, fileSizeNew, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (memVirtual == MAP_FAILED) {
		close(fd);
		perror("mmap");
		exit(1);
	}

	memcpy(memVirtual + offset, contenido, tamanio);

	if (msync(memVirtual, fileSizeNew, MS_SYNC) == -1) {
		perror("Could not sync the file to disk");
	}

	if (munmap(memVirtual, fileSizeNew) == -1) {
		close(fd);
		perror("Error un-mmapping the file");
		exit(1);
	}

	close(fd);

}

void* leerMMAP(uint32_t offset, uint32_t tamanio){

	void* memVirtual;
	void* contenido = malloc(tamanio);

	int fd = 0;
	struct stat fileInfo;


	if ((fd = open(pathSwap, O_RDWR | O_CREAT, (mode_t)0664 )) == -1) {
		perror("open");
		exit(1);
	}
	if (stat(pathSwap, &fileInfo) == -1) {
		perror("stat");
		exit(1);
	}

	memVirtual = mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (memVirtual == MAP_FAILED) {
		close(fd);
		perror("mmap");
		exit(1);
	}

	//Copio algo en mi memoria virtual
	memcpy(contenido, memVirtual + offset , tamanio);

	if (munmap(memVirtual, fileInfo.st_size) == -1) {
		close(fd);
		perror("Error un-mmapping the file");
		exit(1);
	}

	close(fd);

	return contenido;
}


/*
Segmento* segALlenar;
	Segmento* pedazoSegALlenarLleno = malloc(sizeof(Segmento));
	Segmento* pedazoSegALlenarVacio = malloc(sizeof(Segmento));
	bool noSeCompacto = true;
	uint32_t direcc_logica = -1;

	segALlenar = encontrar_segmento_segun_algor(contenido, tamContenido);

	if(segALlenar == NULL && noSeCompacto){
		//compactacion();
		//en compactacion volver a llamar a segmento a ver si entró
		noSeCompacto = 0;
	}
	else if(segALlenar == NULL && !noSeCompacto){
		log_error(logger, "Aun despues de compactar, no hay lugar en memoria para guardar el contenido");
		return direcc_logica;
	}
	else {
		pedazoSegALlenarLleno->base = segALlenar->base;
		pedazoSegALlenarLleno->limite = pedazoSegALlenarLleno->base + tamContenido - 1; // si tengo un contenido de 3, guardo del 0 al 2
		pedazoSegALlenarLleno->ocupado = 1;
		agregarSegmento(pedazoSegALlenarLleno);
	// si justo ocupa un hueco entero sin dejar nada libre, entonces no existe mas el hueco

		guardar_contenido_en_memoria(pedazoSegALlenarLleno->base, contenido, tamContenido);
		if(pedazoSegALlenarLleno->limite < segALlenar->limite){
			// ahora el hueco sigue existiendo pero es mas chico, actualizo el tam
			pedazoSegALlenarVacio->base = pedazoSegALlenarLleno->limite + 1;
			pedazoSegALlenarVacio->limite = segALlenar->limite;
			pedazoSegALlenarVacio->ocupado = 0;
			agregarSegmento(pedazoSegALlenarVacio);
			log_info(logger, "Se actualiza el tam de un hueco para que vaya desde la direcc %d a la %d", pedazoSegALlenarVacio->base, pedazoSegALlenarVacio->limite);
		}
		eliminarSegmento(segALlenar);
		//guardar_contenido_en_memoria(pedazoSegALlenarLleno->base, contenido, tamContenido);
		direcc_logica = pedazoSegALlenarLleno->base;
		log_info(logger, "La direccion logica del comienzo del segmento guardado es %d", direcc_logica);
	}
	free(segALlenar);
	return direcc_logica;




// --------------------GUARDAR EN MEMORIA ES ALGO ASI:--------------------//
/*
memcpy(memoria[segmentoDelPCB.base], patotaID, 4); // ver bien el orden de los parametros de memcpy
memcpy(memoria[segmentoDelPCB.base + offset], 4, direccionInicioTareas);
*/


// dejo ideas de compactar:

/*uint32_t limiteUlimoOcupado = 0;


	for(int i = 0; i < list_size(memoriaListSeg); i ++)
	{
		Segmento* segEnIndice = list_get(memoriaListSeg,i);
		if(!(segEnIndice->ocupado))
		{
			//como sacamos el ultimo ocupado? no deberia ser fuera del if?
			//limiteUlimoOcupado = ultimoOcupado -> limite;


		}

		while (memoriaListSeg != NULL){   // ?????
			//itero hasta que segEnIndice ocupado
		}

		//busco el primer libre. una vezx que lo encontre, busco el proximo ocupado. La base de ese ocupado va a ser el valor 'i' en el que nos quedamos

		//en una struct podemos inicalizar un valor? preguntamos x tamanioSegmento

		limiteUlimoOcupado = 0;
	}

*/

//recorre y busco primer hueco->base. t odo lo que este arriba de ese hueco se queda quieto.
	/* seg 2 (base 151 lim 200 tam 50 -> le pongo ka base del hueco
	  hueco (base 111 limt 150 tam 40  --> lo actualizo cuando termino. no voy actualizandolo en el medio.
	  seg 1 (base 100, lim 110  tam 11 //se queda quieto
	  seg o base 0 lim 99  //se queda quieto */

	//podria hacerse en un for, o un while con un for.

	//t_list* listaOrdenadaDeHuecos = list_sorted(segmentos_huecos, esDeMenorTamanio);

	/*se me ocurre cargar a una lista todos los ocupados, cargar en otra los heucos
	borrar la memoria
	volver a cargar primero todos los ocupados
	probelma -> reasignar las bases y limites*/





