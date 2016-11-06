/*
*	Grupo 12
* @author Daniel Santos 44887
* @author Luis Barros  47082
* @author Marcus Dias 44901
*/


/*
	Programa cliente para manipular tabela de hash remota.
	Os comandos introduzido no programa não deverão exceder
	80 carateres.

	Uso: table-client <ip servidor>:<porta servidor>
	Exemplo de uso: ./table_client 10.101.148.144:54321
*/

#include <string.h>
#include <stdio.h>

#include "../include/network_client-private.h"
#include "../include/message-private.h"

// Definindo os tipos de comandos
#define BADKEY -1
#define PUT 1
#define GET 2
#define UPDATE 3
#define DEL 4
#define SIZE 5

const char space[2] = " ";

// Estrutura para poder associar os comandos
// aos comandos
static struct commands_t lookUpTabble[] = {
	{"put", PUT}, 
	{"get", GET}, 
	{"update", UPDATE}, 
	{"del", DEL}, 
	{"size", SIZE} 
};

// Numero de comandos definidos de maneira automatica
#define NKEYS (sizeof(lookUpTabble) / sizeof(struct commands_t))

// Funcao que me dá o valor de um comando
// correspondente a uma string para usar no switch
int keyfromstring(char *key) {
	int i;
	// Estrutura definida em network_client-private.h
	struct commands_t p;

  for (i = 0; i < NKEYS; i++) {
  	p = lookUpTabble[i];
	if (strcmp(p.key, key) == 0)
    	return p.val;
   }
    return BADKEY;
}

// Devolve um apontador de apontadores
// contendo o resto dos tokens
char ** getTokens (char* token) {
	char **tokens, **result;
	char *p;
	int i, size;

	tokens = (char**)malloc(3 * sizeof(char*));
	if (tokens == NULL) { return NULL; }
	// Le o próximo token
	token = strtok(NULL, space);
	
	size = 0;
	i = 0;
	while (token != NULL) {
		tokens[i] = strdup(token);
		token = strtok(NULL, space);
		i++;
		size++;
	}

	result = (char**)malloc((size + 1) * sizeof(char*));
	
	for(i = 0; i < size; i++) {
		result[i] = strdup(tokens[i]);
		free(tokens[i]);
	}

	free(tokens);
	result[size] = NULL;

	return result;
}

// Função que imprime uma mensagem 
void print_msg(struct message_t *msg,const char* title) {
	int i;
	printf("%s\n", title);
	//printf("opcode = %i\n", msg->opcode);
	printf("opcode: %d, c_type: %d\n", msg->opcode, msg->c_type);
	switch(msg->c_type) {
		case CT_ENTRY:{
			printf("key: %s\n", msg->content.entry->key);
			printf("datasize: %d\n", msg->content.entry->value->datasize);
		}break;
		case CT_KEY:{
			printf("key: %s\n", msg->content.key);
		}break;
		case CT_KEYS:{
				//for(i = 0; msg->content.keys[i] != NULL; i++) {
				int i = 0;
				while(msg->content.keys[i] != NULL){
					printf("key[%d]: %s\n", i, msg->content.keys[i]);
					i++;
				}
		}break;
		case CT_VALUE:{
			printf("data: %s\n",(char*)msg->content.data->data);
			printf("datasize: %d\n", msg->content.data->datasize);
			
		}break;
		case CT_RESULT:{
			printf("result: %d\n", msg->content.result);
		};
	}
	printf("-------------------\n");
}





/************************************************************
 *                    Main Function                         *
 ************************************************************/
