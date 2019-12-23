// Rediet Negash
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

#define MIN(left,right) ((left<right) ? left : right)
#define MAX(left,right) ((left>right) ? left : right)
#define N 3
typedef struct{
  int id;
  sem_t *left;
  sem_t *right;
}Philosopher;


void *thread_func(void *vargp){
  Philosopher *p=(Philosopher*)vargp;
  int i;
  for(i=0; i<100; i++){
    fprintf(stderr,"%d: thinking\n", p->id);
    fprintf(stderr,"%d: getting left\n",p->id);
   
    sem_wait(MIN(p->left,p->right));
    fprintf(stderr,"%d: getting right\n",p->id);
  
    sem_wait(MAX(p->left,p->right));
    fprintf(stderr,"%d: eating\n",p->id);
    fprintf(stderr,"%d: putting left\n",p->id);
    sem_post(p->left);
    fprintf(stderr,"%d: putting right\n",p->id);
    sem_post(p->right);
  }
}
int main(){
  pthread_t tid[N];
  sem_t stick[N];
  Philosopher p[N];
  int i;
  for(i=0;i<N;i++){
    sem_init(stick+i, 0,1);
    p[i].id=i;
    p[i].left= &stick[i%N];
    p[i].right= &stick[(i+1)%N];
  }
  for(i=0;i<N;i++){
    pthread_create(&tid[i],NULL,thread_func,&p[i]);
  }
  for(i=0;i<N;i++){
    pthread_join(tid[i],NULL);
  }
  for(i=0;i<N;i++){
    sem_destroy(stick+i);
  }
  return 0;
}
