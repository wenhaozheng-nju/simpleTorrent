#include "util.h"
#include "btdata.h"

void *listen_peers(void *p)
{
    int listenfd = make_listen_port(g_peerport);
    while(1)
    {
        struct sockaddr_in cliaddr;
        int clilen = sizeof(cliaddr);
        int sockfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
        printf("listen_peer accept sockfd : %d\n",sockfd);
        char *ip = (char*)malloc(17*sizeof(char));
        memset(ip,0,17);
        strcpy(ip, inet_ntoa(cliaddr.sin_addr));
        int port = cliaddr.sin_port;
        printf("\033[31m""I have listen %s:%d\n""\033[m",ip,port);
        int i = 0;
        for(; i < MAXPEERS; i ++)
        {
            peer_t *ptr = &peers_pool[i];
            if(ptr->used == 1 && strcmp(ptr->ip, ip) == 0)
            {
                printf("\033[31m""111\n""\033[m");
                pthread_mutex_lock(&ptr->sock_mutex);
                if(ptr->sockfd < 0)
                {
                    ptr->sockfd = sockfd;
                    pthread_mutex_unlock(&ptr->sock_mutex);
                    break;
                }
                pthread_mutex_unlock(&ptr->sock_mutex);
            }
        }
        if(i != MAXPEERS )
        {
            pthread_t thread;

            int rc = pthread_create(&thread, NULL, recv_from_peer, (void *)i);
            if(rc)
            {
                printf("Error, return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }
        else
        {
            printf("listen_peers close %d\n",sockfd);
            close(sockfd);
            sockfd = -1;
        }
        free(ip);
    }
}
