//#include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include "connect_utility.c"
//#include <errno.h>
#define BUFFER_SIZE 256

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    perror("error: please enter an IP address following the program name");
    exit(EXIT_FAILURE);
  }
  if (argc < 3)
  {
    perror("error: please enter a port number after the IP address");
    exit(EXIT_FAILURE);
  }

  int Res=0;
  char buffer[100];
  int port = argv[2];
  int SocketFD = connect_to_server(argv[1], atoi(argv[2]), &Res);
  /* perform read write operations ... */

  char username[10];
  printf("Please enter a username:\n");
  scanf("%s", username);
  printf("Your username is %s\n", username);
  
  //connect to server. the user can never send the join command
  bool move_on = false;
  while(!move_on)
  {
    if(!move_on)
    {  
      char status[2];
      char join_request[15];

      sprintf(join_request, "join %s", username);
      //printf("join request:%s\n", join_request);
   
            
      Res = send(SocketFD, join_request, 100, 0);
      //printf("bytesrecv: %lu\n", recv(SocketFD, status, 1, 0));
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
  }

  printf("Enter a command\n");
  while(true)
  {
    gets(buffer);
    char command[100];
    sscanf(buffer, "%s", command);
    
    if(strcmp(command, "bye") == 0)
    {
      printf("byeing\n");
    }
    else if(strcmp(command, "crea") == 0)
    {
      char crName[19];
      char command[23];
      char created[2];
      printf("Enter a chatroom name:\n");
      gets(buffer);

      sprintf(crName, "%s", buffer);

      sprintf(command, "crea %s\0", crName);

      Res = send(SocketFD, command, strlen(command)+1, 0);
      Res = recv(SocketFD, created, 2, 0);

      if(strcmp(created, "1") == 0)
      {
        printf("Chatroom %s has been created.\n", crName);
      }
      else
      {
        printf("Chatroom could not be created.\n");
      }      
    
    }
    else if(strcmp(command, "subs") == 0)
    {
      printf("Enter a chatroom you would like to subsribe to:\n");
      gets(buffer);
      char crName[20];
      char command[24];
      char subscribed[2];
      sscanf(buffer, "%s", crName);

      sprintf(command, "subs %s\0", crName);

      Res = send(SocketFD, command, strlen(command)+1, 0);
      Res = recv(SocketFD, subscribed, 2, 0);

      if(strcmp(subscribed, "1") == 0)
      {
        printf("You have been subscribed to chatroom %s.\n", crName);
      }
      else
      {
        printf("Either the chatroom does not exist, there is no more space in the chatroom, or you are already subsribed to this chatroom.\n");
      }
    }
    else if(strcmp(command, "unsu") == 0)
    {
      printf("Enter a chatroom you would like to unsubsribe from:\n");
      gets(buffer);
      char crName[20];
      char command[24];
      char subscribed[2];
      sscanf(buffer, "%s", crName);

      sprintf(command, "unsu %s\0", crName);

      Res = send(SocketFD, command, strlen(command)+1, 0);
      Res = recv(SocketFD, subscribed, 2, 0);

      if(strcmp(subscribed, "1") == 0)
      {
        printf("You have been unsubscribed to chatroom %s.\n", crName);
      }
      else
      {
        printf("Either the chatroom does not existor you are not subscribe to the chatroom.\n");
      }
    } 
    else if(strcmp(command, "shut") == 0)
    {
      printf("shuting down\n");
    }       
    else if(strcmp(command, "defa") == 0)
    {
      printf("Enter a chatroom that will serve as the default chatroom.\n");
      gets(buffer);
      char crName[20];
      char command[24];
      char changed[2];
      sscanf(buffer, "%s", crName);

      sprintf(command, "defa %s\0", crName);

      Res = send(SocketFD, command, strlen(command)+1, 0);
      Res = recv(SocketFD, changed, 2, 0);
      if(strcmp(changed, "1") == 0)
      {
        printf("Your defualt chatroom has been changed to %s.\n", crName);
      }
      else
      {
        printf("Either the chatroom does not exist or the chatroom is full.\n");
      }
    }
    else if(strcmp(command, "lscr") == 0)
    {
      printf("Requesting list of chatrooms.\n");
      char list[BUFFER_SIZE];
      Res = send(SocketFD, "lscr\0", 5, 0);
      Res = recv(SocketFD, list, BUFFER_SIZE, 0);

      printf("List of chatrooms: %s\n", list);
    }
    else if(strcmp(command, "lssu") == 0)
    {
      char list[BUFFER_SIZE];

      Res = send(SocketFD, "lssu\0", 5, 0);      
      Res = recv(SocketFD, list, BUFFER_SIZE, 0);
      printf("List of subscribed chatrooms: %s\n", list);
    }

    else
    {
      printf("Command not recognized.\n");
    }
    
    
    // if( Res==0 )
    // {
    //   //0==other side terminated conn
    //   printf("\nSERVER terminated connection\n");
    //   close(SocketFD);
    //   exit(EXIT_FAILURE);
    // }
//printf("igothere125\n");
  }
  
  (void) shutdown(SocketFD, SHUT_RDWR);
    
  close(SocketFD);
  return EXIT_SUCCESS;
}
