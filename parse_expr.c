// parse_expr.c

// Rediet Negash
// 111820799
// CSE 320 HW3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define ERRQUIT(msg)  (err_quit((msg), __FILE__, __func__, __LINE__))
#define MATCH(buf, c) (match(buf, c) || ERRQUIT("unexpected char"))

#define HEADER "\t.section .rodata \nfmt : \n\t.string  \"result: %%d\\n\" \n\t.globl main \n\t.text \nmain: \n"

#define FOOTER "\tmovq  $fmt,%%rdi \n\tpopq  %%rsi \n\tmovq  $0,%%rax \n\tcall printf \n\tret\n"

#define POPTOPUSHADD "\n\tpopq  %%rax \n\tpopq  %%rbx \n\taddq  %%rbx,%%rax \n\tpushq %%rax \n"

#define POPTOPUSHSUB "\n\tpopq  %%rbx \n\tpopq  %%rax \n\tsubq  %%rbx,%%rax \n\tpushq %%rax \n"

#define POPTOPUSHMUL "\n\tpopq  %%rax \n\tpopq  %%rbx \n\tmulq  %%rbx \n\tpushq %%rax \n"

#define POPTOPUSHDIV "\n\tpopq  %%rbx \n\tpopq  %%rax  \n\txor %%rdx, %%rdx \n\tdivq  %%rbx \n\tpushq %%rax \n"

# define POPTONEGATEPUSHRESULT "\n\tpopq %%rbx \n\tneg %%rbx \n\tpush %%rbx"


struct Buffer {
    char str[100];
    int index;
};

void parse_cond_exp(struct Buffer *buf);
void parse_add_exp(struct Buffer *buf);
void parse_mul_exp(struct Buffer *buf);
void parse_uminus_exp(struct Buffer *buf);
void parse_factor(struct Buffer *buf);

int err_quit(char *msg, const char* file, const char *func, int line) {
    printf("Error in function %s (%s, line %d): %s\n", func, file, line, msg);
    exit(1);
    return 0;
}

int new_label() {
    static int label = 0;
    return label++;
}

void init_buf(struct Buffer *buf, char *str) {
    strcpy(buf->str, str);
    buf->index = 0;
};

char get_char(struct Buffer *buf) {
    if(buf->index >= sizeof(buf->str))  ERRQUIT("index out of bound");
    return buf->str[buf->index];
}

int match(struct Buffer *buf, char c) {
    if(get_char(buf) != c) 
        return 0;
    buf->index++;
    return 1;
}

void parse_exp(struct Buffer *buf) {
    //TODO: print out the header that defines a fmt string for printf
    //      and the start of main function
    printf(HEADER);
    parse_cond_exp(buf);
    MATCH(buf, '\0');
    //TODO: print ouf the footer that calls printf with the parameters
    //      fmt and the evaluation result
    printf(FOOTER);
}

void parse_cond_exp(struct Buffer *buf) {
    parse_add_exp(buf);
    if(get_char(buf) == '?') {
        int label_else = new_label();
        int label_end  = new_label();

        MATCH(buf, '?');
	printf("\n\tpopq %%rax \n\tcmpq $0, %%rax \n\tje label_%d\n", label_else);
        //TODO: pop a number, jump to label_else if it is 0
        parse_add_exp(buf);

	printf("\n\tjmp label_%d\n", label_end);

       
        MATCH(buf, ':');
        //TODO: put label_else here
        printf("\nlabel_%d:\n", label_else);
        parse_add_exp(buf);
	printf("label_%d:\n", label_end);
        //TODO: put label_end here

    }
}

void parse_add_exp(struct Buffer *buf) {
    parse_mul_exp(buf);
    while(1) {
        char c = get_char(buf);
        if(c == '+' || c == '-') {
            MATCH(buf, c); 
            parse_mul_exp(buf);
           if(c == '+'){

		printf(POPTOPUSHADD);
		
	    }
	      else {
		printf(POPTOPUSHSUB);
		}

            //DO: pop two numbers from the stack,
            //      perform the operation and push the result
        }
        else
            return;
    }
}

void parse_mul_exp(struct Buffer *buf) {
    parse_uminus_exp(buf);
    while(1) {
        char c = get_char(buf);
        if(c == '*' || c == '/') {
            MATCH(buf, c);
            parse_uminus_exp(buf);
	    if(c=='*'){
		printf(POPTOPUSHMUL);
		}
	     else{
		printf(POPTOPUSHDIV);
		}
            //TODO: pop two numbers from the stack,
            //      perform the operation and push the result
        }
        else
            return;
    }
}

void parse_uminus_exp(struct Buffer *buf) {
    int i = 0;
    for(; get_char(buf) == '-'; i++)
        MATCH(buf, '-');

    parse_factor(buf);
    if(i & 1) {
	printf(POPTONEGATEPUSHRESULT);
        //TODO: pop a number, negate it, and push the result
    }
}

void parse_factor(struct Buffer *buf) {
    char c = get_char(buf);
    if('0' <= c && c <= '9') {
        MATCH(buf, c);
	printf("\n\t pushq $%d", c - '0');
        //TODO: push the number to the stack 
        //      convert the ascii code to a number
    }
    else if(c == '(') {
        MATCH(buf, '(');
        parse_cond_exp(buf);
        MATCH(buf, ')');
    }
    else
        ERRQUIT("unexpected char");
}

int main() {
    struct Buffer buf;
    char str[100];
    fprintf(stderr, "enter expr: ");
    scanf("%99s", str);
    init_buf(&buf, str);
    parse_exp(&buf);
    return 0;
}


