#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void handle_request(int nfd)
{
   FILE *network = fdopen(nfd, "r");
   char *line = NULL;
   size_t size;
   ssize_t num;

   if (network == NULL)
   {
      perror("fdopen");
      close(nfd);
      return;
   }

   while ((num = getline(&line, &size, network)) >= 0)
   {
      write(nfd, line, num);
   }

   free(line);
   fclose(network);
}

void run_service(int fd)
{
   while (1)
   {
      int nfd = accept_connection(fd);
      if (nfd != -1)
      {
        pid_t pid = fork();
        if(pid < 0){
            perror("Error: fork failed");
        }else if(pid == 0){
            close(fd);
            printf("Connection established\n");
            handle_request(nfd);
            printf("Connection closed\n");
            close(nfd);
        }else{
            close(nfd);
        }
         
      }
   }


}

int main(int argc, char *argv[]){
    if (argc != 2){
        perror("Error: need the port number");
        return -1;
    }

    int port = atoi(argv[1]);

    if (port < 1024 || port > 65535){
        perror("Error: port needs to be in between 1024 and 65535");
        return -1;
    }

   int fd = create_service(port);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", port);
   run_service(fd);
   close(fd);

   return 0;
}
