#include<iostream>
#include"lib.h"
#include<vector>
#include<fstream>
#include<string>
using namespace std;
#define SA struct sockaddr
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3


class ClientInfo{
    public:
        string info[3];
        int cliFD;
        struct sockaddr_in seraddr;
        fstream fp;
        int can_write;
        string wrt_buff;
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


    //    execvp("cat", argv);  

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
//    strcpy(kk, "h1=&p1=&f1=&h2=192.168.55.55&p2=9192&f2=big.txt&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=");    
//   strcpy(kk, "h1=192.168.55.55&p1=9195&f1=big.txt&h2=192.168.55.55&p2=9196&f2=t2.txt&h3=192.168.55.55&p3=9198&f3=t1.txt&h4=192.168.55.55&p4=9205&f4=t4.txt&h5=192.168.55.55&p5=9201&f5=t5.txt ");
//    strcpy(kk,"h1=192.168.55.55&p1=9191&f1=t3.txt&h2=192.168.55.55&p2=9191&f2=t2.txt&h3=192.168.55.55&p3=9191&f3=t1.txt&h4=192.168.55.55&p4=9191&f4=t4.txt&h5=192.168.55.55&p5=9191&f5=t5.txt");   
    string data; 
    data.assign(kk);


    for(int i=0; i<5; i++){                   
        for(int k=0; k<3; k++){
            string substr;
            int pos = data.find("&");
            if(pos > 0){                
                substr = data.substr(0,pos);
                data.erase(0, pos+1);
                int pos_equal = substr.find("=");               
                cli_info[i].info[k] = substr.substr(pos_equal+1);
            }
            else{
                int pos_equal = data.find("=");            
                cli_info[i].info[k] = data.substr(pos_equal+1);
            }
        }
    }

    socklen_t n;

    for(int i=0; i<5; i++){
        cli_info[i].cliFD = -1;
        if(cli_info[i].info[0].size()>0){
            int clientFD = socket(AF_INET,SOCK_STREAM,0);
            int flags = fcntl(clientFD, F_GETFL, 0);
            fcntl(clientFD, F_SETFL, flags | O_NONBLOCK);
            he = gethostbyname(cli_info[i].info[0].c_str());
            socket_init(&cli_info[i].seraddr, he, cli_info[i].info[1].c_str());    
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


    int statusA = F_CONNECTING;

//    cout<<cli_info[4].cliFD<<endl;    
//    close(1);

    while(conn > 0){
        memcpy(&act_rfd, &all_rfd, sizeof(all_rfd));
        memcpy(&act_wfd, &all_wfd, sizeof(all_wfd));

        select( (fd_size+1), &act_rfd, &act_wfd, NULL, NULL);
        /*        if( conn != write_complete ){
                  if( select(fd_size+1, &act_rfd, &act_wfd, NULL, NULL) < 0 ) exit(0);
                  }
                  else{
                  if( select(fd_size+1, &act_rfd, NULL, NULL, NULL) < 0 ) exit(0);            
                  }*/

        int cliFD;
        for(int i=0; i<5; i++){            
            if( (cliFD = cli_info[i].cliFD) == -1)
                continue;           
            int error; 
//            if (getsockopt(cliFD, SOL_SOCKET, SO_ERROR, &error, &n) < 0 ) {return (-1);}
            if(FD_ISSET(cliFD, &act_rfd)){ //READ
                bzero(read_buff, sizeof(read_buff));
                int r_s;
                r_s = read(cliFD, read_buff, sizeof(read_buff));                
                int rflag = 0;                        
                string tmp_buff(read_buff);
                int h;
                if( (h =tmp_buff.find('%')) >= 0){
                    rflag = 1;
//                    cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \"<br/>"<<tmp_buff.find('%')<<"<br/>\";</script>"<<endl;
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
//                                cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \"<b>!!!!"<<re_s<<"!!!!</b>\";</script>"<<endl;                                 
                                cli_info[i].wrt_buff.append( tmp_buff );
                            }
//                            cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \"<b>!!!!"<<re_s<<"####</b>\";</script>"<<endl;
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


                        cout<<"<script>document.all[\"m"<<i<<"\"].innerHTML += \"<u>"<<tmp_buff<<"</u>\";</script>"<<endl;
                        if(!tmp_buff.find("exit")){
//                            fprintf(stderr, "over %d\n", i);
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
        }
    }
    cout<<"</font></body></html>"<<endl;

    return 0;    
}
