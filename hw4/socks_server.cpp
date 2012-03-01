#include "lib.h"
#include <iostream>
#include <fstream>
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
static int socket_init(struct sockaddr_in *seraddr, unsigned int port){
    int listenfd;

    bzero(seraddr,sizeof(&seraddr));
    seraddr->sin_family=AF_INET;
    seraddr->sin_addr.s_addr=htonl(INADDR_ANY);
    seraddr->sin_port=htons(port);

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

int order(int a){
    int k=1;
    while(a/=10){
        k++;
    }
    return k;
}

void server_info(char * buffer, unsigned dst_port,int c_b, int match, char *msg, struct sockaddr_in &cliaddr){
    long int tmp_addr = ntohl(cliaddr.sin_addr.s_addr);
    char ip[40];
    bzero(ip, sizeof(ip));
    char *tmp_addr_str = ip;
    int end_addr_ptr = 0;
    for(int i=0; i<4; i++){
        int addr_tmp_int = (tmp_addr>>((3-i)*8)) & 0xff;
        gcvt( addr_tmp_int , order(addr_tmp_int), tmp_addr_str);
        tmp_addr_str += sizeof(char)*(order(addr_tmp_int))+1;
        if(i!=3){
            *(tmp_addr_str - 1 )='.';
        }
        end_addr_ptr += (sizeof(char)*(order(addr_tmp_int)))+1;
    }
    end_addr_ptr--;
    ip[23]='\0';
    ip[end_addr_ptr++] = '/';
    gcvt(  htons(cliaddr.sin_port), order(htons(cliaddr.sin_port)), tmp_addr_str);
    ip[ end_addr_ptr+order(htons(cliaddr.sin_port)) ]='\0';

    cout<<"Source ip/port:"<<ip<<" "; 
    cout<<"Destination ip:"<<((int)buffer[4]&0xff)<<"."<<((int)buffer[5]&0xff)<<"."<<((int)buffer[6]&0xff)<<"."<<((int)buffer[7]&0xff)<<"/"<<dst_port<<" "; 
    if(c_b){
        cout<<"Command:Bind ";
    }else{
        cout<<"Command:Connect ";
    }
    if(match){
        cout<<"Reply:Accept ";
    }else{
        cout<<"Reply:Reject ";
    }
    if(msg){
        if(strlen(msg) > 0){        
            cout<<"Content:";
            for(int i=0; i<strlen(msg); i++){
                if(i == 15)break;
                cout<<msg[i];
            }
        } 
    }
    cout<<endl;
}


int main(int argc,char **argv,char **env)
{
    int listenfd, pid, clifd;
    socklen_t cliaddr_len, cliaddr2_len;
    struct sockaddr_in seraddr,cliaddr, cliaddr2;
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
    int erno;
    erno = bind(listenfd,(SA*)&seraddr,sizeof(seraddr));
    if(erno == -1){
        cout<<"Bind Error\n";
        exit(1);
    }
    listen(listenfd,0);

    //init
    cliaddr_len = sizeof(cliaddr);
    bzero(msg_buff, 99999);


    /* set environment*/
    //    chdir("/home/Master1_up/NP/hw3/client");    
    //    setenv("PATH","bin:.",1);    

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

                char buffer[100];
                bzero(buffer, sizeof(buffer));
                int s;
                s  = read(clifd, buffer, 100);

                /*Check if packet legal*/
                if(s < 8){
                    cout<<"Length too short!!\n";
                    exit(1);
                }
                if( buffer[0]&0xff !=4){
                    cout<<"Error Version!\n";
                    exit(1);
                }
                if( (buffer[1]&0xff)!=1 && (buffer[1]&0xff)!=2){
                    cout<<"Error CD!\n";
                    exit(1);
                }
                if( (buffer[4]&0xff)>=255 || (buffer[5]&0xff)>=255 || (buffer[6]&0xff)>=255 || (buffer[7]&0xff)>=255){
                    cout<<"Error IP!\n";
                    exit(1);
                }
                if( (buffer[4]&0xff)==127 && (buffer[5]&0xff)==0 && (buffer[6]&0xff)==0 && (buffer[7]&0xff)==1){
                    cout<<"can't connect self\n";
                    exit(1);
                }
                if( (buffer[4]&0xff)==140 && (buffer[5]&0xff)==113 && (buffer[6]&0xff)==179 && (buffer[7]&0xff)==235){
                    cout<<"can't connect self\n";
                    exit(1);
                }
                /*end of check*/

                /*check Domain name*/              
                if(buffer[4]&0xff==0 && buffer[5]&0xff==0 && buffer[6]&0xff==0 ){
                    int ptr;
                    for(ptr=8; ptr<100; ptr++){
                        if(buffer[ptr]==0)break;
                    }
                    ptr++;
                    struct hostent *ip_s;
                    char *tmp_ptr;
                    tmp_ptr = buffer+ptr;
                    ip_s = gethostbyname(tmp_ptr);
                    unsigned long ip_i;
                    memcpy(&ip_i, ip_s->h_addr, sizeof(struct in_addr));
                    for(int k=0; k<4; k++){
                        buffer[4+k] = (ip_i>>(8*k)) & 0xff; 
                    } 
                }
                /*end check*/
                



                unsigned char VN = buffer[0] ;
                unsigned char CD = buffer[1] ;
                unsigned int DST_PORT = (buffer[2]&0xff) << 8 | (buffer[3]&0xff) ;
                unsigned long DST_IP = (buffer[4]&0xff) << 24 | (buffer[5]&0xff) << 16 | (buffer[6]&0xff) << 8 | (buffer[7]&0xff) ;
                unsigned long DST_IP2 = (buffer[7]&0xff) << 24 | (buffer[6]&0xff) << 16 | (buffer[5]&0xff) << 8 | (buffer[4]&0xff) ;
                char* USER_ID = buffer + 8 ;
                fd_size = clifd;
                int clientFD;

                int connect_bind = 0;
                if( (buffer[1]&0xff) == 2){  //Bind Mode
                    connect_bind = 1;
                }

                bool match_c = true;
                bool match_b = true;
                fstream rfd;
                rfd.open("socks.conf");
                string rules;
                getline(rfd, rules);
                while(!rfd.eof()){
                    int p_d_flag;
                    unsigned long ip_rule = 0;
                    unsigned long ip_tmp = DST_IP;
                    string ip_t;
                    ip_t.clear();
                    int c_b_flag = -1; 

                    if( ((int)rules.find("permit")) >=0){
                        p_d_flag = 1;
                    }    
                    else{
                        p_d_flag = 0;
                    }
                    rules = rules.substr(rules.find(' ')+1);

                    if(rules[0] == 'c')
                        c_b_flag = 0;
                    else if(rules[0] == 'b')
                        c_b_flag = 1;

                    ip_t = rules.substr(rules.find(' ')+1, rules.rfind(' ')-rules.find(' ')-1);
                    int mask = 0;
                    for(int count=3; count>=0; count--){                        
                        string ips_tmp;
                        if( ((int)ip_t.find('.')) >= 0){
                            ips_tmp = ip_t.substr(0, ip_t.find('.'));
                            ip_t = ip_t.substr(ip_t.find('.')+1);
                        }
                        else{
                            ips_tmp = ip_t;
                        }

                        if( ((int)ips_tmp.find('*')) >= 0){
                            mask++;
                        }
                        else{
                            int tmp_i = atoi(ips_tmp.c_str());
                            ip_rule |= ((tmp_i&0xff) << (8*count));
                        }
                    }

                    switch(mask){
                        case(1):
                            ip_tmp &= 0xffffff00;
                            break;
                        case(2):
                            ip_tmp &= 0xffff0000;
                            break;
                        case(3):
                            ip_tmp &= 0xff000000;
                            break;
                        case(4):
                            ip_tmp = 0;
                            break;                                
                    }
                    unsigned int port_rule;
                    ip_t = rules.substr(rules.rfind(' ')+1);
                    if( ((int)ip_t.find('*')) >= 0)
                        port_rule = DST_PORT;
                    else
                        port_rule = atoi(ip_t.c_str());


                    if( ((int)(ip_tmp xor ip_rule)==0) && (port_rule==DST_PORT ) && (connect_bind == c_b_flag) ){
                        cout<<"Match rules!\n";
                        if(p_d_flag == 1){
                            if(connect_bind == 1)
                                match_b = true;
                            else
                                match_c = true;
                        }
                        else{
                            if(connect_bind == 1)
                                match_b = false;
                            else
                                match_c = false;
                        }
                        break;
                    }

                    ip_t.clear();
                    rules.clear();
                    getline(rfd, rules);    
                }
                rfd.close();



                if( (buffer[1]&0xff) == 2){  //Bind Mode
                    struct sockaddr_in seraddr_b;
                    int listenfd_dst;
                    int bind_sucess = -1;
                    int dst_port;
                    srand(time(NULL));
                    while(bind_sucess < 0){
                        dst_port = (rand() % 300)+9000;
                        listenfd_dst = socket(AF_INET,SOCK_STREAM,0);
                        socket_init(&seraddr_b, dst_port);
                        bind_sucess = bind(listenfd_dst,(SA*)&seraddr_b,sizeof(seraddr_b));
                    }
                    listen(listenfd_dst,0);      
                    buffer[0] = 0;

                    server_info(buffer, DST_PORT,connect_bind, match_b, NULL, cliaddr);
                    if(match_b){ //Match rules   
//                        cout<<"Bind Mode "<<getpid()<<endl;
                        buffer[1] = (unsigned char) 90;                        
                        buffer[2] = dst_port / 256;
                        buffer[3] = dst_port % 256;
                        buffer[4] = 140; buffer[5] = 113; buffer[6] = 179; buffer[7] = 235; 
                        write(clifd, buffer, 8);
                        clientFD = accept(listenfd_dst,(SA*)&cliaddr2,&cliaddr2_len);
                        write(clifd, buffer, 8);
                        close(listenfd_dst);
                    }
                    else{
                        buffer[1] = (unsigned char) 91;
                        write(clifd, buffer, 8);
                        close(listenfd_dst);
                        close(clifd);
                        break;
                    }                     
                }
                else{                       //Connect Mode
                    struct hostent *he;
                    struct sockaddr_in seraddr_s;
                    clientFD = socket(AF_INET,SOCK_STREAM,0);
                    struct in_addr addr;
                    addr.s_addr = DST_IP2;
                    socket_init_connect(&seraddr_s, &addr, DST_PORT);   
                    int n; 
                    buffer[0] = 0;
                    server_info(buffer, DST_PORT,connect_bind, match_c, NULL, cliaddr);
                    if(match_c){ //Match rules                                                
                        if(n = connect(clientFD, (SA*)&seraddr_s,sizeof(seraddr_s)) <0 ){
                            if(errno != EINPROGRESS){
                                cout<<"error\n";
                                return (-1);
                            }
                        }
//                        cout<<"Connect Mode "<<getpid()<<endl;
                        buffer[1] = (unsigned char) 90;
                        write(clifd, buffer, 8);
                    }
                    else{
                        buffer[1] = (unsigned char) 91;
                        write(clifd, buffer, 8);
                        close(clientFD);
                        close(clifd);
                        break;
                    }                    
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


                usleep(300000);
                while(conn > 0){
                    memcpy(&act_rfd, &all_rfd, sizeof(all_rfd));
                    select( (fd_size+2), &act_rfd, NULL, NULL, NULL);
                    for(int g=0 ; g<2 ; g++){
                        int cliFD = FD[g];
                        if(FD_ISSET(cliFD, &act_rfd) ){ //READ
                            bzero(msg_buff, 99999);
                            int read_size;
                            read_size = read(cliFD, msg_buff, 99999);                            
                            if(read_size == 0 && g == 1){
                                close(FD[1]);
                                close(FD[0]);
                                conn = 0;
                                break;
                            }
                            if(g==1){
//                                cout<<getpid()<<": "<<msg_buff;
                                write(FD[0], msg_buff, read_size);
                            }
                            else{
                                for(int kk=0; kk<strlen(msg_buff); kk++){
                                    if(msg_buff[kk] <= 'z' && msg_buff[kk] >= 'a'){
                                        msg_buff[kk] -= 32;
                                    }
                                }
                                write(FD[1], msg_buff, read_size);
                            }
                        }
                    }
                }

                break;
            }   
            else{
                close(clifd);
                printf("we have client %d\n",clifd);
            }            
        }
        else{
            printf("Server error\n");
            return 1;
        }
    }
}


