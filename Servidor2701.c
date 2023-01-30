#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>
#include <mysql.h>

//------------------------------------------------------------------------------
int contador;
//Estructura para acceso excluyente
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//------------------------------------------------------------------------------
//Variables globales:
//	SQL:
MYSQL* conn;
int err;
MYSQL_RES *resultado;
MYSQL_ROW row;

//Variables para la lista sockets
int i=0;
int sockets[100];
int idP;//ID de partida

//	C:
//	Variable para contar posiciones en nuestra lista de conectados:
int s = 0;
//------------------------------------------------------------------------------
//Estructuras:
//	Declaramos la clase Conectdo
typedef struct {
	//Declaramos una estructura que almacena un nombre y su socket
	char nombre[20];
	int socket;
}Conectado;
//	ListaConectados
typedef struct {
	Conectado conectados[100];
	int num;
}ListaConectados;
//Declaramos la clase Partida
typedef struct {
	int IDPartida;
	int numJugadores;
	int turno;
	char tablero[30];
	
	//Quien crea la partida
	char Host[20];
	
	//Quienes se unen a la partida del host
	char nom2[20];
	char nom3[20];
	char nom4[20];
	
	//Sockets ce hasta un maximo de 4 participantes
	int sock[4];
	//Usaremos un 0 si la partida esta vacia y un 1 si esta ocupada
	int ocupado;
}Partida;
//Declaramos la lista de Partidas
typedef struct {
	Partida Partidas [100];
	int num;
} ListaPartidas;
//	Creamos la lista
ListaConectados miLista;
ListaPartidas miListaPartidas;
//------------------------------------------------------------------------------
//FUNCIONES GENERAL:
// 	Inicializamos nuestras lista
void Inicializar(ListaConectados *listaC,ListaPartidas *listaP){
	//La funcion Inicializar se encarga de preparar nuestras listas, ListaConectados
	//y ListaPartidas. Para ello la variable num, usada para indicar el numero de 
	//conectados y partidas que hay acutalmente en nuestras listas, hasta un maximo
	//de 100 para ambas.
	
	//Ademas para la ListaPartidas nos encargaremos de que todos las partidas de la 
	//lista queden vacia, es decir con la variable ocupado igual a zero.
	
	printf("Inicializando....\n");
	listaC->num=0;
	listaP->num=0;
	for (i=0;i<100;i++){listaP->Partidas[i].ocupado=0;}
	printf("Inicializado....\n");
}
//Limpia todos los char para evitar el ruido
void Limpieza(char peticion[1024],char respuesta [1024],char conectado [200], char notificionG [1024], char notificionP [1024],char nG [1024], char nP [1024]){
	//Esta funcion busca asegurarse de ningun tipo de ruido se cuele en una peticion 
	//por lo que borra el contenido almecenado en todas las variables introducidas. 
	printf("Iniciando proceso de limpieza.......................................\n");
	peticion[0]='\0'; printf("Peticion limpia....%s \n",peticion);
	respuesta[0]='\0'; printf("Respuesta limpia....%s \n",respuesta);
	conectado[0]='\0'; printf("Conectado limpio....%s \n",conectado);
	notificionG[0]='\0'; printf("Notificacio General limpia.... %s \n",notificionG);
	notificionP[0]='\0'; printf("Notificacio Partida limpia.... %s \n",notificionP);
	nG[0]='\0'; printf("Auxiliar General limpio.... %s \n", nG);
	nP[0]='\0'; printf("Auxiliar Partida limpio.... %s \n", nP);
	printf("Proceso de limpieza finalizado.......................................\n");
}
//Para pruebas y comprobaciones de codigo
int PruebaConfirmacion(ListaConectados *lista, char respuesta [1024], char confirmacion [10], char nomO [20], char nomD [20], int *sock_conn){
	//Prueba de confimracion simplemente es una funcion con la unica finalidad de
	//hacer pruebas sin necesidad de poner en marcha el C y C#. No tiene una funcion
	//real en nuestro juego.
	//int socketD; int socketO;
	//socketD = ReturnSocket(&miLista, nomD);
	//socketO =  ReturnSocket(&miLista, nomO);
	//printf("%d %d \n",socketD, socketO);
	//if (strcmp(confirmacion, "SI")==0){sprintf(respuesta,"10/ %s ha aceptado el desafio",nomD);}
	//else{sprintf(respuesta,"10/ Vaya, %s esta ocupado",nomD);}
	//*sock_conn = socketO;
}
//Cerrar el AtenderCliente
void Cierre(char respuesta [1024], int terminar){
	//Se encarga de cerrar el servidor, para ello volevemos la variable terminar a 
	//1 por lo que el loop que hace funcionar al servidor se detendra, y prepara
	// una notificacion para que posteriormente sea enviada.
	printf("Cierre.......................................\n");
	sprintf(respuesta,"Adios");
	sprintf(respuesta,"%d/%s",0,respuesta);
	terminar=1;
}
//______________________________________________________________________________
//FUNCIONES DE BUSQUEDA:
//	Buscamos la posicion de un usuario en nuestra lista
int SearchPosition (ListaConectados *lista, char nombre[20]){
	//Accedemos a nuestra lista de conectado y buscamos al dueño del vector de 
	//caracteres conocido como nombre. Para ello usaremos un while, while  que 
	//recorrera la lista entera o se detendra si este encuentra una coincidencia.
	//La funcion puede devorlver o bien la posicion donde se encuentra el nombre
	//o bien devuelve un menos uno, este ultimo indicando que no ha enctrado 
	//coincidencia alguna.
	printf("Inicio SearchPosition.......................................\n");
	int i =0;
	int encontrado=0;	
	while ((i<lista->num) && (encontrado==0)){
		if (strcmp(lista->conectados[i].nombre,nombre)==0){encontrado=1;} else{i=i+1;}
	}
	if (encontrado == 1){return i;} else{return -1;}
	printf("Final SearchPosition.......................................\n");
}
//Funcion que devuelve la posicion o -1 si no esta en la lista de conectados.
int GiveMeSocket (ListaConectados *lista, char nombre[20]){
	//Similar a la funcio SearchPosition, esta funcion recorrera nuestra lista de
	//conectados buscando que la variable o vector de caracteres conocida como nombre
	//coincida con la variable nombre de la lista. Si encuentra una coincidencia 
	//devuelve su socket, por otro lado si no hay coincidencias devolvera un -1, 
	//indicando que no se ha encontrado coincidencia.
	int i = 0; int encontrado = 0;
	
	printf("Iniciando el GiveMeSocket.......................................\n");
	while((i < 100) && !encontrado)
	{
		if (strcmp(lista->conectados[i].nombre,nombre) == 0){encontrado = 1;}
		if(!encontrado){i = i+1;}
	}
	if (encontrado){return lista->conectados[i].socket;} else{ return -1;}
	printf("Final GiveMeSocket.......................................\n");
}
//Devuelve el Socket en el caso de lo que lo encuentre
int ReturnSocket (ListaConectados *lista, char nombre[20]){
	//Similar a la funcio SearchPosition, esta funcion recorrera nuestra lista de
	//conectados buscando que la variable o vector de caracteres conocida como nombre
	//coincida con la variable nombre de la lista. Si encuentra una coincidencia 
	//devuelve su socket, por otro lado si no hay coincidencias devolvera un -1, 
	//indicando que no se ha encontrado coincidencia.
	int i = 0;
	int encontrado = 0;
	printf("Inicio ReturnSocket.......................................\n");
	printf("Numero: %d\n",lista->num);
	while((i < 100) && (encontrado==0))
	{
		if (strcmp(lista->conectados[i].nombre,nombre) == 0){printf("%d\n",lista->conectados[i].socket);encontrado = 1;}
		if(encontrado==0){i = i+1;}
	}
	if (encontrado==1){return lista->conectados[i].socket;} else{return -1;}
	printf("Final ReturnSocket.......................................\n");
}
int GiveMeSocketsJP(ListaPartidas *listaP, int id, int socketsJug[4]) {
	//Esta funcion recibe una posicion de la lista partidas. Posicion que usara
	//para acceder a los datos de una partida en particular. Los datos que recojera
	//en un vector de integrers son los sockets de los jugadores. Devuelve ademas el
	//numero de juegadores de la partida.
	int i=0;
	printf("Inicio GiveMeSocketsJP.......................................\n");
	while(i< listaP->Partidas[id].numJugadores){socketsJug[i]= listaP->Partidas[id].sock[i]; i++;}
	return listaP->Partidas[id].numJugadores;
	printf("Final GiveMeSocketsJP.......................................\n");
}
//______________________________________________________________________________
//GESTION CONECTADOS:
//	Creamos un string con todos los conectados 
void DameConectados (ListaConectados *lista, char respuesta [1024]){
	//Llenamos nuestro vector de caracteres respuesta con los nombres de todos los
	//jugadores que se encuentran actualemente conectados. Para ello usaremos una 
	//funcion for que recorrera la lista tanta veces como usuarios tenga hasta un 
	//maximo de 100 iteraciones.
	
	//El mensaje que se devuelve es un vector de caracteres donde primeramente tendremos
	//el numero que identifica el tipo de mensaje, para que el C# sepa como operar con el
	//luego el nuemro total de personas conectadas y posteriormente los nombres de los
	//conectados, todo ello separado por /. Por ejemplo: 6/3/Juan/Maria/Carla 
	
	char resp[1024];
	resp[0]='\0';
	printf("Resp: %s \n",resp);
	printf("Inicio DameConectados.......................................\n");
	//sprintf(respuesta,"%d",s);
	printf("Respuesta: %s\n",respuesta);
	
	int i;
	for (i=0; i<lista->num; i++){
		sprintf(resp, "%s%s/",resp, lista->conectados[i].nombre); printf("%s\n",resp);
	}
	sprintf(respuesta,"6/%s",resp);
	printf("Final DameConectados.......................................\n");
}
//	Aï¿½adimos un usuario a nuestra lista
int Add(ListaConectados *lista, char nombre[20],int socket){
	//Recibimos un vector de caracteres llamado nombre y in integrer llamado socket
	//ambos valores seran, si hay espacio, añadios a nuestra lista. Primero de todo
	//miramos si nuestra lista de conectados esta llena lista->num ==100, si este es 
	//es el caso se devuelve un -1, indicado que no se ha podido unir. Por otro lado,
	//si la lista no esta llena usaremos el parametro lista->num para inidcar en que 
	//posicion deveran guardarse el nombre y el socket, posterior a este valor num le
	//añadiremos 1 y la funcion devolvera un 0.
	printf("Inicio Add.......................................\n");
	
	printf("Numero: %d\n",lista->num);
	int a = lista->num;
	if (lista->num==100)
		return -1;
	else{
		printf("Añadiendo\n");
		strcpy(lista->conectados[lista->num].nombre,nombre);
		printf("Nombre: %s\n",lista->conectados[lista->num].nombre);
		lista->conectados[lista->num].socket = socket;
		printf("Socket: %d\n",socket);
		lista->num=lista->num+1;
		printf("Numero: %d\n",lista->num);
		s = s +1;
		printf("S: %d\n",s);
		printf("Nombre:%s Socket: %d\n",lista->conectados[a].nombre,lista->conectados[a].socket);
		
		printf("Usuario añadido\n");
		return 0;
	}
	printf("Final Add.......................................\n");
}
//	Eliminamos al usuario de la ListaConectados, al desconectarse
int Disconect(char *p,ListaConectados *lista, char notificacion [1024]){
	//Disconect hace de uso de la funcion SearchPostion para buscar el vector de caracteres,
	//nombre. Si no se encuentra al usuario la funcion disconnect devolvera un -1, indicando
	//que no se ha podido llevar acabo la operacion. Pero si encuentra el usario que se ha
	//desconectado empezaremos el proceso de borrado.
	
	//El proceso de borrado consta empezar un bucle por la posicion donde se encunetra el 
	//usuario a borrar y lo que se hace es avanzar todos los valores que le siguen una posicion
	//hacia delante. Este bucle debera detenerse como mucho a las posicon 99 pues no existe
	//posicion 101 y si el programa colapsaria si tratase de obtener datos de una posicion que 
	//no existe. Una vez hemos movido todos los demas usuarios a su nueva posicion a la variable 
	//num de nuestra lista le restaremos 1 y devolveremos un 0, indicando que la operacion ha 
	//sido un exito.
	
	printf("Inicio Disconect.......................................\n");
	char nombre [20];
	char contrasena [20];
	p = strtok( NULL, "/");
	//Obtenemos el nombre y la contraseÃ±a
	strcpy (nombre, p);
	p=strtok(NULL,"/");
	strcpy(contrasena,p);
	printf ("Nombre: %s Contraseña: %s \n", nombre, contrasena);
	int pos = SearchPosition(lista, nombre);
	printf("Posicion: %d \n",pos);
	int i;
	//la no se recibe por referncia pues ya la recibimos por referencia el en disconnect
	if (pos==-1){return -1;}
	else{
		pthread_mutex_lock(&mutex);	
		for (i=pos; i<=lista->num;i++){
			miLista.conectados[i] = miLista.conectados[i+1];
			strcpy(miLista.conectados[i].nombre, miLista.conectados[i+1].nombre);
			miLista.conectados[i].socket=miLista.conectados[i+1].socket;
		}
		miLista.num--;
		pthread_mutex_unlock(&mutex);	
		strcpy(notificacion,"21/");
		return 0;
	}
	printf("Final Disconect.......................................\n");
}
int Delete(ListaConectados *lista, char nombre[20]){
	//Disconect hace de uso de la funcion GiveMeSocket para buscar el vector de caracteres,
	//nombre. Si no se encuentra al usuario la funcion dlete devolvera un -1, indicando
	//que no se ha podido llevar acabo la operacion. Pero si encuentra el usario que se ha
	//desconectado empezaremos el proceso de borrado.
	
	//El proceso de borrado consta empezar un bucle por la posicion donde se encunetra el 
	//usuario a borrar y lo que se hace es avanzar todos los valores que le siguen una posicion
	//hacia delante. Este bucle debera detenerse como mucho a las posicon 99 pues no existe
	//posicion 101 y si el programa colapsaria si tratase de obtener datos de una posicion que 
	//no existe. Una vez hemos movido todos los demas usuarios a su nueva posicion a la variable 
	//num de nuestra lista le restaremos 1 y devolveremos un 0, indicando que la operacion ha 
	//sido un exito.
	
	printf("Iniciando Delete.......................................\n");
	int pos = GiveMeSocket (lista,nombre);
	if (pos == -1){return -1;}
	else
	{
		pthread_mutex_lock(&mutex);	
		for (int i = pos; i < lista->num-1; i++)
		{
			strcpy(lista->conectados[i].nombre, lista->conectados[i+1].nombre);
			lista->conectados[i].socket = lista->conectados[i+1].socket;
		}
		lista->num--;
		pthread_mutex_unlock(&mutex);	
		return 0;
	}
	printf("Final Delete.......................................\n");
}
//______________________________________________________________________________
//GESTION DE USUARIOS:
//	Comprueba si un usuario existe y lo aï¿½ade a la lista conectados
int ConectarUsuario (char *p,char respuesta [1024], int socket, ListaConectados *lista){
	//Cuando un usuario trata de registrase recogeremos su nombre, contraseña y socket.
	//Estos dos primeros datos vendran en un unico vector de caracteres. Es por eso que
	//en primero con la funcion strtok dividiremos esta primera cadena de caracteres y 
	//extraeremos el nombre y la contaseña. 
	
	//Posteriormente accedermos a nuestra base de datos donde buscaremos a un usuario
	//que presente el mismo nombre y contraseña. Si la consulta nos devuelve algo
	//distinto a zero, significa que se ha producido un error por lo que el vector de
	//caracteres respuesta se cargara un mensaje indicando que se a producido un error
	//en la consulta. Por otro lado si no hay ningun error en respuesta cargaremos un
	//mensaje de bienvenida que recibira este particular  usuario y añadiremos el nombre
	//y socket a nuestra lista de conectados.
	
	printf("Inicio ConectarUsuario.......................................\n");
	//Variables locales
	char nombre [20];
	char contrasena [20];
	char consulta [1024];
	char resp[1024];
	
	// Conectarse con un usuario registrado
	p = strtok( NULL, "/");
	
	//Obtenemos el nombre y la contraseÃ±a
	strcpy (nombre, p);
	p=strtok(NULL,"/");
	strcpy(contrasena,p);
	printf ("Nombre: %s Contraseña: %s \n", nombre, contrasena);
	
	//Establecemos la busqueda
	sprintf(consulta,"SELECT id FROM Jugador Where usuario = '%s' AND contrasena ='%s';",nombre,contrasena);
	//escribimos en el buffer respuestra la longitud del nombre
	
	int err = mysql_query(conn, consulta);
	if (err != 0)
	{
		printf("Ha habido un error al hacer la consuta\n");
		exit(1);
	}
	
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	
	if (row == NULL)
	{
		printf("El USERNAME y el PASSWORD no coinciden\n");
		strcpy(respuesta, "1/Vaya, parece que el nombre o el usuario son erroneos");
		write (socket, respuesta, strlen (respuesta));
	}
	
	else
	{
		while (row != NULL)
		{
			printf("Bienvenido %s \n", row[0]);
			row = mysql_fetch_row(resultado);
			sprintf(respuesta, "1/Bienvenido de vuelta %s",nombre);
			printf ("Error al consultar datos de la base %u %s \n",mysql_errno(conn), mysql_error(conn));
			pthread_mutex_lock(&mutex);	
			int r = Add(lista,nombre,socket);
			pthread_mutex_unlock(&mutex);
		}
	}
}	
//	Aï¿½ade un usuario y lo guarda en la lista conectados
int RegistrarUsuario(char *p,char respuesta [1024], int socket, ListaConectados *lista){
	//Nuevamente, pareciendose al ConectarUsuario recibiremos un vector array con tanto
	//el nombre como la contraseña, datos que ectraemos con la funcion strtok. Una vez 
	//tengamos las variables pertinentes haremos una modificacion en nuestra base de 
	//datos, añadimos al usuario que se acabamos de recibir. 
	
	//Nuevamente si ocuure algun error, err !=0 caragaremos en respuesta un mensaje 
	//informando del error. Por otro lado si la operacion se ha realizado sin mas 
	//complicaciones añadimos nuestro nuevo usuario a nuestra lista de conectados y
	//cargamos un mensaje que de la bienvenida al nuevo miembro.
	
	printf("Inicio RegistrarUsuario.......................................\n");
	printf("Socket: %d",socket);
	//Variables locales
	char nombre [20];
	char contrasena [20];
	char consulta [1024];
	char resp[1024];
	
	//Crear nuevo ususario
	p = strtok( NULL, "/");
	
	//Obentenmos su nombre y contrasenya
	strcpy (nombre, p);
	p=strtok(NULL,"/");
	strcpy(contrasena,p);
	printf ("Nombre: %s Contraseña: %s \n", nombre, contrasena);
	
	//Aï¿½adimos la funcion que queremos que haga
	sprintf(consulta, "INSERT INTO Jugador(usuario, contrasena, email, partidas_ganadas, partidas_jugadas, conectado) VALUES ('%s', '%s', 'correo@gmail.com', 2, 3, 1);", nombre,contrasena);
	//escribimos en el buffer respuestra la longitud del nombre
	
	err=mysql_query (conn, consulta);
	if (err!=0)
	{
		printf ("Error al consultar datos de la base %u %s \n",mysql_errno(conn), mysql_error(conn));
		sprintf(respuesta,"Vaya un error se ha producido un error \n");
	}
	else{
		sprintf(resp,"Bienvenido al club %s",nombre);
		pthread_mutex_lock(&mutex);	
		int r = Add(lista,nombre, socket);
		pthread_mutex_unlock(&mutex);		
	}
	sprintf(respuesta,"2/%s",resp);
	printf("Final RegistrarUsuario.......................................\n");
}
//______________________________________________________________________________
//CONSULTAS PANTALLA DE INICIO:
//	Nos devuelve en numero de victorias de un jugador
void PartidasGanadas(char *p,char respuesta [1024]){
	//La funcion PartidasGanadas, es una consulta a nuestra base de datos. La consulta en
	//si es conocer cuantas partidas ha ganado un determinado usuario, seamos nosostros o
	//no. Similar a las consultas previamente explicadas primero miraremos de conectarnos
	//a la base de datos, si ocurre cualquier problema nuestro programa cargara en respuesta
	//un mensaje informado que ha ocurrido un error. Si por el contrario, la consulta se 
	//realiza sin mayor dificultad caragremos un mensaje informando de las partidas que ha 
	//ganado un determinado usuario. Por ejemplo: 4/Maria ha ganado 5 partidas
	printf("Inicio PartidasGanadas.......................................n");
	//Variables locales
	char nombre [20];
	char consulta [1024];
	char resp[1024];
	
	////Obtenemos el nombre
	p = strtok( NULL, "/");
	strcpy(nombre,p);
	printf("Nombre: %s\n",nombre);
	
	//Realizamos la consulta
	sprintf(consulta,"SELECT Jugador.partidas_ganadas FROM Jugador WHERE Jugador.usuario ='%s' ",nombre);
	
	//Comprobacion de errores
	err=mysql_query(conn, consulta);
	if (err!=0){
		printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	printf("%s\n",row[0]);
	if (row == NULL){printf("No se ha obtenido la consulta \n"); }
	else
	{
		int ans = atoi(row[0]);
		sprintf(resp,"%s ha ganado %d partidas \n",nombre,ans);
	}
	sprintf(respuesta,"4/%s",respuesta);
	printf("Final PartidasGanadas.......................................\n");
}
//	Pregunta si un usuario esta conectado
void EstaConectado(char *p,char respuesta [1024], ListaConectados *lista){
	//Recibimos una cadena de arrays de la que extreremos un nombre, nuevamente con la
	//funcion SearchPosition buscaremos a dicho usuario. Entonces si SearchPosition nos
	//devuelve -1, cargaremos en respuesta 3/Carlos no esta conectado. Por otro lado 
	//se devuelve un valor diferente a -1, significa que ha encontrado al usuario y
	//devolvera 3/Carlos esta conectado. 
	
	//Esta funcio busca facilitar la busqueda de un jugador en caso de que nuestro data
	//gridView tenga que mostrar muchos usuario conectados simultanieamente.
	
	printf("Inicio EstaConectado.......................................\n");
	//Variables locales
	char nombre [20];
	char consulta [1024];
	char resp[1024];
	
	//Buscamos si un usuario esta conectado
	p = strtok( NULL, "/");
	strcpy (nombre, p);
	printf ("Nombre: %s\n", nombre);
	
	//Realizamos nuestra consulta, aqui no ayudaremos del SearchPosition para 
	//comprobar si esta conectado. Si nos devuelve -1 no estara conectado y 
	//si es diferente de -1 sera que si que lo esta.
	int r = SearchPosition(lista, nombre);
	
	if (r ==-1){
		printf ("No encontrado\n"); sprintf(respuesta, "%s no esta conectado en este momento", nombre);
	}
	else {
		printf ("Encontrado\n"); sprintf(resp, "%s esta conectado en este momento", nombre);
	}
	sprintf(respuesta,"3/%s",resp);
	printf("Final EstaConectado.......................................\n");
}
// Devuelve el total de partidas ganadas
void PartidasJugadas(char *p,char respuesta[1024]){
	//Recibimos una cadena de caracteres de la obtendremos un nombre, usando la funcio strtok,
	//con este nombre realizaremos una funcion en la que contaremos cuantas partidas tiene un
	//determinado usuario. Al igual que cualquier otra consulta realizada en este proyecto dos
	//posibles mensajes pueden ser cargados en la variable respuesta. Si por el motivo que sea
	//ha ocurrido un error al intenatar realizar la consulta, un mensaje informado de que ha
	//ocurrido una incidencia sera copiado en respuesta. Si se ha podido realizar la consulta
	//Se copiara un mensaje como el siguente 5/Mario ha jugado 3 partidas.
	
	printf("Inicio PartidasJugadas.......................................\n");
	//Variables locales
	char nombre [20];
	char consulta [1024];
	char resp[1024];
	
	//Obtenemos el nombre
	strcpy (nombre, p);
	printf ("Nombre: %s\n", nombre);
	
	//Realizamos la busqueda
	sprintf(consulta, "SELECT count(*) FROM (Jugador, Participacion) WHERE (Jugador.usuario = '%s') AND (Participacion.id_jugador = Jugador.id)",nombre);
	
	//Comprobacion de errores
	err=mysql_query(conn, consulta);
	if (err!=0){
		printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	printf("%s\n",row[0]);
	if (row == NULL){printf("No se ha obtenido la consulta\n"); }
	else
	{
		int ans = atoi(row[0]);
		sprintf(resp,"%s ha jugado %d partidas\n",nombre,ans);
	}
	sprintf(respuesta,"%d/%s",5,resp);
	printf("Final PartidasJugadas.......................................\n");
}
//Funcion eleccion avatars
int Avatars(char* p, char respuesta[1024]) {
	//Recibimos un vector de caracteres del que extremos la variable, el proceso es el mismo
	//que los casos comentado con anterioridad. Una vez el nombre es obtenido haremos una consulta
	//donde mediante el nombre buscaremos que avatar se ha asignado, aqui no recibiremos la imagen
	//sino un numero que luego el C# ya tiene asignado una determinada imagen. Lo mismo que con el
	//resto de consultas, si hay un error copiamos en resupesta una notificacion del error, si la 
	//consulta se ha realizado con exito cargarmos un mensaje como el siguiente: 7/Marcos/1
	printf("Inicio EleccionAvatars\n");
	//Variables locales
	char nombre[20];
	char consulta[1024];
	char resp[1024];
	
	//Realizamos la busqueda
	sprintf(consulta, "SELECT Partida.avatar FROM (Partida,Jugador) WHERE Jugador.usuario = '%s'", nombre);
	
	//Comprobacion de errores
	err = mysql_query(conn, consulta);
	if (err != 0) {
		printf("Error al consultar datos de la base %u %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	printf("%s", row[0]);
	if (row == NULL){printf("No se ha obtenido la consulta \n");}
	else{
		int ans = atoi(row[0]);
		sprintf(resp, "%s/%d \n", nombre, ans);
	}
	sprintf(respuesta, "7/%s",resp);
	printf("Final EleccionAvatars....\n");
}
//______________________________________________________________________________
//GESTION DE PARTIDAS:
//Busca si hay algun espacio libre para crear una partida.
int EspacioLibre (char nombre [20],ListaConectados *listaC, ListaPartidas *lista, int sock_conn){
	//Introducimos un vector de arrays que contendra el nombre y el socket del usuario que quiere 
	//crear una partida. Primero miraremos si nuestra lista de partidas esta llena, hay 100 partidas 
	//en funcionamiento, si no hemos alcanzado el maximo de partidas empezaremos a buscar en nuestra 
	//lista un espacio libre para ello haremos un loop que buscara un espacio vacio si lo encuentra 
	//detendra la busqueda y devolvera un 1. Si no encuentra ningun espacio devolvuera un 0. En ambos
	//casos decidira si una partrida esta ocupada o no con una de las variables de partida llamada
	//ocupado.
	
	int e=0; int i=0; int r=0; int IdPartida=0;
	printf("Iniciando busqueda.......................................\n");
	pthread_mutex_lock(&mutex);	
	printf("Numero de partidas: %d\n",lista->num);
	if (lista->num<100){
		printf("Menos de 100\n");
		while ((e==0) && (i<100)){
			printf("Iniciando while\n");
			if ((lista->Partidas[i].ocupado == 0)){
				printf("Espacio libre encontrado.......................................\n");
				strcpy(lista->Partidas[i].Host, nombre);
				printf("Nombre del Host: %s\n",lista->Partidas[i].Host);
				int t = SearchPosition(&miLista,lista->Partidas[i].Host);
				lista->Partidas[i].sock[0]=miLista.conectados[t].socket;
				printf("Socket del jugador: %d\n",lista->Partidas[i].sock[0]);
				lista->Partidas[i].IDPartida=i;
				printf("ID: %d\n",lista->Partidas[i].IDPartida);
				IdPartida = lista->Partidas[i].IDPartida;
				lista->Partidas[i].numJugadores=1;
				printf("Numero de jugadores: %d\n",lista->Partidas[i].numJugadores);
				lista->Partidas[i].ocupado=1;
				printf("Ocupado: %d\n",lista->Partidas[i].ocupado);
				lista->num++;
				printf("Nuemro de lista: %d\n",lista->num);
				printf("Host añadido.......................................\n");
				r=1;
				e=1;
			}
			i=i+1;
		}
	}
	else{r=-1;}
	pthread_mutex_unlock(&mutex);	
	printf("Final de la busqueda.......................................\n");
	return r;
}
//Cuando un jugador quiere crear una partida, busca un espacio libre y se establece 
//como host. En esta partida se guaradar los distintos detos de esta.
void CrearPartida (char nombre [20], char respuesta [1024],ListaConectados *listaC, ListaPartidas *lista, int sock_conn) {
	//Como bien indica el nombre CrearPartida busca crear crear una partida, para  ellos le
	//intreduciremos un nombre y un socket. Primero llamaremos a la funcion EspacioLibre si
	//esta  nos devuelve un 0, un error tratando de crear la partida, copiaremos en respuesta
	//esto mismo. Si la funcion nos devuelve un -1 signifa que la lista de partidas esta llenas
	//copiaremos en respuesta esto mismo, finalmente si no devuelve nada de esto significa que la
	//partida ha sido creada con exito, copiando un mensaje que indica esto mismo.
	printf("Creando partida.......................................\n");
	printf("Abriendo espacio libre\n");
	printf("Socket host: %d\n",sock_conn);
	printf("Numero de partidas: %d\n",lista->num);
	int r = EspacioLibre(nombre,&miLista,&miListaPartidas,sock_conn);
	printf("Cerrando espacio libre\n");
	if (r==0){
		sprintf(respuesta,"%s, vaya ha habido un error durante la creacion de la partida", nombre);
		printf("Error al crear la partida\n");
	}
	else if (r==-1){
		sprintf(respuesta,"%s, todas nuestras partidas estan ocupadas le rogamos que lo intente mas adelante",nombre);
		printf("Error al crear la partida \n");
	}
	else{
		sprintf(respuesta, "%s, su partida ha sido creada con exito", nombre);
		printf("Partida creada.......................................\n");
	}
}
//Cerramos una partida y la limpiamos para que pueda ser usada en un futuro
int FinalPartida (char nombre [20],ListaPartidas *lista){
	//La funcion FinalPartida recibe un vector array nombre y realizamos una busqueda
	//en nuestra lista de partidas buscando una partida donde la variable host coincida
	//con el nombre introducido. Una vez encontrado las variable ocupado pasara a 0 y 
	//los nombre seran borrados para evitar que interfieran con los siguentes nombre, por
	//ultimo restaremos 1 al total de partidas. Finalmente devolvera un 1 indicando que la
	//operacion se ha realizado con exito. Si por el motivo que sea, por ejemplo no encuentra
	//la partida a borrar, devolvera un 0.

	int e=0;
	int i=0;
	printf("Cerrando partida.......................................\n");
	pthread_mutex_lock(&mutex);	
	while ((e==0) && (i < 100)){
		if ((strcmp(lista->Partidas[i].Host,nombre)==0) && (lista->Partidas[i].ocupado ==1)){
			lista->Partidas[i].ocupado=0;
			lista->Partidas[i].Host[0]='\0';
			lista->Partidas[i].nom2[0]='\0';
			lista->Partidas[i].nom3[0]='\0';
			lista->Partidas[i].nom4[0]='\0';
			lista->num--;
			e=1;
		}
		i++;
	}
	pthread_mutex_unlock(&mutex);	
	printf("Partida cerrada con exito.......................................\n");
	return e;
}
//Busca una partida en entre una lista de estas y devuelve su posicion.
int BuscarPartida(char nombre [20], ListaPartidas *lista){
	//Recibe un nombre, vector de caracteres, y busca una coincidencia entre los distinos nombre
	//almacenados en una partida. Al encontrar la partida devolveremos la posicion de la partida 
	//donde se encuentra la coincidnecia y si no lo encuentra devolveremos un -1. Para encontrar
	//la coincidencia realizaremos una busqueda.
	
	int e=0;
	int i=0;
	printf("Iniciando BuscarPartida.......................................\n");
	while ((e==0) && (i < lista->num)){
		printf("%s // %s\n",lista->Partidas[i].Host,nombre);
		if (((strcmp(miListaPartidas.Partidas[i].Host,nombre)==0))){ //|| (strcmp(lista->Partidas[i].nom2,nombre)==0) || (strcmp(lista->Partidas[i].nom3,nombre)==0) || (strcmp(lista->Partidas[i].nom4,nombre)==0)) && (lista->Partidas[i].ocupado ==1)){
			e=1;
		}
		if (e==0){i++;}
	}
	printf("Posicion: %d Encontrado: %d\n",i,e);
	if (e==1){return i;} else{return -1;}
	printf("Cerrando BuscarPartida.......................................\n");
}
//Busca la partida usando el nombre del host. Devuelve -1 si hay algun error,
//0 si la partida esta llena, 1 si se ha podido unir y -2 si no se ha encontrado
//la partida.
int UnirsePartida(char Host[20],char nombre[20],ListaConectados *listaC, ListaPartidas *listaP){
	//Recibimos dos vectores de arrays una con el nombre de host y el otro con el nombre del
	//jugador que quiere unirse. Primero llamaremos a la funcion BuscarPartida y buscamos el
	//nombre del host. Si no encontramos la partrida devolvera un -2, por otro lado si la
	//encontramos miraremos quantos jugadores hay si no hay ningun jugador devolvera un 0, si
	//ya hay 4 juagadores devolvera un 0. Sino mirara cual es el numero de jugadores, en funcion
	//del numero que le toque que asignara al jugador 2, jugador 3 y jugador 4. Al unirse a la 
	//partida devolvera un 1.
	printf("Iniciando UnirsePartida.......................................\n");
	printf("Host: %s\n",Host);
	int posicion = BuscarPartida(Host,&miListaPartidas);
	if (posicion==-1){return -2;}
	else {
		if (listaP->Partidas[posicion].numJugadores==0){return -1;}
		else if (listaP->Partidas[posicion].numJugadores==4){return 0;}
		else if (listaP->Partidas[posicion].numJugadores==1){
			strcpy(listaP->Partidas[posicion].nom2,nombre);
			int socket = ReturnSocket(listaC, nombre);
			listaP->Partidas[posicion].sock[listaP->Partidas[posicion].numJugadores]=socket;
			listaP->Partidas[posicion].numJugadores=2;
			return 1;
		}
		else if (listaP->Partidas[posicion].numJugadores==2){
			strcpy(listaP->Partidas[posicion].nom3,nombre);
			int socket = ReturnSocket(listaC, nombre);
			listaP->Partidas[posicion].sock[listaP->Partidas[posicion].numJugadores]=socket;
			listaP->Partidas[posicion].numJugadores=3;
			return 1;
		}
		else if (listaP->Partidas[posicion].numJugadores==3){
			strcpy(listaP->Partidas[posicion].nom4,nombre);
			int socket = ReturnSocket(listaC, nombre);
			listaP->Partidas[posicion].sock[listaP->Partidas[posicion].numJugadores]=socket;
			listaP->Partidas[posicion].numJugadores=3;
			return 1;
		}
		else{return -1;}
	}
	printf("Cerrando UnirsePartida.......................................\n");
}
int BuscarTuPartida(int socket, ListaPartidas *lista){
	//Recibe un socket y busca este socek entre los sockets guardados en cada uno de 
	//los sockets.
	int e=0; int i=0;
	pthread_mutex_lock(&mutex);
	while ((i<lista->num) && (e==0)){
		for (int j=0;j<lista->Partidas[i].numJugadores;j++){if (lista->Partidas[i].sock[j]==socket) {e=1;}}
		if (e==0) {i++;}
	}
	pthread_mutex_unlock(&mutex);
	if (e==1){return i;} else {return -1;}
}
//Definicion
void IniciarPartida (char *p, char respuesta [1024], ListaPartidas *listaP){
	//Esta funcion prepara el mensaje que creara la partida. Primero buscaremos si la
	//partida existe, si la partida no es encontrada enviara un mensaje informando del
	//error. Por otro lado la partida es encontrada preparara los mensajes con los datos
	//de inicio de partida. La estructura del mensaje sera una lista de los nombre y 
	//sockets ordenado por orden de union, host, jugador 2, .... Antes de esta lista no 
	//obstante estara el nombre del jugador, por ende hay mensaje personalizado por 
	//jugador. Por ejemplo: 13/Maria/Jose/1/Maria/2/
	p=strtok(NULL,"/");
	char nombre[20];
	strcpy(nombre,p);
	printf("Nombre: %s\n",nombre);
	int r = BuscarPartida(nombre,&miListaPartidas);
	if (r ==-1){sprintf(respuesta,"20/No se ha podido crear la partida");}
	else{
		//Establecemos las variables
		char res1 [1024];
		char res2 [1024];
		char res3 [1024];
		char res4 [1024];
		
		char h [20];
		strcpy(h,listaP->Partidas[r].Host);
		char j2 [20];
		strcpy(j2,listaP->Partidas[r].nom2);
		char j3 [20];
		strcpy(j3,listaP->Partidas[r].nom3);
		char j4 [20];
		strcpy(j4,listaP->Partidas[r].nom4);
		printf("A\n");
		int sh = listaP->Partidas[r].sock[0];
		int sj2 = listaP->Partidas[r].sock[1];
		int sj3 = listaP->Partidas[r].sock[2];
		int sj4 = listaP->Partidas[r].sock[3];
		printf("B\n");
		int conectado [4];
		for (int i =0;i<4;i++){conectado[i]=0;}
		for (int i =0;i<listaP->Partidas[r].numJugadores;i++){conectado[i]=1;}
		printf("C\n");
		if (conectado[0]==1){
			sprintf(res1,"13/%s;%d;%s;%d;%d;%s;%d;%d;%s;%d;%d;%s;%d",h,conectado[0],h,sh,conectado[1],j2,sj2,conectado[2],j3,sj3,conectado[3],j4,sj4);	
			printf("%s\n",res1);
			write (listaP->Partidas[r].sock[0],res1, strlen(res1));
		}
		if (conectado[1]==1){
			sprintf(res2,"13/%s;%d;%s;%d;%d;%s;%d;%d;%s;%d;%d;%s;%d",j2,conectado[0],h,sh,conectado[1],j2,sj2,conectado[2],j3,sj3,conectado[3],j4,sj4);
			printf("%s\n",res2);
			write (listaP->Partidas[r].sock[1],res2, strlen(res2));
		}
		if (conectado[2]==1){
			sprintf(res3,"13/%s;%d;%s;%d;%d;%s;%d;%d;%s;%d;%d;%s;%d",j3,conectado[0],h,sh,conectado[1],j2,sj2,conectado[2],j3,sj3,conectado[3],j4,sj4);
			printf("%s\n",res3);
			write (listaP->Partidas[r].sock[2],res3, strlen(res3));
		}
		if (conectado[3]==1){
			sprintf(res3,"13/%s;%d;%s;%d;%d;%s;%d;%d;%s;%d;%d;%s;%d",j4,conectado[0],h,sh,conectado[1],j2,sj2,conectado[2],j3,sj3,conectado[3],j4,sj4);
			printf("%s\n",res4);
			write (listaP->Partidas[r].sock[3],res3, strlen(res3));
		}
		//NotificacionSelectiva(res1,res2,res3,res4,r,conectado,miListaPartidas);
		printf("D\n");
	}
}
//______________________________________________________________________________
//GESTION DE INVITACIONES:
//Generamos la invitacion
void Invitacion(char *p,ListaConectados *listaC,ListaPartidas *listaP, char respuesta [1024], int sock_conn){
	//Recibimos vector de arrays, extraemos el nombre de quien envia la invitacion y su destinatario. 
	//Con estos datos y la funcion ReturnSocket obtenemos el socket de destino para enviar la 
	//invitacion del destinatario y copiamos el mensaje en el vector de caracteres respuesta. Si la
	//partida no existe para el momento en que estamos lanzando la invitacion lo que haremos sera 
	//crearla usando la funcion CrearPartida.

	//Variables locales:
	char nomO [20];
	char nomD [20];
	int IdP;
	
	printf("Preperando invitacion.......................................\n");
	p = strtok( NULL, "/");
	sprintf(nomO,"%s",p);
	printf("Origen: %s\n",nomO);
	p = strtok (NULL,"/");
	sprintf(nomD,"%s",p);
	printf("Destino: %s\n",nomD);
	int D = SearchPosition(&miLista,p);
	int O = SearchPosition(&miLista,nomO);
	printf("D:%d O:%d \n",D,O);
	int socketD = ReturnSocket(listaC, nomD);
	int socketO =  ReturnSocket(listaC, nomO);
	printf("Socket Destino:%d Socket Origen:%d\n",socketD, socketO);
	
	//Creamos la partida
	int r = BuscarPartida(nomO,listaP);
	printf("%d\n",r);
	if (r==-1){
		CrearPartida(nomO,respuesta,listaC,listaP,socketO);
		printf("Partida lista.......................................\n");
	}
	//Devolveremos el codigo, la respuesta y el nombre del destino de la invitacion
	sprintf(respuesta,"9/Quieres jugar contra %s?/%s",nomO,nomD);
	printf("Respuesta: %s\n", respuesta);
	printf("Socket destino: %d\n",socketD);
	sock_conn = socketD;
	printf("Invitacion enviada.......................................\n");
}
//El destinatario dice si acepta o no unirse a la partida
void ConfirmarInvitacion(char *p, ListaConectados *listaC,ListaPartidas *listaP, char respuesta[1024], int sock_conn) {
	//Este mensaje recibe la respuesta de la invitacion que genera la funcion Invitacion
	//Si recibimos la un SI llamaremos a la funcion UnirsePartida, dependiendo del resultado
	//copiaremos un mensaje u otro, el unico que varia de los comentados anteriormente es el 
	//del caso donde creamos la partida sin problemas. Donde copiaremos un mensaje parecidoa al
	//siguiente: 11/Maria se ha hundio a la partida.
	
	//Variables locales
	char confirmacion[10];
	char nomO [20];
	char nomD[20];
	char respuesta1 [1024];
	
	printf("Preperando Respuesta.......................................\n");
	//Extraccion de datos.
	p = strtok( NULL, "/");
	strcpy(confirmacion,p);
	printf("Respuesta: %s\n",confirmacion);
	p = strtok (NULL,"/");
	strcpy(nomD,p);
	printf("Destino: %s\n",nomD);
	p = strtok (NULL,"/");
	strcpy(nomO,p);
	printf("Origen: %s\n",nomO);
	int socketD = ReturnSocket(listaC, nomD);
	int socketO =  ReturnSocket(listaC, nomO);
	printf("Socket Destino:%d Socket Origen:%d\n",socketD, socketO);
	
	//Generacion de la respuesta.
	if (strcmp(confirmacion,"SI")==0){
		printf("A\n");
		int r = UnirsePartida(nomD,nomO, listaC,listaP);
		printf("%d\n",r);
		printf("B\n");
		if (r==-2){
			sprintf(respuesta1,"10/La partida dirigida por %s ya no existe.",nomD);
			sprintf(respuesta,"11/%s no ha podido unirse a vuestra partida",nomO);
		}
		if (r==-1){
			sprintf(respuesta1,"10/Error al unirse a la partida de %s.",nomD);
			sprintf(respuesta,"11/%s no ha podido unirse a vuestra partida",nomO);
		}
		if (r==0){
			sprintf(respuesta1,"10/La partida de %s esta llena.",nomD);
			sprintf(respuesta,"11/%s no ha podido unirse a vuestra partida",nomO);
		}
		if (r==1){
			sprintf(respuesta1,"10/Se ha unido con a la partida de %s",nomD);
			sprintf(respuesta,"11/%s se unido a vuestra partida",nomO);
		}
		printf("C\n");
		printf("Respuesta: %s\n",respuesta);
		printf("Respuesta1: %s\n",respuesta1);
		write (socketO,respuesta1, strlen(respuesta));
		printf("D\n");
	}
	else {sprintf(respuesta,"11/%s no ha podido unirse a vuestra partida",nomO);}
	printf("E\n");
	sock_conn = socketD;
	printf("Respuesta lista.......................................\n");
}
//______________________________________________________________________________
//NOTIFICACIONES:
//Crea la notificacion para la lista de conectados
void NotificacionGeneral(char notificacion [1024]){
	//Esta funcion recibe un vector de arrays que contine un mensaje, en este caso este
	//el mensaje sera recibido por todos los usuarios conectados al servidor. La notificacion
	//que compartiremos sera el vector de arrays que genera la funcion DameConectados y lo
	//enviaremos a todos los sockets que hay guardados.
	
	//Esta funcion utiliza otra funcion, DameConectados, de donde obtenie la lista 
	//de jugadores conectados. Luego usando un for lo enviara a todos los 
	//dispositivos conectados.
	printf("Inicio de NotificacionGeneral.......................................\n");
	for (int j=0; j<miLista.num; j++){write(miLista.conectados[j].socket,notificacion, strlen(notificacion));}
	printf("Final de NotificacionGeneral.......................................\n");
}
//Notificacion para los miembros de una partida unicamente
void NotificacionPartida(char notificacion [1024],char nombre [20], ListaPartidas *lista){
	//Esta funcion rescibe un mensaje, a modo de vector de caracteres, junto con un nombre. Usando
	//la funcion buscar partida busca en que partida se encuentra y envia el mensaje a todos los 
	//miembros de la partida usando los sockets que hay guardados en esta.
	int j;
	printf("Inicio de NotificacionPartida.......................................\n");
	int posicion = BuscarPartida(nombre,&miListaPartidas);
	for (j=0; j<lista->Partidas[posicion].numJugadores; j++){write (lista->Partidas[posicion].sock[j],notificacion, strlen(notificacion));}
	printf("Final de NotificacionPartida.......................................\n");
}
void NotificacionSelectiva(char n1 [1024],char n2 [1024],char n3 [1024],char n4 [1024],int posicion,int conectado [4], ListaPartidas *lista){
	//Esta funcio permite enviar mensajes concretos, en lugar de generales, casos 
	//presentados con anterioridad. Con esto buscamos poder enviar mensajes concretos.
	printf("Inicio de NotificacionSelectiva.......................................\n");
	if (conectado[0]==1){write (lista->Partidas[posicion].sock[0],n1, strlen(n1));}
	if (conectado[1]==1){write (lista->Partidas[posicion].sock[1],n2, strlen(n2));}
	if (conectado[2]==1){write (lista->Partidas[posicion].sock[2],n3, strlen(n3));}
	if (conectado[3]==1){write (lista->Partidas[posicion].sock[3],n4, strlen(n4));}
	printf("Final de NotificacionSelectiva.......................................\n");
}
//Definicion
void NotificacionPosicion(char *p, ListaPartidas *lista, int socket){
	//Esta funcion se encarga de comparitir el mensjae con las nuevas posiciones a
	//todos los jugadores de la partida.
	printf("Iniciando NotificacionPosicion....\n");
	printf("Mensaje: %s\n",p);
	int r = BuscarTuPartida(socket,&miListaPartidas);
	printf("A\n");
	char notificacion [1024];
	//int turno = CambioTurno(listaP->Partidas[i].numJugadores);
	sprintf(notificacion,"%s",p);
	for (i=0;i<lista->Partidas[r].numJugadores;i++){
		printf("Socket: %d\n",lista->Partidas[r].sock[i]);
		write (lista->Partidas[r].sock[i],notificacion, strlen(notificacion));
	}
}
void DardeBaja(char p [200], char respuesta [200], MYSQL* conn,int socket){	
	//En esta función se tiene que escrbir el usario y la contraseña de
	// un usuario creado y así podras darlo de baja (eliminarlo)
	// de la base de datos
	char username [30];
	p = strtok(NULL, "/");
	strcpy(username, p);
	char password [30];
	p = strtok(NULL, "/");
	strcpy(password, p);
	int err;
	MYSQL_RES* resultado;
	MYSQL_ROW row;
	char consulta[1024];
	sprintf(consulta, "SELECT * FROM Jugador WHERE Jugador.usuario='%s' AND Jugador.contrasena='%s';",username,password);
	err = mysql_query(conn, consulta);
	if (err != 0)
	{
		printf("Consulta mal hecha %u %s\n", mysql_errno(conn), mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if (row != NULL)
	{
		sprintf(consulta,"DELETE FROM Jugador WHERE Jugador.usuario='%s' AND Jugador.contrasena='%s';",username,password);
		
		printf("consulta = %s\n", consulta);
		
		err = mysql_query(conn, consulta);
		//parte de mysql
			if (err != 0)
		{
			printf("Error al crear la conexion: %u %s\n", mysql_errno(conn), mysql_error(conn));
			strcpy(respuesta,"22/F");
			write(socket,respuesta,strlen(respuesta));
			exit(1);
		}
			
			strcpy(respuesta,"22/Si");
			write(socket,respuesta,strlen(respuesta));
	}
	else{
		strcpy(respuesta,"22/No");
		write(socket,respuesta,strlen(respuesta));
	}
}
//OPERACIONES DENTRO DE UNA PARTIDA:
//Procesa el mensje y lo envia a todos los miembros de la Partida
void CargarMensaje(char *p,char notificacion [1024]){
	//Esta funcion se encarga de enviar mensajes introducidos por los usuarios de un
	//jugador al resto, usadno la funcion de NotificacionPartida.
	printf("Cargando mensaje.......................................\n");
	char mens [1024];
	char nombre [20];
	
	p = strtok( NULL, "/");
	strcpy(nombre,p);
	printf("Nombre: %s\n",nombre);
	p = strtok( NULL, "/");
	strcpy(mens, p);
	printf("Mensaje: %s\n",mens);
	sprintf(notificacion,"12/%s",mens);
	NotificacionPartida(notificacion,nombre,&miListaPartidas);
	printf("Mensaje enviado.......................................\n");
}
//______________________________________________________________________________
//------------------------------------------------------------------------------
//Funcion atender cliente:
void *AtenderCliente (void *socket){
	//Variables locales del atender cliente
	char peticion[1024];
	char respuesta[1024];
	char conectado[200];
	char notificacionG[1024];
	char nG [1024];
	char notificacionP[1024];
	char nP [1024];
	
	int contadorSocket=0;
	int ret;
	
	int socketO;//Origen
	int socketD;//Destino
	char nomO[20];
	char nomD[20];
	
	printf("Inicio.......................................\n");
	
	//Socket para la ListaConectados
	int sock_conn;
	int *s;
	s=(int *) socket;
	sock_conn = *s;
	
	printf("Socket: %d\n",sock_conn);
	
	//Establecemos la conexion con nuestro servidor
	conn = mysql_init(NULL);
	if (conn==NULL){
		printf ("Error al crear la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	conn = mysql_real_connect (conn, "localhost","root", "mysql","T6_Juego1311",0, NULL, 0);
	//conn = mysql_real_connect (conn, "shiva2.upc.es","root", "mysql","T6_Juego1311",0, NULL, 0);
	
	printf("Conexion iniciada\n");
	if (conn==NULL){
		printf ("Error al inicializar la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	int terminar =0;
	//Abrimos un bucle que atendera a todas las peticiones que entren de un 
	//determinado cliente hasta que este se desconecte
	
	while(terminar==0){
		printf("******************************************************************\n");
		//Proceso de limpieza:
		Limpieza(peticion, respuesta,conectado,notificacionG,notificacionP,nG,nP);
		
		//Variables que usaremos en nuestro while
		int codigo;
		char nombre[20];
		char contrasena[20];
		int partidas_ganadas;
		char consulta [80];
		
		// Ahora recibimos su peticion
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido\n");
		
		// Tenemos que aÃ±adirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		
		//Escribimos el nombre en la consola para comprobar el mensaje recibido
		printf ("Se ha conectado: %s\n",peticion);
		char aux_peticion [512];
		strcpy(aux_peticion,peticion);
		
		//Recogemos el codigo obtenido
		char *p = strtok( peticion, "/"); 
		// Troceamos el mensaje adquirido dividiendo entre funcion y datos
		codigo =  atoi (p); 
		printf("Peticion: %s\n",peticion);
		int numForm;
		
		//Comprobacion del codigo obtenido
		printf ("Codigo: %d\n",codigo);
		
		//Deconectamos a un usuario
		if (codigo==0){ Cierre(respuesta,terminar);}
		else {
			//Conectamos a un usuario
			if (codigo == 1){ConectarUsuario(p,respuesta,sock_conn,&miLista);}
			printf("1\n");
			//Registramos a un usuario
			if(codigo == 2){RegistrarUsuario(p,respuesta,sock_conn,&miLista);}
			printf("2\n");
			//Miramos si un usuario en concreto esta conectado
			if(codigo == 3){EstaConectado(p, respuesta, &miLista);}
			printf("3\n");
			//Devuelve las partidas ganadas
			if (codigo == 4){PartidasGanadas(p, respuesta);}
			printf("4\n");
			//Devuelve las partidas jugadas
			if (codigo == 5){ PartidasJugadas(p,respuesta);}
			printf("5\n");
			//Devuelve la lista de conectados manualmente
			if (codigo == 6){DameConectados(&miLista,respuesta);}
			printf("6\n");
			//Devuelve el avatar elejido
			if (codigo == 7){int r = Avatars(p, respuesta);}
			printf("7\n");
			//Devuelve el numero de consultas hechas
			if (codigo == 8){sprintf (respuesta,"8/%d",contador);}
			printf("8\n");
			//Invitacion
			if (codigo == 9){Invitacion(p, &miLista,&miListaPartidas,respuesta,sock_conn);}
			printf("9\n");
			//Recibir respuesta de invitacion
			if (codigo == 10){ConfirmarInvitacion(p,&miLista,&miListaPartidas,respuesta,sock_conn);}
			printf("10\n");
			if (codigo==11)	{
				idP=miListaPartidas.num;
				miListaPartidas.num++;
				sprintf(respuesta,"11/Selecciona contra quien quieres jugar :)");
				printf("Respuesta: %s\n", respuesta);
				// Enviamos la respuesta
				//write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("11\n");
			if (codigo==12){NotificacionGeneral(aux_peticion);}
			printf("12\n");
			if (codigo ==13){
				pthread_mutex_lock(&mutex);	
				IniciarPartida(p, respuesta, &miListaPartidas);
				pthread_mutex_unlock(&mutex);	
			}
			printf("13\n");
			if (codigo==14){NotificacionPosicion(aux_peticion,&miListaPartidas,sock_conn);}
			printf("14\n");
			if (codigo==15){NotificacionPosicion(aux_peticion,&miListaPartidas,sock_conn);}
			printf("15\n");
			if(codigo==18){NotificacionPosicion(aux_peticion,&miListaPartidas,sock_conn);}
			printf("18\n");
			if(codigo==19){NotificacionPosicion(aux_peticion,&miListaPartidas,sock_conn);}
			printf("19\n");
			if (codigo == 22){DardeBaja(p,respuesta,conn,sock_conn);}
			printf("22\n");
			if (codigo==21){Disconect(p,&miLista,respuesta);}
			//Enviamos la respuesta de nuestras consultas
			if (codigo != 0){
				printf("Respuesta: %s\n", respuesta);
				printf("Socket receptor: %d\n",sock_conn);
				// Enviamos la respuesta
				write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("Enviar consulta\n");
			//Contamos en numero de consultas
			if ((codigo==1)||(codigo==2)|| (codigo==3)||(codigo==4)|| (codigo==5)|| (codigo==6)|| (codigo==7) || (codigo==8)|| (codigo==9) ||(codigo==11) || (codigo==12) || (codigo==19) || (codigo == 22) || (codigo==21)){
				pthread_mutex_lock(&mutex);//no interrumpas
				contador = contador +1;
				pthread_mutex_unlock(&mutex);//puedes interrumpirme
				printf("Peticion contada\n");
				
				//Notificamos a todos los clientes conectados
				DameConectados(&miLista,nG);
				printf("%s\n",notificacionG);
				printf("%s\n",nG);
				sprintf (notificacionG,"%s",nG);
				//Enviamos a todos los clientes conectados
				int j;
				for (j=0; j<miLista.num; j++){
					printf("Lista sockets %d\n",sockets[j]);
					printf("Lista conectados %d\n",miLista.conectados[j].socket);
					write (miLista.conectados[j].socket,notificacionG, strlen(notificacionG));
				}
			}
		}
	printf("Numero conectados: %d\n",miLista.num);
	printf("Nuemro de partidas: %d\n",miListaPartidas.num);
	printf("Usuario: %s\n",miLista.conectados[miLista.num].nombre);
	printf("Final.......................................\n");
	printf("******************************************************************\n");
	// Se acabo el servicio para este cliente
	}
	close(sock_conn); 
}

int main(int argc, char *argv[]){
	//Variables
	int sock_conn, sock_listen, ret;
	struct sockaddr_in serv_adr;
	
	// INICIALITZACIONS
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		//socket en el que el servidor espera un pedido
		printf("Error creant socket\n");
		// Hacemos el bind al puerto
	}
	
	memset(&serv_adr, 0, sizeof(serv_adr));
	// inicialitza a zero serv_addr
	
	serv_adr.sin_family = AF_INET;
	//asocio el socket con la ip de la maquina que lo ha generado
	
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY configura siempre 
	//la ip asignada
	
	// escucharemos en el port 9050 este port puede variar en funcion del pc que
	//haga de srvidor
	serv_adr.sin_port = htons(9090); // indicamos el puerto
	
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0) {printf ("Error al bin \n");}
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0) {
		// indicamos que es sock pasivo, el dos marca el numero de peticiones 
		//maximas en cola
		printf("Error en el Listen\n");
	}
	pthread_t thread;
	
	ListaConectados miLista;
	ListaPartidas miListaPartidas;
	Inicializar(&miLista,&miListaPartidas);
	printf("Conectados:%d Partidas:%d\n",miLista.num,miListaPartidas.num);
	
	for(;;){
		printf ("Escuchando\n"); //No ayuda a saber si hemos empezado a escuchar
		sock_conn = accept(sock_listen, NULL, NULL);
		//este sock se comunica con el programa con el que hemos conectado

		printf ("He recibido conexion\n"); //comprovamos si todo en orden
		//sock_conn es el socket que usaremos para este cliente
		//Bucle de atencion al cliente
		
		//Llenamos el vector de sockets
		sockets[i]=sock_conn;
		printf("%d//%d",sockets[i],sock_conn);
		pthread_create (&thread,NULL,AtenderCliente,&sockets[i]);
		i++;
	}
}
