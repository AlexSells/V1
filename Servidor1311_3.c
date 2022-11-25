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

//	Creamos la lista
ListaConectados miLista;

//Declaramos la lista de Partidas
typedef struct {
	int ID;
	int numJugadores;
	char tablero[30];
	//Sockets
	char nom1[20];
	char nom2[20];
	char nom3[20];
	char nom4[20];
	int sock[4];
}Partida;

typedef struct {
	Partida Partidas [1000];
	int num;
} ListaPartidas;

ListaPartidas miListaPartidas;
//------------------------------------------------------------------------------
//Funciones:
// 	Inicializamos nuestra lista
int Inicializar(ListaConectados *lista){
	lista->num=0;
}

//	Añadimos un usuario a nuestra lista
int Add (ListaConectados *lista, char nombre[20],int socket){
//Añade un nuevo conectado. Si la lista esta llena retorna -1 y si ha 
//añadido con exito al nuevo jugador retorna 0.
	
	printf("Inicio Add\n");
	printf("Numero: %d",lista->num);
	int a = lista->num;
	
	if (lista->num==100)
		return -1;
	else{
		printf("Añadiendo \n");
		strcpy(lista->conectados[lista->num].nombre,nombre);
		printf("Nombre: %s\n",lista->conectados[lista->num].nombre);
		lista->conectados[lista->num].socket = socket;
		printf("Socket: %d\n",socket);
		lista->num=a+1;
		printf("Numero: %d",lista->num);
		s = s +1;
		printf("%d\n",s);
		printf("Nombre:%s Socket: %d\n",lista->conectados[a].nombre,lista->conectados[a].socket);
		return 0;
	}
	
	printf("Final Add \n");
}

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
	while((i < 100) && encontrado==0)
	{
		if (strcmp(lista->conectados[i].nombre,nombre) == 0)
		{
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


//	Eliminamos al usuario de la ListaConectados, al desconectarse
int Disconect (ListaConectados *lista, char nombre[20]){
//retorna 0 si el usuario se ha desconectado con exito y -1 si ha ocurrido 
//un error.
	printf("Inicio Disconect\n");
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
	printf("Final Disconect\n");
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

//	Creamos un string con todos los conectados 
void DameConectados (ListaConectados *lista, char respuesta [1024]){
	//Devuelve llena la lista conectados. Esta se llenara con el nombre de 
	//todos los conectados separados por /. Ademas primero nos dara el numero 
	//de conectados.
	
	char resp[1024];
	
	printf("Inicio DameConectados\n");
	sprintf(respuesta,"%d",s);
	printf("%s\n",respuesta);
	
	int i;
	for (i=0; i<lista->num; i++){
		sprintf(resp, "%s/%s \n",resp, lista->conectados[i].nombre);
		printf("%s\n",resp);
	}
	sprintf(respuesta,"%d/%s",6,resp);
	printf("Final DameConectados\n");
}

//	Comprueba si un usuario existe y lo añade a la lista conectados
int ConectarUsuario (char *p,char respuesta [1024], int socket, ListaConectados *lista){
//Despues de comprobar si elusuario se encuentra en la base de datos lo añado a 
//la ListaConectados y genera el mensaje pertinente. Devuelve 1 si ha 
//realizado la funcion con exito y 0 alternativamente.
	
	printf("Inicio ConectarUsuario\n");
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
	printf("Final ConectarUsuario\n");
}

//	Añade un usuario y lo guarda en la lista conectados
int RegistrarUsuario(char *p,char respuesta [1024], int socket){
//Añadimos un nuevo usuario a nuestra base de datos y a nuestra Listadeconectados
//devolvemos 1 si hemos realizado la accion correctamente y 0 alternativamente.
	
	printf("Inicio RegistrarUsuario\n");
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
	
	
	//Añadimos la funcion que queremos que haga
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
	printf("Final RegistrarUsuario\n");
}


//	Nos devuelve en numero de victorias de un jugador
int PartidasGanadas(char *p,char respuesta [1024]){
	
	printf("Inicio PartidasGanadas\n");
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
	printf("Final PartidasGanadas\n");
}
//	Pregunta si un usuario esta conectado
int EstaConectado(char *p,char respuesta [1024], ListaConectados *lista){
//Obtenemos el nombre de un usuario y buscamos si esta conectado. Esta funcion 
//podria obtenerse de una consulta sql o bien de nuestra lista conectados. En 
//este nos aprovecharemos de nuestra lista conectados.
	
	printf("Inicio EstaConectado\n");
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
	printf("Final PartidasJugadas\n");
}

//Crea la notificacion para la lista de conectados
int Notificacion(char notificacion [1024]){
//Esta funcion utiliza otra funcion, DameConectados, de donde obtenie la lista 
//de jugadores conectados. Luego usando un for lo enviara a todos los 
//dispositivos conectados.
	
	DameConectados(&miLista,notificacion);
	int j;
	for (j=0; j<i; j++){
		write (sockets[j],notificacion, strlen(notificacion));
	}
}

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

int Invitacion (ListaConectados *lista,char respuesta[1024], char nomO[20],int socketO, char nomD[20],int socketD, int *sock_conn){
	int i = 0;
	socketD = ReturnSocket(lista, nomD);
	socketO =  ReturnSocket(lista, nomO);
	printf("%d %d \n",socketD, socketO);
	//Devolveremos el codigo, la respuesta y el nombre del destino de la invitacion
	sprintf(respuesta,"9/Quieres jugar contra %s?/%s",nomO,nomD);
	*sock_conn = socketO;
}

int ConfirmarInvitacion(ListaConectados *lista,ListaPartidas *listaP, char respuesta[1024], char confirmacion[10], int socketO,int socketD, char nomO [20], char nomD[20], int *sock_conn)
{

	sprintf(respuesta,"%s",123);
	*sock_conn = socketO;
}
int GiveMeSocketsJP(ListaPartidas *listaP, ListaConectados *lista, int id, int socketsJug[4])
{
	int i=0;
	while(i< listaP->Partidas[id].numJugadores)
	{
		socketsJug[i]= listaP->Partidas[id].sock[i];
		i++;
	}
	return listaP->Partidas[id].numJugadores;
}

int PruebaConfirmacion(ListaConectados *lista, char respuesta [1024], char confirmacion [10], char nomO [20], char nomD [20], int *sock_conn){
	int socketD;
	int socketO;
	socketD = ReturnSocket(lista, nomD);
	socketO =  ReturnSocket(lista, nomO);
	printf("%d %d \n",socketD, socketO);
	if (strcmp(confirmacion, "SI")==0){
		//AñadirJugador
		sprintf(respuesta,"10/ %s ha aceptado el desafio",nomD);
	}
	else{
		sprintf(respuesta,"10/ Vaya, %s esta ocupado",nomD);
	}
	*sock_conn = socketO;
	
}


//------------------------------------------------------------------------------
//Funcion atender cliente:
void *AtenderCliente (void *socket){
	printf("Inicio \n");
	//Socket para la ListaConectados
	
	int sock_conn;
	int *s;
	s=(int *) socket;
	sock_conn = *s;
	
	printf("Socket: %d \n",sock_conn);
	
	//Aqui recogermos la peticion del usuario y la respuesta del servidor
	char peticion[1024];
	char respuesta[1024];
	char conectado[200];
	int contadorSocket=0;
	int ret;
	
	int socketO;//Origen
	int socketD;//Destino
	char nomO[20];
	char nomD[20];
	
	//Creamos e inicializamos la lista
	
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
		//Variables que usaremos en nuestro while
		int codigo;
		char nombre[20];
		char contrasena[20];
		int partidas_ganadas;
		char consulta [80];
		
		// Ahora recibimos su peticion
		ret=read(sock_conn,peticion, sizeof(peticion));
		printf ("Recibido \n");
		
		// Tenemos que aÃ±adirle la marca de fin de string 
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
			//Funcion de desconecion
			printf("Cierre \n");
			sprintf(respuesta,"Adios");
			sprintf(respuesta,"%d/%s",0,respuesta);
			terminar=1;
		}
		else {
			//Conectamos a un usuario
			if (codigo == 1) {
					ConectarUsuario(p,respuesta,sock_conn,&miLista);
				pthread_mutex_unlock(&mutex);
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
			if (codigo == 9)
			{
				printf("A\n");
				printf ("Peticion: %s", peticion);
				p = strtok( NULL, "/");
				printf("%s\n");
				strcpy(nomO,p);
				printf("%s\n",nomO);
				p = strtok (NULL,"/");
				strcpy(nomD,p);
				printf("%s\n",nomD);
				
				pthread_mutex_lock( &mutex );
				Invitacion(&miLista,respuesta,nomO,socketO,nomD,socketD,&sock_conn);
				pthread_mutex_unlock( &mutex);
				printf("Respuesta: %s \n", respuesta);
			}
			printf("9\n");
			
			//Recibir respuesta de invitacion
			if (codigo == 10)
			{				
				char confirmacion[10];
				p = strtok( NULL, "/");
				strcpy(confirmacion,p);
				p=strtok(NULL,"/");
				strcpy(nomO,p);
				printf("Invitador: %s\n",nomO);
				socketO = ReturnSocket(&miLista,p);
				p= strtok (NULL,"/");
				strcpy(nomD,p);
				printf("Invitado: %s\n",nomD);
				socketD = ReturnSocket(&miLista,p);
				printf("%d %d\n",socketD, socketO);
				printf("%s\n",peticion);
				
				
				
				pthread_mutex_lock( &mutex );
				printf("Preparando confirmacion: \n");
				//ConfirmarInvitacion(&miLista,&miListaPartidas,confirmacion,socketO,socketD,respuesta,nomO,nomD,&sock_conn);
				PruebaConfirmacion(&miLista,respuesta,confirmacion,nomO,nomD,&sock_conn);
				pthread_mutex_unlock( &mutex);
				printf("Respuesta: %s \n", respuesta);
			}
			printf("10\n");
			
			if (codigo==11)
			{
				idP=miListaPartidas.num;
				miListaPartidas.num++;
				sprintf(respuesta,"11/Selecciona contra quien quieres jugar :)");
				printf("Respuesta: %s \n", respuesta);
				// Enviamos la respuesta
				//write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("11\n");
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
				char notificacion[1024];
				char n [1024];
				DameConectados(&miLista,n);
				sprintf (notificacion,"%s",n);
				//Enviamos a todos los clientes conectados
				int j;
				for (j=0; j<i; j++){
					write (sockets[j],notificacion, strlen(notificacion));
				}
				//memset(notificacion,"",1024);
				//sprintf("%s",notificacion);
			}
		}
	printf("Numero: %d \n",miLista.num);
	printf("Usuario: %s \n",miLista.conectados[miLista.num].nombre);
	printf("Final\n");
	// Se acabo el servicio para este cliente
	}
	close(sock_conn); 
}

int main(int argc, char *argv[])
{
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
	miLista.num=0;
	
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
