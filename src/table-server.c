/*
*		Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/


/*
   Programa que implementa um servidor de uma tabela hash com chainning.
   Uso: table-server <porta TCP> <dimensão da tabela>
   Exemplo de uso: ./table_server 54321 10
*/
#include <error.h>
#include <errno.h>
#include <poll.h>

#include "../include/inet.h"
#include "../include/table-private.h"
#include "../include/message-private.h"
#include "../include/table_skel.h"

#define ERROR -1
#define OK 0
#define NFDESC 4 // Número de sockets (uma para listening)
#define TIMEOUT 50 // em milisegundos

/* Função para preparar uma socket de receção de pedidos de ligação.
*/
int make_server_socket(short port){
  int socket_fd;
  struct sockaddr_in server;

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("Erro ao criar socket");
    return -1;
  }

  server.sin_family = AF_INET;
  server.sin_port = htons(port);  
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(socket_fd, (struct sockaddr *) &server, sizeof(server)) < 0){
      perror("Erro ao fazer bind");
      close(socket_fd);
      return -1;
  }

  if (listen(socket_fd, 0) < 0){
      perror("Erro ao executar listen");
      close(socket_fd);
      return -1;
  }
  return socket_fd;
}

/* Função que garante o envio de len bytes armazenados em buf,
   através da socket sock.
*/
int write_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len > 0){
		int res = write(sock, buf, len);
		if(res < 0){
			if(errno == EINTR) continue;
			perror("write failed:");
			return res;
		}
		buf+= res;
		len-= res;
	}
	return bufsize;
}

/* Função que garante a receção de len bytes através da socket sock,
   armazenando-os em buf.
*/
int read_all(int sock, char *buf, int len){
	int bufsize = len;
	while(len > 0){
		int res = read(sock, buf, len);
		if(res < 0){
			if(errno == EINTR) continue;
			perror("read failed:");
			return res;
		}
		buf+= res;
		len-= res;
	}
	return bufsize;
}



/* Função "inversa" da função network_send_receive usada no table-client.
   Neste caso a função implementa um ciclo receive/send:

	Recebe um pedido;
	Aplica o pedido na tabela;
	Envia a resposta.
*/
int network_receive_send(int sockfd){

	char *message_resposta, *message_pedido;
	int msg_length;
	int message_size, msg_size, result;
	struct message_t *msg_pedido, *msg_resposta;

	/* Com a função read_all, receber num inteiro o tamanho da 
	   mensagem de pedido que será recebida de seguida.*/
	result = read_all(sockfd, (char *) &msg_size, _INT);
	/* Verificar se a receção teve sucesso */
	if(result != _INT){return ERROR;}

	/* Alocar memória para receber o número de bytes da
	   mensagem de pedido. */
	message_size = ntohl(msg_size);
	message_pedido = (char *) malloc(message_size);

	/* Com a função read_all, receber a mensagem de resposta. */
	result = read_all(sockfd, message_pedido, message_size);

	/* Verificar se a receção teve sucesso */
	if(result != message_size){return ERROR;}
	/* Desserializar a mensagem do pedido */
	msg_pedido = buffer_to_message(message_pedido, message_size);

	/* Verificar se a desserialização teve sucesso */
	if(msg_pedido == NULL){return ERROR;}

	/* Processar a mensagem */
	msg_resposta = invoke(msg_pedido);

	/* Serializar a mensagem recebida */
	message_size = message_to_buffer(msg_resposta, &message_resposta);
	/* Verificar se a serialização teve sucesso */
	if(message_resposta <= OK){return ERROR;}
	/* Enviar ao cliente o tamanho da mensagem que será enviada
	   logo de seguida
	*/
	msg_size = htonl(message_size);
	result = write_all(sockfd, (char *) &msg_size, _INT);
	/* Verificar se o envio teve sucesso */
	if(result != _INT){return ERROR;}

	/* Enviar a mensagem que foi previamente serializada */
	result = write_all(sockfd, message_resposta, message_size);

	/* Verificar se o envio teve sucesso */
	if(result != message_size){return ERROR;}
	/* Libertar memória */

	free(message_resposta);
	free(message_pedido);
	free(msg_resposta);
	free(msg_pedido);
	return OK;
}



