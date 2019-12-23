//Name : Rediet Negash
// ID 111820799

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXLINE 256
#define MAXARGS 16
#define MAXPIDS 16
#define MAXPATH 1024
#define EOL     1
#define EOLSTR  "\x1"
#define ERRQUIT(msg)            (err_quit((msg), __FILE__, __func__, __LINE__))
#define CHKBOOLQUIT(exp, msg)   ((exp) || ERRQUIT(msg))
#define MATCH(tokens, str, msg) (CHKBOOLQUIT(match(tokens, str), msg))
#define IS_WS(chr)              ((chr)==' ' || (chr)=='\t' || (chr)=='\n' || (chr)=='\r')
#define IS_OP(chr)              ((chr)==';' || (chr)=='&'|| (chr)==EOL)

struct TokenBuf {
    char buf[MAXLINE*2];
    char *tokens[MAXLINE];
    int pos;
};
struct Command {
    char *argv[MAXARGS];    //arguments
    int argc;               //# of valid argv
};
struct Path {
    char *dir[MAXPATH];
    int ndir;
};
int err_quit(char *msg, const char *file, const char *func, const int line) {
    printf("Error in function %s (%s, line %d): %s: last err: %s\n", func, file, line, msg, strerror(errno));
    exit(1);
    return 0;
}
char *skip_whitespace(char *str) {
    while(IS_WS(*str))
        str++;
    return str;
}
//break str into token strings
void tokenize(char *str, struct TokenBuf *tb) {
    int count = 0;
    char *buf = tb->buf;
    tb->pos = 0;
    for(str = skip_whitespace(str); *str; ) {
        tb->tokens[count++] = buf;
        if(IS_OP(*str))
            *buf++ = *str++;
        else
            while(*str && !IS_WS(*str) && !IS_OP(*str))
                *buf++ = *str++;
        *buf++ = 0;
        str = skip_whitespace(str);
    }
    tb->tokens[count++] = EOLSTR;
}
int match(struct TokenBuf *tb, char *str) {
    int cmp = (strcmp(tb->tokens[tb->pos], str) == 0);
    tb->pos++;
    return cmp;
}
//build argv array
void parse_cmd(struct TokenBuf *tb, struct Command *cmd) {
    cmd->argc = 0;
    while(!IS_OP(tb->tokens[tb->pos][0])){
     // copy token references from tb->tokens[tb->pos] to cmd->argv[cmd->argc]	
      cmd->argv[cmd->argc]=tb->tokens[tb->pos];
      // increase tb->pos and cmd->argc during the copy
      tb->pos++;
      cmd->argc++;
    }
    // set the last argv to NULL
    cmd->argv[cmd->argc]=NULL;
    //Done: build a command with options
}
int run_internal_cmd(struct Command *cmd) {
    CHKBOOLQUIT(cmd->argc > 0, "empty command");
    if(strcmp(cmd->argv[0], "exit") == 0)
        exit(0);
    else if(strcmp(cmd->argv[0], "cd") == 0) {
        CHKBOOLQUIT(cmd->argc <= 2, "too many params");
        if(cmd->argc == 2)  CHKBOOLQUIT(chdir(cmd->argv[1]) == 0, "cd failed");
        else                CHKBOOLQUIT(chdir(getenv("HOME")) == 0, "cd HOME failed");
        return 1;
    }
    return 0;
}
int run_cmd(struct Command *cmd, struct Path *path) {
    int pid;
    char fullpath[MAXLINE];
    CHKBOOLQUIT(cmd->argc > 0, "empty command");
   // call fork to launch a child process
    pid = fork();
    if(pid==0){
      //  for each dir in path, build a fullpath by appending argv[0] to it.
      for( int i=0 ; i<path->ndir ;i++){
        char *toAppend = path->dir[i];
        snprintf(fullpath, MAXPATH,"%s/%s", toAppend ,cmd->argv[0]);
	// access : checks if the file in the fullpath form is executable
        if(access(fullpath, X_OK) == 0){
           // if a full path is found, update cmd->argv[0] 
            cmd->argv[0]=fullpath;
	    // and execute it using execv;
            execv(fullpath, cmd->argv);
        }
      }
      printf("%s",fullpath);
      printf("%s",cmd->argv[0]);
      // otherwise, exit with an error message
      ERRQUIT("Unexecutable Path");
    }
    else{
        // the parent process simply returns the child process' pid;
        return pid;
    }
    //Done: in a child process, execute argv[0] in its fullpath form.
}
void run_cmd_list(struct TokenBuf *tb, struct Path *path) {
    while(tb->tokens[tb->pos][0] != EOL) {
        struct Command cmd;
        char *token;
        pid_t pids[MAXPIDS];
        int npids = 0;

        parse_cmd(tb, &cmd);
        token = tb->tokens[tb->pos];
        if(token[0] == EOL || token[0] == ';' || token[0] == '&') {
            int status, i;
            if(token[0] == ';') MATCH(tb, ";", "unexpected token");
            if(token[0] == '&') MATCH(tb, "&", "unexpected token");

            if(!run_internal_cmd(&cmd))
                pids[npids++] = run_cmd(&cmd, path);
	    // wait for the foreground processes or
            // print pids for the background processes
            for(i = 0; i < npids; i++) {
	        if(token[0]=='&'){
		 //cmd is a background process. so print its pid
                  printf("bg proc, pid: %d\n",pids[i]);

                }
                else{
		    //cmd is a foreground process so Wait until it exits
                    waitpid(pids[i],&status,0);
                }

                //Done: wait for the foreground processes or
            
            }
        }
        else
            ERRQUIT("unexpected token");
    }
    // if any child processes have been terminated, reap them all.
    pid_t terminate;
    int status;
    while((terminate = waitpid(-1, &status, WNOHANG))>0){
      printf("bg proc, pid: %d, terminated\n", terminate);
    }
    //Done: if any child processes have been terminated, reap them all.
    //1. use waitpid with -1 for the pid and WNOHANG for the option
    //2. print out the terminated processes' pid
}
void eval(char *cmdline, struct Path *path) {
    struct TokenBuf tb;
    tokenize(cmdline, &tb);
    run_cmd_list(&tb, path);
}
//get PATH from the environment and parse it into path
void parse_path(struct Path *path) {
    char *myPath;
    path->ndir=0;
    path->dir[path->ndir++]=".";
    if(getenv("PATH")!=NULL){
      // if it is not NULL, copy it using strdup
      myPath=strdup(getenv("PATH"));
      // split the copy delimited by ':' characters and save them in path->dir
      while(strchr(myPath, ':')!=NULL){
        char *splitted= strchr(myPath, ':');
        *splitted='\0';
        path->dir[path->ndir]=myPath;
        myPath=splitted+1;
        path->ndir++;
      }
    }
    path->dir[path->ndir] = myPath;
    //Done: get PATH from the environment and parse it into path
   
}
int main(int argc, char **argv) {
    char cmdline[MAXLINE];
    struct Path path;
    parse_path(&path);
    while (1) {
        printf("> ");
        fgets(cmdline, sizeof(cmdline), stdin);
        if (feof(stdin))
            exit(0);
        eval(cmdline, &path);
    }
    if (path.ndir > 1)
        free(path.dir[0]);
}


