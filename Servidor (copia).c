#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <mysql.h>

int main(int argc, char *argv[])
{
	MYSQL *conn;
	
	int err;
	MYSQL_RES *resultado;
	MYSQL_ROW row;
	char consulta [80];
	conn = mysql_init(NULL);
	char str_query[512];
	char str_query1[512];
	int partidas_ganadas;
	
	if (conn==NULL)
	{

		printf ("Error al crear la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}

	conn = mysql_real_connect (conn, "localhost","root", "mysql",
			"Juego",0, NULL, 0);
	if (conn==NULL)
	{

		printf ("Error al inicializar la conexion: %u %s\n",
				mysql_errno(conn), mysql_error(conn));
		exit (1);
	}

	int sock_conn, sock_listen, ret;
	struct sockaddr_in serv_adr;
	char peticion[512];
	char respuesta[512];
	// INICIALITZACIONS
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		//socket en el que el servidor espera un pedido
		printf("Error creant socket");
	// Fem el bind al port
	
	
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
	
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY configura siempre 
	//la ip asignada
	// escucharemos en el port 9050
	serv_adr.sin_port = htons(9050); // indicamos el puerto
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bind");
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0) 
		// indicamos que es sock pasivo, el dos marca el numero de peticiones 
		//maximas en cola
		printf("Error en el Listen");
	
	int i;
	// Atenderemos solo 7 peticione
	for(;;){
		
		printf ("Escuchando\n"); //No ayuda a saber si hemos empezado a escuchar
		
		sock_conn = accept(sock_listen, NULL, NULL);
		//este sock se comunica con el programa con el que hemos conectado
		printf ("He recibido conexion\n"); //comprovamos si todo en orden
		//sock_conn es el socket que usaremos para este cliente
		//Bucle de atencion al cliente
		int terminar =0;
		while(terminar==0){
			// Ahora recibimos su nombre, que dejamos en buff
			ret=read(sock_conn,peticion, sizeof(peticion));
			//lle un mensaje de text que llega a traves del sock y lo guarda en el 
			//buffer peticion, obteenmos el tama�o del mensaje
			printf ("Recibido\n");
			
			// Tenemos que a�adirle la marca de fin de string 
			// para que no escriba lo que hay despues en el buffer
			peticion[ret]='\0';
			
			//Escribimos el nombre en la consola
			printf ("Se ha conectado: %s\n",peticion);
			char *p = strtok( peticion, "/"); 
			// troceamos el mensaje adquirido dividiendo entre funcion y datos
			int codigo =  atoi (p); 
			char nombre[20];
			printf ("Codigo: %d \n",codigo);
			
			if (codigo==0){
				printf("cierre");
				sprintf(respuesta,"Adios");
				printf ("Codigo a : %d \n",codigo);
				terminar=1;
				
			}
			else 
			{
				//p = strtok( NULL, "/");
				//strcpy (nombre, p);
				//p=strtok(NULL,"/");
				//char contraseya[20];
				//strcpy(contraseya,p);
				//printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
				//printf("%s %s \n",nombre, contraseya);
				
				if (codigo ==1) {// Conectarse 
					p = strtok( NULL, "/");
					strcpy (nombre, p);
					p=strtok(NULL,"/");
					char contraseya[20];
					strcpy(contraseya,p);
					printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
					printf("%s %s \n",nombre, contraseya);
					sprintf(str_query, "SELECT ID FROM Jugadors WHERE Usuari= '%s' AND Contrasenya='%s';", nombre,contraseya);
					//escribimos en el buffer respuestra la longitud del nombre
					err=mysql_query (conn, str_query);
					printf("1\n");
					if (err!=0)
					{
						printf ("Error al consultar datos de la base %u %s \n",
								mysql_errno(conn), mysql_error(conn));
						sprintf(respuesta,"Vaya un error se ha producido1");
					}
					else{
						sprintf(respuesta,"Bien venido %s",nombre);
					}
					printf("2\n");
					strcpy(str_query,"select * from Jugadors");
					printf("%s\n",str_query);
					err=mysql_query (conn, str_query);
					printf("prueba\n");
					
				}
				printf("3\n");
				if(codigo==2){
					p = strtok( NULL, "/");
					strcpy (nombre, p);
					p=strtok(NULL,"/");
					char contraseya[20];
					strcpy(contraseya,p);
					printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
					
					printf("%s %s \n",nombre, contraseya);
					sprintf(str_query, "INSERT INTO Jugadors(Usuari, Contrasenya, Email,Conectat) VALUES ('%s', '%s', 'correo@gmail.com',1);", nombre,contraseya);
					//escribimos en el buffer respuestra la longitud del nombre
					err=mysql_query (conn, str_query);
					if (err!=0)
					{
						printf ("Error al consultar datos de la base %u %s \n",
								mysql_errno(conn), mysql_error(conn));
						sprintf(respuesta,"Vaya un error se ha producido2");
					}
					else{
						sprintf(respuesta,"Bien venido al club %s",nombre);
					}
				}
				if(codigo==3){
					
					p = strtok( NULL, "/");
					strcpy (nombre, p);
					printf ("Codigo: %d, Nombre: %s\n", codigo, nombre);
					printf("%s\n",nombre);
					printf("a\n");
					//sprintf(str_query, "SELECT Conectat FROM Jugadors WHERE Usuari= '%s';", nombre);
					strcpy(str_query,"select * from Jugadors");
					printf("%s\n",str_query);
					err=mysql_query (conn, str_query);
					printf("prueba\n");
					if (err!=0)
					{
						printf ("Error al consultar datos de la base %u %s \n",
								mysql_errno(conn), mysql_error(conn));
						sprintf(respuesta,"Vaya un error se ha producido un error");
						printf("b\n");
					}
					else {
						resultado = mysql_store_result (conn);
						row = mysql_fetch_row (resultado);
						int consulta1 = atoi(row[0]);
						printf("c\n");
						
						if (row == NULL)
							printf ("No se han obtenido datos en la consulta\n");
						
						else{
							printf("d\n");
							if (consulta1==1)
								sprintf ("%s esta conectado",nombre);
							if (consulta1==0)
								sprintf("%s no esta conectado",nombre);
							
							row = mysql_fetch_row (resultado);
							int consulta1 = atoi(row[0]);
							printf("e\n");
							//while (row != NULL)
							//{
								
							//}
						}
					}
				}
				
				if (codigo == 4)
				{ 
					strcpy(nombre,p);
					printf("Nombre: %s, Codigo: %d, partidas_ganadas: %d \n",nombre, codigo, partidas_ganadas);
					sprintf(str_query,"SELECT Jugador.partidas_ganadas FROM Jugador WHERE Jugador.usuario ='%s' ",nombre);
					err=mysql_query(conn, str_query);
					if (err!=0){
						printf ("Error al consultar datos de la base %u %s\n", mysql_errno(conn),mysql_error(conn));
						exit(1);
					}
					resultado = mysql_store_result(conn);
					row = mysql_fetch_row(resultado);
					if (row == NULL)
					{
						printf("No se ha obtenido la consulta \n"); 
					}
					else
					{
						int ans = atoi(row[0]);
						printf("El usuario ha ganado %d partidas \n",ans);
						sprintf(respuesta,"%s",row[0]);
					}
				}
				if (codigo==5)
				{
					char nombre1[20];
					printf("Numero de partidas jugadas por: ");
					scanf("%s", nombre1);
					char consulta[1000];
					strcpy(consulta, "SELECT count(*) FROM (Jugador, Participacion) WHERE (Jugador.usuario = 'nombre1') AND (Participacion.id_usuario = Jugador.id)");
					strcat(consulta, nombre1);
				}
			}
			printf("4\n");
			
			
			
			// convertimos el primer fragmento del mensaje en integrer
			
			if (codigo==0){
				printf (" 1 %s\n", respuesta);
				// Y lo enviamos
				write (sock_conn,respuesta, strlen(respuesta));
			}
			if (codigo!=0){
				write (sock_conn,respuesta, strlen(respuesta));
			}
			printf("5\n");
		}
		printf("6\n");
		// Se acabo el servicio para este cliente
		close(sock_conn); 
	}
}
