/*
 * Programa servidor:
 *   Recebe uma string do cliente e envia o tamanho dessa string.
 *   Utiliza poll para tratar várias ligações simultaneamente.
 *   Apenas responde a um cliente de cada vez.
 *
 *   Este programa é apenas um exemplo. Os alunos não têm que fazer 
 *   exatamente como está exemplificado. O exemplo não trata um número de
 *   situações que em situação real requerem tratamento.
 *
 * Como compilar:
 *   gcc -Wall -g server.c -o server
 *
 * Como executar:
 *   server <porto_servidor>
 */

#define NFDESC 4 // Número de sockets (uma para listening)
#define TIMEOUT 50 // em milisegundos

#include "inet.h"
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
  struct pollfd connections[NFDESC]; // Estrutura para file descriptors das sockets das ligações
  int sockfd; // file descriptor para a welcoming socket
  struct sockaddr_in server, client; // estruturas para os dados dos endereços de servidor e cliente
  char str[MAX_MSG + 1];
  int nbytes, count, nfds, kfds, i;
  socklen_t size_client;

  // Verifica se foi passado algum argumento
  if (argc != 2){
    printf("Uso: ./server <porto_servidor>\n");
    printf("Exemplo de uso: ./server 12345\n");
    return -1;
  }

  // Cria socket TCP. Função *SOCKET*.
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    perror("Erro ao criar socket");
    return -1;
  }

  // Preenche estrutura server para bind
  server.sin_family = AF_INET;  // Adress Family
  server.sin_port = htons(atoi(argv[1]));  // argv[1] é o primeiro argumento da linha de comando (porta)
  server.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY significa "Qualquer endereço".

  // Faz bind. Liga a socket ao(s) endereço(s). Função *BIND*.
  if (bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0){
      perror("Erro ao fazer bind");
      close(sockfd);
      return -1;
  };

  // Faz listen. Marca a socket como sendo welcoming socket. Função *LISTEN*.
  if (listen(sockfd, 0) < 0){
      perror("Erro ao executar listen");
      close(sockfd);
      return -1;
  };

  printf("Servidor à espera de dados\n");
  size_client = sizeof(struct sockaddr);

  for (i = 0; i < NFDESC; i++)
    connections[i].fd = -1;    // poll ignora estruturas com fd < 0

  connections[0].fd = sockfd;  // Vamos detetar eventos na welcoming socket
  connections[0].events = POLLIN;  // Vamos esperar ligações nesta socket

  nfds = 1; // número de file descriptors

  // Retorna assim que exista um evento ou que TIMEOUT expire. * FUNÇÂO POLL *.
  while ((kfds = poll(connections, nfds, 10)) >= 0) // kfds == 0 significa timeout sem eventos
    
    if (kfds > 0){ // kfds é o número de descritores com evento ou erro

      if ((connections[0].revents & POLLIN) && (nfds < NFDESC))  // Pedido na listening socket ?
        if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0){ // Ligação feita ?
          connections[nfds].events = POLLIN; // Vamos esperar dados nesta socket
          nfds++;
      }
      for (i = 1; i < nfds; i++) // Todas as ligações

        if (connections[i].revents & POLLIN) { // Dados para ler ?

          // Lê string enviada pelo cliente do socket referente à conexão i
          if((nbytes = read(connections[i].fd, str, MAX_MSG)) < 0){
            perror("Erro ao receber dados do cliente");
            close(connections[i].fd);
            connections[i].fd = -1;
            continue;
          }

          // Coloca terminador de string
          str[nbytes] = '\0';
          printf("Recebido do cliente %d: %s\n", i, str);

          // Conta numero de caracteres
          count = strlen(str);

          // Converte count para formato de rede
          count = htonl(count);

          // Envia tamanho da string ao cliente através do socket referente a conexão
          if((nbytes = write(connections[i].fd, &count, sizeof(count))) != sizeof(count)){
            perror("Erro ao enviar resposta ao cliente");
            close(connections[i].fd);
            connections[i].fd = -1;
            continue;
          }
        }
    }

  // Fecha socket de listening (só executado em caso de erro...)
  close(sockfd);
  return 0;
}

