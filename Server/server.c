#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <arpa/inet.h>

//#define BYTESIZE 3000000
#define PORT 2040

struct sockaddr_in server; // structura folosita de server
struct sockaddr_in from;
int sd;                  // descriptorul de socket
int client;
long long FILESIZE = -1;

void initialSetup();
void receiveRequest(char *fileName);
void sendFail();
bool searchForFile(char *fileName);
bool sendData(int sock, void *buf, int buflen);
bool sendFile(int sock, FILE *source);

int main() {
  initialSetup();

  char *fileName = "";
  FILE *source;

  while(1) {
    fileName = (char*)malloc(100*sizeof(char));
    receiveRequest(fileName);

    source = fopen(fileName, "rb");

    if(searchForFile(fileName)) {
      sendFile(client, source);
    }
    else {
      sendFail();
    }

    close(client);
    fclose(source);
    free(fileName);
  }
}



bool sendData(int sock, void *buf, int buflen)
{
  unsigned char *pbuf = (unsigned char *)buf;

  while (buflen > 0)
  {
    int num = write(sock, pbuf, buflen);
    if(num < 0)
      perror("[sendData] num");
    pbuf += num;
    buflen -= num;
  }

  return true;
}

bool sendFile(int sock, FILE *source)
{

  if (FILESIZE == EOF)
    return false;

  long long nValue = htonl(FILESIZE);
  if (!sendData(sock, &nValue, sizeof(nValue)))
    return false;


  printf("[server] Sending file of size %lld\n", FILESIZE);
  if (FILESIZE > 0) {
    char buffer[1024];
    do {
      long long MAX_SIZE;
      if (FILESIZE <= sizeof(buffer))
        MAX_SIZE = FILESIZE;
      else
        MAX_SIZE = sizeof(buffer);
      /// reading the bytes from source file up until MAX_SIZE
      long long bytes_read = fread(buffer, 1, MAX_SIZE, source);
      if (bytes_read < 1)
        return false;
      if (!sendData(sock, buffer, bytes_read))
        return false;
      FILESIZE -= bytes_read;
    } while (FILESIZE > 0);
  }
  printf("[server] File sent!\n");
  return true;
}

bool searchForFile(char *fileName) {
  DIR *folder;
  struct dirent *entry;
  struct stat st;
  int files = 0;

  folder = opendir("./");
  if (folder == NULL) {
    perror("Unable to read directory");
    return false;
  }

  while ((entry = readdir(folder))) {
    files++;
    stat(entry->d_name, &st);
    printf("File %d: %s, with %lld bytes: \n", files, entry->d_name, st.st_size);
    if(strcmp(fileName, entry->d_name) == 0) {
      FILESIZE = st.st_size;
      return true;
    }
    
  }
  closedir(folder);
  return false;
}

void sendFail() {
  long long nValue = htonl(0);
  sendData(client, &nValue, sizeof(nValue));
}

void receiveRequest(char *fileName) {
  unsigned int length = sizeof(from);

  printf("[server] Asteptam la portul %d...\n", PORT);
  fflush(stdout);

  /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
  client = accept(sd, (struct sockaddr *)&from, &length);
  /* eroare la acceptarea conexiunii de la un client */
  if (client < 0) {
    perror("[server] Eroare la accept().\n");
    exit(4);
  }

  /* s-a realizat conexiunea, se astepta mesajul */
  bzero(fileName, 100);
  printf("[server] Asteptam numele fisierului...\n");
  fflush(stdout);

  /* citirea mesajului */
  if (read(client, fileName, 100) <= 0)
  {
    perror("[server] Eroare la read() de la client.\n");
    close(client); /* inchidem conexiunea cu clientul */
    exit(4);        /* continuam sa ascultam */
  }
  printf("[server] fileName = %s\n", fileName);
}

void initialSetup() {
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) // af_inet -> internet, nu local
  {
    perror("[server]Eroare la socket().\n");
    exit(1);
  }

  /* pregatirea structurilor de date */
  bzero(&server, sizeof(server));
  bzero(&from, sizeof(from));

  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  /* utilizam un port utilizator */
  server.sin_port = htons(PORT);

  /* atasam socketul */
  if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
    perror("[server]Eroare la bind().\n");
    exit(2);
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen(sd, 5) == -1) {
    perror("[server]Eroare la listen().\n");
    exit(3);
  }
}
