typedef int bool;
#define true 1
#define false 0

int connect_to_server(char* ip, int port, int* res)
{
  
  struct sockaddr_in stSockAddr;
//  int res;
  int socketFD;
  socketFD =   socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); 
  if (-1 == socketFD)
  {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }
    
  memset(&stSockAddr, 0, sizeof(stSockAddr));
    
  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(port);
  *res = inet_pton(AF_INET, ip, &stSockAddr.sin_addr);
  
  if (0 > res)
  {
    perror("error: first parameter is not a valid address family");
    close(socketFD);
    exit(EXIT_FAILURE);
  }
  else if (0 == res)
  {
    perror("char string (second parameter does not contain valid ipaddress)");
    close(socketFD);
    exit(EXIT_FAILURE);
  }
    
  if (-1 == connect(socketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
  {
    perror("connect failed");
    close(socketFD);
    exit(EXIT_FAILURE);
  }

  return socketFD;
}