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
#include "../include/client_stub-private.h"
#include "../include/codes.h"

const char space[2] = " ";
const char all_keys[2] = "!";

// Estrutura para poder associar os comandos
// aos comandos
static struct commands_t lookUpTabble[] = {
	{"put", PUT}, 
	{"get", GET}, 
	{"update", UPDATE}, 
	{"del", DEL}, 
	{"size", SIZE},
	{"quit", QUIT}
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

// Talvez tenha de sr definido
// Função que imprime uma mensagem 
void print_msg(struct message_t *msg, const char* title) {
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

void printErrors(int code) {
	switch (code) {
		case SEM_ARG :
			printf("Comando nao conhecido, por favor tente de novo\n");
			printf("Exemplo de uso: put <key> <data>\n");
			printf("Exemplo de uso: update <key> <value>\n");
			printf("Exemplo de uso: get <key>\n");
			printf("Exemplo de uso: get !\n");
			printf("Exemplo de uso: del <key>\n");
			printf("Exemplo de uso: size\n");
			printf("Exemplo de uso: quit\n");
		break;

		case PUT_NO_ARGS:
			printf("Comando put sem argumentos, por favor tente de novo\n");
			printf("Exemplo de uso: put <key> <data>\n");
			break;

		case NO_COMMAND :
			printf("Erro ao introduzir comando. Por favor tente de novo\n");
			printf("Exemplo de uso: put <key> <data>\n");
			printf("Exemplo de uso: update <key> <value>\n");
			printf("Exemplo de uso: get <key>\n");
			printf("Exemplo de uso: get !\n");
			printf("Exemplo de uso: del <key>\n");
			printf("Exemplo de uso: size\n");
			printf("Exemplo de uso: quit\n");
			break;

		case GET_NO_ARG :
			printf("Comando get sem argumento. Por favor, tente de novo\n");
			printf("Exemplo de uso: get <key>\n");
			printf("Exemplo de uso: get !\n");
			break;

		case ERROR_SYS :
			printf("Erro de sistema. Por favor tente de novo\n");
			break;

		case UPDATE_NO_ARGS :
			printf("Comando update sem argumentos. Por favor tente de novo\n");
			printf("Exemplo de uso: update <key> <value>\n");
			break;

		case DEL_NO_ARG :
			printf("Comando del sem argumento. Por favor tente de novo\n");
			printf("Exemplo de uso: del <key>\n");
	}
}


/************************************************************
 *                    Main Function                         *
 ************************************************************/
int main(int argc, char **argv){

	struct rtable_t *table;
	char input[81];
	int stop, sigla, result, size;
	char *command, *token;
	char **arguments, **keys, *key;
	struct data_t *data;
	struct message_t *msg;

	//const char quit[5] = "quit";
	const char ip_port_seperator[2] = ":";
	const char get_all_keys[2] = "!";
	const char msg_title_out[31] = "Mensagem enviada para servidor";
	const char msg_title_in[30] = "Mensagem recebida do servidor";

	/* Testar os argumentos de entrada */
	// Luis: o nome do programa conta sempre como argumento
	if (argc != 2 || argv == NULL || argv[1] == NULL) { 
		printf("Erro de argumentos.\n");
		printf("Exemplo de uso: /table_client 10.101.148.144:54321\n");
		return ERROR; 
	}

	/* Usar r_table_bind para ligar-se a uma tablea remota */
	// Passa ip:porto
	table = rtable_bind(argv[1]);

	/* Fazer ciclo até que o utilizador resolva fazer "quit" */
	stop = 0;
 	while (stop == 0) { 

		printf(">>> "); // Mostrar a prompt para receber comando

		// Recebe o comando por parte utilizador
		fgets(input,80,stdin);
		// Retirar o caracter \n
		command = input;
		while (*command != '\n') { command++; }
		// Actualiza
		*command = '\0';
		// Faz o token e verifica a opção
		token = strtok(input, space);
		//Confirma se tem comando
		if (token == NULL) {
			printErrors(NO_COMMAND);
			continue;
		}

		// Sigla do comando para poder correr o switch
		sigla = keyfromstring(token);
		// Determina argumentos caso existam
		arguments = getTokens(token);
		// Cria a mensagem a encapsular os resultados
		// obtidos do servidor
		msg = (struct message_t *)malloc(sizeof(struct message_t *));

		// Faz o switch dos comandos
		switch(sigla) {
			
			case BADKEY :					
				// Comando inválido ok
				printErrors(NO_COMMAND);
				break;

			
			case PUT :
				// Verifica possiveis erros
				if (arguments == NULL || arguments[0] == NULL || arguments[1] == NULL) {
					// Possivel mensagem de erro
					printErrors(PUT_NO_ARGS);
					continue;
				}
				// Tamanho do data
				size = strlen(arguments[1]) + 1;
				// Criar o data 
				data = data_create2(size, arguments[1]);
				// Verifica data
				if (data == NULL) {
					// Possivel mensagem de erro ok
					printErrors(ERROR_SYS);
					continue;
				}
				key = arguments[0];
				// Faz o pedido PUT
				result = rtable_put(table, key, data);
				// Libertar memória
				data_destroy(data);
				// Cria a mensagem a imprimir
				msg->opcode = OC_PUT + 1;
				msg->c_type = CT_RESULT;
				msg->content.result = result;
				break;

			
			case GET :
				// Argumento do get
				if (arguments == NULL || arguments[0] == NULL) {
					// Possivel mensagem de erro ok
					printErrors(NO_COMMAND);
					continue;
				}
				// Faz o pedido GET key ou GET all_keys
				if (strcmp(arguments[0], all_keys) == 0) {
					keys = rtable_get_keys(table);
					// Cria a mensagem a imprimir
					msg->opcode = OC_GET + 1;
					msg->c_type = CT_KEYS;
					msg->content.keys = keys;
				}
				else {
					key = arguments[0];
					data = rtable_get(table, key);
					// Cria a mensagem a imprimir
					msg->opcode = OC_GET + 1;
					msg->c_type = CT_VALUE;
					msg->content.data = data;
				}
				break;

			
			case UPDATE :
				if (arguments == NULL ||arguments[0] == NULL || arguments[1] == NULL) {
					// Possivel mensagem de erro
					printErrors(UPDATE_NO_ARGS);
					continue;
				}
				// Tamanho do data
				size = strlen(arguments[1]) + 1;
				// Criar o data 
				data = data_create2(size, arguments[1]);
				// Verifica data
				if (data == NULL) {
					// Possivel mensagem de erro
					printErrors(ERROR_SYS);
					continue;
				}
				// Faz o pedido UPDATE
				result = rtable_update(table, arguments[0], data);
				// Cria a mensagem a imprimir
				msg->opcode = OC_UPDATE + 1;
				msg->c_type = CT_RESULT;
				msg->content.result = result;
				data_destroy(data);
				break;

				
			case DEL : 
				// Verifica argumentos
				if (arguments == NULL || arguments[0] == NULL) {
					// Possivel mensagem de erro
					printErrors(DEL_NO_ARG);
					continue;
				}
				// Faz o pedido DEL
				result = rtable_del(table, arguments[0]);
				// Cria a mensagem a imprimir
				msg->opcode = OC_DEL + 1;
				msg->c_type = CT_RESULT;
				msg->content.result = result;
				break;

				
			case SIZE :	
				// Se o comando é SIZE
				// arguments deve vir igual a null
				// Verifica, se for diferente de NULL
				// Significa que escreveu size qualquer_coisa_mais
				if (arguments != NULL) {
					// Possivel mensagem de erro
					printErrors(NO_COMMAND);
					continue;
				}
				// Faz pedido de SIZE
				result = rtable_size(table);
				// Cria mensagem e imprimir
				msg->opcode = OC_SIZE + 1;
				msg->c_type = CT_RESULT;
				msg->content.result = result;
				break;

			case QUIT :
				// Nota: arguments deve vir a null 
				// Caso contrário consideramos erro
				// Exemplo: quit qualquer_coisa_mais
				if (arguments != NULL) {
					// Possivel mensagem de erro
					printErrors(NO_COMMAND);
					continue;
				}
				// Informa client_stub que cliente deseja sair
				result = rtable_unbind(table);
				// Sai do ciclo
				stop = 1;
				break;
			}
			// Fim do switch
			// Envia a mensagem a ser imprimida
			if (sigla != QUIT) {
				print_msg(msg, msg_title_in);
				free_message(msg);
				list_free_keys(arguments);
			}
			else {
				free_message(msg);
				// Liberta argumentos
				list_free_keys(arguments);
				return result;
			}	
	}
	// Fim do ciclo...
	return OK;
}