//printbin.c
#include <stdio.h>

void printbin(int n, int shift){
	
	if (n >= (1 << shift)){
		
		if((n & (1 << shift)) == 0){
			
			printbin(n, shift+1);
			printf("0");			

		}else{
			
			printbin(n, shift + 1);
			printf("1");
		}
	}
}
int main(){
int n;
printf("positive decimal number: ");
scanf("%d", &n);
printf("%d in binary: ", n);
printbin(n,0);
printf("\n");

}
