#include<iostream>
#include"lib.h"
#include<vector>
#include<fstream>
#include<string>
using namespace std;
#define SA struct sockaddr


class ClientInfo{
    public:
        string info[5];
        int cliFD;
        struct sockaddr_in seraddr;
        fstream fp;
        int can_write;
        string wrt_buff;
        bool first;
};


static int socket_init(struct sockaddr_in *seraddr,struct hostent *he, const char* port){
    int listenfd;
    bzero(seraddr,sizeof(&seraddr));
    seraddr->sin_family=AF_INET;
    seraddr->sin_addr= *((struct in_addr*)he->h_addr);
    seraddr->sin_port=htons(atoi(port));
    return listenfd;
}

int main(int argc, char **argv){
    char kk[300];
    ClientInfo cli_info[6];  // [][0] is ip, [][1] is port, [][2] is file name
    int listenfd,cliaddr_len,pid;
    struct sockaddr_in cliaddr;
    struct hostent *he;
    char read_buff[1024];



    /*INIT FD SET*/
    fd_set all_rfd, all_wfd;
    fd_set act_rfd, act_wfd;
    int conn = 0;
    int fd_size = 0;
    FD_ZERO(&all_rfd); 
    FD_ZERO(&all_wfd); 
    FD_ZERO(&act_rfd); 
    FD_ZERO(&act_wfd); 


    chdir("/home/Master1_up/NP/hw3/client");

    
    bzero(kk, sizeof(kk));
    strcpy(kk, getenv("QUERY_STRING")); 
//    strcpy(kk, "h1=192.168.55.55&p1=8100&f1=t2.txt&sh1=192.168.55.55&sp1=8000&h2=&p2=&f2=&sh2=&sp2=&h3=&p3=&f3=&sh3=&sp3=&h4=&p4=&f4=&sh4=&sp4=&h5=&p5=&f5=&sh5=&sp5="); 
//    strcpy(kk, "h1=192.168.55.55&p1=8100&f1=t2.txt&sh1=192.168.55.55&sp1=8000&h2=192.168.55.55&p2=8101&f2=t2.txt&sh2=192.168.55.55&sp2=8000&h3=192.168.55.55&p3=8103&f3=t3.txt&sh3=192.168.55.55&sp3=8000&h4=192.168.55.55&p4=8106&f4=t2.txt&sh4=192.168.55.55&sp4=8000&h5=192.168.55.55&p5=8110&f5=t5.txt&sh5=192.168.55.55&sp5=8000");
    string data; 
    data.assign(kk);


    for(int i=0; i<5; i++){                   
        for(int k=0; k<5; k++){
            string substrs;
            int pos = data.find("&");
            if(pos > 0){                
                substrs = data.substr(0, pos);
                data.erase(0, pos+1);
                int pos_equal = substrs.find("=");          
                if(substrs.size() > (pos_equal+1) ) 
                    cli_info[i].info[k] = substrs.substr(pos_equal+1);
            }
            else{
                int pos_equal = data.find("=");            
                cli_info[i].info[k] = data.substr(pos_equal+1);
            }
        }
    }
    data.clear();
    bzero(kk, sizeof(kk));
    

    socklen_t n;
    /*Initialize and Connect to server or Socks*/
    for(int i=0; i<5; i++){
        cli_info[i].cliFD = -1;
        cli_info[i].first = true;
        if(cli_info[i].info[0].size()>0){
            int clientFD = socket(AF_INET,SOCK_STREAM,0);
            int flags = fcntl(clientFD, F_GETFL, 0);
            fcntl(clientFD, F_SETFL, flags | O_NONBLOCK);

            /*Set connect to Server or Socks*/
            if(cli_info[i].info[3].size() == 0){
                he = gethostbyname(cli_info[i].info[0].c_str());
                socket_init(&cli_info[i].seraddr, he, cli_info[i].info[1].c_str());
            }
            else{
                he = gethostbyname(cli_info[i].info[3].c_str());
                socket_init(&cli_info[i].seraddr, he, cli_info[i].info[4].c_str());
            }

            if(n = connect(clientFD, (SA*)&cli_info[i].seraddr,sizeof(cli_info[i].seraddr)) <0 ){
                if(errno != EINPROGRESS)return (-1);
            }
            cli_info[i].cliFD = clientFD;
            cli_info[i].can_write = 0;
            cli_info[i].fp.open(cli_info[i].info[2].c_str(), fstream::in);            
            fd_size = clientFD>fd_size?clientFD:fd_size;
            FD_SET(clientFD, &all_rfd);
            FD_SET(clientFD, &all_wfd);
            conn++;
        }
    }
    

    /*Output Html Header */
    cout<<"Content-type:text/html\n\n";
    cout<<"<html><head>"<<endl;
    cout<<"<meta http-equiv='Content-Type' content='text/html; charset=big5' />"<<endl;
    cout<<"<title>Network Programming Homework 3</title>"<<endl;
    cout<<"<link rel=\"stylesheet\" type=\"text/css\" href=\"http://140.113.179.235:2222/style.css\" />"<<endl;
    cout<<"<body ><table width='800' border='1'><tr>"<<endl;    
    for(int i=0; i<5; i++){
        if(cli_info[i].cliFD != -1){
            cout<<"<td>"<<cli_info[i].info[0]<<"</td>"<<endl;
        }
    }
    cout<<"</tr><tr>"<<endl;
    for(int i=0; i<5; i++){
        if(cli_info[i].cliFD != -1){
            cout<<"<td valign='top' id='m"<<i<<"'></td>"<<endl;
        }
    }
    fflush(NULL);
    cout<<"</tr></table>"<<endl;

    act_rfd = all_rfd;
    act_wfd = all_wfd;
    int write_complete = 0;



    while(conn > 0){
        memcpy(&act_rfd, &all_rfd, sizeof(all_rfd));
        memcpy(&act_wfd, &all_wfd, sizeof(all_wfd));
        select( (fd_size+1), &act_rfd, &act_wfd, NULL, NULL);
        int cliFD;
        for(int i=0; i<5; i++){            
            if( (cliFD = cli_info[i].cliFD) == -1)
                continue;           
            int error; 
            if(FD_ISSET(cliFD, &act_rfd)){ //READ
                bzero(read_buff, sizeof(read_buff));
                int r_s;
                r_s = read(cliFD, read_buff, sizeof(read_buff));                
                int rflag = 0;                        
                string tmp_buff(read_buff);
                int h;
                if( (h =tmp_buff.find('%')) >= 0){
                    rflag = 1;
                }
                int g;
                while( (g = tmp_buff.find('\n')) >= 0){
                    if(g!=0 && tmp_buff[g-1] == '\r'){
                        tmp_buff.replace(g-1, 1, "<");
                        tmp_buff.replace(g, 1, "b");
                        tmp_buff.insert(g+1,"r/>");
                    }
                    else{
                        tmp_buff.replace(g, 1, "<");
                        tmp_buff.insert(g+1,"br/>");
                    }
                }
                while( (g = tmp_buff.find('\r')) >= 0){
                    tmp_buff.replace(g, 1, "<");
                    tmp_buff.insert(g+1,"br/>");
                }
                while( (g = tmp_buff.find('\"')) >= 0){
                    tmp_buff.replace(g, 1, "'");                   
                }


                if( rflag == 1){
                    cli_info[i].can_write = 1;
                }

                if(tmp_buff.size() > 0){
                    cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \""<<tmp_buff<<"\";</script>"<<endl;
                }

                if(cli_info[i].can_write == 1){
                    if(cli_info[i].fp){
                        tmp_buff.clear();           
                        getline(cli_info[i].fp, tmp_buff);
                        

                        if(tmp_buff[tmp_buff.size()-1] != '\n'){                                                
                            tmp_buff.push_back('\n');
                        }
                        int re_s;
                        re_s = write(cliFD, tmp_buff.c_str(), tmp_buff.size()  );
                        if(re_s < tmp_buff.size()){
                            if(re_s != -1){
                                cli_info[i].wrt_buff.append( tmp_buff.substr(re_s) );
                            }
                            else{
                                cli_info[i].wrt_buff.append( tmp_buff );
                            }
                        }
                        int g;
                        while( (g = tmp_buff.find('\n')) >= 0){
                            if(g!=0 && tmp_buff[g-1] == '\r'){
                                tmp_buff.replace(g-1, 1, "<");
                                tmp_buff.replace(g, 1, "b");
                                tmp_buff.insert(g+1,"r/>");
                            }
                            else{
                                tmp_buff.replace(g, 1, "<");
                                tmp_buff.insert(g+1,"br/>");
                            }
                        }
                        while( (g = tmp_buff.find('\r')) >= 0){
                            tmp_buff.replace(g, 1, "<");
                            tmp_buff.insert(g+1,"br/>");
                        }


                        cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \"<b>"<<tmp_buff<<"</b>\";</script>"<<endl;
                        if(!tmp_buff.find("exit")){
                            cli_info[i].fp.close();
                            FD_CLR(cli_info[i].cliFD, &all_wfd);
                            write_complete++; 
                            FD_CLR(cli_info[i].cliFD, &all_rfd);
                            close(cliFD);
                            cli_info[i].cliFD = -1;
                            cli_info[i].can_write = 0;
                            conn--;
                        }
                    }
                    cli_info[i].can_write = 0;
                }


            }

            if(FD_ISSET(cliFD, &act_wfd) &&  (!cli_info[i].wrt_buff.empty()) ){ //WRITE 
                int re_s;
                re_s = write(cli_info[i].cliFD, cli_info[i].wrt_buff.c_str(), cli_info[i].wrt_buff.size());
                if(re_s < cli_info[i].wrt_buff.size()){
                    if(re_s != -1){
                        cli_info[i].wrt_buff = cli_info[i].wrt_buff.substr(re_s);
                    }                                        
                }               
                else{
                    cli_info[i].wrt_buff.clear();
                }
            }

            /*Request to Socks server*/
            if(FD_ISSET(cliFD, &act_wfd) &&  cli_info[i].first == true && cli_info[i].info[4].size() > 0){ //WRITE 
                cli_info[i].first = false;
                char buffer[20];
                bzero(buffer, 20);
                buffer[0] = 4;
                buffer[1] = 1;                   //Connect protocal
                int port = atoi(cli_info[i].info[1].c_str());
                buffer[2] = port / 256;
                buffer[3] = port % 256;
                string t_ip(cli_info[i].info[0]);

                for(int ip_counter = 0 ; ip_counter<4; ip_counter++){
                    string ip_str;
                    int t_ptr = -1;
                    t_ptr = t_ip.find(".");
                    if(t_ptr >= 0){
                        ip_str.append(t_ip.substr(0, t_ptr));
                        t_ip = t_ip.substr(t_ip.find(".")+1);
                    }else{
                        ip_str.append(t_ip);
                    }
                    buffer[4+ip_counter] = ((atoi(ip_str.c_str()))&0xff);
                    ip_str.clear();
                }
                write(cliFD, buffer, 9);
//                read(cliFD, read_buff, sizeof(read_buff));                
                bzero(read_buff, sizeof(read_buff));
            }            

        }
    }


    cout<<"</font></body></html>"<<endl;

    return 0;    
}
