#include "util.h"
#include "btdata.h"

void *listen_peers(void *p){
    printf("enter listen_peer in port :%d\n",g_peerport);
    int listenfd = make_listen_port(g_peerport);
    printf("listenfd is %d\n",listenfd);
    while(1){
        struct sockaddr_in cliaddr;
        int clilen = sizeof(cliaddr);
        int sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        printf("have accept :%d\n",sockfd);
        char *ip = (char*)malloc(16*sizeof(char));
        strcpy(ip, inet_ntoa(cliaddr.sin_addr));
        int port = cliaddr.sin_port;
        int i = 0;
        for(; i < MAXPEERS; i ++){
            peer_t *ptr = &peers_pool[i];
            if(ptr->used == 1 && strcmp(ptr->ip, ip) == 0 && ptr->port == port){
                pthread_mutex_lock(&ptr->sock_mutex);
                if(ptr->sockfd < 0){
                    ptr->sockfd = sockfd;
                    pthread_mutex_unlock(&ptr->sock_mutex);
                    break;
                }
                pthread_mutex_unlock(&ptr->sock_mutex);
            }
        }
        if(i != MAXPEERS ){
            pthread_t thread;
            int rc = pthread_create(&thread, NULL, recv_from_peer, (void *)i);
            if(rc){
                printf("Error, return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }
    }
}
