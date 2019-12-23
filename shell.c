//Rediet Negash
// 111820799

#define _GNU_SOURCE
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
#define EOL     1
#define EOLSTR  "\x1"
#define ERRQUIT(msg)            (err_quit((msg), __FILE__, __func__, __LINE__))
#define CHKBOOLQUIT(exp, msg)   ((exp) || ERRQUIT(msg))
#define MATCH(tokens, str, msg) (CHKBOOLQUIT(match(tokens, str), msg))
#define IS_WS(chr)              ((chr)==' ' || (chr)=='\t' || (chr)=='\n' || (chr)=='\r')
#define IS_OP(chr)              ((chr)==';' || (chr)=='&'|| (chr)=='|' || (chr)=='<' || (chr)=='>' || (chr)==EOL)

struct TokenBuf {
    char buf[MAXLINE*2];
    char *tokens[MAXLINE];
    int pos;
};
struct Command{
    char *argv[MAXARGS];    //arguments
    int argc;               //# of valid argv
    int fdin;               //file descriptor for input
    int fdout;              //file descriptor for output
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
    char *buf = tb->buf;
    int count = 0;
    tb->pos = 0;
    str = skip_whitespace(str);
    while(*str) {
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
    cmd->fdin = 0; 
    cmd->fdout = 1;
    for(; !IS_OP(tb->tokens[tb->pos][0]); tb->pos++) {
        CHKBOOLQUIT(cmd->argc < MAXARGS, "too many args");
        cmd->argv[cmd->argc++] = tb->tokens[tb->pos];
    }
    CHKBOOLQUIT(cmd->argc < MAXARGS, "too many args");
    cmd->argv[cmd->argc] = NULL;
    return;
}
void parse_redirection(struct TokenBuf *tb, struct Command *cmd) {
    struct Command cmd1, cmd2;
    parse_cmd(tb, &cmd1);
    if(tb->tokens[tb->pos][0] == '<') {
        MATCH(tb, "<", "unexpected token");
        parse_cmd(tb, &cmd2);
        CHKBOOLQUIT(cmd2.argc == 1, "invalid input redirection");
	 //TODO: open cmd2.argv[0] and set cmd1.fdin
	
	cmd1.fdin = open(cmd2.argv[0],O_RDONLY,0);
        CHKBOOLQUIT(cmd1.fdin != -1, "cannot open input file");
    }
    if(tb->tokens[tb->pos][0] == '>') {
        MATCH(tb, ">", "unexpected token");
        parse_cmd(tb, &cmd2);
        CHKBOOLQUIT(cmd2.argc == 1, "invalid output redirection");
	
        //TODO: open cmd2.argv[0] and set cmd1.fdout
	cmd1.fdout = open(cmd2.argv[0], O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR
|S_IWUSR);
        CHKBOOLQUIT(cmd1.fdout != -1, "cannot open output file");
    }
    *cmd = cmd1;
}
int run_internal_cmd(struct Command *cmd, char **envp) {
    CHKBOOLQUIT(cmd->argc > 0, "empty command");
    if(strcmp(cmd->argv[0], "exit") == 0)
        exit(0);
    else if(strcmp(cmd->argv[0], "cd") == 0) {
        CHKBOOLQUIT(cmd->argc <= 2, "too many params");
        if(cmd->argc == 2)  CHKBOOLQUIT(chdir(cmd->argv[1]) == 0, "cd failed");
        else CHKBOOLQUIT(chdir(getenv("HOME")) == 0, "cd HOME failed");
        return 1;
    }
    return 0;
}
int run_cmd(struct Command *cmd, char **envp) {
    int pid = fork();
    if(pid==0){
    //TODO: before calling execvpe, if cmd->fdin/fdout are set,
   //copy them to the standard input/ouput and close them
	    if (cmd->fdin != 0){
		dup2(cmd->fdin, 0);
		close(cmd->fdin);
	    }if(cmd->fdout != 1){
		dup2(cmd->fdout, 1);
		close(cmd->fdout);	
	    }
	 execvpe(cmd->argv[0], cmd->argv, envp);
    }
    else{
	//TODO: if cmd->fdin/fdout are set, parent process should close them
	if(cmd->fdin != 0){
	    close(cmd->fdin);
	}
	if(cmd->fdout !=1){
	    close(cmd->fdout);	
	}
        return pid;
    }
}
void run_cmd_list(struct TokenBuf *tb, char **envp) {
    while(tb->tokens[tb->pos][0] != EOL) {
        struct Command cmd1, cmd2;
        char *token;
        pid_t pids[MAXPIDS];
        int npids = 0;
        parse_redirection(tb, &cmd1);
        // run a list of pipe'd commands
        while((token = tb->tokens[tb->pos])[0] == '|') {
            MATCH(tb, "|", "unexpected token");
            CHKBOOLQUIT(cmd1.fdout == 1, "cmd needs an output redirection");
            parse_redirection(tb, &cmd2);
            CHKBOOLQUIT(cmd2.fdin == 0, "cmd has an input redirection");
            if(!run_internal_cmd(&cmd1, envp)) {
            //TODO: using pipe, prepare cmd1.fdout and cmd2.fdin
		int fd[2];
		pipe(fd);
		cmd2.fdin = fd[0];
		cmd1.fdout = fd[1];
                
                CHKBOOLQUIT(npids < MAXPIDS, "too many piped commands");
                pids[npids++] = run_cmd(&cmd1, envp);
            }
            cmd1 = cmd2; //exiting the loop, the last cmd1 needs to be run
        }
        if(token[0] == EOL || token[0] == ';' || token[0] == '&') {
            int status, i;
            if(token[0] == ';') MATCH(tb, ";", "unexpected token");
            if(token[0] == '&') MATCH(tb, "&", "unexpected token");
            if(!run_internal_cmd(&cmd1, envp))
                pids[npids++] = run_cmd(&cmd1, envp);
	    //TODO: if token[0] is '&' print the process ids in pids;
            //otherwise wait for the processes in the pids array
	    for(i = 0; i < npids; i++) {
	        if(token[0]=='&'){
		 //cmd is a background process. so print its pid
                  printf("bg proc, pid: %d\n",pids[i]);
                }
                else{
                    waitpid(pids[i],&status,0);
                }
            }
        }
        else
            ERRQUIT("unexpected token");
    }
    pid_t pid;
    while((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        printf("bg proc terminated. pid: %d\n", pid);    
}
void eval(char *cmdline, char **envp) {
    struct TokenBuf tb;
    tokenize(cmdline, &tb);
    run_cmd_list(&tb, envp);
}
int main(int argc, char **argv, char **envp) {
    char cmdline[MAXLINE];
    while(1) {
        printf("> ");
        fgets(cmdline, sizeof(cmdline), stdin);
        if(feof(stdin))
            exit(0);
        eval(cmdline, envp);
    }
}


