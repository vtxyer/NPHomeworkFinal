#include<stdio.h>
#include<stdlib.h>
#define SIZE 1024*1024*100
int main(){
	int i, j, times;
	char *ptr[100];
	scanf("%d", times);
	for(i=0; i<times; i++){
		ptr[i] = (char *)malloc(SIZE);	
		if(ptr[i]==NULL){
			printf("alloc error %d[M]\n", i*100);
			times = i;
			break;
		}
	}
	while(1){
		for(i=0; i<times; i++){
			for(j=0; j<SIZE; j++){
				ptr[i][j] = 5;
			}
		}
	}
	return 0;
}
