#include"lib.h"
#include "shell.c"
#define SA struct sockaddr 


void work(){
    int i; 
    int *yclose;
    yclose = malloc(sizeof(int) * LINE_S);
    for(i=0; i<LINE_S; i++){
        yclose[i] = -1;
    }  
}

int findEmptyPtr(CliData *ary){
    int i;
    for(i=1; i<USERSIZE; i++){
        if(ary[i].FD==0)
            return i;
    }
}

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
    int listenFD,cliaddr_len,pid, cliFD, maxFD, i, now_cliSize, selFD_num;
    fd_set all_set, sel_set;
    struct sockaddr_in seraddr,cliaddr, seraddr2;

    //init global pipe
    int global_pipe[USERSIZE][2];   
    for(i=0; i<USERSIZE; i++){
        global_pipe[i][0] = 0;
        global_pipe[i][1] = 0;
    }

    //init User Data
    UserData cli_data[USERSIZE];
    CliData all_cliFD[USERSIZE];
    for(i=0 ;i<USERSIZE; i++){
        cli_data[i].nowptr = 0;
        all_cliFD[i].FD = 0;
    }

    char *msg_buff = malloc( sizeof(char)*1000 );
    if(argc < 2){
        printf("Need 1 argument 'port'\n");        
        return 1;
    }

    //init FD sets
    FD_ZERO(&all_set);
    FD_ZERO(&sel_set);
    now_cliSize = 0;


    //port = argv[1]
    listenFD=socket(AF_INET,SOCK_STREAM,0);
    socket_init(&seraddr,argv[1]);
    int jj;
    jj = bind(listenFD,(SA*)&seraddr,sizeof(seraddr));
    if(jj < 0){
        printf("bind error\n");
        exit(1);
    }
    listen(listenFD,0);
    maxFD = listenFD;

    //init
    char *welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n\0";
    char *come_msg1 = "*** User '(no name)' entered from ";
    char *come_msg2 = ". ***\n\0";
    cliaddr_len = sizeof(cliaddr);
    bzero(msg_buff,sizeof(msg_buff));
    FD_SET(listenFD, &all_set);


    //shell variable
    int *yclose;
    char *preg;
    yclose = malloc(sizeof(int) * LINE_S);
    for(i=0; i<LINE_S; i++){
        yclose[i] = -1;
    }
    preg = malloc( sizeof(char) * LINE_S);


    /* set environment*/
    chdir("/home/Master1_up/NP/hw2/RAS");    
    setenv("PATH","bin:.",1);    





    while(1){       
//        bcopy( all_set, sel_set, now_cliSize);
        sel_set = all_set;      
        selFD_num = select(maxFD+1, &sel_set, NULL, NULL, NULL);
        if( FD_ISSET(listenFD, &sel_set) ){
            cliFD = accept(listenFD,(SA*)&cliaddr,&cliaddr_len);
            FD_SET(cliFD, &all_set);

            /*init CliData array*/  
            int ptr = findEmptyPtr(all_cliFD);         
            all_cliFD[ptr].FD = cliFD;
            bzero(all_cliFD[ptr].name, sizeof(all_cliFD[ptr].name));
            bzero(all_cliFD[ptr].PATHs, sizeof(all_cliFD[ptr].PATHs));
            bzero(all_cliFD[ptr].PATHd, sizeof(all_cliFD[ptr].PATHd));
            long int tmp_addr = ntohl(cliaddr.sin_addr.s_addr);
            char *tmp_addr_str = malloc(sizeof(char) * 30);
            bzero(tmp_addr_str,sizeof(tmp_addr_str));
            int end_addr_ptr = 0;
            all_cliFD[ptr].ip = tmp_addr_str;  
/*            for(i=0; i<4; i++){
                int addr_tmp_int = (tmp_addr>>((3-i)*8)) & 0xff;
                gcvt( addr_tmp_int , order(addr_tmp_int), tmp_addr_str);
                tmp_addr_str += sizeof(char)*(order(addr_tmp_int))+1;
                if(i!=3){
                    *(tmp_addr_str - 1 )='.';
                }
                end_addr_ptr += (sizeof(char)*(order(addr_tmp_int)))+1;
            }
            end_addr_ptr--;
            all_cliFD[ptr].ip[23]='\0';
            all_cliFD[ptr].ip[end_addr_ptr++] = '/';
            gcvt(  htons(cliaddr.sin_port), order(htons(cliaddr.sin_port)), tmp_addr_str);
            all_cliFD[ptr].ip[ end_addr_ptr+order(htons(cliaddr.sin_port)) ]='\0';*/
            strcpy(all_cliFD[ptr].ip,"nctu/5566\0");
//            printf("%s\n",all_cliFD[ptr]);
            /*End init CliData array*/


            maxFD = cliFD>maxFD?cliFD:maxFD;
            cli_data[ptr].nowptr = 0;
            cli_data[ptr].ptr_all = malloc( sizeof(int*) * 1002);
            for(i=0; i<=1001; i++){
                cli_data[ptr].ptr_all[i]=NULL;
            }

            //sent welcome messages
            write(cliFD,welcome,strlen(welcome));
            for(i=0 ; i<USERSIZE; i++){
                if(all_cliFD[i].FD!=0){
                    write(all_cliFD[i].FD, come_msg1, strlen(come_msg1));
                    write(all_cliFD[i].FD, all_cliFD[ptr].ip, strlen(all_cliFD[ptr].ip) );
                    write(all_cliFD[i].FD, come_msg2, strlen(come_msg2));
//                    if(cliFD != all_cliFD[i].FD){
//                       write( all_cliFD[i].FD ,"% ",2);
//                    }
                }
            }
            write(cliFD,"% ",2);
            printf("%d comin \n",cliFD);
            if( --selFD_num == 0)continue;
        }
                
        int echo; 
        for(i=0; i<maxFD+1; i++){
            if( (cliFD = all_cliFD[i].FD) ==0 )
                continue;
            if( FD_ISSET(cliFD, &sel_set) ){
                if( strlen(all_cliFD[FdToPtr(all_cliFD,cliFD)].PATHs) > 0){
                    setenv(all_cliFD[FdToPtr(all_cliFD,cliFD)].PATHs, all_cliFD[FdToPtr(all_cliFD,cliFD)].PATHd,1);
                    printf("%d client have changed env %s %s\n",cliFD, all_cliFD[FdToPtr(all_cliFD,cliFD)].PATHs, all_cliFD[FdToPtr(all_cliFD,cliFD)].PATHd);
                }
                else{
                    setenv("PATH","bin:.",1);
                }
                   
                echo = shell(cliFD, &cli_data[i], yclose, preg, all_cliFD, global_pipe);

                if(echo == 1){ //exit
                    close(cliFD);
                    printf("close %d\n",cliFD);
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    if(strlen(all_cliFD[FdToPtr(all_cliFD,cliFD)].name) > 0){
                        sprintf(tmp_msg, "*** User '%s' left. ***\n\0", all_cliFD[FdToPtr(all_cliFD,cliFD)].name);
                    }
                    else{
                        sprintf(tmp_msg, "*** User '(no name)' left. ***\n\0");
                    }
                    free(all_cliFD[i].ip);
                    all_cliFD[i].FD=0;
                    free(cli_data[i].ptr_all);
                    if(strlen(all_cliFD[i].PATHs) > 0){
                        bzero(all_cliFD[i].PATHs, sizeof(all_cliFD[i].PATHs));
                    }
                    if(strlen(all_cliFD[i].PATHd) > 0){
                        bzero(all_cliFD[i].PATHd, sizeof(all_cliFD[i].PATHd));
                    }
                    cli_data[i].nowptr=0;
                    FD_CLR(cliFD,&all_set);
                    if(global_pipe[i][0] != 0){
                        close(global_pipe[i][0]);
                    }
                    else if(global_pipe[i][1] != 0){
                        close(global_pipe[i][1]);
                    }
                    int j;
                    for(j=0; j<USERSIZE; j++){
                        if(all_cliFD[j].FD !=0 && cliFD!=all_cliFD[j].FD){
                            write(all_cliFD[j].FD, tmp_msg, strlen(tmp_msg));
                        }
                    }            
                    bzero(all_cliFD[i].name, sizeof(all_cliFD[i].name));
                }
            }
        }
    }
    return 1;
}


