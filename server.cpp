#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <iostream>
#include <poll.h>

using namespace std;

#define MAXBUFLEN 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[100];
    char ack[100];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    int base, nextseqnum, N;
    
    ofstream serverLog("../logs/server_log.txt");

    if (argc != 2) {
        fprintf(stderr,"usage: server listeningport\n");
        exit(1);
    }
    const char* listening_port = argv[1];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, listening_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    cout << "GETADDRINFO GOOD" << endl;

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        cout<< "looping" << endl;
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, 99, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    serverLog << "from " << &their_addr <<endl;

    printf("listener: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
                     get_in_addr((struct sockaddr *) &their_addr),
                     s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);



    //open file and get length
    ifstream file(buf, ifstream::binary);

    if(file.fail()){
        cout << "BAD FILE" << endl;
        string closed = "File could not be opened.";
        char closedc[closed.length()];
        memcpy(closedc, closed.c_str(), closed.length());

        if ((numbytes = sendto(sockfd, closedc, sizeof(closedc), 0,
                               (struct sockaddr *) &their_addr, addr_len)) == -1) {
            perror("talker: file doesn't open");
            exit(1);
        }
    }else{
        const auto begin = file.tellg();
        file.seekg(0, ios::end);
        const auto end = file.tellg();
        const auto fsize = end-begin;
        file.seekg(0, ios::beg);
        int rfsize = fsize-1;

        int needAcks = -1;

        int difference = 1;
        base = 0;
        nextseqnum = 0;
        N = 4;
        struct pollfd pfds[1]; // More if you want to monitor more

        pfds[0].fd = sockfd;
        pfds[0].events = POLLIN; // Tell me when ready to read

        while(needAcks != 0){

            while(nextseqnum <= (base+(MAXBUFLEN*N)) && difference>0){
                serverLog<<"NEXTSEQ: " << nextseqnum << endl;
                serverLog<<"(base+(MAXBUFLEN*N)): " << (base+(MAXBUFLEN*N)) << endl;
                if (difference>0){
                    if ((rfsize - nextseqnum) > MAXBUFLEN){
                        char payload[MAXBUFLEN];
                        file.seekg(nextseqnum);
                        memset(payload,0, MAXBUFLEN);
                        file.read(payload, MAXBUFLEN);
                        string header = "seqnum="+to_string(nextseqnum)+";data@";
                        nextseqnum += MAXBUFLEN;
                        serverLog << "filesize: " << fsize<< endl;
                        serverLog << "read: " << file.gcount()<< endl;

                        for (int abc = 0; abc < sizeof(payload); abc++) {
                            serverLog << payload[abc];
                        }

                        int n = header.length();
                        char response[n + MAXBUFLEN];
                        memcpy(response, header.c_str(), n);
                        memcpy(&response[n], payload, MAXBUFLEN);
                        //memcpy(&response[n+MAXBUFLEN], "*", 1);
                        int len = sizeof(response);

                        if ((numbytes = sendto(sockfd, response, len, 0,
                                               (struct sockaddr *) &their_addr, addr_len)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }

                        int num_events = poll(pfds, 1, 2500); // 2.5 second timeout

                        serverLog<<endl;
                        printf("server: sent %d bytes to client\n", numbytes);
                        if (num_events == 0) {
                            serverLog << "RESEND PACKET" << endl;
                            printf("Poll timed out!\n");
                            if ((numbytes = sendto(sockfd, response, len, 0,
                                                   (struct sockaddr *) &their_addr, addr_len)) == -1) {
                                perror("talker: sendto");
                                exit(1);
                            }


                            printf("server: re-sent %d bytes to client\n", numbytes);
                            continue;

                        }
                        if (needAcks == -1){
                            needAcks = 1;
                        }else{
                            needAcks++;
                        }
                    }else{
                        serverLog<< "DIFFERENCE:" << (rfsize-nextseqnum) << endl;
                        difference = rfsize-nextseqnum;

                        char payload[difference];
                        file.seekg(nextseqnum);
                        memset(payload,0, (difference));
                        file.read(payload, difference);
                        string header = "seqnum="+to_string(nextseqnum)+";data@";
                        nextseqnum += (difference);
                        serverLog << "filesize: " << fsize<< endl;
                        serverLog << "read: " << file.gcount()<< endl;

                        for (int abc = 0; abc < sizeof(payload); abc++) {
                            serverLog << payload[abc];
                        }

                        int n = header.length();
                        char response[n + difference];
                        memcpy(response, header.c_str(), n);
                        memcpy(&response[n], payload, difference);
                        //memcpy(&response[n+difference], "*", 1);
                        int len = sizeof(response);

                        if ((numbytes = sendto(sockfd, response, len, 0,
                                               (struct sockaddr *) &their_addr, addr_len)) == -1) {
                            perror("talker: sendto");
                            exit(1);
                        }

                        int num_events = poll(pfds, 1, 2500); // 2.5 second timeout

                        serverLog<<endl;
                        printf("server: sent %d bytes to client\n", numbytes);
                        if (num_events == 0) {
                            serverLog << "RESEND PACKET" << endl;
                            printf("Poll timed out!\n");
                            if ((numbytes = sendto(sockfd, response, len, 0,
                                                   (struct sockaddr *) &their_addr, addr_len)) == -1) {
                                perror("talker: sendto");
                                exit(1);
                            }
                            int num_events = poll(pfds, 1, 2500); // 2.5 second timeout
                            serverLog<<endl;
                            printf("server: re-sent %d bytes to client\n", numbytes);
                            continue;
                        }
                        if (needAcks == -1){
                            needAcks = 1;
                        }else{
                            needAcks++;
                        }
                    }
                }

            }

            if ((numbytes = recvfrom(sockfd, ack, 100 - 1, 0,
                                     (struct sockaddr *) &their_addr, &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }

            if(numbytes>0){
                int pos = 0;
                int seqst = 0;
                int seqen = 0;
                bool NACK = false;


                for (int abc = 0; abc < sizeof(ack); abc++) {
                    if(strncmp(ack+abc, "=", 1) ==0){
                        seqst = abc+1;
                    }
                    if(strncmp(ack+abc, ";", 1) ==0){
                        seqen = abc;
                    }
                    if(strncmp(ack+abc, "@",1) ==0 ) {
                        pos = abc + 1;
                        if (strncmp(ack + abc + 1, "N", 1) == 0) {
                            NACK = true;
                        } else {
                            NACK = false;
                        }
                    }
                }

                serverLog << "ACK: ";
                for (int abc = 0; abc < sizeof(ack); abc++) {
                    if(strncmp(ack+abc, "\0", 1) != 0 ){
                        serverLog << ack[abc];
                    }
                }

                string recseq;
                int seqlen = seqen-seqst;

                for(int i=0; i<seqlen; i++){
                    recseq += ack[seqst+i];
                }
                serverLog << "RECSEQ: " << recseq << endl;
                if(stoi(recseq) == rfsize){
                    serverLog<< "ACK EOF" << endl;
                    break;
                }
                if(base == stoi(recseq) && !NACK) {
                    serverLog << "Correct ACK Num" << endl;
                    base += MAXBUFLEN;
                    needAcks--;
                }else{
                    serverLog << "WRONG ACK NUM, RETRANSMITTING" << endl;
                    serverLog << "BASE: " << base << endl;
                    nextseqnum = stoi(recseq);
                    needAcks--;
                    difference = 1;
                    continue;
                }

                serverLog << "BASE: " << base << endl;
            }
        }


    }





    close(sockfd);

    return 0;
}