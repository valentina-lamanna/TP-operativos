#include "mi-ram-hq.h"


//-------HEADER-------//
void dump();
void escucharSenialDump();
void esperarSenialParaDump();

//------FUNCIONES-----//
void dump(){

  char* tiempo = temporal_get_string_time("%H:%M:%S:%MS"); //("%H:%M:%S:%MS") => "12:51:59:331"
  char* nombreArchivoDump = string_new();
  char* textoDump = string_new();

  string_append(&nombreArchivoDump, "Dump_");
  string_append(&nombreArchivoDump, tiempo);
  string_append(&nombreArchivoDump, ".dmp"); // o sea, el nombre del archivo queda algo tipo "Dump_12:51:59:331.dmp"


  string_append(&textoDump, "-------------------------------------------------------------------------------------------\n");
  string_append(&textoDump, "Dump: ");
  string_append(&textoDump, temporal_get_string_time("%d/%m/%y %H:%M:%S")); //  => "30/09/20 12:51:59"
  string_append(&textoDump, "\n");

  if(strcmp(esquemaMemoria,"SEGMENTACION") == 0){
		pthread_mutex_lock(&mutex_segmentos);

		int i = 1;

		void dumpSegmentos(Segmento* segmento)
		{
			if(segmento->ocupado){
				string_append(&textoDump, "Proceso: ");
				string_append(&textoDump, string_itoa(segmento->PID));
				string_append(&textoDump, "\t Segmento: ");
				string_append(&textoDump, string_itoa(i));
				string_append(&textoDump, "\t Inicio: ");
				string_append(&textoDump, string_itoa(segmento->base));
				string_append(&textoDump, "\t Tam: ");
				string_append(&textoDump, string_itoa(segmento->tamanio));
				string_append(&textoDump, "\n");
			}
			else if(segmento->PID == 0) {
				string_append(&textoDump, "Proceso: ");
				string_append(&textoDump, "-");
				string_append(&textoDump, "\t Segmento: ");
				string_append(&textoDump, string_itoa(i));
				string_append(&textoDump, "\t Inicio: ");
				string_append(&textoDump, string_itoa(segmento->base));
				string_append(&textoDump, "\t Tam: ");
				string_append(&textoDump, string_itoa(segmento->tamanio));
				string_append(&textoDump, "\n");
			}

			i ++;
		}

		void iteradorDePatotas(Patota* patota)
		{
			i = 1;
			list_iterate(patota->tablaSegmentos, (void*)dumpSegmentos);
		}

		if(!list_is_empty(listaPatotas)){
			pthread_mutex_lock(&mutex_patotas); // para que no se pueda iniciar una patota mientras las listo
			list_iterate(listaPatotas, (void*)iteradorDePatotas);
			pthread_mutex_unlock(&mutex_patotas);
		}
		else {
			i = 1;
			list_iterate(memoriaListSeg, (void*)dumpSegmentos);
		}
		pthread_mutex_unlock(&mutex_segmentos);

  } else if(strcmp(esquemaMemoria,"PAGINACION") == 0){

		pthread_mutex_lock(&mutex_pags_ram);

		void dumpFrames(Frame* frame)
		{
			string_append(&textoDump, "Marco: ");
			string_append(&textoDump, string_itoa(frame->nroFrame));
			string_append(&textoDump, "\t Estado: ");
			if(frame->ocupado){
				string_append(&textoDump, "Ocupado");
				string_append(&textoDump, "\t Proceso: ");
				string_append(&textoDump, string_itoa(frame->PID));
				string_append(&textoDump, "\t Pagina: ");
				string_append(&textoDump, string_itoa(frame->nroPagina));
				string_append(&textoDump, "\n");
			}
			else {
				string_append(&textoDump, "Libre \t");
				string_append(&textoDump, "\t Proceso: ");
				string_append(&textoDump, "-");
				string_append(&textoDump, "\t Pagina: ");
				string_append(&textoDump, "-");
				string_append(&textoDump, "\n");
			}
		}

		list_iterate(listaFramesTodos, (void*)dumpFrames);
		pthread_mutex_unlock(&mutex_pags_ram);
	}

  string_append(&textoDump, "-------------------------------------------------------------------------------------------");

  //creo el archivo y lo escribo
  FILE *fDump = fopen(nombreArchivoDump, "w+"); //TODO: descomentar esto

  fprintf(fDump,"%s", textoDump); //escribe dentro del archivo dump

  fclose(fDump);

  //printf("%s", textoDump); // esto se va a sacar desp TODO: comentar esto
}

void escucharSenialDump(){
	while(1) // se queda escuchando infinitamente y cuando recibe la senial, hace el dump
	{
		sleep(1);
		signal(SIGUSR2,dump);
	}
}

void esperarSenialParaDump(){
	pthread_t hiloDump;
	pthread_create(&hiloDump, NULL, (void*)escucharSenialDump, NULL);
	pthread_detach(hiloDump);
}
