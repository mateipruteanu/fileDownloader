#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>


int sd;                    
struct sockaddr_in server; 
char fileName[100] = "";
void *serverOutput;
char cFILESIZE[10000] = "";
long long FILESIZE = 0;

void initialSetup(int argc, char *argv[]);
void getFileName();
void sendRequest();
bool readData(int sock, void *buf, int buflen);
bool readFile(int sock, FILE *f);

int main(int argc, char *argv[])
{
  initialSetup(argc, argv);

  getFileName();
  sendRequest();

  FILE *copiedFile = fopen(fileName, "wb+");
  if(readFile(sd, copiedFile) == false)
    printf("Removal status: %d\n", remove(fileName));

  fclose(copiedFile);
  free(serverOutput);
}

bool readData(int sock, void *buf, int buflen) {
  unsigned char *pbuf = (unsigned char *)buf;

  while (buflen > 0) {
    int num = recv(sock, pbuf, buflen, 0);
    if (num == 0)
      return false;
    pbuf += num;
    buflen -= num;
  }

  return true;
}

bool readFile(int sock, FILE *f) {
  long long filesize = 0;
  /// getting the size of the file from server
  if(!readData(sock, &filesize, sizeof(filesize)))
      return false;
  filesize = ntohl(filesize);


  if (filesize > 0) {
    printf("Receiving file of size %lld\n", filesize);
    char buffer[1024];
    do {
      long long MAX_SIZE;
      if(filesize <= sizeof(buffer))
        MAX_SIZE = filesize;
      else
        MAX_SIZE = sizeof(buffer);

      if (!readData(sock, buffer, MAX_SIZE))
        {
          printf("HERE\n");
          return false;
        }
      int offset = 0;
      do {
        /// writing into the file until we write current buffer
        int written = fwrite(&buffer[offset], 1, MAX_SIZE - offset, f);
        if (written < 1)
          return false;
        offset += written;
      } while (offset < MAX_SIZE);
      filesize -= MAX_SIZE;
    } while (filesize > 0);
    printf("File received!\n");
    return true;
  }
  else
    return false;
}


void sendRequest() {
  if (write(sd, fileName, sizeof(fileName)) <= 0) {
    perror("[client] Eroare la write() spre server.\n");
    exit(2);
  }
  printf("[client] Send request to server. \n");
}

void getFileName() {
  printf("[client] Introduceti numele fisierului: ");
  fgets(fileName, sizeof(fileName), stdin);
  fileName[strlen(fileName) - 1] = '\0';
}

void initialSetup(int argc, char *argv[]) {
  if (argc != 3)
  {
    printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
    exit(3);
  }

  int port = atoi(argv[2]);
  printf("Received port %d\n", port);

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Eroare la socket().\n");
    exit(4);
  }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons(port);

  printf("ipadress: %d\n", server.sin_addr.s_addr);

  printf("[client] Connecting to server...\n");
  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[client]Eroare la connect().\n");
    exit(5);
  }
  printf("[client] Connected to server.\n");
}

