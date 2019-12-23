//Rediet Negash
// 111820799
#define _GNU_SOURCE
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define EMPTY    '.'
#define EQU3(a, b, c) ((a) == (b) && (a) == (c) && (a) != EMPTY)
#define OTHER(player) ((player)=='X' ? 'O' : 'X')


int hasWinner(char *board){
    char (*b)[3]  = (char(*)[3])board;
    int i;
    for (i =0; i < 3; i++){
        if(EQU3(b[i][0], b[i][1], b[i][2]) || EQU3(b[0][i], b[1][i], b[2][i])){
            return 1;
        }
        
    }return EQU3(b[0][0], b[1][1], b[2][2]) || EQU3(b[0][2], b[1][1], b[2][0]);
}

int simulatePlay(char player, char *board, int *index){
  int i,win=0;
  for(i=0;i<9 && !win; i++){
    if(board[i]== EMPTY){
      board[i]=player;
      win = hasWinner(board);
      board[i] = EMPTY;
      *index=i;  //wining place or the last empty place
    }
  }
  return win;
}

int mark(char player, char *board){
  int i=-1;
  if(!simulatePlay(OTHER(player), board, &i))
    simulatePlay(player,board,&i);
  if(i>=0)
    board[i]= player;
  return i>=0; //true if marked
}

typedef struct{
    char *player;
    char *board;
    sem_t *my_turn, *your_turn;
    
}player_t;

void print_board(char player, char *board){
    
    printf("%c: %lx\n", player, pthread_self());
    printf("%c %c %c\n", board[0], board[1], board[2] );
    printf("%c %c %c\n", board[3], board[4], board[5] );
    printf("%c %c %c\n", board[6], board[7], board[8] );
    
}


void* game(void* vargs){
    player_t* p = (player_t*)vargs;
    while(1){
        sem_wait(p->my_turn);
        if(hasWinner(p->board)){
	    sem_post(p->your_turn);
            break;
        }
	//TODO: play, print, and wake up the other
	mark(p->player[0], p->board);
	print_board(p->player[0], p->board);
	sem_post(p->your_turn);
    }
    pthread_exit(NULL);
}

int main(){
    pthread_t tidx, tido;
    sem_t turn_x, turn_o;
    char board[] = ".........";
    // initialize player x and o below
    /*initialize player x and y*/
    player_t x = {"X", board, &turn_x, &turn_o};
    player_t o = {"O", board, &turn_o, &turn_x};
    // initialize the sempahores turn_x and turn_o
    sem_init(&turn_x, 0, 1);
    sem_init(&turn_o, 0, 0);
    // create two threads for player x and player o 
    pthread_create(&tidx, NULL, game, &x);
    pthread_create(&tido, NULL, game, &o);
    // wait for threads to terminate
    pthread_join(tidx, NULL);
    pthread_join(tido, NULL);
        
    // TODO: destroy turn_x and turn_o
    sem_destroy(&turn_o);
    sem_destroy(&turn_x);
    return 0;
    
}
