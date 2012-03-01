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
    char msg_buff[10000];

    signal(SIGCHLD, retrieve);

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

                read(clifd, msg_buff, sizeof(msg_buff));   
                cout<<msg_buff<<endl;
                exit(1);

                read(clifd, msg_buff, sizeof(msg_buff));
                raw_msg.append(msg_buff);
                analyze_str(msg, raw_msg);                          

                string execfile;
                string rfile("/home/Master1_up/NP/hw3/client");
                if( ((int)msg[0].find('?')) >= 0 ){
                    execfile = msg[0].substr(0,msg[0].find('?'));
                    execfile = execfile.substr(execfile.find(" ")+1);
                }
                else{
                    execfile = msg[0].substr(0,msg[0].rfind(' '));
                    execfile = execfile.substr(execfile.find(" ")+1);                   
                }
                rfile.append(execfile);



                msg[0] = msg[0].substr(msg[0].find('?')+1);
                msg[0] = msg[0].substr(0, msg[0].rfind('H')-1);


                dup2(clifd, 1);
                dup2(clifd, 2);

                /*CGI Enviromment variable*/
                char *tmp_ptr = *cgi_envp;
                for(int k=0; k<9; k++){
//                      tmp_ptr = tmp_ptr + k;
                      cgi_envp[k]  = (char *)malloc(sizeof(char) * 100);                   
                      bzero(cgi_envp[k], 100);
                }
                string env_tmp;
                env_tmp.append("QUERY_STRING=");
                env_tmp.append(msg[0].c_str());
                strcpy(cgi_envp[0],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("CONTENT_LENGTH=1000");
                strcpy(cgi_envp[1],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REQUEST_METHOD=GET");
                strcpy(cgi_envp[2],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REMOTE_HOST=test.edu.tw");
                strcpy(cgi_envp[3],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REMOTE_ADDR=192.168.1.1");
                strcpy(cgi_envp[4],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("ANTH_TYPE=ANTH_TYPE");
                strcpy(cgi_envp[5],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REMOTE_USER=REMOTE_USER");
                strcpy(cgi_envp[6],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REMOTE_USER=REMOTE_USER");
                strcpy(cgi_envp[7],env_tmp.c_str());
                env_tmp.clear();

                env_tmp.append("REMOTE_IDENT=REMOTE_IDENT");
                strcpy(cgi_envp[8],env_tmp.c_str());
                env_tmp.clear();
                cgi_envp[9] = NULL;
 
                write(clifd, header.c_str(), header.size());                                  
                execvpe(rfile.c_str(), NULL, cgi_envp);                
                fprintf(stderr,"hie\n");
//                cout<<msg[0].c_str()<<endl;                    
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


