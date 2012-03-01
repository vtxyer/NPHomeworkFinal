//shell.c 
#include "lib.h"
#include "regex.h" 
#include <unistd.h>

int delim_last = 0;

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
        reti = regcomp(&regex, "(![0-9]*|>|[|][0-9]*)", REG_EXTENDED);
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



void retrieve(){
    union wait status;
    int i;
    wait3(&status, WNOHANG, (struct rusage*)0);  
//    wait();
}

void shell(int clifd, char **env)
{
    //    shell(0);
    int line_size,arg_size,i,j,pid,maxfd,nowptr=0,close_ptr=0;  //startfd need to revise
    int **ptr_all;
    int *yclose;
    char **tmp_cmd;
    ptr_all = malloc(sizeof(int *) * 1002);
    yclose = malloc(sizeof(int) * LINE_S);
    for(i=0; i<=1001; i++){
        ptr_all[i]=NULL;
    }
    for(i=0; i<LINE_S; i++){
        yclose[i] = -1;
    }   
    char *preg, *tmp_preg;
    preg = malloc( sizeof(char)*LINE_S );
    //    char PATH[200];

    
    while(1){
        bzero(preg, (sizeof(char)*LINE_S) );                
        line_size = 0;
        tmp_preg = preg;       
        while(line_size < LINE_S ){
            int tmp_read;
            tmp_read = read(clifd, tmp_preg, 1);
            if(tmp_preg[0] == 13 ||  tmp_preg[0] == '\n'){
                tmp_preg[0] = '\0';
                break;           
            }
            if(tmp_read<=0){
                exit(1);
            }
            line_size++;
            tmp_preg++;            
        }
        line_size++;
//        printf("read over\n");

        char **arg, **cmd; 
        arg = malloc( sizeof(char*)*line_size+1 ); 
        char **delim ; 
        delim = malloc( sizeof(char*)*line_size+1 );
        bzero(arg, (sizeof(char*)*line_size+1) );
        bzero(delim, (sizeof(char*)*line_size+1) );

//        *(index(preg,'\n')) = '\0';
//        *(index(preg,13)) = '\0';                
//        printf("%s\n",preg);

        if(!strcmp(preg,"exit"))break;
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

//                printf("NOW : %d\n",nowptr);
        for(i=0; i<arg_size; i++){    
            int tmp;
            char tmpc;
            if( i==0 && (strlen(delim[delim_last-1]) > 1) ){              
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
                    if( (strlen(delim[delim_last-1])) > 1 ){                      
                        if( delim[delim_last-1][0]=='!' ){
                            dup2(ptr_all[ ((nowptr+tmp)%1001) ][1],2);
                            dup2( ptr_all[ ((nowptr+tmp)%1001) ][1],1);
                        }                      
                        else{
                            dup2( ptr_all[ ((nowptr+tmp)%1001) ][1],1);                        
                            dup2(clifd,2);
                        }
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
                    else if(delim[i-1][0] == '>'){                    
                        exit(0);
                    }                   
                }
                if(i==0 && ptr_all[nowptr]){
                    dup2(ptr_all[nowptr][0],0);
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
                wait();
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
    return;
}

