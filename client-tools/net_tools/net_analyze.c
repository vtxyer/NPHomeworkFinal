#include <stdio.h>
#include <stdlib.h>

double total_packet_size, analyze1,x2,x;
int ana1;

double cal_speed(int model){
	FILE *p, *tp, *temp;
	double speed;
	if(model==1){
	    p = fopen("/home/netlog/net2","r");
	}
	else if(model==2){
	    p = fopen("/home/netlog/tx_ring_log","r");
	}

	if(p != NULL){	
   	    while(fscanf(p,"%lf", &x) != EOF ){
			//printf("x:%lf\n", x);
	    }
	}
	if(ana1 == 0){
	    ana1 = 1;
	    analyze1 = x;
	}
	//printf("x_finall:%.2lf\n", x);
	fclose(p);

   	usleep(1000000);

	 if(model==1){
	     p = fopen("/home/netlog/net2","r");
	 }
     else if(model ==2){
         p = fopen("/home/netlog/tx_ring_log","r");
     }
     if(p!=NULL){
		while(fscanf(p,"%lf", &x2) != EOF){
				
		}
     }
	 total_packet_size = x2;
	 //   printf("x2_finall:%.2lf\n", x2);
	    fclose(p);
	    return (x2-x)/(double)1024;
	//printf("x1 :%.1lf   x2 :%.1lf\n", x, x2);
}

int main(){
	int i=0, model, total_sec;
	double speed;
	while(1){
	    printf("***VM network throughput analyze***\n\n");
	    printf("1. rx_ring packet throughput speed\n");
	    printf("2. tx_ring packet throughput speed\n");
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
	            printf("rx_ring: %.2lf Mib\n", speed);
		}
		printf("\n***********************************\n");
		printf("Analyze size(during analyze time): %.3lf Mib\n", (x2-analyze1)/(double)1048576);
		printf("VM total packet size(form booting to right now): rx_total_size: %.3f Mib\n\n\n",total_packet_size/(double)1048576);
	    }
	    else if(model == 2){
		ana1 = 0;
		for(i=0; i<total_sec; i++){
		    speed = cal_speed(2);
		    printf("tx_ring: %.2lf Mib\n",speed);
		}
		printf("\n***********************************\n");
		printf("Analyze size(during analyze time): %.3lf Mib\n", (x2-analyze1)/(double)1048576);
		printf("VM total packet size(form booting to right now): tx_total_size: %.3lf Mib\n\n\n",total_packet_size/(double)1048576);
		
		
	    }
	    else{
		printf("Wrong input!\n");
	    }
	}
	return 0;
}
