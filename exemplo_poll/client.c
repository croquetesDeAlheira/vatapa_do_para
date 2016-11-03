/*
 * Programa cliente:
 *   Envia uma string ao servidor, e recebe o tamanho dessa string.
 *
 * Como compilar:
 *   gcc -Wall -g client.c -o client
 * 
 * Como executar:
 *   ./client <ip_servidor> <porto_servidor> <string>
 */

#include "inet.h"
#include <errno.h>
#include <time.h>
#include <signal.h>

#define N 20 // Número de mensagens entre clientes e servidor

int testInput(int argc)
{
  if (argc != 3){
    printf("Uso: ./client <ip_servidor> <porto_servidor>\n");
    printf("Exemplo de uso: ./client 127.0.0.1 12345\n");
    return -1;
  } 
    
  return 0;
}

int main(int argc, char **argv){
  int sockfd;  // Para guardar o file descriptor da socket
  struct sockaddr_in server;  // Para guardar dados do endereço do servidor 
  char str[] = "Vamos ver até onde isto vai...", c;
  int count, nbytes, i, k;
  struct sigaction s;

  srandom(getpid()); // Para que vários clientes tenham uma sequência aleatória diferente

  s.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &s, NULL) < 0){
    printf("Could not ignore SIGPIPE. Aborting.\n");
    return -1;
  }
  
  // Verifica se foi passado algum argumento
  if (testInput(argc) < 0) return -1;
    
  // Cria socket TCP. Função *SOCKET*
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Erro ao criar socket TCP");
    return -1;
  }
	
  // Preenche estrutura server com a informação de endereçamento do
  // servidor, para depois estabelecer conexão
  server.sin_family = AF_INET;             // Address Family
  server.sin_port = htons(atoi(argv[2]));  // argv[2] é o segundo argumento na linha de comandos (a porta)
  if (inet_pton(AF_INET, argv[1], &server.sin_addr) < 1) {  // argv[1] é o primeiro (o IP)
    printf("Erro ao converter IP\n");
    close(sockfd);
    return -1;
  }

  // Estabelece conexão com o servidor definido em server. Função *CONNECT*.
  if (connect(sockfd,(struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("Erro ao conectar-se ao servidor");
    close(sockfd);
    return -1;
  }

  for (i = 0; i < N; i++){

    k = 10 + (random() % 21);
    c = str[k];
    str[k] = '\0';

    // Envia string. Função *WRITE*.
    if((nbytes = write(sockfd, str, strlen(str))) != strlen(str)){
      perror("Erro ao enviar dados ao servidor");
      close(sockfd);
      return -1;
    }

    str[k] = c;
  
    printf("Á espera de resposta do servidor ...\n");

    // Recebe tamanho da string. Função *READ*.
    if((nbytes = read(sockfd,&count,sizeof(count))) != sizeof(count)){
      perror("Erro ao receber dados do servidor");
      close(sockfd);
      return -1;
    }

    printf("O tamanho da string enviada é %d!\n", ntohl(count));

    sleep((random() % 3) + 1);

  }

  // Fecha o socket. Função *CLOSE*.
  close(sockfd);
  return 0;
}

