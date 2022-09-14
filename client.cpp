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
#include <iostream>
#include <fstream>
#include <poll.h>

using namespace std;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buf[1000];
    int rv;
    int numbytes;
    char s[INET6_ADDRSTRLEN];

    ofstream clientLog ("../logs/client_log.txt");


    if (argc != 5) {
        fprintf(stderr,"usage: client hostname serverport remotepath localpath\n");
        exit(1);
    }

    // remote path: /home/CS/users/rcostell/.linux/COS331/adjones-rcostell/destinations/gold.txt
    // local path: /home/CS/users/rcostell/.linux/COS331/adjones-rcostell/vault/filename.txt

    const char* server_port = argv[2];
    const char* remote_path = argv[3];
    const char* local_path = argv[4];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    cout << "GETADDRINFO GOOD" << endl;

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        cout << "LOOPING" << endl;
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    const char* message = "HELLO!";

    if ((numbytes = sendto(sockfd, remote_path, strlen(remote_path), 0,
                           p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    cout << p->ai_addr << endl;

    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);

    printf("listener: waiting to recvfrom...\n");

    int expectedseqnum = 0;
    //int rounds = 50;
    bool waiting = false;
    ofstream file;

    struct pollfd pfds[1]; // More if you want to monitor more

    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN; // Tell me when ready to read

    char resend[1000];

    while(1){
        numbytes = -1;
        addr_len = sizeof their_addr;

        int num_events = poll(pfds, 1, 4000); // 4.0 second timeout

        if (num_events == 0) {

            clientLog << "RESEND ACK" << endl;
            if ((numbytes = sendto(sockfd, resend, strlen(resend), 0,
                                   p->ai_addr, p->ai_addrlen)) == -1) {
                perror("talker: send ACK");
                exit(1);
            }
            printf("RESEND NACK/ACK");
            clientLog<< "SENT ACK" << endl;
            continue;
        }

        clientLog << "num_events: " << num_events << endl;

        if ((numbytes = recvfrom(sockfd, buf, 1000, 0,
                                 (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        printf("client: got packet from %s\n",
               inet_ntop(their_addr.ss_family,
                         get_in_addr((struct sockaddr *) &their_addr),
                         s, sizeof s));
        printf("client: packet is %d bytes long\n", numbytes);
        buf[numbytes] = '\0';
        printf("client: packet contains \"%s\"\n", buf);

        //check for bad file

        if (strcmp(buf, "File could not be opened.") == 0){
            clientLog << "File could not be opened." << endl;
            break;
        }else{
            int pos = 0;
            int seqst = 0;
            int seqen = 0;
            int len = 0;
            bool capture = false;
            string data = "";


            //parse packet
            clientLog<<"BUF :";
            for (int abc = 0; abc < sizeof(buf); abc++) {
                clientLog << buf[abc];
                if(strncmp(buf+abc, "=", 1) ==0){
                    seqst = abc+1;
                }
                if(strncmp(buf+abc, ";", 1) ==0){
                    seqen = abc;
                }
                if(strncmp(buf+abc, "\0",1) ==0 ){
                    len = abc;
                    capture = false;
                    break;
                }
                if(capture){
                    data += buf[abc];
                }
                if(strncmp(buf+abc, "@",1) ==0 ){
                    pos = abc+1;
                    capture = true;
                }
            }
            clientLog<<endl;
            clientLog << "POS: " << pos << endl;

            string recseq;
            int seqlen = seqen-seqst;

            for(int i=0; i<seqlen; i++){
                recseq += buf[seqst+i];
            }

            clientLog << "EXPECTED: " << expectedseqnum << " RECEIVED: " << recseq << endl;
            string ACK = "";
            if(expectedseqnum == stoi(recseq)){
                ACK = "seqnum="+to_string(expectedseqnum)+";data@ACK";
                waiting = false;
            }else{
                ACK = "seqnum="+to_string(expectedseqnum)+";data@NACK";
            }
            int n = ACK.length();
            char response[n];
            memset(response,0, n);
            memcpy(response, ACK.c_str(), n);
            memset(resend,0,1000);
            memcpy(resend, response, sizeof(response));

            clientLog << "ACK RESPONSE: ";
            for (int abc = 0; abc < sizeof(response); abc++) {
                clientLog << response[abc];
            }
            clientLog<< endl;
            if (!waiting){
                if ((numbytes = sendto(sockfd, response, strlen(response), 0,
                                       p->ai_addr, p->ai_addrlen)) == -1) {
                    perror("talker: send ACK");
                    exit(1);
                }
                clientLog<< "SENT ACK" << endl;
                /*int num_events = poll(pfds, 1, 5000); // 5 second timeout
                if (num_events == 0) {
                    printf("Poll timed out!\n");
                    break;
                }*/
            }else{
                continue;
            }


            if(expectedseqnum == stoi(recseq)){
                clientLog << "Correct Seq Num" << endl;

                file.open(local_path, ios::out | ios::app);

                const char *in = data.c_str();
                int nulls = 0;
                if (data.length() == 0){
                    clientLog << "EOF" << endl;
                    break;
                }
                if (data.length() < 8){
                    nulls = 8-data.length();
                }else{
                    nulls = data.length()%8;
                }

                clientLog << "SIZE OF IN: " << sizeof(in) << endl;
                clientLog<< "IN: ";
                for (int abc = 0; abc < sizeof(in); abc++) {
                    clientLog << in[abc];
                }
                clientLog << endl;
                if (file.is_open())
                {
                    clientLog << "EXPECTEDSEQNUM: " << expectedseqnum << endl;
                    //file.seekp(expectedseqnum);
                    clientLog << "POSITION: " <<file.tellp() << endl;
                    clientLog << "NULLS: " << nulls << endl;
                    file.write(in, sizeof(in)-nulls);
                    //file.seekp(0);
                    file.close();
                }
                printf("File saved\n");
                expectedseqnum += (len - pos);

            }else{
                waiting = true;
            }
            //rounds--;
        }
    }



    close(sockfd);

    return 0;
}