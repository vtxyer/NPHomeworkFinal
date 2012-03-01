//shell.c 
#include "lib.h"
#include "regex.h" 
#include <unistd.h>
#define USERSIZE 50

//global variable
char Tab[2];
char Newline[2];
int delim_last = 0;

typedef struct{
    int FD;
    int pid;
    char name[25];
    char ip[50]; 
    key_t sem_key;
    char buff[1025];
    int fifoUsed;
    int online;
}CliData;


//CliData **all_cliFD;
CliData *all_cliFD[USERSIZE];

int order(int a){
    int k=1;
    while(a/=10){
        k++;
    }
    return k;
}
int FdToPtr(CliData *a[], int FD){
    int i;
    for(i=1; i<USERSIZE; i++){
        if(a[i]->FD==FD)
            return i;
    }
    return -1;
}
char *trim(char *ary){
    int start = 0;
    int end = strlen(ary);
    int i=0;
    while(ary[i]==' '){
        start++;
        i++;
    }
    i = strlen(ary);
    while(ary[i-1]==' '){
        end--;
        i--;
    }    
    char *str = malloc(sizeof(char)*(end - start));
    int k=0;
    for(i=start; i<end ;i++){
        str[k] = ary[i];
        k++;        
    }
    str[k]='\0';
    free(ary);
    return str;
}

char *substr(char *ary,int start, int end){
    char *str = malloc(sizeof(char)*(end-start+2));
    bzero(str,(sizeof(char)*(end-start+2)));
    int i,index=0;
    for(i=start;i<end;i++){
        str[index] = ary[i];
        index++;
    }
    str[index] = '\0';
    str = trim(str);
    return str;
}

int line_analy(char *line, char **ans, char **delim, int line_size){
    regex_t regex;
    regmatch_t m[2];
    int reti, ans_size=0, flag=0;
    char *preg;
    preg = line;    
    while(1){
        reti = regcomp(&regex, "(![0-9]*|>|>[|]|[|][0-9]*|<[0-9]*)", REG_EXTENDED);
        if(reti)printf("compile error!\n");
        reti = regexec(&regex, preg, 2, m, REG_NOTBOL);
        if(!reti){
            int i=1;
            flag = 1;
            delim[ans_size] = substr(preg, m[1].rm_so, m[1].rm_eo);              
            delim_last++; 
            if(preg[0]==' ')
                ans[ans_size] = substr(preg, 1, m[1].rm_so);
            else
                ans[ans_size] = substr(preg, 0, m[1].rm_so);

            preg += sizeof(char)*m[i].rm_eo;
            ans_size++;
            if((strlen(preg)==0))break;
        }
        else if( reti==REG_NOMATCH ){
            ans[ans_size] = substr(preg, 0, line_size);           
            while(preg[0]==' ' && strlen(preg)>0){
                preg += sizeof(char);
            }
            if((strlen(preg)==0))break;
            if(preg[0]==' ')
                ans[ans_size] = substr(preg, 1, line_size);
            else
                ans[ans_size] = substr(preg, 0, line_size);            
            if(flag == 0){
                delim[0] = malloc(sizeof (char) * 2);
                delim[0][0]='n';
                delim[0][1]='\0';
                delim_last++;
            } 
            //            printf("%s\n",ans[ans_size]);
            ans_size++;
            break;
        }
    }
    return ans_size;
}

int  communi(char *preg, int *ptr, int *to_cliFD){
    char *buff_ptr;
    if( (buff_ptr = strstr(preg, "who")) ){
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        buff_ptr+=3;
        (*ptr)+= 3; 
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        return 1;
    }
    else if( (buff_ptr = strstr(preg, "tell")) ){
        char *tmp_int;
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        buff_ptr+=4;
        (*ptr) += 4;
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        *to_cliFD += buff_ptr[0]-48;
        buff_ptr++;
        (*ptr)++;
        while(buff_ptr[0]!=' '){
            *to_cliFD = *to_cliFD * 10;
            *to_cliFD += buff_ptr[0]-48;
            buff_ptr++;
            (*ptr)++;
        }
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        return 2;
    }
    else if( (buff_ptr = strstr(preg, "yell")) ){
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        buff_ptr+=4;
        (*ptr) = (*ptr) + 4;
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        return 3;
    }
    else if( (buff_ptr = strstr(preg, "name")) ){
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        buff_ptr+=4;
        (*ptr)+= 4;
        while(buff_ptr[0]==' '){
            buff_ptr++;
            (*ptr)++;
        }
        return 4;
    }
    else {
        return 0;
    }
}