int main(int argc, char **argv){

	struct server_t *server;
	char input[81];
	struct message_t *msg_out, *msg_resposta, *msg_size;
	int i, stop, sigla, m_size, size;
	char *command, *token;
	char **arguments, **dataToNetwork;
	struct data_t *data;

	const char quit[5] = "quit";
	const char ip_port_seperator[2] = ":";
	const char get_all_keys[2] = "!";
	const char msg_title_out[31] = "Mensagem enviada para servidor";
	const char msg_title_in[30] = "Mensagem recebida do servidor";

	/* Testar os argumentos de entrada */
	// Luis: o nome do programa conta sempre como argumento
	if (argc != 2 || argv == NULL || argv[1] == NULL) { 
		printf("Erro de argumentos.\n");
		printf("Exemplo de uso: /table_client 10.101.148.144:54321\n");
		return -1; 
	}

	/* Usar network_connect para estabelecer ligação ao servidor */
	// Passa ip:porto
	server = network_connect(argv[1]);
	if(server == NULL){exit(0);}
	/* Fazer ciclo até que o utilizador resolva fazer "quit" */
	stop = 0;
 	while (stop == 0){ 

		printf(">>> "); // Mostrar a prompt para receber comando

		/* Receber o comando introduzido pelo utilizador
		   Sugestão: usar fgets de stdio.h
		   Quando pressionamos enter para finalizar a entrada no
		   comando fgets, o carater \n é incluido antes do \0.
		   Convém retirar o \n substituindo-o por \0.
		k*/
		fgets(input,80,stdin);

		// Retirar o caracter \n
		command = input;
		while (*command != '\n') { command++; }
		// Actualiza
		*command = '\0';


		/* Verificar se o comando foi "quit". Em caso afirmativo
		   não há mais nada a fazer a não ser terminar decentemente.
		 */
		// Luis: Ler o primeiro token para avaliar
		token = strtok(input, space);

		// Leu quit sai do ciclo while
		if (strcmp(quit,token) == 0) { 
			stop = 1; 
		} else {
			/* Caso contrário:
			Preparar msg_out;
			Usar network_send_receive para enviar msg_out para
			o server e receber msg_resposta.
			*/

			// Sigla do comando para poder correr o switch
			sigla = keyfromstring(token);
			// Inicializa a mensagem
			msg_out = (struct message_t*)malloc(sizeof(struct message_t));
			if (msg_out == NULL) { break; } // Salta o ciclo

			switch(sigla) {
				case BADKEY :
					// Algo como
					printf("Comando nao conhecido, por favor tente de novo\n");
					printf("Exemplo de uso: put <key> <data>\n");
					printf("Exemplo de uso: get <key>\n");
					printf("Exemplo de uso: get !\n");
					printf("Exemplo de uso: del <key>\n");
					printf("Exemplo de uso: size\n");
					printf("Exemplo de uso: quit\n");
					break;

				case PUT :
					// argumentos do put
					arguments = getTokens(token);
					size = strlen(arguments[1]) + 1;
					// Criar o data 
					data = data_create2(size, arguments[1]);
					//Criar o entry
					// Atributos de msg
					msg_out->opcode = OC_PUT;
					msg_out->c_type = CT_ENTRY;
					msg_out->content.entry = 
						entry_create(arguments[0], data);

					// Libertar memória
					data_destroy(data);
					break;

				case GET :
					arguments = getTokens(token);
					// Atributos de msg
					msg_out->opcode = OC_GET;
					msg_out->c_type = CT_KEY;
					// Verifica o tipo de comando
					msg_out->content.key = strdup(arguments[0]);
					break;

				case UPDATE :
					// argumentos do put
					arguments = getTokens(token);
					size = strlen(arguments[1]) + 1;
					// Criar o data 
					data = data_create2(size, arguments[1]);
					// Atributos de msg
					msg_out->opcode = OC_UPDATE;
					msg_out->c_type = CT_ENTRY;
					msg_out->content.entry = 
						entry_create(arguments[0], data);
					// Libertar memória
					data_destroy(data);
					break;

				case DEL : 
					arguments = getTokens(token);		
					msg_out->opcode = OC_DEL;
					msg_out->c_type = CT_KEY;
					msg_out->content.key = strdup(arguments[0]);
					break;

				case SIZE :	
					msg_out->opcode = OC_SIZE;
					msg_out->c_type = CT_RESULT; 
					break;
			}

			// Mensagens carregadas
			// Trata de enviar e receber 
			// Faz os prints necessários
			if (sigla != BADKEY) {
				
			
				// Envia a msg com o pedido e aguarda resposta
				//prt("aqui");
				print_msg(msg_out, msg_title_out);
				msg_resposta = network_send_receive(server, msg_out);
				// Imprime msg a enviar
				// Imprime a msg recebida
				print_msg(msg_resposta, msg_title_in);
				// Liberta memoria dos argumentos e da memoria
				
				/*ISSO AQUI EM BAIXO TA A DAR ALGUM ERRO...TEMOS DE VER DEPOIS COMO FAZEMOS ESSES FREES */
				if (sigla != SIZE)
					list_free_keys(arguments);
				free_message(msg_out);
				free_message(msg_resposta);
					
			}
			
		}
	}
	int result = network_close(server);
	printf("result = %d\n", result );
  	//return network_close(server);
  	return 0;
}