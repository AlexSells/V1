DROP DATABASE IF EXISTS Juego;
CREATE DATABASE Juego;
USE Juego;

CREATE TABLE Jugador(
  id INTEGER AUTO_INCREMENT,
  usuario VARCHAR(30) NOT NULL,
  contrasena VARCHAR(80) NOT NULL,
  email VARCHAR(255) NOT NULL,
  partidas_ganadas INT,
  conectado INTEGER,
  PRIMARY KEY (id)
)ENGINE = InnoDB;
 
CREATE TABLE Partida (
  id INT NOT NULL,
  dia VARCHAR(12),
  hora VARCHAR(10),
  matchtime INT,
  ganador VARCHAR(30),
  PRIMARY KEY (id)
)ENGINE = InnoDB;

CREATE TABLE Participacion (
  id_jugador INTEGER,
  id_partida INTEGER,
  posicion INTEGER,
  FOREIGN KEY (id_jugador) REFERENCES Jugador(id),
  FOREIGN KEY (id_partida) REFERENCES Partida(id) 
)ENGINE = InnoDB;


INSERT INTO Jugador(usuario, contrasena, email, conectado) VALUES ("Anna", sha2("1111", 256), "Anna@gmail.com",1);
INSERT INTO Jugador(usuario, contrasena, email, conectado) VALUES ("Taha", sha2("2222", 256), "Taha@gmail.com",1);
INSERT INTO Jugador(usuario, contrasena, email, conectado) VALUES ("Alex", sha2("3333", 256), "Alex@gmail.com",1);
INSERT INTO Jugador(usuario, contrasena, email, conectado) VALUES ("Juan", sha2("3333", 256), "Juan@gmail.com",1);

INSERT INTO Partida VALUES (1, '11/10/2022','10:20', 1, 'Anna');
INSERT INTO Partida VALUES (2, '11/10/2022','12:42', 2, 'Taha');
INSERT INTO Partida VALUES (3, '11/10/2022','14:30', 1, 'Alex');
INSERT INTO Partida VALUES (4, '11/10/2022','16:16', 3, 'Juan');
INSERT INTO Partida VALUES (5, '12/10/2022','12:22', 2, 'Anna');
INSERT INTO Partida VALUES (6, '12/10/2022','18:47', 1, 'Taha');
INSERT INTO Partida VALUES (7, '12/10/2022','19:23', 3, 'Alex');
INSERT INTO Partida VALUES (8, '12/10/2022','20:12', 1, 'Juan');


INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (1, 1, 1);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (2, 1, 2);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (3, 1, 3);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (4, 1, 4);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (1, 2, 4);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (2, 2, 3);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (3, 2, 2);
INSERT INTO Participacion(id_jugador, id_partida, posicion) VALUES (4, 2, 1);
