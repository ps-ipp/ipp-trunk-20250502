#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include "ippc.h"
#include "ippc_messages.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include "ippc_command.h"

#define MAX_KIDS    256
#define WAIT_FOR_KIDS    5

/** \file ippc_commandserver.c
 * \brief This is basically a big fat security problem.
 */

/* global variables */
int    kids = 0;

void  decrement_kids(int sig_num);

int main(int argc, char *argv[])
{
  int    server_sockfd, client_sockfd, server_len, client_len;
  int    result;
  char    buffer[STRLEN], *portString;
  uint16_t  netSafe;
  uint32_t  auth_token;
  unsigned short  status;
  struct  sockaddr_in server_address;
  struct  sockaddr_in client_address;

  if (IPPC_DEFAULT_VERBOSITY > 1) {
    gprint (GP_ERR, "%s : THIS PROGRAM _IS_ A REMOTE EXPLOIT!!!\n", argv[0]);
  }

  if (!(portString = getenv("IPPC_COMMANDSERVER_PORT"))) {
    portString = "1035";
  }

  server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_sockfd == 0) {
                perror("socket");
                exit(EXIT_FAILURE);
        }

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(atoi(portString));
  server_len = sizeof(server_address);

  result = bind(server_sockfd, (struct sockaddr *)&server_address, server_len); 
        if(result == -1) {
                perror("bind");
                exit(EXIT_FAILURE);
        }

  result = listen(server_sockfd, 10);
        if(result == -1) {
                perror("listen");
                exit(EXIT_FAILURE);
        }
  
  /* decrement the kiddie count after a child exits */
  signal(SIGCHLD, decrement_kids);

  /* flush dem pesky chars outa my buffer */
  memset(buffer, 0, sizeof(buffer));

  while(1) {
    if (IPPC_DEFAULT_VERBOSITY > 1) {
      gprint (GP_ERR, "%s : pid: %d - waiting for connection...\n", argv[0], getpid());
    }

    if (kids < MAX_KIDS) {
      client_len = sizeof(client_address);
      client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
    } else {
      if (IPPC_DEFAULT_VERBOSITY > 1) {
        gprint (GP_ERR, "%s : pid: %d - kids: %d - child limit already reached, sleeping %ds...\n",
          argv[0], getpid(), kids, WAIT_FOR_KIDS);
      }
      sleep(WAIT_FOR_KIDS);
      continue;
    }

    /* increment kiddie count before forking */
    kids++;

    if (fork() == 0) {
      if (IPPC_DEFAULT_VERBOSITY > 1) {
        gprint (GP_ERR, "%s : pid: %d - new kid number: %d\n", argv[0], getpid(), kids);
      }

      /* a non-zero result doesn't always mean an error occured
       * I need to find out more about how to handle error checking here
       */

      /* receive and check authentication key */
      recv(client_sockfd, &auth_token, sizeof(auth_token), 0);

      if (ntohl(auth_token) != COMMAND_KEY) {
        if (IPPC_DEFAULT_VERBOSITY > 1) {
          gprint (GP_ERR, "%s : pid: %d - invalid authentication key\n", argv[0], getpid());
        }

        netSafe = htons(-1);
        send(client_sockfd, &netSafe, sizeof(status), 0);

        close(client_sockfd);
        exit(EXIT_FAILURE);
      }

      /* receive command */
      recv(client_sockfd, buffer, STRLEN, 0);
  
      if (IPPC_DEFAULT_VERBOSITY > 1) {
        gprint (GP_ERR, "%s : pid: %d - recieved: %s\n", argv[0], getpid(), buffer);
      }
  
      /* oh man is this dangerous */
      status = system(buffer);
      if (status != 0) {
        if (IPPC_DEFAULT_VERBOSITY > 1) {
          gprint (GP_ERR, "%s : pid: %d - command failed - code: %d\n", argv[0], getpid(), status);
        }
        netSafe = htons(status);
        send(client_sockfd, &netSafe, sizeof(netSafe), 0);
      } else {
        if (IPPC_DEFAULT_VERBOSITY > 1) {
          gprint (GP_ERR, "%s : pid: %d - command successful\n", argv[0], getpid());
        }
        netSafe = htons(status);
        send(client_sockfd, &netSafe, sizeof(status), 0);
      }

      status = shutdown(client_sockfd, SHUT_RDWR);
      if (status) {
        perror("shutdown");
        exit(EXIT_FAILURE);
      }
  
      close(client_sockfd);
      exit(EXIT_SUCCESS);
    } else {
      close(client_sockfd);
    }
  }
}

void  decrement_kids(int sig_num)
{
  /* sysv requires the handler to be reinstalled */
  signal(SIGCHLD, decrement_kids);

  /* reap child exit value */
  while (waitpid(-1, NULL, WNOHANG) > 0) { kids--; }
}