char **args(char *str,int flag){
    int i,index=0,size=0,start = 0,len = strlen(str);
    char **arg ;
    for(i=0; i<len; i++){
        if(str[i]==' ')
            size++;;
    }
    arg = malloc( sizeof(char*)*(size+2) );
    bzero(arg,(sizeof(char*)*(size+2)));
    for(i=0; i<len; i++){
        if(str[i]==' ' ){
            arg[index] = substr(str, start, i);
            start = i+1;
            index++;
        }
        else if( i==(len-1) ){
            arg[index] = substr(str, start, i+1);
            index++;
        }
    }
    arg[index]=NULL;
    return arg;
}


void broadcast(char *str,int clifd, CliData *all_cliFD[], int self){
    int i,FD;
    char msg[150];
    bzero(msg,sizeof(msg));
    strcpy(msg,str);
    strcat(msg,"% \0");
    usleep(200);    
    for(i=1; i<USERSIZE; i++){
        if( (FD=all_cliFD[i]->FD) != 0){
            //WRITE BUFF
            strcpy(all_cliFD[i]->buff, str);
            if(FD == clifd){
                if(self == 1 ){                
                    write(clifd, all_cliFD[i]->buff, strlen(all_cliFD[i]->buff));
                }
                bzero(all_cliFD[i]->buff, sizeof(all_cliFD[i]->buff));
            }                
            else if(all_cliFD[i]->pid != -1){            
                kill(all_cliFD[i]->pid, SIGUSR1);
            }
            else{
//                printf("broadcast [%d] error\n",i);
            }
        }
    }
}

int PidToPtr(int pid){
    int i;
    for(i=0; i<USERSIZE; i++){
        if( (all_cliFD[i]->pid) == pid)return i;
    }
}

void readbuff(int sig){
   int pid = getpid();
   int ptr = PidToPtr(pid);
   int clifd = all_cliFD[ptr]->FD;
   write(clifd, all_cliFD[ptr]->buff, strlen(all_cliFD[ptr]->buff));
   bzero(all_cliFD[ptr]->buff, sizeof(all_cliFD[ptr]->buff));
//   write(clifd, "\n% ", 3);
}

int fileExist(char *file){
    struct stat a;
    int k = stat(file, &a);
    if(k!=-1){
        return 1;
    }
    else{
        return 0;
    }
}



void retrieve(){
    union wait status;
    int i;
    wait3(&status, WNOHANG, (struct rusage*)0);  
    for(i=1; i<USERSIZE; i++){
        if( all_cliFD[i]->FD!=0 && all_cliFD[i]->online==0 ){
            close(all_cliFD[i]->FD);
            all_cliFD[i]->FD = 0;
        }
    } 
//    wait();
}
void pauseAgain(){
    pause();
}


