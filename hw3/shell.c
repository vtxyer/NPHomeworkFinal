//shell.c 
#include "lib.h"
#include "regex.h" 
#include <unistd.h>
#define USERSIZE 50
char Tab[2];
char Newline[2];
int delim_last = 0;

typedef struct{
    int nowptr;
    int **ptr_all;
}UserData;


typedef struct{
    int FD;
    char name[25];
    char *ip;
    char PATHs[15];
    char PATHd[15];
}CliData;

int order(int a){
    int k=1;
    while(a/=10){
        k++;
    }
    return k;
}
int FdToPtr(CliData *a, int FD){
    int i;
    for(i=1; i<USERSIZE; i++){
        if(a[i].FD==FD)
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


void broadcast(char *str,int clifd, CliData *all_cliFD){
    int i,FD;
    for(i=1; i<USERSIZE; i++){
        if( (FD=all_cliFD[i].FD) != 0){
            if(FD != clifd){
                write(FD, str, strlen(str));
            }
        }
    }
}


int shell(int clifd, UserData* userData, int *yclose, char *preg,CliData *all_cliFD, int global_pipe[][2])
{
    Tab[0] = 9;
    Tab[1] = '\0';
    Newline[0] = 10;
    Newline[1] = '\0';
    //    shell(0);
    int nowptr = userData->nowptr;
    int line_size,arg_size,i,j,pid,close_ptr=0;  //startfd need to revise
    char **tmp_cmd;

    bzero(preg, (sizeof(char)*LINE_S) );        
    line_size = read(clifd, preg, LINE_S);                        

    //        printf("%d\n",line_size);
    char **arg, **cmd; 
    arg = malloc( sizeof(char*)*line_size+1 ); 
    char **delim ; 
    delim = malloc( sizeof(char*)*line_size+1 );
    bzero(arg, (sizeof(char*)*line_size+1) );
    bzero(delim, (sizeof(char*)*line_size+1) );

    *(index(preg,'\n')) = '\0';
    *(index(preg,13)) = '\0';
    //        printf("%s\n",preg);

    char Tab[2];
    Tab[0] = 9;
    Tab[1] = '\0';
    if(!strcmp(preg,"exit"))
        return 1;
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
                write(clifd , wel_name, strlen(wel_name));
                for(i=0; i<USERSIZE; i++){
                    if(all_cliFD[i].FD !=0){
                        char clifd_str[5];
                        gcvt( i, order(i), clifd_str);
                        write(clifd, clifd_str, order(i));
                        write(clifd, Tab, 1);
                        if( strlen(all_cliFD[i].name) != 0 ){
                            write(clifd , all_cliFD[i].name, strlen(all_cliFD[i].name) );
                        }
                        else{
                            write(clifd , "(no name)", 9);
                        }
                        write(clifd, Tab, 1);                        
                        write(clifd, all_cliFD[i].ip, strlen(all_cliFD[i].ip));
                        write(clifd, Tab, 1);
                        if(clifd == all_cliFD[i].FD){
                            write(clifd, "<-me", 4);
                        }
                        write(clifd, Newline, 1);
                    }
                }
                break;
            case 2:     //tell
                if(all_cliFD[to_cliFD].FD !=0){
                    int realFD = all_cliFD[to_cliFD].FD;
                    write(realFD , "*** ", 4);
                    if( strlen(all_cliFD[FDptr].name) != 0 )
                        write(realFD , all_cliFD[FDptr].name, strlen(all_cliFD[FDptr].name) );
                    else{
                        write(realFD , "(no name)", 9 );
                    }
                    write(realFD, " told you ***: ",15);
                    preg += buff_ptr;
                    write(realFD, preg, strlen(preg));
    //                printf("%s\n", preg);
//                    write(realFD, Newline, 1);
//                    write(realFD ,"% ",2);
                }
                else{
                    char *no1 = "*** Error: user #\0";
                    char *no2 = " does not exist yet. ***\0"; 
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
                    if( (to_cliFD = all_cliFD[i].FD) !=0 ){
                        write(to_cliFD , "*** ", 4);                        
                        if( strlen(all_cliFD[FDptr].name) != 0)
                            write(to_cliFD , all_cliFD[FDptr].name, strlen(all_cliFD[FDptr].name) );
                        else{
                            write(to_cliFD , "NULL", 4 );
                        }
                        write(to_cliFD, " yelled ***: ", 13);
                        write(to_cliFD, preg, strlen(preg));
//                        write(to_cliFD, Newline, 1);
//                        if(clifd != to_cliFD)write(to_cliFD ,"% ",2);
                    }
                }
                break;
            case 4:     //name
                preg += buff_ptr;                
                for(i=0; i<strlen(preg); i++){
                    all_cliFD[FDptr].name[i] = preg[i];
                }
                all_cliFD[FDptr].name[i] = '\0';

                for(i=0; i<USERSIZE; i++){
                    if( (to_cliFD = all_cliFD[i].FD) !=0){
                        write(to_cliFD , "*** User from ", 14);                       
                        write(to_cliFD, all_cliFD[FDptr].ip, strlen(all_cliFD[FDptr].ip));
                        write(to_cliFD, " is named '", 11); 
                        write(to_cliFD , all_cliFD[FDptr].name, strlen(all_cliFD[FDptr].name) );
                        write(to_cliFD, "'. ***", 6);
//                        write(to_cliFD, Newline, 1);
//                        if(clifd != to_cliFD)write(to_cliFD ,"% ",2);
                    }
                }
                break;
        }
        for(i=0;i<close_ptr;i++){            
            yclose[i] = -1;
        }   
        if(userData->ptr_all[nowptr]){
            free(userData->ptr_all[nowptr]);             
            userData->ptr_all[nowptr] = NULL;
        }
        close_ptr = 0;     
        free(arg);
        free(delim);
        userData->nowptr = (nowptr+1)%1001;
        delim_last = 0;
        write(clifd ,"\n% ",2);
        return 0;
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


    for(i=0; i<arg_size; i++){       
        int clifdPtr = FdToPtr(all_cliFD,clifd);
        int tmp;
        char tmpc;
        if( i==0 && (strlen(delim[delim_last-1]) > 1) && (delim[delim_last-1][0] == '|' || delim[delim_last-1][0] == '!')){              
            sscanf(delim[delim_last-1],"%c%d",&tmpc,&tmp);
            if(!userData->ptr_all[ (nowptr+tmp)%1001 ]){
                userData->ptr_all[ (nowptr+tmp)%1001 ] = malloc(sizeof(int)*2);
                pipe(userData->ptr_all[ (nowptr+tmp)%1001 ]);
            }
            //                printf("pipe to %d \n",(nowptr+tmp)%1001);
        }
        else if( i==(arg_size-1) && (strlen(delim[delim_last-1]) > 1) && delim[delim_last-1][0] == '>'){
            if( (global_pipe[clifdPtr][0] != 0)  ||  (global_pipe[clifdPtr][1] !=0) ){
                char *tmp_msg = "*** Error: your pipe already exists. ***\n\0";
                write(clifd, tmp_msg, strlen(tmp_msg));
                if(arg_size>1){
                    if(i==(arg_size-1) && i!=0 && arg_size!=1){
                        close(pipe_c[i-1][0]);
                    }
                }
                continue;
//                return 0;                
            }
            else{
                pipe(global_pipe[clifdPtr]);
                printf("!!!!!!!!!!!%d\n",global_pipe[clifdPtr][0]);
            }
        }
        if(i!=0 && i!=arg_size-1){
            pipe(pipe_c[i]);
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
                        dup2(userData->ptr_all[ ((nowptr+tmp)%1001) ][1],2);
                        dup2( userData->ptr_all[ ((nowptr+tmp)%1001) ][1],1);
                    }                      
                    else{
                        dup2( userData->ptr_all[ ((nowptr+tmp)%1001) ][1],1);                        
                        dup2(clifd,2);
                    }
                }
                else if((strlen(delim[delim_last-1])) > 1  && delim[delim_last-1][0] == '>'){
                    if( delim[delim_last-1][1]=='!' ){ 
                        dup2(global_pipe[clifdPtr][1],2);
                        dup2(global_pipe[clifdPtr][1],1);
                    }
                    else{
                        dup2(global_pipe[clifdPtr][1],1);
                        dup2(clifd,2);
                    }
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    sprintf(tmp_msg, "*** %s (#%d) just piped '%s' into his/her pipe. ***\n\0",all_cliFD[clifdPtr].name, clifdPtr,preg);
                    write(clifd, tmp_msg, strlen(tmp_msg));
                    broadcast(tmp_msg, clifd, all_cliFD);
                }
                else{
                    dup2(clifd,1);                   
                    dup2(clifd,2);
                }
            }


            //Analyze pre
            if(i!=0){
                if( delim[i-1][0] == '|' || delim[i-1][0] == '!' ){                       
                    dup2(pipe_c[i-1][0],0);                     
                }
                else if( (strlen(delim[delim_last-1]))==1 && delim[i-1][0] == '>'){                    
                    exit(0);
                }                   
            }
            if(i==0 && userData->ptr_all[nowptr]){
                dup2(userData->ptr_all[nowptr][0],0);
            }
            if( (i!=(arg_size-1)) && (strlen(delim[i]) > 1) &&  (delim[i][0] == '<') ){
                sscanf(delim[i],"%c%d",&tmpc,&tmp);
                if(global_pipe[tmp][0] != 0){
                    dup2(global_pipe[tmp][0],0);
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    sprintf(tmp_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                            all_cliFD[FDptr].name, FDptr, all_cliFD[tmp].name, tmp, preg
                    );
                    broadcast(tmp_msg, clifd, all_cliFD);
                }
                else{
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    sprintf(tmp_msg, "*** Error: the pipe from #%c  does not exist yet. ***\n\0",delim[i][1]);
                    write(clifd, tmp_msg, strlen(tmp_msg));
                    exit(0);                    
                }
            }
            if( (i==(arg_size-1)) && (delim[delim_last-1][0] == '<') && (strlen(delim[(delim_last-1)]) > 1) && ((delim_last-1) == i)){
                sscanf(delim[delim_last-1],"%c%d",&tmpc,&tmp);
                if(global_pipe[tmp][0] != 0){
                    dup2(global_pipe[tmp][0],0);
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    sprintf(tmp_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                             all_cliFD[FDptr].name, FDptr, all_cliFD[tmp].name, tmp, preg
                    );
                    broadcast(tmp_msg, clifd, all_cliFD);
                }
                else{
                    char tmp_msg[150];
                    bzero(tmp_msg, sizeof(tmp_msg));
                    sprintf(tmp_msg, "*** Error: the pipe from #%c  does not exist yet. ***\n\0",delim[i][1]);
                    write(clifd, tmp_msg, strlen(tmp_msg));
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
                if( userData->ptr_all[j] ){
                    close(userData->ptr_all[j][0]);
                    close(userData->ptr_all[j][1]);
                }                
            }
            for(j=0; j<USERSIZE; j++){
                if(all_cliFD[j].FD != 0){
                    close(all_cliFD[j].FD);
                }
                if(global_pipe[j][0] != 0)
                    close(global_pipe[j][0]);
                else if(global_pipe[j][1] != 0)
                    close(global_pipe[j][1]);
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
                strcpy(all_cliFD[FDptr].PATHs, tmp_cmd[1]); 
                strcpy(all_cliFD[FDptr].PATHd, tmp_cmd[2]); 
                printf("client have changed env\n"); 
            }
            if(arg_size>1){
                if(i!=(arg_size-1) && i!=0 && arg_size!=1){
                    if(pipe_c[i][1]!=0){
                        close(pipe_c[i][1]);
                    }
                    if(pipe_c[i-1][0]!=0){
                        close(pipe_c[i-1][0]);
                    }
                }
                if(i==0 && arg_size!=1){
                    if(pipe_c[i][1]!=0){
                        close(pipe_c[i][1]);
                    }
                }
                if(i==(arg_size-1) && i!=0 && arg_size!=1){
                    if(pipe_c[i-1][0]!=0){
                        close(pipe_c[i-1][0]);
                    }
                }
            }
            if(i==0 && userData->ptr_all[nowptr]){
                if(userData->ptr_all[nowptr][0] !=0){                    
                    close(userData->ptr_all[nowptr][0]);
                }
                if(userData->ptr_all[nowptr][1] !=0){
                    close(userData->ptr_all[nowptr][1]);
                }
            }
            if( i==(arg_size-1) && (strlen(delim[delim_last-1]) > 1) && delim[delim_last-1][0] == '>' && global_pipe[clifdPtr][1] !=0){
                close(global_pipe[clifdPtr][1]);
                global_pipe[clifdPtr][1] = 0;
            }
            char tmp_msg[150];
            bzero(tmp_msg, sizeof(tmp_msg));
            if(i!=(arg_size-1) && (strlen(delim[i]) > 1) && delim[i][0] == '<'){
                sscanf(delim[i],"%c%d",&tmpc,&tmp);
                if(global_pipe[tmp][0] != 0 ){
                    sprintf(tmp_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                            all_cliFD[FDptr].name, FDptr, all_cliFD[tmp].name, tmp, preg
                    );
                    close(global_pipe[tmp][0]);
                    global_pipe[tmp][0] = 0;
                }
            }
            if(i == (arg_size-1) && delim[delim_last-1][0] == '<'  && (delim_last-1) == i){
                sscanf(delim[delim_last-1],"%c%d",&tmpc,&tmp);
                if(global_pipe[tmp][0] != 0 ){
                    sprintf(tmp_msg, "*** %s (#%d) just received the pipe from %s (#%d) by '%s' ***\n\0",
                            all_cliFD[FDptr].name, FDptr, all_cliFD[tmp].name, tmp, preg
                    );
                    close(global_pipe[tmp][0]);
                    global_pipe[tmp][0] = 0;
                }
            }
            wait();
            if( strlen(tmp_msg)!=0 ){
                write(clifd, tmp_msg, strlen(tmp_msg));;
            } 
            free(tmp_cmd);
        }
    }
    write(clifd,"% ",2);
    for(i=0;i<close_ptr;i++){            
        yclose[i] = -1;
    }   
    if(userData->ptr_all[nowptr]){
        free(userData->ptr_all[nowptr]);             
        userData->ptr_all[nowptr] = NULL;
    }
    close_ptr = 0;     
    free(pipe_c);
    free(arg);
    free(delim);
    userData->nowptr = (nowptr+1)%1001;
    delim_last = 0;

    return 0;
}

