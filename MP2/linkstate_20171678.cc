#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <limits.h>

#define MAXDATASIZE 1000
#define MAXNODESIZE 100

typedef struct _node {
  int distance[MAXNODESIZE];
  int path[MAXNODESIZE];
  int visited[MAXNODESIZE];
} node;

node routers[MAXNODESIZE];
int paths[MAXNODESIZE][MAXNODESIZE];
int numtot;
int stack[MAXNODESIZE];
int *sp = stack;

void readTopology(FILE *, FILE *);
void sendMessages(FILE *, FILE *);
void updateChanges(FILE *, FILE *, FILE*);
void resetState();
void calcSPT();
void printRoutingTables(FILE*);

void readTopology(FILE *fp, FILE *op){
  int start, dest, cost;

  fscanf(fp, "%d\n", &numtot);

  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      paths[i][j] = -999;
      routers[i].distance[j] = -999;
    }
    paths[i][i] = 0;
  }
  
  while(1){
    if(EOF == fscanf(fp, "%d %d %d\n", &start, &dest, &cost)) break;
    
    paths[start][dest] = cost;
    paths[dest][start] = cost;
  }

  calcSPT();
  printRoutingTables(op);

  return ;
}

void sendMessages(FILE *mp, FILE* op){
  char message[MAXDATASIZE];
  int start, dest, current;

  while(1){
    if(EOF == fscanf(mp, "%d %d ", &start, &dest)) break;
    fgets(message, sizeof(message), mp);
    
    current = dest;
    if (routers[start].distance[dest] == -999){
      fprintf(op, "from %d to %d cost infinite hops unreachable message %s", start, dest, message);
      continue;
    }
    fprintf(op, "from %d to %d cost %d hops ", start, dest, routers[start].distance[dest]);
    
    if(start == dest){
      fprintf(op, "%d ", start);
    }
    else{
      *sp = routers[start].path[current];
        while(routers[start].path[current] != start){
          current = routers[start].path[current];
          sp += sizeof(int);
          *sp = routers[start].path[current];
        }
    
      fprintf(op, "%d ", *sp);
      
      while(sp != stack){
        sp -= sizeof(int);
        fprintf(op, "%d ", *sp);
      }
    }

    fprintf(op, "message %s", message);
  }

  fprintf(op, "\n");
}

void updateChanges(FILE * fp, FILE *mp, FILE *op){
  int start, dest, cost;
  while(1){
    if(EOF == fscanf(fp, "%d %d %d\n", &start, &dest, &cost)) break;
    
    paths[start][dest] = cost;
    paths[dest][start] = cost;

    calcSPT();
    printRoutingTables(op);
    
    rewind(mp);
    sendMessages(mp, op);
  }
}

void resetState(){
  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      routers[i].distance[j] = -999;
      routers[i].visited[j] = 0;
      if(paths[i][j] != -999){
        routers[i].distance[j] = paths[i][j];
        routers[i].path[j] = i;
      }
    }
    routers[i].distance[i] = 0;
  }

}

void calcSPT(){
  resetState();

  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      int min = 101;
      int index = -1;
      for(int k = 0; k < numtot; k++){
        if(routers[i].visited[k] == 1) continue;
        if(routers[i].distance[k] == -999) continue;
        if(min > routers[i].distance[k]){
          min = routers[i].distance[k];
          index = k;
        }
      }
      
      if(index == -1){
        break;
      }

      routers[i].visited[index] = 1;
      for(int k = 0; k < numtot; k++){
        if(routers[i].visited[k] == 1) continue;
        if(paths[index][k] != -999){
          if(routers[i].distance[k] == -999 ||
            (routers[i].distance[k] == routers[i].distance[index] + paths[index][k]  && routers[i].path[k] > index)||
            routers[i].distance[k] > routers[i].distance[index] + paths[index][k]){
            routers[i].distance[k] = routers[i].distance[index] + paths[index][k];
            routers[i].path[k] = index;
          }
        }
      }
      
    }
  }
}

void printRoutingTables(FILE *op){
  int next;
  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      if(routers[i].distance[j] == -999) continue;
      if(i == j) next = i;
      else{
        next = j;
        while(routers[i].path[next] != i){
          next = routers[i].path[next];
        }
      }
      fprintf(op, "%d %d %d\n", j, next, routers[i].distance[j]);
    }
    fprintf(op, "\n");
  }
}

int main (int argc, char *argv[]){
  
  FILE *topologyfile, *messagefile, *changesfile, *outputfile;
   
  if(argc != 4){
    fprintf(stderr, "usage: linkstate topologyfile messagesfile changesfile\n");
    exit(1);
  }

  // File Open
  topologyfile = fopen(argv[1], "r");
  messagefile = fopen(argv[2], "r");
  changesfile = fopen(argv[3], "r");
  outputfile = fopen("output_ls.txt", "w");

  if(!(topologyfile && messagefile && changesfile)){
    fprintf(stderr, "Error: open input file.\n");
    exit(1);
  }
  
  readTopology(topologyfile, outputfile);
  
  sendMessages(messagefile, outputfile);
  
  updateChanges(changesfile, messagefile, outputfile);
  
  fprintf(stdout, "Complete. Output file written to output_ls.txt.\n");

  fclose(topologyfile);
  fclose(messagefile);
  fclose(changesfile);
  fclose(outputfile);
}
