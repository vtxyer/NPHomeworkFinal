#include "lib.h"
#include <iostream>
using namespace std;
#define SA struct sockaddr 



static int socket_init(struct sockaddr_in *seraddr, const char* port){
    int listenfd;

    bzero(seraddr,sizeof(&seraddr));
    seraddr->sin_family=AF_INET;
    seraddr->sin_addr.s_addr=htonl(INADDR_ANY);
    seraddr->sin_port=htons(atoi(port));

    return listenfd;
}


static int socket_init_connect(struct sockaddr_in *seraddr,struct in_addr *addr,unsigned int port){
    int listenfd;
    bzero(seraddr,sizeof(&seraddr));
    seraddr->sin_family = AF_INET;
    seraddr->sin_addr = *(addr);
    seraddr->sin_port=htons(port);
    return listenfd;
}


void retrieve(int k){
    union wait status;
    int i;
    //    wait3(&status, WNOHANG, (struct rusage*)0);
    wait();
}

void analyze_str(string *str_ary, string msg){
    string newline;
    char ss = 13;
    newline.push_back(ss);
    ss = 10;
    newline.push_back(ss);
    for(int i=0; i<8 ;i++){
        int g = msg.find(newline);
        str_ary[i].append(msg.substr(0,g));
        //        cout<<str_ary[i]<<endl;
        msg.erase(0, g+2);;
    }

}


int main(int argc,char **argv,char **env)
{
    int listenfd, pid, clifd;
    socklen_t cliaddr_len;
    struct sockaddr_in seraddr,cliaddr;
    char *msg_buff;

    signal(SIGCHLD, retrieve);


    /*INIT FD SET*/
    fd_set all_rfd, all_wfd;
    fd_set act_rfd, act_wfd;
    int conn = 0;
    int fd_size = 0;
    FD_ZERO(&all_rfd); 
    FD_ZERO(&all_wfd); 
    FD_ZERO(&act_rfd); 
    FD_ZERO(&act_wfd);

    msg_buff = (char *)malloc(sizeof(char) * 99999);


    if(argc < 2){
        printf("Need 1 argument 'port'\n");        
        return 1;
    }

    //port = argv[1]
    listenfd=socket(AF_INET,SOCK_STREAM,0);
    socket_init(&seraddr,argv[1]);
    bind(listenfd,(SA*)&seraddr,sizeof(seraddr));
    listen(listenfd,0);

    //init
    cliaddr_len = sizeof(cliaddr);
    bzero(msg_buff, 99999);


    /* set environment*/
    chdir("/home/Master1_up/NP/hw3/client");    
    //    setenv("PATH","bin:.",1);    

    string header;
    header.append("HTTP/1.1 200 OKServer: MyservAccept-Ranges: bytes");

    char **cgi_envp;   
    cgi_envp = (char **) malloc(sizeof(char *) * 10);

    string raw_msg;
    string msg[10];
    while(1){        
        if( (clifd = accept(listenfd,(SA*)&cliaddr,&cliaddr_len)) ){
            if((pid = fork() ) < 0){
                printf("Fork error\n");
                return 1;
            }
            if(pid == 0){  //child
                close(listenfd);

                char buffer[20];
                bzero(buffer, sizeof(buffer));
                int s;
                s  = read(clifd, buffer, 20);
                unsigned char VN = buffer[0] ;
                unsigned char CD = buffer[1] ;
                unsigned int DST_PORT = buffer[2] << 8 | buffer[3] ;
                unsigned int DST_IP = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7] ;                
                unsigned int DST_IP2 = buffer[7] << 24 | buffer[6] << 16 | buffer[5] << 8 | buffer[4] ;
                char* USER_ID = buffer + 8 ;

                write(clifd, buffer, 8);
                /*connect to server and wait for read or write*/
                struct hostent *he;
                struct sockaddr_in seraddr;

                int clientFD = socket(AF_INET,SOCK_STREAM,0);
                int flags = fcntl(clientFD, F_GETFL, 0);
                fcntl(clientFD, F_SETFL, flags | O_NONBLOCK);                
                struct in_addr addr;
                addr.s_addr = DST_IP2;
                socket_init_connect(&seraddr, &addr, DST_PORT);   
                int n; 
                if(n = connect(clientFD, (SA*)&seraddr,sizeof(seraddr)) <0 ){
                    if(errno != EINPROGRESS)return (-1);
                }

                fd_size = clientFD>fd_size?clientFD:fd_size;
                FD_SET(clifd, &all_rfd);
                FD_SET(clientFD, &all_rfd);
                FD_SET(clifd, &all_wfd);
                FD_SET(clientFD, &all_wfd);
                conn++;

                int FD[2];
                FD[0] = clientFD; //server
                FD[1] = clifd;  //client


                break;
            }   
            else{
                printf("we have client %d\n",clifd);
                close(clifd);
                //                break;
            }            
        }
        else{
            printf("Server error\n");
            return 1;
        }
    }
}


