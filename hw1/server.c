#include"lib.h"
#include "shell.c"
#define SA struct sockaddr




static int socket_init(struct sockaddr_in *seraddr, const char* port){
    int listenfd;

    bzero(seraddr,sizeof(&seraddr));
    seraddr->sin_family=AF_INET;
    seraddr->sin_addr.s_addr=htonl(INADDR_ANY);
    seraddr->sin_port=htons(atoi(port));

    return listenfd;
}




int main(int argc,char **argv,char **env)
{
    int listenfd,cliaddr_len,pid, clifd;
    struct sockaddr_in seraddr,cliaddr;
    char msg_buff[20000];

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
    bzero(msg_buff,sizeof(msg_buff));

    signal(SIGCHLD, retrieve);
    /* set environment*/
    chdir("/home/Master1_up/NP/hw1/RAS");
    setenv("PATH","bin:.",1);


    while(1){
        if( (clifd = accept(listenfd,(SA*)&cliaddr,&cliaddr_len)) ){
            if((pid = fork() ) < 0){
                printf("Fork error\n");
                return 1;
            }
            if(pid == 0){ //child
                close(listenfd);
                char *welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n\0";
                write(clifd,welcome,strlen(welcome));
                write(clifd,"% ",2);
                shell(clifd, env);
                close(clifd);
                break;
            }
            else{
                printf("we have client %d\n",clifd);
                close(clifd);
//                wait();
            }
        }
        else{
            printf("Server error\n");
            return 1;
        }
    }
}
