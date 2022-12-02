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
MYSQL *conn;
int err;
MYSQL_RES *resultado;
MYSQL_ROW row;


//Variables para la lista sockets
int i=0;
int sockets[100];
int idP;//ID de partida

//	C:
//		Variable para contar posiciones en nuestra lista de conectados:
int s = 0;


//------------------------------------------------------------------------------
//Estructuras:
//	Conectdo
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
//Declaramos la Partida
typedef struct {
	int IDPartida;
	int numJugadores;
	char tablero[30];
	
	//Quien crea la partida
	char Host[20];
	
	//Quienes se unen a la partida del host
	char nom2[20];
	char nom3[20];
	char nom4[20];
	
	//Sockets
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
// 	Inicializamos nuestra lista
void Inicializar(ListaConectados *listaC,ListaPartidas *listaP){
	listaC->num=0;
	listaP->num=0;
}
//Limpia todos los char para evitar el ruido
void Limpieza(char peticion[1024],char respuesta [1024],char conectado [200], char notificionG [1024], char notificionP [1024],char nG [1024], char nP [1024]){
	printf("Iniciando proceso de limpieza.... \n");
	peticion[0]='\0';
	printf("Peticion limpia.... %s \n",peticion);
	respuesta[0]='\0';
	printf("Respuesta limpia.... %s \n",respuesta);
	conectado[0]='\0';
	printf("Conectado limpio.... %s \n",conectado);
	notificionG[0]='\0';
	printf("Notificacio General limpia.... %s \n",notificionG);
	notificionP[0]='\0';
	printf("Notificacio Partida limpia.... %s \n",notificionP);
	nG[0]='\0';
	printf("Auxiliar General limpio.... %s \n", nG);
	nP[0]='\0';
	printf("Auxiliar Partida limpio.... %s \n", nP);
	printf("Proceso de limpieza finalizado \n");
	printf("\n");
	
}
//Para pruebas y comprobaciones de codigo
int PruebaConfirmacion(ListaConectados *lista, char respuesta [1024], char confirmacion [10], char nomO [20], char nomD [20], int *sock_conn){
	int socketD;
	int socketO;
	socketD = ReturnSocket(lista, nomD);
	socketO =  ReturnSocket(lista, nomO);
	printf("%d %d \n",socketD, socketO);
	if (strcmp(confirmacion, "SI")==0){
		//A�adirJugador
		sprintf(respuesta,"10/ %s ha aceptado el desafio",nomD);
	}
	else{
		sprintf(respuesta,"10/ Vaya, %s esta ocupado",nomD);
	}
	*sock_conn = socketO;
	
}
//Cerrar el AtenderCliente
void Cierre(char respuesta [1024], int terminar){
	printf("Cierre \n");
	sprintf(respuesta,"Adios");
	sprintf(respuesta,"%d/%s",0,respuesta);
	terminar=1;
}
//______________________________________________________________________________
//FUNCIONES DE BUSQUEDA:
//	Buscamos la posicion de un usuario en nuestra lista
int SearchPosition (ListaConectados *lista, char nombre[20]){
	//Devuelve la posicion o -1 si no lo encuentra.
	
	printf("Inicio SearchPosition\n");
	int i =0;
	int encontrado=0;
	while ((i<100) && (!encontrado)){
		if (strcmp(lista->conectados[i].nombre,nombre)==0)
			encontrado=1;
		else{
			i=i+1;
		}
	}
	if (encontrado == 1)
		return i;
	else{
		return -1;
	}
	printf("Final SearchPosition\n");
}
//Funcion que devuelve la posicion o -1 si no esta en la lista de conectados.
int GiveMeSocket (ListaConectados *lista, char nombre[20]){
	int i = 0;
	int encontrado = 0;
	while((i < 100) && !encontrado)
	{
		if (strcmp(lista->conectados[i].nombre,nombre) == 0)
		{
			encontrado = 1;
		}
		if(!encontrado)
		{
			i = i+1;
		}
	}
	if (encontrado)
	{
		return lista->conectados[i].socket;
	}
	else
	{
		return -1;
	}
}
//Devuelve el Socket en el caso de lo que lo encuentre
int ReturnSocket (ListaConectados *lista, char nombre[20]){
	int i = 0;
	int encontrado = 0;
	printf("Numero: %d\n",lista->num);
	while((i < 100) && (encontrado==0))
	{
		if (strcmp(lista->conectados[i].nombre,nombre) == 0)
		{
			printf("%d\n",lista->conectados[i].socket);
			encontrado = 1;
		}
		if(encontrado==0)
		{
			i = i+1;
		}
	}
	if (encontrado==1)
	{
		return lista->conectados[i].socket;
	}
	else
	{
		return -1;
	}
}
int GiveMeSocketsJP(ListaPartidas *listaP, ListaConectados *lista, int id, int socketsJug[4]) {
	int i=0;
	while(i< listaP->Partidas[id].numJugadores)
	{
		socketsJug[i]= listaP->Partidas[id].sock[i];
		i++;
	}
	return listaP->Partidas[id].numJugadores;
}
//______________________________________________________________________________
//GESTION DE USUARIOS:
//	Comprueba si un usuario existe y lo a�ade a la lista conectados
int ConectarUsuario (char *p,char respuesta [1024], int socket, ListaConectados *lista){
	//Despues de comprobar si elusuario se encuentra en la base de datos lo a�ado a 
	//la ListaConectados y genera el mensaje pertinente. Devuelve 1 si ha 
	//realizado la funcion con exito y 0 alternativamente.
	
	printf("Inicio ConectarUsuario....\n");
	//Variables locales
	char nombre [20];
	char contrasena [20];
	char consulta [1024];
	char resp[1024];
	
	// Conectarse con un usuario registrado
	p = strtok( NULL, "/");
	
	//Obtenemos el nombre y la contraseña
	strcpy (nombre, p);
	p=strtok(NULL,"/");
	strcpy(contrasena,p);
	printf ("Nombre: %s Contrase�a: %s \n", nombre, contrasena);
	
	//Establecemos la busqueda
	sprintf(consulta,"SELECT id FROM Jugador Where usuario = '%s' AND contrasena ='%s';",nombre,contrasena);
	//escribimos en el buffer respuestra la longitud del nombre
	
	err=mysql_query (conn, consulta);
	if (err!=0)
	{
		printf ("Error al consultar datos de la base %u %s \n",
				mysql_errno(conn), mysql_error(conn));
		sprintf(resp,"Vaya un error se ha producido");
	}
	
	else{
		resultado = mysql_store_result(conn);
		row = mysql_fetch_row(resultado);
		printf("%s\n",row[0]);
		sprintf(resp,"Bien venido %s",nombre);
		int r = Add(lista,nombre, socket);
	}
	sprintf(respuesta,"1/%s",resp);
	printf("%s\n",respuesta);
	printf("Final ConectarUsuario....\n");
}
//	A�ade un usuario y lo guarda en la lista conectados
int RegistrarUsuario(char *p,char respuesta [1024], int socket){
	//A�adimos un nuevo usuario a nuestra base de datos y a nuestra Listadeconectados
	//devolvemos 1 si hemos realizado la accion correctamente y 0 alternativamente.
	
	printf("Inicio RegistrarUsuario....\n");
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
	printf ("Nombre: %s Contrase�a: %s \n", nombre, contrasena);
	
	//A�adimos la funcion que queremos que haga
	sprintf(consulta, "INSERT INTO Jugador(usuario, contrasena, email, partidas_ganadas, partidas_jugadas, conectado) VALUES ('%s', '%s', 'correo@gmail.com', 2, 3, 1);", nombre,contrasena);
	//escribimos en el buffer respuestra la longitud del nombre
	
	err=mysql_query (conn, consulta);
	if (err!=0)
	{
		printf ("Error al consultar datos de la base %u %s \n",
				mysql_errno(conn), mysql_error(conn));
		sprintf(respuesta,"Vaya un error se ha producido un error \n");
	}
	else{
		sprintf(resp,"Bien venido al club %s",nombre);
		int r = Add(&miLista,nombre, socket);
	}
	sprintf(respuesta,"%d/%s",2,resp);
	printf("Final RegistrarUsuario....\n");
}
//______________________________________________________________________________
//GESTION CONECTADOS:
//	Creamos un string con todos los conectados 
void DameConectados (ListaConectados *lista, char respuesta [1024]){
	//Devuelve llena la lista conectados. Esta se llenara con el nombre de 
	//todos los conectados separados por /. Ademas primero nos dara el numero 
	//de conectados.
	
	char resp[1024];
	resp[0]='\0';
	printf("Resp: %s \n",resp);
	
	printf("Inicio DameConectados....\n");
	//sprintf(respuesta,"%d",s);
	printf("Respuesta: %s\n",respuesta);
	
	int i;
	for (i=0; i<lista->num; i++){
		sprintf(resp, "%s%s/ \n",resp, lista->conectados[i].nombre);
		printf("%s\n",resp);
	}
	sprintf(respuesta,"%d/%s",6,resp);
	printf("Final DameConectados....\n");
}
//	A�adimos un usuario a nuestra lista
int Add (ListaConectados *lista, char nombre[20],int socket){
//A�ade un nuevo conectado. Si la lista esta llena retorna -1 y si ha 
//a�adido con exito al nuevo jugador retorna 0.
	printf("Inicio Add....\n");
	printf("Numero: %d",lista->num);
	int a = lista->num;
	
	if (lista->num==100)
		return -1;
	else{
		printf("A�adiendo \n");
		strcpy(lista->conectados[lista->num].nombre,nombre);
		printf("Nombre: %s\n",lista->conectados[lista->num].nombre);
		lista->conectados[lista->num].socket = socket;
		printf("Socket: %d\n",socket);
		lista->num=a+1;
		printf("Numero: %d",lista->num);
		s = s +1;
		printf("%d\n",s);
		printf("Nombre:%s Socket: %d\n",lista->conectados[a].nombre,lista->conectados[a].socket);
		printf("Usuario a�adido");
		return 0;
	}
	printf("Final Add.... \n");
}
//	Eliminamos al usuario de la ListaConectados, al desconectarse
int Disconect (ListaConectados *lista, char nombre[20]){
	//retorna 0 si el usuario se ha desconectado con exito y -1 si ha ocurrido 
	//un error.
	printf("Inicio Disconect....\n");
	int pos = SearchPosition(lista, nombre);
	int i;
	//la no se recibe por referncia pues ya la recibimos por referencia el en disconnect
	if (pos==-1){
		return -1;
	}
	else{
		for (i=pos; i<lista->num-1;i++){
			lista ->conectados[i] = lista->conectados[i+1];
			strcpy(lista->conectados[i].nombre, lista->conectados[i+1].nombre);
			lista->conectados[i].socket==lista->conectados[i+1].socket;
		}
		lista->num--;
		return 0;
	}
	printf("Final Disconect....\n");
}
int Delete (ListaConectados *lista, char nombre[20]){
	//retorna 0 si elimina a la persona y -1 si ese usuario no esta conectado.
	int pos = GiveMeSocket (lista,nombre);
	if (pos == -1)
	{
		return -1;
	}
	else
	{
		int i;
		for (i = pos; i < lista->num-1; i++)
		{
			strcpy(lista->conectados[i].nombre, lista->conectados[i+1].nombre);
			lista->conectados[i].socket = lista->conectados[i+1].socket;
		}
		lista->num--;
		return 0;
	}
}
//______________________________________________________________________________
//CONSULTAS PANTALLA DE INICIO:
//	Nos devuelve en numero de victorias de un jugador
int PartidasGanadas(char *p,char respuesta [1024]){
	
	printf("Inicio PartidasGanadas....\n");
	//Variables locales
	char nombre [20];
	char consulta [1024];
	char resp[1024];
	
	////Obtenemos el nombre
	p = strtok( NULL, "/");
	strcpy(nombre,p);
	printf("Nombre: %s",nombre);
	
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
	printf("%s",row[0]);
	if (row == NULL)
	{
		printf("No se ha obtenido la consulta \n"); 
	}
	else
	{
		int ans = atoi(row[0]);
		sprintf(resp,"%s ha ganado %d partidas \n",nombre,ans);
	}
	sprintf(respuesta,"%d/%s",4,respuesta);
	printf("Final PartidasGanadas....\n");
}
//	Pregunta si un usuario esta conectado
int EstaConectado(char *p,char respuesta [1024], ListaConectados *lista){
//Obtenemos el nombre de un usuario y buscamos si esta conectado. Esta funcion 
//podria obtenerse de una consulta sql o bien de nuestra lista conectados. En 
//este nos aprovecharemos de nuestra lista conectados.
	
	printf("Inicio EstaConectado....\n");
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
	int r = SearchPosition(&lista, nombre);
	
	if (r ==-1){
		printf ("No encontrado");
		sprintf(respuesta, "%s no esta conectado en este momento", nombre);
	}
	else {
		printf ("Encontrad0");
		sprintf(resp, "%s esta conectado en este momento", nombre);
	}
	sprintf(respuesta,"%d/%s",3,resp);
	printf("Final EstaConectado\n");
}
// Devuelve el total de partidas ganadas
int PartidasJugadas(char *p,char respuesta[1024]){
	// A partir de un nombre que recibimos, realizamos una busqueda usando un count 
	// obteniendo asi el total de opartidas jugadas por un usuario.
	
	printf("Inicio PartidasJugadas\n");
	//Variables locales
	char nombre [20];
	char consulta [1024];
	char resp[1024];
	
	//Obtenemos el nombre
	strcpy (nombre, p);
	printf ("Nombre: %s\n", nombre);
	
	//Realizamos la busqueda
	sprintf(consulta, "SELECT count(*) FROM (Jugador, Participacion) WHERE (Jugador.usuario = '%s') AND (Participacion.id_usuario = Jugador.id)",nombre);
	
	//Comprobacion de errores
	err=mysql_query(conn, consulta);
	if (err!=0){
		printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	printf("%s",row[0]);
	if (row == NULL)
	{
		printf("No se ha obtenido la consulta \n"); 
	}
	else
	{
		int ans = atoi(row[0]);
		sprintf(resp,"%s ha jugado %d partidas \n",nombre,ans);
	}
	sprintf(respuesta,"%d/%s",5,resp);
	printf("Final PartidasJugadas....\n");
}
//Funcion tablero
int Tablero (char *p, char respuesta[1024]){
	char nombre [20];
	char consulta [1024];
	int codigo;
	
	strcpy (nombre, p);
	printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
	sprintf(consulta,"SELECT Jugador.partidas_jugadas FROM Jugador WHERE Jugador.usuario = '%s'",nombre);
	printf("%s",consulta);
	err=mysql_query(conn, consulta);
	if (err!=0)
	{
		printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
		exit(1);
	}	
	//Recogemos el resultado de la consulta 
	resultado = mysql_store_result(conn);
	row = mysql_fetch_row(resultado);
	if (row == NULL)
	{
		printf("No se ha obtenido la consulta\n"); 
	}
	else
	{
		printf("Nombre: %s, Partidas jugadas: %s\n", nombre, row[0]);
		int Partidas_jugadas = atoi(row[0]);
		
		if(Partidas_jugadas==0)
		{
			sprintf(respuesta,"7/No se ha jugado ninguna partida\n");
		}
		if (Partidas_jugadas!=0)
		{
			sprintf (consulta, "SELECT Partida.tablero FROM (Partida,Jugador,Participacion) WHERE Jugador.usuario = '%s' AND Jugador.id=Participacion.id_jugador AND Participacion.id_partida=Partida.id",nombre); 
			err=mysql_query(conn, consulta);
			if (err!=0)
			{
				printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
				exit(1);
			}
			//Recogemos el resultado de la consulta 
			resultado = mysql_store_result(conn);
			row = mysql_fetch_row(resultado);
			char tableros[200] = " ";
			if (row == NULL)
			{
				printf("No se ha obtenido la consulta \n"); 
			}
			else
			{
				while (row!=NULL)
				{
					printf("Tablero: %s \n",row[0]);
					sprintf(tableros,"%s , %s",tableros,row[0]);
					row = mysql_fetch_row(resultado);
				}
				
				sprintf(respuesta,"7/%s",tableros);
			}
		}
	}
}
//______________________________________________________________________________
//GESTION DE PARTIDAS:
//Cuando un jugador quiere crear una partida, busca un espacio libre y se establece 
//como host. En esta partida se guaradar los distintos detos de esta.
int CrearPartida (char nombre [20], char respuesta [1024], ListaPartidas *lista, int *sock_conn) {
	printf("Creando partida....\n");
	int r = 0;
	printf("Abriendo espacio libre\n");
	r = EspacioLibre(nombre,lista,sock_conn);
	printf("Cerrando espacio libre\n");
	if (r==0){
		sprintf(respuesta,"%s, vaya ha habido un error durante la creacion de la partida", nombre);
		printf("Error al crear la partida \n");
	}
	else if (r==-1){
		sprintf(respuesta,"%s, todas nuestras partidas estan ocupadas le rogamos que lo intente mas adelante",nombre);
		printf("Error al crear la partida \n");
	}
	else{
		sprintf(respuesta, "%s, su partida ha sido creada con exito", nombre);
		printf("Partida creada....\n");
	}
	

}
//Busca si hay algun espacio libre para crear una partida.
int EspacioLibre (char nombre [20], ListaPartidas *lista, int IdPartida,int sock_conn){
//Buscamos una partida que tenga el campo de ocupado en 0, valor asigando a vacio
//Si hay un error devolvera un 0, si la lista esta llena devuelve un -1 y si hay
//espacio libre devolvemos 1. Ademas llenaremos el campo IdPartida con la posicion
//de la partida pues este valor sera su id.
	
	int e=0;
	int i=0;
	int r=0;
	
	printf("Iniciando busqueda....\n");	
	if (lista->num < 100){
		while ((e==0) && (i<100)){
			if ((lista->Partidas[i].ocupado == 0)){
				printf("Espacio libre encontrado....\n");
				strcpy(lista->Partidas[i].Host, nombre);
				printf("Nombre del Host: %s\n",lista->Partidas[i].Host);
				lista->Partidas[i].sock[0]=sock_conn;
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
				printf("Host a�adido....\n");
				r=1;
				e=1;
			}
			i++;
		}
	}
	else{
		r=-1;
	}
	printf("I: %d\n");
	printf("Final de la busqueda....\n");
	return r;
	
}
//Cerramos una partida y la limpiamos para que pueda ser usada en un futuro
int FinalPartida (char nombre [20],ListaPartidas *lista){
//Esta funcion vacia un espacio de partida. Devuelve 1 se la accion se ha 
//realizado con exito y 0 si no ha podido.
	int e=0;
	int i=0;
	printf("Cerrando partida....\n");
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
	printf("Partida cerrada con exito....\n");
	return e;
}
//Busca una partida en entre una lista de estas y devuelve su posicion.
int BuscarPartida(char nombre [20], ListaPartidas *lista){
//Dado el nombre de un usuario buscamos dentro de nuestras partidas la partida 
//que este jugando en este momento. Si la encontramos devolveremos la posciion
//si no devolveremos un -1.
	int e=0;
	int i=0;
	printf("Iniciando BuscarPartida....\n");
	while ((e==0) && (i < 100)){
		if (((strcmp(lista->Partidas[i].Host,nombre)==0) || (strcmp(lista->Partidas[i].nom2,nombre)==0) || (strcmp(lista->Partidas[i].nom3,nombre)==0) || (strcmp(lista->Partidas[i].nom4,nombre)==0)) && (lista->Partidas[i].ocupado ==1)){
			e=1;
		}
		if (e==0){
			i++;
		}
	}
	printf("Posicion: %d Encontrado: %d\n",i,e);
	if (e==1){
		return i;
	}
	else{
		return -1;
	}
	printf("Cerrando BuscarPartida....\n");
}
//Busca la partida usando el nombre del host. Devuelve -1 si hay algun error,
//0 si la partida esta llena, 1 si se ha podido unir y -2 si no se ha encontrado
//la partida.

int UnirsePartida(char Host[20],char nombre[20],ListaConectados *listaC, ListaPartidas *listaP){
	printf("Iniciando UnirsePartida....\n");
	printf("Host: %s\n",Host);
	int posicion = BuscarPartida(Host,listaP);
	if (posicion==-1){
		return -2;
	}
	else {
		if (listaP->Partidas[posicion].numJugadores==0){
			return -1;
		}
		else if (listaP->Partidas[posicion].numJugadores==4){
			return 0;
		}
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
		else{
			return -1;
		}
	}
	printf("Cerrando UnirsePartida....\n");
}
//______________________________________________________________________________
//OPERACIONES DENTRO DE UNA PARTIDA:
//Procesa el mensje y lo envia a todos los miembros de la Partida
int CargarMensaje(char *p,ListaPartidas *listaP,char notificacion [1024]){
	printf("Cargando mensaje....\n");
	char mens [1024];
	char nombre [20];
	
	p = strtok( NULL, "/");
	strcpy(nombre,p);
	printf("Nombre: %s\n",nombre);
	p = strtok( NULL, "/");
	strcpy(mens, p);
	printf("Mensaje: %s\n",mens);
	sprintf(notificacion,"12/%s",mens);
	NotificacionPartida(notificacion,nombre,listaP);
	printf("Mensaje enviado....\n");
}

//______________________________________________________________________________
//GESTION DE INVITACIONES:
//Generamos la invitacion
int Invitacion(char *p,ListaConectados *listaC,ListaPartidas *listaP, char respuesta [1024], int *sock_conn){
//Recibimos una peticion, extraemos el nombre de quien envia la invitacion y su 
//destinatario. Con estos datos obtenemos el socket de destino para enviar la 
//invitacion del destinatario y generamos el mensaje.
	
	pthread_mutex_lock( &mutex );
	//Variables locales:
	char nomO [20];
	char nomD [20];
	int IdP;
	
	printf("Preperando invitacion....\n");
	p = strtok( NULL, "/");
	strcpy(nomO,p);
	printf("Origen: %s\n",nomO);
	p = strtok (NULL,"/");
	strcpy(nomD,p);
	printf("Destino: %s\n",nomD);
	int socketD = ReturnSocket(listaC, nomD);
	int socketO =  ReturnSocket(listaC, nomO);
	printf("Socket Destino:%d Socket Origen:%d \n",socketD, socketO);
	
	//Creamos la partida
	int r = BuscarPartida(nomO,listaP);
	printf("%d",r);
	if (r==-1){
		CrearPartida(nomO,respuesta,listaP,socketD);
		printf("Partida lista....\n");
	}
	
	//Devolveremos el codigo, la respuesta y el nombre del destino de la invitacion
	sprintf(respuesta,"9/Quieres jugar contra %s?/%s",nomO,nomD);
	printf("Respuesta: %s \n", respuesta);
	printf("Socket destino: %d\n",socketD);
	
	sock_conn = socketD;
	printf("Invitacion enviada....\n");
	pthread_mutex_unlock( &mutex);
}
//El destinatario dice si acepta o no unirse a la partida
int ConfirmarInvitacion(char *p, ListaConectados *listaC,ListaPartidas *listaP, char respuesta[1024], int *sock_conn) {
	//Variables locales
	char confirmacion[10];
	char nomO [20];
	char nomD[20];
	char respuesta1 [1024];
	
	pthread_mutex_lock( &mutex );
	printf("Preperando Respuesta....\n");
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
	printf("Socket Destino:%d Socket Origen:%d \n",socketD, socketO);
	
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
		printf("%s",respuesta);
		printf("%s",respuesta1);
		write (socketO,respuesta1, strlen(respuesta));
		printf("D\n");
	}
	else {
		sprintf(respuesta,"11/%s no ha podido unirse a vuestra partida",nomO);
	}
	printf("E\n");
	sock_conn = socketD;
	printf("Respuesta lista....\n");
	pthread_mutex_unlock( &mutex);
}
//______________________________________________________________________________
//NOTIFICACIONES:
//Crea la notificacion para la lista de conectados
int NotificacionGeneral(char notificacion [1024]){
	//Esta funcion utiliza otra funcion, DameConectados, de donde obtenie la lista 
	//de jugadores conectados. Luego usando un for lo enviara a todos los 
	//dispositivos conectados.
	
	DameConectados(&miLista,notificacion);
	int j;
	for (j=0; j<i; j++){
		write (sockets[j],notificacion, strlen(notificacion));
	}
}
//Notificacion para los miembros de una partida unicamente
int NotificacionPartida(char notificacion [1024],char nombre [20] , ListaPartidas *lista){
	int j;
	int posicion = BuscarPartida(nombre, lista);
	for (j=0; j<lista->Partidas[posicion].numJugadores; j++){
		write (lista->Partidas[posicion].sock[j],notificacion, strlen(notificacion));
	}
}

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
	
	printf("Inicio \n");
	
	//Socket para la ListaConectados
	int sock_conn;
	int *s;
	s=(int *) socket;
	sock_conn = *s;
	
	printf("Socket: %d \n",sock_conn);
	
	//Establecemos la conexion con nuestro servidor
	conn = mysql_init(NULL);
	if (conn==NULL){
		
		printf ("Error al crear la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}
	
	conn = mysql_real_connect (conn, "localhost","root", "mysql","T6_Juego1311",0, NULL, 0);
	
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
		printf ("Recibido \n");
		
		// Tenemos que añadirle la marca de fin de string 
		// para que no escriba lo que hay despues en el buffer
		peticion[ret]='\0';
		
		//Escribimos el nombre en la consola para comprobar el mensaje recibido
		printf ("Se ha conectado: %s \n",peticion);
		
		//Recogemos el codigo obtenido
		char *p = strtok( peticion, "/"); 
		// Troceamos el mensaje adquirido dividiendo entre funcion y datos
		codigo =  atoi (p); 
		
		//Comprobacion del codigo obtenido
		printf ("Codigo: %d \n",codigo);
		
		//Deconectamos a un usuario
		if (codigo==0){
			Cierre(respuesta,terminar);
		}
		else {
			//Conectamos a un usuario
			if (codigo == 1){
					ConectarUsuario(p,respuesta,sock_conn,&miLista);
				
			}
			printf("1\n");
			//Registramos a un usuario
			if(codigo == 2){
				RegistrarUsuario(p, respuesta, sock_conn);
			}
			printf("2\n");
			//Miramos si un usuario en concreto esta conectado
			if(codigo == 3){
				int s = EstaConectado(p, respuesta, &miLista);
			}
			printf("3\n");
			//Devuelve las partidas ganadas
			if (codigo == 4){ 
				int r = PartidasGanadas(p, respuesta);
			}
			printf("4\n");
			//Devuelve las partidas jugadas
			if (codigo == 5){
				int r = PartidasJugadas(p,respuesta);
			}
			printf("5\n");
			//Devuelve la lista de conectados manualmente
			if (codigo == 6){
				DameConectados(&miLista,respuesta);
			}
			printf("6\n");
			//Falta adaptar funcio
			if (codigo == 7){
				strcpy (nombre, p);
				printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
				sprintf(consulta,"SELECT Jugador.partidas_jugadas FROM Jugador WHERE Jugador.usuario = '%s'",nombre);
				printf("%s",consulta);
				err=mysql_query(conn, consulta);
				if (err!=0)
				{
					printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
					exit(1);
				}
				//Recogemos el resultado de la consulta 
				resultado = mysql_store_result(conn);
				row = mysql_fetch_row(resultado);
				if (row == NULL)
				{
					printf("No se ha obtenido la consulta\n"); 
				}
				else
				{
					printf("Nombre: %s, Partidas jugadas: %s\n", nombre, row[0]);
					int Partidas_jugadas = atoi(row[0]);
					
					if(Partidas_jugadas==0)
					{
						printf("Ninguna partida jugada\n");
						sprintf(respuesta,"0/No se ha jugado ninguna partida\n");
					}
					if (Partidas_jugadas!=0)
					{
						sprintf (consulta, "SELECT Partida.tablero FROM (Partida,Jugador,Participacion) WHERE Jugador.usuario = '%s' AND Jugador.id=Participacion.id_jugador AND Participacion.id_partida=Partida.id",nombre); 
						err=mysql_query(conn, consulta);
						if (err!=0)
						{
							printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
							exit(1);
						}
						//Recogemos el resultado de la consulta 
						resultado = mysql_store_result(conn);
						row = mysql_fetch_row(resultado);
						char tableros[200] = " ";
						if (row == NULL)
						{
							printf("No se ha obtenido la consulta \n"); 
						}
						else
						{
							while (row!=NULL)
							{
								printf("Tablero: %s \n",row[0]);
								sprintf(tableros,"%s , %s",tableros,row[0]);
								row = mysql_fetch_row(resultado);
							}
							
							sprintf(respuesta,"%d/%s",7,tableros);
						}
					}
				}
			}
			printf("7\n");
			//Devuelve el numero de consultas hechas
			if (codigo == 8){
				sprintf (respuesta,"%d/%d",8,contador);
			}
			printf("8\n");
			//Invitacion
			if (codigo == 9){
				Invitacion(p, &miLista,&miListaPartidas,respuesta,sock_conn);
			}
			printf("9\n");
			//Recibir respuesta de invitacion
			if (codigo == 10){				
				ConfirmarInvitacion(p,&miLista,&miListaPartidas,respuesta,sock_conn);
			}
			printf("10\n");
			if (codigo==11)	{
				idP=miListaPartidas.num;
				miListaPartidas.num++;
				sprintf(respuesta,"11/Selecciona contra quien quieres jugar :)");
				printf("Respuesta: %s \n", respuesta);
				// Enviamos la respuesta
				//write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("11\n");
			if (codigo==12){
				CargarMensaje(p,&miListaPartidas,notificacionP);
			}
			
			//escribir mensaje seguent setmana
			/*if (codigo == 12)
			{	
			int sockJug[4];
			char mensaje[100];
			char EscribeMensage[30];
			p= strtok(NULL, "/");
			int id = atoi(p);
			p= strtok(NULL, "/");
			strcpy (EscribeMensage,p);
			printf("%s\n",EscribeMensage);
			p= strtok(NULL, "/");
			strcpy (mensaje,p);
			
			int numJ = GiveMeSocketsJP(id,&miListaPartidas,&miLista,sockJug);
			sprintf(respuesta,"12/%d/%s/%s",id,EscribeMensage,mensaje);
			
			for (int i=0;i<numJ;i++)
			{
			if(sockJug[i] != sock_conn)
			{
			printf("Respuesta: %s\n",respuesta);
			write(sockJug[i],respuesta,strlen(respuesta));
			}
			}
			}
			printf("12\n");*/
			
			//Enviamos la respuesta de nuestras consultas
			if (codigo != 0){
				printf("Respuesta: %s \n", respuesta);
				printf("Socket receptor: %d\n",sock_conn);
				// Enviamos la respuesta
				write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("Enviar consulta\n");
			//Contamos en numero de consultas
			if ((codigo==1)||(codigo==2)|| (codigo==3)||(codigo==4)|| (codigo==5)|| (codigo==6)|| (codigo==7) || (codigo==8)|| (codigo==9) ||(codigo==11) || (codigo==12)){
				pthread_mutex_lock(&mutex);//no interrumpas
				contador = contador +1;
				pthread_mutex_unlock(&mutex);//puedes interrumpirme
				printf("Peticion contada\n");
				
				//Notificamos a todos los clientes conectados
				DameConectados(&miLista,nG);
				sprintf (notificacionG,"%s",nG);
				//Enviamos a todos los clientes conectados
				int j;
				for (j=0; j<i; j++){
					write (sockets[j],notificacionG, strlen(notificacionG));
				}
				//memset(notificacion,"",1024);
				//sprintf("%s",notificacion);
			}
		}
	printf("Numero: %d \n",miLista.num);
	printf("Usuario: %s \n",miLista.conectados[miLista.num].nombre);
	printf("Final\n");
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
		printf("Error creant socket \n");
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
	serv_adr.sin_port = htons(9050); // indicamos el puerto
	
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bin \n");
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0) 
		// indicamos que es sock pasivo, el dos marca el numero de peticiones 
		//maximas en cola
		printf("Error en el Listen \n");
	
	pthread_t thread;
	
	ListaConectados miLista;
	ListaPartidas miListaPartidas;
	Inicializar(&miLista,&miListaPartidas);
	
	for(;;){
		printf ("Escuchando\n"); //No ayuda a saber si hemos empezado a escuchar
		sock_conn = accept(sock_listen, NULL, NULL);
		//este sock se comunica con el programa con el que hemos conectado

		printf ("He recibido conexion\n"); //comprovamos si todo en orden
		//sock_conn es el socket que usaremos para este cliente
		//Bucle de atencion al cliente
		
		//Llenamos el vector de sockets
		sockets[i]=sock_conn;
		pthread_create (&thread,NULL,AtenderCliente,&sockets[i]);
		i++;
	}
}
