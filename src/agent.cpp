/*********************************************
 *  agent.c
 *  UNSW Session 1, 2015
*/

#include <string.h>
#include <cstdio>
#include <cstdlib>

#include "pipe.h"

#define map_size 80
#define world_size map_size*2

int   pipe_fd;
FILE* in_stream;
FILE* out_stream;

char view[5][5];

char world[world_size][world_size];
char lastMove;
int posX;
int posY;
int heading;

void transpose(char (&tran)[5][5]){
    int i,j;
    char swap;
    for(i=0;i<5;i++){
        for(j=0;j<5;j++){
            if(i<j){
                swap = tran[i][j];
                tran[i][j] = tran[j][i];
                tran[j][i] = swap;
            }
        }
    }
}
void revRows(char (&rev)[5][5]){
    int i,start,end;
    char swap;
    for(i=0;i<5;i++){
        start = 0;
        end = 4;
        while(start<end){
            swap = rev[i][start];
            rev[i][start] = rev[i][end];
            rev[i][end] = swap;
            start++;
            end--;
        }
    }
}

void revCols(char (&rev)[5][5]){
    int i,start,end;
    char swap;
    for(i=0;i<5;i++){
        start = 0;
        end = 4;
        while(start<end){
            swap = rev[start][i];
            rev[start][i] = rev[end][i];
            rev[end][i] = swap;
            start++;
            end--;
        }
    }
}

void rotCW(char (&rot)[5][5]){
    transpose(rot);
    revRows(rot);
}

void rotCC(char (&rot)[5][5]){
    transpose(rot);
    revCols(rot);
}

void rotHalf(char (&rot)[5][5]){
    revCols(rot);
    revRows(rot);
}

void print_view(char (&dis)[5][5])
{
  int i,j;

  printf("\n+-----+\n");
  for( i=0; i < 5; i++ ) {
    putchar('|');
    for( j=0; j < 5; j++ ) {
        putchar( dis[i][j] );
    }
    printf("|\n");
  }
  printf("+-----+\n");
}

void print_map()
{
  int i,j;

  for( i=0; i < world_size; i++ ) {
    putchar('|');
    for( j=0; j < world_size; j++ ) {
      if(( i == posX )&&( j == posY )) {
        putchar( 'X' );
      }
      else {
        putchar( view[i][j] );
      }
    }
    printf("|\n");
  }
}

char get_action( char view[5][5] ) {
    char copy[5][5];
    memcpy(copy,view,sizeof (char)*5*5);
    switch(heading){
        case 1:
            rotCW(copy);
            copy[2][2] = '>';
            break;
        case 2:
            rotHalf(copy);
            copy[2][2] = 'v';
            break;
        case 3:
            rotCC(copy);
            copy[2][2] = '<';
            break;
        default:
            copy[2][2] = '^';
            break;
    }

    print_view(copy);
    /*
    int i,j;
    for(i = 0; i<5; i++){
        for(j = 0; j<5; j++){
            if(posX+i-2 >= world_size ||
                posX+i-2 < 0 ||
                posY+j-2 >= world_size ||
                posY+j-2 < 0 ){
                continue;
            }
            world[posX+i-2][posY+j-2] = copy[i][j];
        }
    }
    */

    if (view[1][2] == '~' || view[1][2] == '*'|| view[1][2] == 'T'){
        lastMove = 'r';
        heading = (heading+1)%4;
    }
    else{
        lastMove = 'f';
    }
    return lastMove;
}



int main( int argc, char *argv[] )
{
  char action;
  int sd;
  int ch;
  int i,j;
  lastMove = '?';
  posX = map_size;
  posY = map_size;
  heading = 0;

  if ( argc < 3 ) {
    printf("Usage: %s -p port\n", argv[0] );
    exit(1);
  }

    // open socket to Game Engine
  sd = tcpopen(atoi( argv[2] ));

  pipe_fd    = sd;
  in_stream  = fdopen(sd,"r");
  out_stream = fdopen(sd,"w");

  for(i=0; i<world_size;i++){
    for(j=0; j<world_size;j++){
        world[i][j] = '?';
    }
  }
  
  while(1) {
      // scan 5-by-5 wintow around current location
    for( i=0; i < 5; i++ ) {
      for( j=0; j < 5; j++ ) {
        if( !(( i == 2 )&&( j == 2 ))) {
          ch = getc( in_stream );
          if( ch == -1 ) {
            exit(1);
          }
          view[i][j] = ch;
        }
      }
    }

    action = get_action( view );
    putc( action, out_stream );
    fflush( out_stream );
  }

  return 0;
}
