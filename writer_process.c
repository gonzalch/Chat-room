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
      char write_command[15];

      sprintf(write_command, "writ %s\n", argv[3]);
      printf("Requesting writer connection at %s, port %s with username \"%s\" \n", argv[1], argv[2], argv[3]);
      
            
      Res = send(SocketFD, write_command, strlen(write_command)+1, 0);
      
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
      gets(buffer);
      //sprintf(buffer, "%s\0", buffer);
      Res = send(SocketFD, buffer, strlen(buffer)+1, 0);      
    }
  }

  

  (void) shutdown(SocketFD, SHUT_RDWR);
    
  close(SocketFD);
  return EXIT_SUCCESS;
}
