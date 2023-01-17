#include "TCPRequestChannel.h"

using namespace std;


TCPRequestChannel::TCPRequestChannel (const std::string _ip_address, const std::string _port_no) {
    
    if (_ip_address == ""){

        // struct sockaddr_in server;
        // int server_sock, bind_stat, listen_stat;
        
        sockfd = socket(AF_INET,SOCK_STREAM, 0);

        if (sockfd < 0){
            exit(-1);
        }
        struct sockaddr_in server;
        int server_sock, bind_stat, listen_stat;
        memset(&server, 0, sizeof server);
        server.sin_family = AF_INET;       
        server.sin_addr.s_addr = INADDR_ANY;
	    server.sin_port = htons(stoi(_port_no)) ; 

        if (bind(sockfd, (struct sockaddr*)&server ,sizeof(server)) < 0){
                exit(-1);
        }

        if (listen(sockfd,30) < 0){
            exit(-1);
        }
        //create a socket on the specified 
        //-specify domain,type,and protocol
        //bind the sock to the address
        //mark socket as listening
    }
    else{
        //struct sockaddr_in server_info;
        //int client_sock, connect_stat;

    
        sockfd = socket(AF_INET,SOCK_STREAM, 0);

        if (sockfd < 0){
            exit(-1);
        }

        struct sockaddr_in server_info;
        int client_sock, connect_stat;

        memset(&server_info, 0, sizeof server_info);
        server_info.sin_family=AF_INET;
        server_info.sin_port= htons(stoi(_port_no));

        if(inet_aton(_ip_address.c_str() ,&server_info.sin_addr) < 0){
            exit(-1);
        }

        if (connect(sockfd,(struct sockaddr*)&server_info, sizeof(server_info)) < 0){
            exit(-1);
        }
        // server_info.sin_family = AF_INET;       
        // server_info.sin_addr.s_addr = INADDR_ANY;
	    // server_info.sin_port = htons(stoi(_port_no)) ; 
        //if client
    //create a socket on the specified 
        //-specify domain,type,and protocol
    //connect to socket to the IP addr of the server

    }

   

}

TCPRequestChannel::TCPRequestChannel (int _sockfd) {
    this->sockfd=_sockfd;
}

TCPRequestChannel::~TCPRequestChannel () {
    //close the sockfd
    close(sockfd);
}

int TCPRequestChannel::accept_conn () {
    //struct sockaddr_storage
    //implementing accept(...)-retval the sockfd of client
    return accept(sockfd,nullptr, nullptr);
}


//read/write , recv/send
int TCPRequestChannel::cread (void* msgbuf, int msgsize) {
    return read(sockfd, msgbuf, msgsize); 
}

int TCPRequestChannel::cwrite (void* msgbuf, int msgsize) {
    return write(sockfd, msgbuf, msgsize);
}
