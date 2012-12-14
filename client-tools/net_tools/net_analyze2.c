#include <stdio.h>
#include <stdlib.h>

double total_packet_size, analyze1,x2,x;
int ana1;
char a[100];
double cal_speed(int model){
	FILE *p, *tp, *temp;
	double speed;
	
   	usleep(1000000);
	if(model==1){
	    p = fopen("/home/netlog/rate_http","r");
	}
	else if(model==2){
	    p = fopen("/home/netlog/rate_ftp","r");
	}

	if(p != NULL){	
   	    while(fscanf(p,"%s", &a) != EOF ){
			//printf("x:%lf\n", x);
	    }
	}
	//printf("x_finall:%.2lf\n", x);
	fclose(p);
	return 0;
}

int main(){
	int i=0, model, total_sec;
	double speed;
	
	while(1){
	    printf("***VM network throughput analyze***\n\n");
	    printf("1. http upload retransmission rate\n");
	    printf("2. ftp upload retransmission rate\n");
	    printf("3. exit\n");
	    printf("enter: ");
	    scanf("%d", &model);	
	    if(model == 3)
		break;
	    scanf("%d", &total_sec);
	    
	    if(model == 1){
		ana1 =0;
		for(i=0; i<total_sec; i++){
	            speed = cal_speed(1);
	            printf("http retransmission rate: %s\n", a);
		}
		printf("\n***********************************\n");
	    }
	    else if(model == 2){
		for(i=0; i<total_sec; i++){
		    speed = cal_speed(2);
		    printf("ftp retransmission rate: %s\n",a);
		}
		
		
	    }
	    else{
		printf("Wrong input!\n");
	    }
	}
	return 0;
}
