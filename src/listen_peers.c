#include "util.h"
#include "btdata.h"

void *listen_peers(void *p){
    int listenfd = make_listen_port(g_peerport);
    while(1){
        struct sockaddr_in cliaddr;
        int clilen = sizeof(cliaddr);
        int sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        pthread_t thread;
         
        int rc = pthread_create(&thread, NULL, recv_from_peer, (void *)sockfd);
        if(rc){
            printf("Error, return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    
    }
}
