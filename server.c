//include <sys/types.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <pthread.h>

typedef int bool;
#define true 1
#define false 0

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256
#define NUM_PTHREADS 2
#define MAX_CHATROOMS 5

int SocketFD;
int server_socket;
int ConnectFD;
struct sockaddr_in stSockAddr;
int tempClientSD; 
  
pthread_mutex_t readerMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readerCond = PTHREAD_COND_INITIALIZER;

int clients_connected = 0;

 /* Used as argument to thread_start() */
struct thread_info 
{   
   pthread_t thread_id;        /* ID returned by pthread_create() */
   int       thread_num;       /* Application-defined thread # */  
};

struct client
{
  struct sockaddr_in address; 
  bool    connected;
  int     command_socket;
  int      reader_socket;
  int     writer_socket;
  fd_set  command_data;
  fd_set  writer_data;
  fd_set  reader_data;
  int     address_length;
  char    username[15];
  char    message[BUFFER_SIZE];
  bool    has_message;
  struct  chatroom *default_chatroom;
};

struct chatroom
{
  char name[20];
  struct client* current_clients[MAX_CLIENTS];
  bool active;
};

struct client clients[MAX_CLIENTS];
struct chatroom chatrooms[5];

void start_server(int port)
{
  int k=0;
  for(k; k<MAX_CHATROOMS; k++)
  {
    if(k==0)
    {
      sprintf(chatrooms[k].name, "%s", "Lobby");
      chatrooms[k].active = true;
    }
    else
    {
      sprintf(chatrooms[k].name, "%s", "");
      chatrooms[k].active = false;
    }
  }

  SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  server_socket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );

  if(-1 == SocketFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }

  memset(&stSockAddr, 0, sizeof(stSockAddr));
    
  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(port);
  stSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  printf("Chat server started on port: %i\n", port);
  

  if(-1 == bind(SocketFD,(struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    
  if(-1 == listen(SocketFD, 10))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
}

bool accept_client(struct client *in_client, char *crw)
{
  if(strcmp(crw, "command")==0)
  {
    in_client->address_length = sizeof ((struct sockaddr *)&in_client->address);
  
    in_client->command_socket = accept(SocketFD, (struct sockaddr *)&in_client->address, &in_client->address_length );
   
    if ( in_client->command_socket == 0)
    { 
      perror("error accept failed");
      close(in_client->command_socket);
    
      return false;  
    }
    else
    {
      in_client->connected = true;
      printf("User has connected via command process\n");

      FD_ZERO ( &in_client->command_data );
      FD_SET ( in_client->command_socket, &in_client->command_data );

      return true;
    }
  }
  if(strcmp(crw, "read")==0)
  {
    in_client->reader_socket = accept(SocketFD, (struct sockaddr *)&in_client->address, &in_client->address_length );
    if ( in_client->reader_socket == 0 )
    { 
      perror("error accept failed");
      close(in_client->reader_socket);
    
      return false;  
    }
    else
    {
      in_client->connected = true;
      printf("User has connected via reader process.\n");

      FD_ZERO ( &in_client->reader_data );
      FD_SET ( in_client->reader_socket, &in_client->reader_data );
      
      return true;
    } 
  }
  if(strcmp(crw, "write")==0)
  {
    in_client->writer_socket = accept(SocketFD, (struct sockaddr *)&in_client->address, &in_client->address_length );
    if ( in_client->writer_socket == 0 )
    { 
      perror("error accept failed");
      close(in_client->writer_socket);
    
      return false;  
    }
    else
    {
      in_client->connected = true;
      printf("User has connected via writer process.\n");

      FD_ZERO ( &in_client->writer_data );
      FD_SET ( in_client->writer_socket, &in_client->writer_data );
      
      return true;
    }
  }
  return false;
}

bool grab_data(struct client *in_client, char *buffer, int size, char *cr)
{
  if(strcmp(cr, "command") == 0){
    if( FD_ISSET(in_client->command_socket, &in_client->command_data) )
    {
      in_client->address_length = recv ( in_client->command_socket, buffer, size, 0 );    
      return true;
    }
  }
  if(strcmp(cr, "write") == 0){

    if ( FD_ISSET(in_client->writer_socket, &in_client->writer_data) )
    {
      in_client->address_length = recv ( in_client->writer_socket, buffer, size, 0 );    
      return true;
    }
  }
  if(strcmp(cr, "read") == 0){
    if ( FD_ISSET(in_client->reader_socket, &in_client->reader_data) )
    {
      in_client->address_length = recv ( in_client->reader_socket, buffer, size, 0 );    
      return true;
    }
  }
  return false;
}

bool create_chatroom(char *crName)
{
  int k=0;
  for(k; k<MAX_CHATROOMS; k++)
  {
    if(!chatrooms[k].active)
    {
      sprintf(chatrooms[k].name, "%s", crName);
      chatrooms[k].active = true;
      return true;
    }
  }
  return false;
}

bool connect_to_chatroom(struct client *inClient, struct chatroom *cr)
{
  int k =0;
  //test to make sure the user isnt alread subs
  for( k; k<MAX_CLIENTS; k++)
  {
    if(cr->current_clients[k] != NULL && strcmp(cr->current_clients[k]->username, inClient->username) == 0) 
    {
      printf("User %s is already subscribed to %s chatroom.\n", inClient->username, cr->name);
      return false;
    }    
  }

  k=0;
  for( k; k<MAX_CLIENTS; k++)
  {
    if(cr->current_clients[k] == NULL)
    {
      cr->current_clients[k] = inClient;
      printf("%s has subsribed to chatroom %s.\n", inClient->username, cr->name);
      return true;
    }
  }
  printf("User %s cannot connect to chatroom: %s chatroom is full.\n", cr->name, cr->name);
  return false;
}

bool disconnect_from_chatroom(struct client *inClient, struct chatroom *cr)
{
  int k =0;
 
  for( k; k<MAX_CLIENTS; k++)
  {
    if(strcmp(cr->current_clients[k]->username, inClient->username) == 0)
    {
      cr->current_clients[k] = NULL;
      printf("%s has unsubscribed from chatroom %s.\n", inClient->username, cr->name);
      return true;
    }
  }
  printf("User %s is not subscribed to chatroom %s.\n", inClient->username, cr->name);
  return false;
}


bool search_join(struct client inClient, char *name)
{
  int k=0;
  for(k; k<MAX_CHATROOMS; k++)
  {
    if(chatrooms[k].active && (strcmp(chatrooms[k].name, name)==0))
    {
      if(connect_to_chatroom(&clients[0], &chatrooms[k]))
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }
  printf("User %s has requested to join a chatroom that does not exist.\n", clients[0].username);
  return false;
}

bool search_leave(struct client inClient, char *name)
{
  int k=0;
  for(k; k<MAX_CHATROOMS; k++)
  {
    if(chatrooms[k].active && (strcmp(chatrooms[k].name, name)==0))
    {
      if(disconnect_from_chatroom(&clients[0], &chatrooms[k]))
      {
        return true;
      }
      else
      {
        return false;
      }
    }
  }
  return false;
}  

bool check_default(struct client inClient, char *crName)
{
  int k=0;
  for(k; k<MAX_CHATROOMS; k++)
  {   
    if(strcmp(crName, inClient.default_chatroom->name)==0)
    {
      printf("Chatroom %s is the default chatroom for user %s.\n", crName, inClient.username );
      return true;
    }
  }
  return false;
}

bool check_subscribed(struct client *inClient, struct chatroom *cr)
{
  int i=0;
  for (i = 0; i < MAX_CLIENTS; ++i)
  { 
    if(cr->current_clients[i] != NULL)
    {
      if(strcmp(cr->current_clients[i]->username, inClient->username)==0)
      {
        return true;
      }
    }
  }
  printf("User %s is not subscribed to chatroom %s.\n", inClient->username, cr->name);
  return false;
}

bool change_defualt(struct client *inClient, char *crName)
{

  int k=0;
  for( k; k<MAX_CHATROOMS; k++)
  {
    if(chatrooms[k].active && (strcmp(chatrooms[k].name, crName)==0))
    {
      if(check_subscribed(inClient, &chatrooms[k]))
      {  
        int j=0;
        for(j; j<MAX_CLIENTS; j++)
        {
          if(chatrooms[k].current_clients[j] == NULL)
          { 
            printf("User %s has changed the his default chatroom to %s\n", inClient->username, crName );
            inClient->default_chatroom = &chatrooms[k];
            return true;
          }
        }
      }
    }
  }
  printf("Either the chatroom %s does not exist or there is the max amount of clients.\n", crName);
  return false;
}

void *command_process(void *cFD)
{
  int clientSD = (int*)cFD;
  int Res = 0;
  clients[0].default_chatroom = &chatrooms[0];
  Res = send(clientSD, "1\0", 2, 0);    
  while(1)
  {
    char buffer[BUFFER_SIZE];
    char command[4];
    char arguments[BUFFER_SIZE-4];

    sprintf(buffer, "%s", "");
    sprintf(command, "%s", "");
    sprintf(arguments, "%s", "");

    while( !grab_data(&clients[0], buffer, BUFFER_SIZE, "command") );

    sscanf(buffer, "%s %s", command, arguments);
    printf("command: %s\n", command);
    printf("arguments: %s\n", arguments);
    
    if(strcmp(command, "lscr") == 0)
    {
        printf("User %s has requested a list of chatrooms.\n", clients[0].username);
        char list[BUFFER_SIZE];
        sprintf(list, "%s", "");
        int k=0;
        for(k; k<MAX_CHATROOMS; k++)
        {
          if(chatrooms[k].active)
          {
            sprintf(list, "%s %s", list, chatrooms[k].name);  
          }          
        }
        sprintf(list, "%s\0", list, chatrooms[k].name);  

        Res = send(clientSD, list, strlen(list)+1, 0);
    }
    else if (strcmp(command, "crea") == 0)
    {
      printf("User %s is requesting a new chatroom.\n", clients[0].username);
      char crName[20];
      sprintf(crName, "%s", arguments);
      if (create_chatroom(crName))
      {
        Res = send(clientSD, "1\0", 2, 0);   
      }
      else
      {
        Res = send(clientSD, "0\0", 2, 0);
        printf("Could not create chatroom.\n");
      }
    }
    else if (strcmp(command, "lssu") == 0)
    {
      char list[BUFFER_SIZE];
      int k=0;
      sprintf(list, "%s", "");
      for( k; k<MAX_CHATROOMS; k++)
      {
        if(chatrooms[k].active)
        {
          int j=0;
          for(j; j<MAX_CLIENTS; j++)
          {
            if(chatrooms[k].current_clients[j] != NULL)
            {
              if(strcmp(chatrooms[k].current_clients[j]->username, clients[0].username) == 0 )
              {
                sprintf(list, "%s %s", list, chatrooms[k].name);
              }
            }
          }
        }
      }
      sprintf(list, "%s\0", list);          
      printf("User %s is subsribed to the following chatrooms: %s\n", clients[0].username, list);
      Res = send(clientSD, list, strlen(list)+1, 0);
    }
    else if (strcmp(command, "subs") == 0)
    {
      char crName[20];
      sprintf(crName, "%s", arguments);

      printf("User %s is requesting to join chatroom %s.\n", clients[0].username, crName);
      
      if(search_join(clients[0], crName))
      {
        Res = send(clientSD, "1\0", 2, 0);
      } 
      else
      {
        Res = send(clientSD, "0\0", 2, 0);
      }
    }
    else if (strcmp(command, "unsu") == 0)
    {
      char crName[20];
      sprintf(crName, "%s", arguments);

      printf("User %s is unsubscribing to chatroom %s.\n", clients[0].username, crName);
      
      
      printf("%s %s\n", crName, clients[0].default_chatroom->name);

      if(search_leave(clients[0], crName) && strcmp(crName, clients[0].default_chatroom->name)!=0)
      {
        Res = send(clientSD, "1\0", 2, 0);
      } 
      else
      {
        Res = send(clientSD, "0\0", 2, 0);
      }
    }
    else if (strcmp(command, "defa") == 0)
    {
      char crName[20];
      sprintf(crName, "%s", arguments);
     
      printf("User %s is attempting to change the default chatroom to %s.\n", clients[0].username, arguments);

      if(change_defualt(&clients[0], crName))
      {
        Res = send(clientSD, "1\0", 2, 0);
      }
      else
      {
        Res = send(clientSD, "0\0", 2, 0);
      }
    }    
  } 
}


void* writer_process(void* cFD)
{
  int clientSD = (int*)cFD;
  int Res = 0;
  send(clients[0].writer_socket, "1\0", 2, 0);
  while(1)
  {
    char buffer[BUFFER_SIZE];
    while( !grab_data(&clients[0], clients[0].message, BUFFER_SIZE, "write") );
    printf("User %s has posted to chatroom %s: %s\n", clients[0].username, clients[0].default_chatroom, clients[0].message);
    pthread_mutex_lock(&readerMutex);
    clients[0].has_message = true;
    pthread_cond_signal(&readerCond);
    pthread_mutex_unlock(&readerMutex); 
  }
}

void* reader_process(void* cFD)
{
  int clientSD = (int*)cFD;
  int Res = 0;
  send(clients[0].reader_socket, "1\0", 2, 0);
  while(1)
  {
    pthread_mutex_lock( &readerMutex );
    if (clients[0].has_message)
    {
      int k = 0;
      for( k; k<MAX_CLIENTS; k++)
      {
        if(clients[0].default_chatroom->current_clients[k] != NULL)
        {
          char message[BUFFER_SIZE];
          sprintf(message, "%s %s %s\0", clients[0].username, clients[0].default_chatroom->name, clients[0].message);
          Res = send(clients[0].default_chatroom->current_clients[k]->reader_socket, message, strlen(message)+1, 0);
          printf("Recieved message: %s\nsize: %i\n", clients[0].message, Res);
        }
      }
      clients[0].has_message = false;
    }
    else
    {
      pthread_cond_wait( &readerCond, &readerMutex);
    }
  pthread_mutex_unlock( &readerMutex );
  }

}


int main(int argc, char *argv[])
{
  if(argc < 2)
  {
    printf("Error: please enter a port number.\n");
    return 0;
  }
  struct thread_info *myThreads;
  myThreads = calloc(NUM_PTHREADS, sizeof(struct thread_info));

  start_server(atoi(argv[1]));

 
  //pthread_create(&myThreads[0s.thread_id, NULL, accept_connection, SocketFD);
  accept_client(&clients[0], "command");
  accept_client(&clients[0], "write");
  accept_client(&clients[0], "read");

  if( clients[0].command_socket != 0 && clients[0].writer_socket != 0  && clients[0].reader_socket != 0)
  {
    char buffer[BUFFER_SIZE];
    char command[25];
    char username[10];
    while( !grab_data(&clients[0], buffer, BUFFER_SIZE, "command") );
    sscanf(buffer, "%s %s", command, username);
    
    sprintf(clients[0].username, "%s", username);
    printf("Username has been set to: %s\n", clients[0].username);
    connect_to_chatroom(&clients[0], &chatrooms[0]);
    
    if(strcmp(command, "join") == 0)
    {      
        pthread_create(&myThreads[0].thread_id, NULL, command_process, (void*)clients[0].command_socket);       
    }
    
    sprintf(command, "%s", "");

    while( !grab_data(&clients[0], buffer, 100, "write") );
    sscanf(buffer, "%s %s", command, username);
    if(strcmp(command, "writ") == 0)
    {
      if(strcmp(clients[0].username, username)==0)
      {
        pthread_create(&myThreads[1].thread_id, NULL, writer_process, (void*)clients[0].writer_socket); 
      }
    }

    sprintf(command, "%s", "");
    while( !grab_data(&clients[0], buffer, 100, "read") );
    sscanf(buffer, "%s %s", command, username);
    if(strcmp(command, "read") == 0)
    {
      if(strcmp(clients[0].username, username)==0)
      {
        pthread_create(&myThreads[2].thread_id, NULL, reader_process, (void*)clients[0].reader_socket); 
      }
    }

  }
  else
  {
    printf("Error: not all sockets connected.\n"); 
  }
  
  while(1);
  close(SocketFD); 
  return EXIT_SUCCESS;  
}
