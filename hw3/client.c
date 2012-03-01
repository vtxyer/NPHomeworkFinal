#include"lib.h"
#define SA struct sockaddr


int main(int argc,char** argv)
{
    int cliFD;
    char buff[10000];
    struct sockaddr_in seraddr;
    
    /*Init Socket*/   
    cliFD=socket(AF_INET,SOCK_STREAM,0);
    bzero(&seraddr,sizeof(seraddr));
    seraddr.sin_family=AF_INET;
    int i;


//    inet_pton(AF_INET,argv[1],&seraddr.sin_addr);
    struct hostent *he;
    he = gethostbyname(argv[1]);
    seraddr.sin_addr= *((struct in_addr*)he->h_addr);
    seraddr.sin_port=htons(atoi(argv[2]));


    connect(cliFD,(SA*)&seraddr,sizeof(seraddr));
    char ss[999];   
    bzero(ss, 999);        
    bzero(buff, 10000);       

   
    printf("ok\n");
    sleep(20);   
    int l;
    l = read(cliFD, buff, 9999);
    printf("over %d\n", l);
}
