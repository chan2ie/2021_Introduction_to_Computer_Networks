#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1000

int strlwr(char*);

int main (int argc, char *argv[]){
  bool usage_error = false;
  
  //detect usage error
  if(argc != 2){
      usage_error = !usage_error;
  }
  else{
    char temp[8];
    strncpy(temp, argv[1], 7);
    temp[8] = '\0';

    if (strcmp(temp, "http://") != 0){
      usage_error = !usage_error;
    }
  }
  if (usage_error){
    fprintf(stderr, "usage: http_client http://hostname[:port]['path/to/file]\n");
    exit(1);
  }

  //Parsing
  char hostname[MAXDATASIZE];
  char port[50] = "80";
  char path[MAXDATASIZE];
  char* port_start, *path_start;

  strcpy(hostname, &(argv[1][7]));
  path_start = strchr(hostname, '/');

  if (path_start == NULL){
    strcpy(path, "/");
  }
  else{
    int start_idx = path_start - hostname;
    strcpy(path, &(hostname[start_idx]));
    hostname[start_idx] = '\0';
  }

  port_start = strchr(hostname, ':');
  
  if (port_start != NULL){
    int start_idx = port_start - hostname;

    strcpy(port, &(hostname[start_idx + 1]));
    hostname[start_idx] = '\0';
  }

  int sockfd, numbytes, writtenbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo;
  int rv;
  char s[INET_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if((rv = getaddrinfo(hostname, port, &hints, &servinfo))!=0){
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }
  
  // create socket
  if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1){
    perror("client: socket");
    exit(1);
  }

  // connect to host
  if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
    close(sockfd);
    perror("connect");
    exit(1);
  }

  freeaddrinfo(servinfo);

  // create http request mesage
  strcpy(buf, "GET ");
  strcpy(&(buf[strlen(buf)]), path);
  strcpy(&(buf[strlen(buf)]), " HTTP/1.1\r\n");

  strcpy(&(buf[strlen(buf)]), "Host: ");
  strcpy(&(buf[strlen(buf)]), hostname);
  strcpy(&(buf[strlen(buf)]), ":");
  strcpy(&(buf[strlen(buf)]), port);
  strcpy(&(buf[strlen(buf)]), "\r\n\r\n");

  // send request
  if(send(sockfd, buf, strlen(buf), 0) == -1){
    perror("send");
    close(sockfd);
    exit(1);
  }

  // recieve first response
  if((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
    perror("send");
    close(sockfd);
    exit(1);
  }
  buf[numbytes] = '\0';
  // print first line of the header
  char first_line[200];

  strncpy(first_line, buf, strstr(buf, "\r\n") + 2 - buf);
  printf("%s", first_line);

  // collect informations from header
  char buf_lwr[MAXDATASIZE];
  strcpy(buf_lwr, buf);
  strlwr(buf_lwr);
  char* c_len_start = strstr(buf_lwr, "content-length");

  if (c_len_start == NULL){
    printf("Content-Length not specified.\n");
    exit(1);
  }
  
  char* c_len_end = strstr(c_len_start, "\r\n");
  char c_len_str[50];

  strncpy(c_len_str, &(c_len_start[16]), c_len_end - c_len_start - 16); 
  int content_length =  strtol(c_len_str, NULL, 10);
  char* c_end = strstr(buf, "\r\n\r\n");

  FILE *fp = fopen("20171678.out", "w");
  fprintf(fp, "%s", &(c_end[4]));

  writtenbytes = numbytes - (c_end + 4 - buf);

  // if content left, recv
  while(writtenbytes < content_length){
    if((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
      perror("send");
      close(sockfd);
      exit(1);
    } 
    buf[numbytes] = '\0';
    fprintf(fp, "%s", buf);
    writtenbytes += numbytes;
  }

  printf("%d bytes written to 20171678.out\n", content_length);

  // close file pointer and connection
  fclose(fp);
  close(sockfd);

  return 0;
}

int strlwr(char *str){
  for(int i = 0 ; i < strlen(str);i++){
    if(*(str+i) >= 'A' && *(str+i) <= 'Z'){
      *(str + i) -= 'A' - 'a';
    }
  }
}
