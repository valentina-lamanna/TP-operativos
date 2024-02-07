# TP Sistemas Operativos - GrupoChicasSistOp2021 🛠️

_1C - 2021_

## A-mongOs 🚀

[Consigna del TP](https://docs.google.com/document/d/1u54jk7uKaa6BOAXgLuNVfeYN_mwPBje94iX_6KqvqJo/edit#)

Esta es una descripcion de los mensajes entre modulos, no es necesario que las estructuras sean tal cual se nuestra aca ni con los mismos nombres


[Un documento de pruebas](https://docs.google.com/document/d/1RVXZn5TePp9IB53Aae4inOGEpwc9Fi9PKy_vEJBNilA/edit#)


El [archivo de pruebas finales](https://docs.google.com/document/d/1rorLRNnwpM9kKxeuHAwM0mmNN20X0Mb20Q-I66y6sdc/edit) del TP


----------------------------------------------------------------------------------------------------

_INICIAR_PATOTA_ 🚀

#### [Discordiador]
Manda primero la patota y despues tripulantes:
- PATOTA
```
 	uint32_t 	PID;
  	uint32_t 	cantTripulantes;
 	uint32_t  	tamTodasLasTareas
 	char*		todasLasTareas;
```
- TRIPULANTE
```
	uint32_t 	TID;
	uint32_t 	posicionX;
	uint32_t 	posicionY;
	uint32_t* 	punteroPID;
```  
#### [RAM] 
Recibe en ese orden

_Detalle: la estructura de tripulantes en RAM es el "TCB", eso tiene patotaID y punteroPCB, que son casi lo mismo pero patotaID es un numero normal y punteroPCB apunta al PID de su patota. Tener el patotaID nos sirve para deseralizar, capaz a futuro se unifican esos dos_

---------------------------------------------------------------------------------------------------

_NUEVO_TRIP_ 🚀

#### [Discordiador]
Manda una estructura de bitacora con el primer mensaje:
- BITACORA
```
	uint32_t 	tripID;
  	uint32_t    	tamAccion;
  	char*		accion;
```  
#### [Mongo] 
Recibe en ese orden, y debe crear su bitacora escribiendo ese primer mensaje


--------------------------------------------------------------------------------------------------

_PROX_TAREA_ 🚀

#### [Discordiador]
Indica quien la necesita (tripulante id) y a que patota pertenece (patota id)
- TRIPULANTE
```
	uint32_t 	TID;
	uint32_t* punteroPID;
```  

#### [RAM] 
Recibe en ese orden, pero como antes, guarda en patotaID (o sea, no en un puntero) lo que recibe
Luego, debe mandar un char* indicando la tarea correspondiente, algo asi:
- TRIPULANTE
```
	char* tarea;
```  

#### [Discordiador]
Recibe la tarea y hace lo que tenga que hacer

----------------------------------------------------------------------------------------------------

_ACTUALIZAR_ESTADO_ 🚀

#### [Discordiador]
Manda el tripulante, el nuevo estado y a que patota pertence
- TRIPULANTE
```
	uint32_t 	TID;
	char*	 	estado;
	uint32_t* 	punteroPID;
```  
#### [RAM] 
Recibe en ese orden

_Detalle: la estructura de tripulantes en RAM es el "TCB", eso tiene patotaID y punteroPCB, que son casi lo mismo pero patotaID es un numero normal y punteroPCB apunta al PID de su patota. Tener el patotaID nos sirve para deseralizar, capaz a futuro se unifican esos dos_

---------------------------------------------------------------------------------------------------

_RECIBIR_UBICACION_TRIPULANTE_ 🚀

#### [Discordiador]
Indica quien se mueve (tripulante id), a que patota pertenece (patota id), y a donde se mueve en el mapa
- TRIPULANTE
```
	uint32_t 	TID;
  	uint32_t 	posicionX;
	uint32_t 	posicionY;
	uint32_t* 	punteroPID;
```  

#### [RAM] 
Recibe en ese orden, pero como antes, guarda en patotaID (o sea, no en un puntero) lo que recibe
Luego, actualiza en el mapa ese tripulante con esas nuevas posiciones

---------------------------------------------------------------------------------------------------

_AGREGAR y CONSUMIR_ 🛠️ falta pensarlo, lo que hay aca es una primera idea

#### [Discordiador]
Indica quien utilizara un recurso (tripulante id)? el recurso, el caracter a agregar/sacar y la cantidad
- RECURSO MENSAJE
```
	uint32_t 	TID; // ver si es necesario
	char*     	recurso;
  	char      	caracter;
  	int       	cantidad;
```  

#### [Mongo] 
Recibe en ese orden, y debe actualizar los archivos correspondiente

---------------------------------------------------------------------------------------------------

_ESCRIBIR_BITACORA_ 🚀

#### [Discordiador]
Manda una estructura de bitacora en los siguientes casos, segun la consigna:
- Se mueve de X|Y a X’|Y’
- Comienza ejecución de tarea X
- Se finaliza la tarea X
- Se corre en pánico hacia la ubicación del sabotaje
- Se resuelve el sabotaje

La estructura es la misma que en _NUEVO_TRIP_
- BITACORA
```
	uint32_t 	tripID;
  	uint32_t   	tamAccion;
  	char*		accion;
```  

#### [Mongo] 
Recibe en ese orden, y debe actualizar la bitacora correspondiente

----------------------------------------------------------------------------------------------------

_EXPULSAR_TRIPULANTE_ 🛠️ falta pensarlo, lo que hay aca es una primera idea

#### [Discordiador]
Indica quien es expulsado (tripulante id) y a que patota pertenece (patota id)
- TRIPULANTE
```
	uint32_t 	TID;
	uint32_t* punteroPID;
```  

#### [RAM] 
Recibe en ese orden, pero como antes, guarda en patotaID (o sea, no en un puntero) lo que recibe
Debe _"dejar de mostrarlo en el mapa y en caso de que sea necesario eliminar su segmento de tareas"_ segun consigna

---------------------------------------------------------------------------------------------------

_OBTENER_BITACORA_ 🚀

#### [Discordiador]
Para simplificar, pasamos la misma estructura que en _ESCRIBIR_BITACORA_
- BITACORA
```
	uint32_t 	tripID;
  	uint32_t   	tamAccion;
  	char*		accion;
```  

#### [Mongo] 
Recibe en ese orden, y debe enviar la bitacora correspondiente

La envia como un mensaje, un ```char*```

----------------------------------------------------------------------------------------------------

_SABOTAJE_ 🛠️ falta pensarlo, lo que hay aca es una primera idea

NOTA: Se utiliza este op_code para cualquier tipo de mensaje dentro de lo que son sabotajes
 
#### [Discordiador]
Apenas termina de crearse la primera patota, el discordiador crea un hilo que va a estar siempre atento a escuchar sabotajes y crea un socket
con Mongo para que le mande por ahi los sabotajes

- SABOTAJE
```
	char* mensaje; // nada relevante, seria algo como "estoy esperando sabotajes"
``` 

#### [Mongo] 
Recibe por ese socket ese mensaje y no hace nada hasta que le llegue la señal de que hay un sabotaje y debe avisarle al discordiador

- SABOTAJE
```
	char* direccion_sabotaje; // la direcc del mapa, algo como "1|2"
``` 
 
#### [Discordiador]
Cuando se encuentra el tripulante indicado para resolver el sabotaje, le avisa a mongo que ya se posiciono, idicandole que puede empezar a reparar lo necesario

- SABOTAJE
```
	char* mensaje; // por ejemplo el mensaje "FSCK"
``` 

#### [Mongo] 
Recibe por ese socket, y ya no debe enviar mas nada, simplemente resuelve el sabotaje

_NOTA: tal vez haya que volver a avisar que se estan esperando nuevos sabotajes_

----------------------------------------------------------------------------------------------------



