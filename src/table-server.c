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

#include "../include/inet.h"
#include "../include/table-private.h"
#include "../include/message-private.h"

#define ERROR -1
#define OK 0

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


/* Função que recebe uma tabela e uma mensagem de pedido e:
	- aplica a operação na mensagem de pedido na tabela;
	- devolve uma mensagem de resposta com o resultado.
*/
struct message_t *process_message(struct message_t *msg_pedido, struct table_t *tabela){
	//mensagem para devolver a resposta
	struct message_t *msg_resposta = (struct message_t*)malloc(sizeof(struct message_t));
	
	if(msg_resposta == NULL){
		return NULL;
	}
	/* Verificar parâmetros de entrada */
	if(msg_pedido == NULL || tabela == NULL){
		return NULL;
	}
	
	/* Verificar opcode e c_type na mensagem de pedido */
	short opcode = msg_pedido->opcode;
	short c_type =msg_pedido->c_type;	
	char *all = "!";
	// Mensagem de uma chave que nao existe
	char* n_existe = "Nao existe";
	struct data_t *dataRet = data_create2(strlen(n_existe) + 1, n_existe);
	/* Aplicar operação na tabela */
	// opcode de resposta tem que ser opcode + 1
	char** all_keys;
	char* msg_chaves_vazias = "Nao existem chaves";
	char* msg_erro = "Erro... Volte a tentar!";
	switch(opcode){
		case OC_SIZE:
			msg_resposta->opcode = OC_SIZE_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_size(tabela);
			break;
		case OC_DEL:
			msg_resposta->opcode = OC_DEL_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_del(tabela, msg_pedido->content.key);
			break;
		case OC_UPDATE:
			msg_resposta->opcode = OC_UPDATE_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_update(tabela, msg_pedido->content.entry->key, msg_pedido->content.entry->value);
			break;
		case OC_GET:
			// c_type CT_VALUE se a chave não existe
			// c_type CT_KEYS
			// c_type se for ! table_get_keys
			// c_type se for key table_get
			printf("key %s\n", msg_resposta->content.key);
			msg_resposta->opcode = OC_GET_R;
			if(strcmp(msg_pedido->content.key,all) == 0){
				// Get de todas as chaves
				msg_resposta->c_type = CT_KEYS;
				all_keys = table_get_keys(tabela);
				if (*all_keys == NULL) {
					// Será erro ou tabela vazia
					if (table_size(tabela) == 0){
						*all_keys = msg_chaves_vazias;
					}else{
						*all_keys = msg_erro;
					}
				}
				msg_resposta->content.keys = all_keys;
			}else if(table_get(tabela, msg_pedido->content.key) == NULL){
				//struct data_t *dataRet = data_create(0);
				msg_resposta->c_type = CT_VALUE;
				msg_resposta->content.data = dataRet;
			}else{
				msg_resposta->c_type = CT_VALUE;
				msg_resposta->content.data = table_get(tabela, msg_pedido->content.key);
			}
			break;			
		case OC_PUT:
			msg_resposta->opcode = OC_PUT_R;
			msg_resposta->c_type = CT_RESULT;
			msg_resposta->content.result = table_put(tabela, msg_pedido->content.entry->key, msg_pedido->content.entry->value);
			break;
		
		default:	
			printf("opcode nao e´ valido, opcode = %i\n", opcode);
	}
	/* Preparar mensagem de resposta */
	//printf("opcode = %i, c_type = %i\n", msg_resposta->opcode, msg_resposta->c_type);	
	return msg_resposta;
}


/* Função "inversa" da função network_send_receive usada no table-client.
   Neste caso a função implementa um ciclo receive/send:

	Recebe um pedido;
	Aplica o pedido na tabela;
	Envia a resposta.
*/
int network_receive_send(int sockfd, struct table_t *table){

	char *message_resposta, *message_pedido;
	int msg_length;
	int message_size, msg_size, result;
	struct message_t *msg_pedido, *msg_resposta;

	/* Verificar parâmetros de entrada */
	if(table == NULL){ return ERROR;}
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
	msg_resposta = process_message(msg_pedido, table);

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

	if ((table = table_create(atoi(argv[2]))) == NULL){
	result = close(listening_socket);
	return -1;
	}
	while ((connsock = accept(listening_socket, (struct sockaddr *) &client, &size_client)) != -1) {
		printf(" * Client is connected!\n");

		while (client_on){

			/* Fazer ciclo de pedido e resposta */
			if(network_receive_send(connsock, table) < 0){
				/* Ciclo feito com sucesso ? Houve erro?
			   	Cliente desligou? */
				printf("maybe Enviar opcode erro\n");
        			client_on = 0;
        			close(connsock);
			}	
		}
	}
}