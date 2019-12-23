
// tictactoe.c
// Name Rediet Negash
// 111820799

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define EMPTY         '.'
#define EQU3(a,b,c)   ((a)==(b) && (a)==(c) && (a) != EMPTY)
#define OTHER(player) ((player)=='X' ? 'O' : 'X')

int hasWinner(char *board){
  char (*b)[3] = (char(*)[3])board;//cast 1D array into 2D array
  int i;
  for(i=0; i<3;i++){
    if(EQU3(b[i][0],b[i][1],b[i][2]) || //check ith row
       EQU3(b[0][i],b[1][i],b[2][i]))   //check ith column
       return 1;
  }
  return EQU3(b[0][0],b[1][1],b[2][2])||  //check diagonals
         EQU3(b[0][2],b[1][1],b[2][0]);
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

void print(char player, char *board){
  printf("\n%c: %d\n", player,getpid());
  printf("%c %c %c\n", board[0], board[1], board[2] );
  printf("%c %c %c\n", board[3], board[4], board[5] );
  printf("%c %c %c\n", board[6], board[7], board[8] );
}

void read_board(FILE *fp, char* board){
  fseek(fp, 0, SEEK_SET);
  fscanf(fp,"%s\n", board);
}

void write_board(FILE *fp,char* board) {
  fseek(fp, 0, SEEK_SET);
  fprintf(fp, "%s\n", board);
  fflush(fp);
}

static volatile sig_atomic_t my_turn=0;

//TODO: implements sigusr handler: it sets my_turn to 1

void sigusr_handler(int sig){
	my_turn = 1;
}

int main(){
  char player[100]= "X";
  char board[100] = ".........";
  FILE *fp;
  sigset_t mask,prev;

  //TODO: register sigusr_handler for SIGUSR1
  signal(SIGUSR1, sigusr_handler);
  //TODO: prepare mask and block signal
  sigprocmask(SIGUSR1, &mask, &prev);
 
  
  fp= fopen("tictactoe.txt","w+");
  pid_t pid_other =fork();
  if(pid_other == 0){
    pid_other=getppid();
    player[0]= OTHER(player[0]);
    write_board(fp,board);
    //TODO: send SIGUSR1 signal to the opponent
    kill(pid_other, SIGUSR1);
    
  }
  while(!hasWinner(board)){
    //TODO: while !my_turn, wait for a signal
    while(!my_turn){
	sigsuspend(&prev);
    }
    //TODO: reset my_turn to 0
   my_turn = 0;
    read_board(fp,board);
    if(hasWinner(board))
      break;

    mark(player[0],board);
    write_board(fp,board);
    print(player[0],board);
    //TODO: send SIGUSR1 signal to opponent
     kill(pid_other, SIGUSR1);
    
  }

  if(pid_other != getpid()){
	waitpid(pid_other, NULL,0);
    //TODO: reap the child if this is parent
    
    }
  fclose(fp);
  return 0;
}