int main(int argc, char **argv){
	/*
	int listening_socket, connsock, result;
	int client_on = 1;
	struct sockaddr_in client;
	socklen_t size_client;
	struct table_t *table;

	// Nota: 3 argumentos. O nome do programa conta!
	if (argc != 3){
	printf("Uso: ./server <porta TCP> <dimensão da tabela>\n");
	printf("Exemplo de uso: ./table-server 54321 10\n");
	return -1;
	}

	if ((listening_socket = make_server_socket(atoi(argv[1]))) < 0) return -1;

	if (table_skel_init(atoi(argv[2])) == ERROR){
		result = close(listening_socket);
		return -1;
	}
	
	while(1){
		printf("waiting client\n");
		connsock = accept(listening_socket, (struct sockaddr *) &client, &size_client);
		if(connsock < 0){
			printf("error\n");
		}
		printf(" * Client is connected!\n");
		while (client_on){
			/* Fazer ciclo de pedido e resposta 
			if(network_receive_send(connsock) < 0){
				printf("maybe Enviar opcode erro\n");
        		client_on = 0;
			}else{
				printf("cliente received everything\n");
			}

		}
		printf("client offline\n");
		close(connsock);
	}
	*/

	struct pollfd connections[NFDESC]; // Estrutura para file descriptors das sockets das ligações
 	int sockfd; // file descriptor para a welcoming socket
 	struct sockaddr_in server, client; // estruturas para os dados dos endereços de servidor e cliente
 	char str[MAX_MSG + 1];
 	int nbytes, count, nfds, kfds, i;
 	socklen_t size_client;


 	// Nota: 3 argumentos. O nome do programa conta!
	if (argc != 3){
		printf("Uso: ./server <porta TCP> <dimensão da tabela>\n");
		printf("Exemplo de uso: ./table-server 54321 10\n");
		return -1;
	}

 	sockfd = make_server_socket(atoi(argv[1]));
 	if(table_skel_init(atoi(argv[2])) == ERROR){
		close(sockfd);
		return -1;
	}
	
 	printf("Servidor à espera de dados\n");
 	size_client = sizeof(struct sockaddr);

  	for (i = 0; i < NFDESC; i++){
  		connections[i].fd = -1;    // poll ignora estruturas com fd < 0
  	}

	connections[0].fd = sockfd;  // Vamos detetar eventos na welcoming socket
  	connections[0].events = POLLIN;  // Vamos esperar ligações nesta socket

  	nfds = 1; // número de file descriptors
  	int client_on = 1;
  	// Retorna assim que exista um evento ou que TIMEOUT expire. * FUNÇÂO POLL *.
  	while ((kfds = poll(connections, nfds, 10)) >= 0){ // kfds == 0 significa timeout sem eventos
    
   		 if (kfds > 0){ // kfds é o número de descritores com evento ou erro

      		if ((connections[0].revents & POLLIN) && (nfds < NFDESC)){  // Pedido na listening socket ?
        		if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0){ // Ligação feita ?
         			connections[nfds].events = POLLIN; // Vamos esperar dados nesta socket
          			nfds++;
      			}
      		}
      		for (i = 1; i < nfds; i++){ // Todas as ligações

        		if (connections[i].revents & POLLIN) { // Dados para ler ?
        			//sim
        			printf(" * Client is connected!\n");
					while (client_on){
						/* Fazer ciclo de pedido e resposta */
						if(network_receive_send(connections[0].fd) < 0){
							printf("maybe Enviar opcode erro\n");
        					client_on = 0;
						}else{
							printf("cliente received everything\n");
						}
					}
				}

			}
			printf("client offline\n");
			close(connections[0].fd);
        }
    }

}