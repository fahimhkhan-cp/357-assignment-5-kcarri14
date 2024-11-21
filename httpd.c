#define _GNU_SOURCE
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

void handle_cgi(int nfd, char *filename, char *query);

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

   if ((num = getline(&line, &size, network)) < 0)
   {
      perror("getline failed");
      close(nfd);
      return;
   }
     printf("are you getting here?-after getline\n");

   char type[8];
   char filename[1024];
   char http_version[16];

   int line_read = sscanf(line, "%s %s %s", type, filename, http_version);
   printf("are you getting here?-after line read\n");
   printf("type: %s\n" ,type);
   printf("name: %s\n" ,filename);
   printf("version: %s\n" ,http_version);
   if(line_read != 3 || (strcmp(type, "GET") != 0 && strcmp(type, "HEAD") != 0)){
      char *response = "HTTP/1.0 400 Bad Request\r\nContent-Type: text/html\r\nContent-Length: 35\r\n\r\n<html><body>400 Bad Request</body></html>";
      write(nfd, response, strlen(response));
      free(line);
      fclose(network);
      return;
   }
   printf("are you getting here?-after get \n");
   if((strcmp(type, "GET") != 0 && strcmp(type, "HEAD") != 0)){
      char *response = "HTTP/1.0 501 Not Implemented\r\nContent-Type: text/html\r\nContent-Length: 44\r\n\r\n<html><body>400 Bad Request</body></html>";
      write(nfd, response, strlen(response));
      free(line);
      fclose(network);
      return;
   }
   printf("are you getting here?-after more errors\n");
   if(strncmp(filename, "/cgi-like/", 10) == 0){
      handle_cgi(nfd, filename, line);
   }
   
   printf("are you getting here?-after cgi like?\n");
   if(filename[0] == '/'){
      while(filename[0] != '\0'){
         filename[0] = filename[1];
         (*filename)++;
      }
   }

   if(strstr(filename, "..") != NULL){
      char *response = "HTTP/1.0 403 Permission Denied\r\nContent-Type: text/html\r\nContent-Length: 40\r\n\r\n<html><body>403 Permission Denied</body></html>";
      write(nfd, response, strlen(response));
      free(line);
      fclose(network);
      return;
   }
   printf("are you getting here?-before filepath\n");
   char filepath[1024];
   snprintf(filepath, sizeof(filepath), "./%s", filename);

   int file = open(filepath, O_RDONLY);
   printf("are you getting here?-after opening file\n");
   if(file == -1){
      char *response = "HTTP/1.0 404 Not Found\r\nContent-Type: text/html\r\nContent-Length: 38\r\n\r\n<html><body>404 Not Found</body></html>";
      write(nfd, response, strlen(response));
   }else{
      struct stat file_stat;
      fstat(file, &file_stat);

      char header[512];
      if(strcmp(type, "GET") == 0 || strcmp(type, "HEAD") == 0){
         snprintf(header, sizeof(header), "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lld\r\n\r\n", file_stat.st_size);
         write(nfd, header, strlen(header));

         if(strcmp(type, "GET") == 0){
            char buffer[1024];
            ssize_t read_bytes;
            while((read_bytes = read(file, buffer,sizeof(buffer)))> 0){
               write(nfd, buffer, read_bytes);
            }
         }
     }
     close(file);
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
            const char *response = "HTTP/1.0 500 Internal Error\r\nContent-Type: text/html\r\nContent-Length: 41\r\n\r\n<html><body>500 Internal Server Error</body></html>";
            write(nfd, response, strlen(response));
            return;
        }else if(pid == 0){
            close(fd);
            printf("Connection established\n");
            printf("%d\n" , nfd);
            handle_request(nfd);
            printf("Connection closed\n");
            close(nfd);
        }else{
            close(nfd);
        }
         
      }
   }


}
void handle_cgi(int nfd, char *filename, char *query){
   printf("are you getting here?-in cgi request?\n");
   if(strncmp(filename, "/cgi-like/", 10) != 0){
      char *response = "HTTP/1.0 403 Permission Denied\r\nContent-Type: text/html\r\nContent-Length: 40\r\n\r\n<html><body>403 Permission Denied</body></html>";
      write(nfd, response, strlen(response));
      return;
   }
   char file_path[1024];
   snprintf(file_path, sizeof(file_path), "./cgi-like/%s", filename + 10);
   printf("filepath %s\n", file_path);
   printf("are you getting here?-before fork\n");
   pid_t pid = fork();
   if(pid < 0){
      const char *response = "HTTP/1.0 500 Internal Error\r\nContent-Type: text/html\r\nContent-Length: 41\r\n\r\n<html><body>500 Internal Server Error</body></html>";
      write(nfd, response, strlen(response));
      return;
   }else if(pid == 0){
      printf("In Child process\n");
      char *arguments[] = {file_path, query, NULL};
      execvp(file_path, arguments);
      printf("Child process successful\n");
   }else{
      printf("In parent process\n");
      wait(NULL);
   }
   printf("are you getting here?-after fork\n");
}

void handle_signal(int sig){
    while(waitpid(-1,NULL,WNOHANG) >0);
}

int main(int argc, char *argv[]){
    if (argc != 2){
        perror("Error: need the port number");
        return -1;
    }

    short port = atoi(argv[1]);

    if (port < 1024 || (int) port > 65535){
        perror("Error: port needs to be in between 1024 and 65535");
        return -1;
    }

   int fd = create_service(port);

   signal(SIGCHLD, handle_signal);

   if (fd == -1)
   {
      perror(0);
      exit(1);
   }

   printf("listening on port: %d\n", port);
   printf("are you getting here?-before run service\n");
   run_service(fd);
   printf("are you getting here?-after run service\n");
   close(fd);

   return 0;
}
