//#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include "connect_utility.c"

#define BUFFER_SIZE 256

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    perror("ussage: writer_process.c <IP ADDRESS> <PORT> <USERNAME>");
    exit(EXIT_FAILURE);
  }
 
  int Res=0;
  char buffer[BUFFER_SIZE];
  int SocketFD = connect_to_server(argv[1], atoi(argv[2]), &Res);

  bool move_on = false;
  while(!move_on)
  {
    if(!move_on)
    {  
      char status[2];
      char read_command[15];

      sprintf(read_command, "read %s\n", argv[3]);
      printf("Requesting reader connection at %s, port %s with username \"%s\" \n", argv[1], argv[2], argv[3]);
      
            
      Res = send(SocketFD, read_command, strlen(read_command)+1, 0);
      
      Res = recv(SocketFD, status, 2, 0);
      
      if(strcmp(status,"1") == 0)
      {
        move_on = true;
        printf("server accepted request.\n");
      }  
      else
      {
        printf("server rejected request.\n");
      }
    }
    while(1)
    {
      char username[20];
      char chatroom[20];
      char message[BUFFER_SIZE];

      Res = recv(SocketFD, buffer, BUFFER_SIZE, 0);
      //printf("%s\n", buffer);
      sscanf(buffer, "%s %s %[^\n]s", username, chatroom, message);

      //printf("%d\n", Res);
      
      if (!Res)
      {
        return 0;
      }

      printf("User %s has posted chatroom %s: %s.\n", username, chatroom, message);
    }
  }

  

  (void) shutdown(SocketFD, SHUT_RDWR);
    
  close(SocketFD);
  return EXIT_SUCCESS;
}