int shell(int clifd, int global_pipe[][2])
{
    Tab[0] = 9;
    Tab[1] = '\0';
    Newline[0] = 10;
    Newline[1] = '\0';
    int line_size,arg_size,i,j,pid,close_ptr=0;  //startfd need to revise
    int nowptr = 0;
    int *yclose;
    char *preg;
    int **ptr_all;
    yclose = malloc(sizeof(int)*LINE_S );
    preg = malloc( sizeof(char)* LINE_S );
    ptr_all = malloc(sizeof(int*) * 1002);
    for(i=0; i<LINE_S; i++){
        yclose[i] = -1;      
    }    
    for(i=0 ;i<1002; i++){
        ptr_all[i] = NULL;
    }


    char *tmp_preg;
    //signal
    signal(SIGUSR1, readbuff);
    signal(SIGUSR2, pauseAgain);
    signal(SIGCHLD, retrieve);

    char **tmp_cmd;
    while(1){
        bzero(preg, sizeof(char)*LINE_S );        
//        line_size = read(clifd, preg, LINE_S);                        

        line_size = 0;
        tmp_preg = preg;
//        while(line_size < LINE_S ){
//            int tmp_read;
//            tmp_read = read(clifd, tmp_preg, sizeof());            
//            if(tmp_preg[0] == 13 ||  tmp_preg[0] == '\n'){
//                tmp_preg[0] = '\0';
//                break;
//            }
//            if(tmp_read<=0){
//                exit(1);
//            }
//            line_size++;
//            tmp_preg++;
//        }
//        line_size++;
        long long int t_size = 0;       
        int ff = 0; 
        while(1){        
            t_size = read(clifd, tmp_preg, 10000);
            line_size += t_size;
            if( tmp_preg[strlen(tmp_preg)-1] == 13 || tmp_preg[strlen(tmp_preg)-1] == '\n'){
                tmp_preg[strlen(tmp_preg)-1] = '\0';
                if(tmp_preg[strlen(tmp_preg)-1] == 13 || tmp_preg[strlen(tmp_preg)-1] == '\n'){
                    tmp_preg[strlen(tmp_preg)-1] = '\0';
                }
                break;
            }
            else{
                tmp_preg += t_size;
            }          
            if(t_size == 0){
                return 1;
            }     
            printf("again\n",preg);
        }
       

        //        printf("%d\n",line_size);
        char **arg, **cmd; 
        arg = malloc( sizeof(char*)*line_size+1 ); 
        char **delim ; 
        delim = malloc( sizeof(char*)*line_size+1 );
        bzero(arg, (sizeof(char*)*line_size+1) );
        bzero(delim, (sizeof(char*)*line_size+1) );

//        *(index(preg,'\n')) = '\0';
//        *(index(preg,13)) = '\0';
//                printf("%s\n",preg);


        if(!strcmp(preg,"exit")){
            break;
        }
        /*HW2 into communication*/   
        int buff_ptr=0;
        int cmd_type=0;
        int to_cliFD=0;
        int FDptr = FdToPtr(all_cliFD, clifd);
        char wel_name[60];
        if( (cmd_type = communi(preg, &buff_ptr, &to_cliFD)) ){
            switch(cmd_type){
                case 1:     //who
                    bzero(wel_name, sizeof(wel_name));
                    sprintf(wel_name,"<ID>%s<nickname>%s<IP/port>%s<indicate me>\n\0", Tab,Tab,Tab);
                    write(clifd, wel_name, strlen(wel_name));
                    for(i=1; i<USERSIZE; i++){
                        if(all_cliFD[i]->FD !=0){                           
                            char clifd_str[5];
                            gcvt( i, order(i), clifd_str);
                            write(clifd, clifd_str, order(i));
                            write(clifd, Tab, 1);
                            if( strlen(all_cliFD[i]->name) != 0 ){
                                write(clifd , all_cliFD[i]->name, strlen(all_cliFD[i]->name) );
                            }
                            else{
                                write(clifd , "(no name)", 9);
                            }
                            write(clifd, Tab, 1);                        
                            write(clifd, all_cliFD[i]->ip, strlen(all_cliFD[i]->ip));
                            write(clifd, Tab, 1);
                            if(clifd == all_cliFD[i]->FD){
                                write(clifd, "<-me", 4);
                            }
                            write(clifd, Newline, 1);
                        }
                    }
                    break;
                case 2:     //tell
                    if(all_cliFD[to_cliFD]->FD !=0){
                        int realFD = all_cliFD[to_cliFD]->FD;                                                                        
                        //WriteBUff
                        if( strlen(all_cliFD[FDptr]->name) != 0 )
                            sprintf(all_cliFD[to_cliFD]->buff,"*** %s told you ***: \0", all_cliFD[FDptr]->name);
                        else{
                            sprintf(all_cliFD[to_cliFD]->buff,"*** (no name) told you ***: \0");
                        }
                        preg += buff_ptr;
                        int length = strlen(preg) < sizeof(all_cliFD[to_cliFD]->buff)?strlen(preg): (sizeof(all_cliFD[to_cliFD]->buff)-3);
                        strncat(all_cliFD[to_cliFD]->buff , preg, length);
                        if(all_cliFD[to_cliFD]->FD != clifd){
//                            strcat(all_cliFD[to_cliFD]->buff, "\n% \0");
//                            strcat(all_cliFD[to_cliFD]->buff, "\n \0");
                        }
                        else{
//                            strcat(all_cliFD[to_cliFD]->buff,"\n\0");
                        }
                        if(all_cliFD[to_cliFD]->pid != -1){
                            kill(all_cliFD[to_cliFD]->pid,SIGUSR1);
                        }
                        else{
                            printf("tell [%d]\n",to_cliFD);
                        }
                    }
                    else{
                        char *no1 = "*** Error: user #\0";
                        char *no2 = " does not exist yet. ***\n\0"; 
                        write(clifd, no1, strlen(no1));
                        char clifd_str[5];
                        gcvt(to_cliFD, order(to_cliFD), clifd_str);
                        write(clifd, clifd_str, order(to_cliFD));
                        write(clifd, no2, strlen(no2));
                    }   
                    break;
                case 3:     //yell
                    preg += buff_ptr;
                    for(i=0; i<USERSIZE; i++){
                        if( ( all_cliFD[i]->FD) !=0 ){
                            //WriteBUff 
                            if( strlen(all_cliFD[FDptr]->name) != 0 )
                                sprintf(all_cliFD[i]->buff,"*** %s yelled ***: \0", all_cliFD[FDptr]->name);
                            else{
                                sprintf(all_cliFD[i]->buff,"*** (no name) yelled ***: \0");
                            }
                            int length = strlen(preg) < sizeof(all_cliFD[i]->buff)?strlen(preg): (sizeof(all_cliFD[i]->buff)-2);
                            strncat(all_cliFD[i]->buff , preg, length);
                            if(all_cliFD[i]->FD != clifd){
//                                strcat(all_cliFD[i]->buff, "\n% \0");
//                                strcat(all_cliFD[i]->buff, "\n \0");
                            }
                            else{
//                                strcat(all_cliFD[i]->buff,"\n\0");
                            }
                            if(all_cliFD[i]->pid){
                                kill(all_cliFD[i]->pid,SIGUSR1);
                            }
                            else{
                                printf("yell [%d]\n",i);
                            }
                        }
                    }
                    break;
                case 4:     //name
                    preg += buff_ptr;                
                    for(i=0; i<strlen(preg); i++){
                        all_cliFD[FDptr]->name[i] = preg[i];
                    }
                    all_cliFD[FDptr]->name[i] = '\0';
                  
                    for(i=0; i<USERSIZE; i++){
                        if( (to_cliFD = all_cliFD[i]->FD) !=0){
                            //WriteBUff 
                            sprintf(all_cliFD[i]->buff ,"*** User from %s is named '%s'. ***\n\0", all_cliFD[FDptr]->ip, all_cliFD[FDptr]->name);
//                            if(to_cliFD!=clifd)
//                                strcat(all_cliFD[i]->buff, "% \0");
                            if(all_cliFD[i]->pid != -1){
                                kill(all_cliFD[i]->pid,SIGUSR1);
                            }
                            else{
                                printf("name [%d]\n",i);
                            }
                        }
                    }
                    break;
            }
            for(i=0;i<close_ptr;i++){            
                yclose[i] = -1;
            }   
            if(ptr_all[nowptr]){
                free(ptr_all[nowptr]);             
                ptr_all[nowptr] = NULL;
            }
            close_ptr = 0;     
            free(arg);
            free(delim);
            nowptr = (nowptr+1)%1001;
            delim_last = 0;
            write(clifd ,"\n% ",3);
            continue;
        }
        /**/


        arg_size = line_analy(preg, arg, delim, strlen(preg));
        //construct pipe
        int **pipe_c;
        pipe_c = malloc(sizeof(int*) * arg_size+1);
        bzero(pipe_c, (sizeof(int*) * arg_size+1) );
        if(arg_size>1){
            for(i=0; i<arg_size-1; i++){
                pipe_c[i] = malloc(sizeof(int)*2);               
                yclose[close_ptr++] = pipe_c[i][0];
                yclose[close_ptr++] = pipe_c[i][1];
            }
        }
        pipe_c[arg_size]=NULL;
        pipe(pipe_c[0]); 
        
        //clifd ptr to string
        char ptrStr[5];
        bzero(ptrStr, sizeof(ptrStr));
        sprintf(ptrStr,"%d\0",FdToPtr(all_cliFD, clifd));

        //FIFOFD
        int FIFOFD=0;

        for(i=0; i<arg_size; i++){      
//            printf("%d/%d\n",i, arg_size-1);

            int clifdPtr = FdToPtr(all_cliFD,clifd);
            int tmp;
            char tmpc;
            if( i==0 && (strlen(delim[delim_last-1]) > 1) && (delim[delim_last-1][0] == '|' || delim[delim_last-1][0] == '!')){              
                sscanf(delim[delim_last-1],"%c%d",&tmpc,&tmp);
                if(!ptr_all[ (nowptr+tmp)%1001 ]){
                    ptr_all[ (nowptr+tmp)%1001 ] = malloc(sizeof(int)*2);
                    pipe(ptr_all[ (nowptr+tmp)%1001 ]);
                }
                //                printf("pipe to %d \n",(nowptr+tmp)%1001);
            }
            if(i!=0 && i!=arg_size-1){
                pipe(pipe_c[i]);
            }


            char yoho_msg[150];
            bzero(yoho_msg,sizeof(yoho_msg));
            if( (i!=(arg_size-1) && (strlen(delim[i]) > 1) && delim[i][0] == '<')  || 
                   (i==(arg_size-1) &&  (strlen(delim[delim_last-1]) > 1) && delim[delim_last-1][0] == '<' && i==(delim_last-1))){
                    sscanf(delim[i],"%c%d",&tmpc,&tmp);
                    if( all_cliFD[tmp]->fifoUsed == 1 ){
                        sprintf(yoho_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                                 all_cliFD[FDptr]->name, FDptr, all_cliFD[tmp]->name, tmp, preg
                               );                       
//                         printf("%d\n",strlen(yoho_msg)); 
                    }
            }

            pid = fork();
            if(pid == 0 ){

                //Analyze next
                if(i!=(arg_size-1)){
                    if (delim[i][0] == '>'){
                        FILE *p;
                        p = fopen(arg[i+1],"w");
                        dup2(p->_fileno,1);
                        //                        dup2(p->_fileno,2);
                        dup2(clifd,2);
                        close(p->_fileno);
                    }
                    else if(delim[i][0]=='|'){                    
                        dup2(pipe_c[i][1],1);
                        dup2(clifd,2);
                    }
                    else if(delim[i][0]=='!'){
                        dup2(pipe_c[i][1],2);
                        dup2(pipe_c[i][1],1);
                    }

                }
                else{
                    if( (strlen(delim[delim_last-1])) > 1  && ( delim[delim_last-1][0] == '|' ||  delim[delim_last-1][0] == '!' )){                      
                        if( delim[delim_last-1][0]=='!' ){
                            dup2(ptr_all[ ((nowptr+tmp)%1001) ][1],2);
                            dup2( ptr_all[ ((nowptr+tmp)%1001) ][1],1);
                        }                      
                        else{
                            dup2( ptr_all[ ((nowptr+tmp)%1001) ][1],1);                        
                            dup2(clifd,2);
                        }
                    }
                    else if((strlen(delim[delim_last-1])) > 1  && delim[delim_last-1][0] == '>'){
                        char file_path[50];
                        bzero(file_path, sizeof(file_path));
                        strcat(file_path, FIFODIR);
                        strcat(file_path, ptrStr);
                        if( all_cliFD[FDptr]->fifoUsed == 1 ){
                            char *tmp_msg = "*** Error: your pipe already exists. ***\n\0";
                            write(clifd, tmp_msg, strlen(tmp_msg));
                            exit(0);
                        }
                        else{     
//                            printf("Weite FIFO pid %d\n",getpid());
                            all_cliFD[FDptr]->fifoUsed = 1;
                            mkfifo(file_path , 0666);
                            char tmp_msg[150];
                            bzero(tmp_msg,sizeof(tmp_msg));
                            sprintf(tmp_msg, "*** %s (#%d) just piped '%s' into his/her pipe. ***\n\0",all_cliFD[clifdPtr]->name, clifdPtr, preg);
                            write(clifd, "\n",1);
                            broadcast(tmp_msg, clifd, all_cliFD, 1);
                            kill(all_cliFD[FDptr]->pid,SIGCHLD);
                            FIFOFD = open(file_path, O_WRONLY);                    
                            if( delim[delim_last-1][1]=='!' ){ 
                                dup2(FIFOFD,2);
                                dup2(FIFOFD,1);
                            }
                            else{
                                dup2(FIFOFD,1);
                                dup2(clifd,2);
                            }
                        }
                    }
                    else{
                        dup2(clifd,1);                   
                        dup2(clifd,2);
                    }
                }


                char file_path[50];
                bzero(file_path, sizeof(file_path));
                //Analyze pre
                if(i!=0){
                    if( delim[i-1][0] == '|' || delim[i-1][0] == '!' ){                       
                        dup2(pipe_c[i-1][0],0);                     
                    }
                    else if( (strlen(delim[delim_last-1]))==1 && delim[i-1][0] == '>'){                    
                        exit(0);
                    }                   
                }
                if(i==0 && ptr_all[nowptr]){
                    dup2(ptr_all[nowptr][0],0);
                }
                if( (i!=(arg_size-1) && (strlen(delim[i]) > 1) && delim[i][0] == '<')  || 
                     (i==(arg_size-1) &&  (strlen(delim[delim_last-1]) > 1) && delim[delim_last-1][0] == '<' && i==(delim_last-1))){
                    sscanf(delim[i],"%c%d",&tmpc,&tmp);
                    bzero(file_path, sizeof(file_path));
                    strcat(file_path,FIFODIR);
                    char tmp_str[10];
                    sprintf(tmp_str,"%d\0",tmp);
                    strcat(file_path, tmp_str);
                    if( all_cliFD[tmp]->fifoUsed == 1 ){
                        all_cliFD[tmp]->fifoUsed = 0;
                        mkfifo(file_path , 0666);
                        FIFOFD = open(file_path, O_RDONLY);                                             
                        dup2(FIFOFD, 0);
                        char tmp_msg[150];
                        bzero(tmp_msg,sizeof(tmp_msg));
                        sprintf(tmp_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                                 all_cliFD[FDptr]->name, FDptr, all_cliFD[tmp]->name, tmp, preg
                               );                        
                        broadcast(tmp_msg, clifd, all_cliFD, 0);
//                        kill(all_cliFD[FDptr]->pid, SIGUSR2);
                    }
                    else{
                        char *tmp_msg2 = " does not exist yet. ***\n\0";
                        char *tmp_msg1 = "*** Error: the pipe from #\0";
                        char *tmp_delim = delim[i];
                        tmp_delim++;                   
                        write(clifd, tmp_msg1, strlen(tmp_msg1));
                        write(clifd, tmp_delim, strlen(tmp_delim));
                        write(clifd, tmp_msg2, strlen(tmp_msg2));
                        exit(0);
                    }
                }

                        
                //close fd except 0,1,2
                if(arg_size>1){
                    if(i!=(arg_size-1) && i!=0 && arg_size!=1){
                        close(pipe_c[i][1]);
                        close(pipe_c[i-1][0]);
                    }
                    if(i==0 && arg_size!=1){
                        close(pipe_c[i][1]);
                    }
                    if(i==(arg_size-1) && i!=0 && arg_size!=1){
                        close(pipe_c[i-1][0]);
                    }
                }               
                for(j=0;j<=1000;j++){                
                    if( ptr_all[j] ){
                        close(ptr_all[j][0]);
                        close(ptr_all[j][1]);
                    }                
                }


                if( FIFOFD!=0 ){
                    close(FIFOFD);
                    FIFOFD = 0;
                }
                close(clifd);



                //exec cmd
                cmd = args(arg[i],1);
                int exec_error;
                if(!strcmp(cmd[0],"printenv")){
                    printf("%s=%s\n",cmd[1],getenv(cmd[1]));
                    exit(0);
                }
                else if(!strcmp(cmd[0],"setenv")){
                    exit(0);
                }                
                exec_error = execvp(cmd[0], cmd);
                if(exec_error == -1){
                    fprintf(stderr,"Unknown command: [%s].\n",cmd[0]);
                    exit(0);
                }
                exit(0);
            }
            else{
                tmp_cmd = args(arg[0],0);
                int y;
                if(!strcmp(tmp_cmd[0],"setenv")){
                    y = setenv(tmp_cmd[1],tmp_cmd[2],1);                     
                    printf("client have changed env\n",y); 
                }
                if(arg_size>1){
                    if(i!=(arg_size-1) && i!=0 && arg_size!=1){
                        close(pipe_c[i][1]);
                        close(pipe_c[i-1][0]);
                    }
                    if(i==0 && arg_size!=1){
                        close(pipe_c[i][1]);
                    }
                    if(i==(arg_size-1) && i!=0 && arg_size!=1){
                        close(pipe_c[i-1][0]);
                    }
                }
                if(i==0 && ptr_all[nowptr]){
                    close(ptr_all[nowptr][0]);
                    close(ptr_all[nowptr][1]);
                }
/*                if( i==(arg_size-1) && (strlen(delim[delim_last-1]) > 1) && delim[delim_last-1][0] == '>'){
                    close(global_pipe[clifdPtr][1]);
                    global_pipe[clifdPtr][1] = 0;
                }
                if(i!=(arg_size-1) && (strlen(delim[i]) > 1) && delim[i][0] == '<'){
                    sscanf(delim[i],"%c%d",&tmpc,&tmp);
                    close(global_pipe[tmp][0]);
                    global_pipe[tmp][0] = 0;
                }
                if(i == (arg_size-1) && delim[delim_last-1][0] == '<'  && (delim_last-1) == i){
                    sscanf(delim[delim_last-1],"%c%d",&tmpc,&tmp);
                    close(global_pipe[tmp][0]);
                    global_pipe[tmp][0] = 0;
                }*/
                pause();
                int yoho_len = strlen(yoho_msg);
                if( yoho_len >0 ){
                    write(clifd, yoho_msg, yoho_len);
//                    write(clifd, "*** user1 (#1) just piped 'number test.html >|' into his/her pipe. ***", 73);
                }                
//                wait();                
                free(tmp_cmd);
            }
        }
        write(clifd,"% ",2);
        for(i=0;i<close_ptr;i++){            
            yclose[i] = -1;
        }   
        if(ptr_all[nowptr]){
            free(ptr_all[nowptr]);             
            ptr_all[nowptr] = NULL;
        }
        close_ptr = 0;     
        free(pipe_c);
        free(arg);
        free(delim);
        nowptr = (nowptr+1)%1001;
        delim_last = 0;
    }
    return 0;
}

