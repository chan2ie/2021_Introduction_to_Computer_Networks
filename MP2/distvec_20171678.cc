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

typedef struct _entry{
  int next;
  int cost;
} entry;

typedef struct _node{
  int update;
  struct _entry table[MAXNODESIZE];  
  int path[MAXNODESIZE];
} node;

int numtot;
node routers[MAXNODESIZE];

void printRoutingTables(FILE*);
void sendMessages(FILE *, FILE *);
void updateChanges(FILE *, FILE *, FILE *);

void printPath(){
  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      fprintf(stdout, "%d %d %d\n", i, j, routers[i].path[j]);
    }
    fprintf(stdout, "\n");
  }
}

void readTopology(FILE *fp){
  int start, dest, cost;

  fscanf(fp, "%d\n", &numtot);
  
  // init routing tables
  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      routers[i].table[j].next = -1;
      routers[i].table[j].cost = -999;

      routers[i].path[j] = -999;
    }
    routers[i].table[i].next = i;
    routers[i].table[i].cost = 0;

    routers[i].update = 1;
    routers[i].path[i] = 0;
  }

  while(1){
    if(EOF == fscanf(fp, "%d %d %d\n", &start, &dest, &cost)) break;
    
    routers[start].path[dest] = cost;
    routers[dest].path[start] = cost;
  }
  

  return ;
}

void sendMessages(FILE *fp, FILE* op){
  char message[MAXDATASIZE];
  int start, dest, current;

  while(1){
    if(EOF == fscanf(fp, "%d %d ", &start, &dest)) break;
    fgets(message, sizeof(message), fp);
    
    current = start;
    if (routers[start].table[dest].cost == -999){
      fprintf(op, "from %d to %d cost infinite hops unreachable message %s", start, dest, message);
      continue;
    }
    fprintf(op, "from %d to %d cost %d hops ", start, dest, routers[start].table[dest].cost);
    while(current!=dest){
      fprintf(op, "%d ", current);
      current = routers[current].table[dest].next;
    }
    fprintf(op, "message %s", message);
  }

  fprintf(op, "\n");
}

void sendAd (int sender, int reciever){
  int cost = routers[sender].path[reciever];

  for(int dest = 0; dest < numtot; dest++){
    if(routers[sender].table[dest].cost >= 0 &&
       routers[sender].table[dest].next != reciever &&
       (routers[sender].table[dest].cost + cost < routers[reciever].table[dest].cost ||
        (routers[sender].table[dest].cost + cost == routers[reciever].table[dest].cost && sender < routers[reciever].table[dest].next) ||
        routers[reciever].table[dest].cost < 0)){
      routers[reciever].table[dest].cost = routers[sender].table[dest].cost + cost;
      routers[reciever].table[dest].next = sender;
      routers[reciever].update = 1;
    }
  }

  return;
}

void updateTables(){
  int update = 1;
  while(update != 0){
    update = 0;

    for(int sender = 0; sender < numtot; sender++){
      for(int reciever = 0; reciever < numtot; reciever++){
        if(routers[sender].update == 1 && routers[sender].path[reciever] > 0){
          sendAd(sender, reciever);
        }
      }
      routers[sender].update = 0;
    }

    for(int i = 0; i < numtot; i++){
      update += routers[i].update;
    }
  }
}

void updateChanges(FILE * fp, FILE *mp, FILE *op){
  int start, dest, cost, current;
  while(1){
    if(EOF == fscanf(fp, "%d %d %d\n", &start, &dest, &cost)) break;
    
    routers[start].path[dest] = cost;
    routers[dest].path[start] = cost;

    for(int i = 0; i < numtot; i++){
      for(int j = 0; j < numtot; j++){
        if((routers[i].table[j].next == start || routers[i].table[j].next == dest) && i != j){
          routers[i].table[j].cost = -999;
        }
        else{
            current = i;
            while(current!=j){
              if((current == start && routers[current].table[j].next == dest) ||
                 (current == dest && routers[current].table[j].next == start)){
                  routers[i].table[j].cost = -999;
                  break;
              }
              current = routers[current].table[j].next;
            }
        }
      }
      routers[i].update = 1;
    }

    updateTables();
    printRoutingTables(op);
    
    rewind(mp);
    sendMessages(mp, op);
  }
}

void printRoutingTables(FILE *fp){
  for(int i = 0; i < numtot; i++){
    for(int j = 0; j < numtot; j++){
      if(routers[i].table[j].cost == -999) continue;
      fprintf(fp, "%d %d %d\n", j, routers[i].table[j].next, routers[i].table[j].cost);
    }
    fprintf(fp, "\n");
  }
}

int main (int argc, char *argv[]){
  
  FILE *topologyfile, *messagefile, *changesfile, *outputfile;

  if(argc != 4){
    fprintf(stderr, "usage: distvec topologyfile messagesfile changesfile\n");
    exit(1);
  }

  // File Open
  topologyfile = fopen(argv[1], "r");
  messagefile = fopen(argv[2], "r");
  changesfile = fopen(argv[3], "r");
  outputfile = fopen("output_dv.txt", "w");

  if(!(topologyfile && messagefile && changesfile)){
    fprintf(stderr, "Error: open input file.\n");
    exit(1);
  }

  readTopology(topologyfile);

  updateTables();
  printRoutingTables(outputfile);
  
  sendMessages(messagefile, outputfile);
  
  updateChanges(changesfile, messagefile, outputfile);

  fprintf(stdout, "Complete. Output file written to output_dv.txt.\n");

  fclose(topologyfile);
  fclose(messagefile);
  fclose(changesfile);
  fclose(outputfile);

  return 0;
}
