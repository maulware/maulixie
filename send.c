#define _XOPEN_SOURCE 700

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  char buffer[BUFSIZ];
  char protoname[] = "tcp";
  struct protoent *protoent;
  char *server_hostname = "94.45.232.48";
  char *user_input = NULL;
  in_addr_t in_addr;
  in_addr_t server_addr;
  int sockfd;
  size_t getline_buffer = 0;
  ssize_t nbytes_read, i, user_input_len;
  struct hostent *hostent;
  /* This is the struct used by INet addresses. */
  struct sockaddr_in sockaddr_in;
  unsigned short server_port = 1234;

  protoent = getprotobyname(protoname);
  if (protoent == NULL) {
      perror("getprotobyname");
      exit(EXIT_FAILURE);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if (sockfd == -1) {
      perror("socket");
      exit(EXIT_FAILURE);
  }

//  sockaddr_in.sin_addr=server_addr;
  inet_pton(AF_INET,"94.45.232.48", &(sockaddr_in.sin_addr));
  sockaddr_in.sin_port=htons(server_port);
  sockaddr_in.sin_family=AF_INET;
  bzero(&(sockaddr_in.sin_zero), 8); 

  if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
      perror("connect");
      return EXIT_FAILURE;
  }

  sleep(1);

  char pri[1000];
  sprintf(pri,"OFFSET 30 1000\n");
  send(sockfd, pri, strlen(pri), 0);
  while(1){
    for(int x=0;x<40;x++){
      for(int y=0;y<40;y++){
        sprintf(pri,"PX %d %d 838119\n",x,y);
        send(sockfd, pri, strlen(pri), 0);
      }
    }
  }
}
