#include "util.h"
#include "btdata.h"

#define BUFSIZE 1500

void sendshkhdmsg(int sockfd){
    printf("\033[34m""I will send shkhdmsg to somebody\n""\033[m");
    unsigned char *shkhdmsg1;
    unsigned char *current;
    int msglen = 0;
    shkhdmsg1 = (unsigned char* )malloc(HANDSHAKE_LEN);
    current = shkhdmsg1;
    int pstrlen = strlen(BT_PROTOCOL);
    memcpy(current, (unsigned char*)&pstrlen, sizeof(int));
    current += sizeof(int);
    strncpy(current, BT_PROTOCOL, pstrlen);
    current += pstrlen;

    memset(current, 0, 8);
    current += 8;

    int i = 0;
    for(; i < 5; i ++){
        int j = 0;
        int part = reverse_byte_orderi(g_infohash[i]);
        unsigned char *p = (unsigned char*)&part;
        for(; j < 4; j ++){
            *current++ = p[j];
        }
    }
    for(i = 0; i < 20; i ++){
        *current = g_my_id[i];
        current ++;
    }

    msglen = current - shkhdmsg1;
    send(sockfd, shkhdmsg1, msglen, 0);
    free(shkhdmsg1);
    shkhdmsg1 = NULL;
    current = NULL;
}

void *recv_from_peer(void *p){
    int k =  (int)p;
    peer_t *my_peer = &peers_pool[k];
    if(peers_pool[k].used == 0)
    {
        perror("监听的peer不是我感兴趣的\n");
        exit(-1);
    }
    int sockfd = my_peer->sockfd;
    char *ip = my_peer->ip;
    int port = my_peer->port;

    unsigned char *buffer = (unsigned char*)malloc(BUFSIZE * sizeof(unsigned char));

    while(1){
        memset(buffer, 0, BUFSIZE);
        printf("now I waiting recv\n");
        int n = recv(sockfd, buffer, 4, 0);
        if(n <= 0)
            break;
        int len = *(int*)buffer;
        
        memset(buffer, 0, BUFSIZE);
        n = recv(sockfd, buffer, len, 0);
        if(n<=0)
            break;
        if(len == 19 && strcmp(buffer, BT_PROTOCOL) == 0){
            //握手报文
            memset(buffer, 0, BUFSIZE);
            n = recv(sockfd, buffer, 8, 0);
            if(n<=0)
                break;

            memset(buffer, 0, BUFSIZE);
            n = recv(sockfd, buffer, 20, 0);
            if(n<=0)
                break;
            int i = 0, flag = 1;
            unsigned char *buffer_temp = buffer;
            for(; i < 5; i ++){
                int j = 0;
                int part = reverse_byte_orderi(g_infohash[i]);
                unsigned char *p = (unsigned char*)&part;
                for(; j < 4; j ++){
                    if(*buffer != p[j]){
                        flag = 0;
                        break;
                    }
                    buffer ++;
                }
            }
            buffer = buffer_temp;
            if(flag == 1){
                memset(buffer, 0, BUFSIZE);
                n = recv(sockfd, buffer, 20, 0);
                if(n<=0)
                    break;
                strncpy(my_peer->id, buffer, 20);
                if(my_peer->status != 2){
                    if(my_peer->status == 0){
                        int *f = (int *)malloc(455*sizeof(int));
                        sendshkhdmsg(my_peer->sockfd);
                        free(f);
                    }
                    my_peer->status = 2;
                }
                printf("shake hands succeed\n");
                sendBitField(my_peer->sockfd);
                pthread_t thread;
                pthread_create(&thread, NULL, check_and_keep_alive, (void*)k);
            }
            else
            {
                perror("torrent file dismatched\n");
                exit(-1);
            }
        }
        else{
            //其他类型
            if(len == 0){
                //keepalive
                my_peer->alive = 1;
            }
            else{
                unsigned char id = buffer[0];
                switch(id){
                    case 0:{
                    //choke
                    break;}
                    case 1:{
                    //unchoke
                    break;}
                    case 2:{
                    //interested
                    break;}
                    case 3:{
                    //not interested
                    break;}
                    case 4:{
                    //have
                    break;}
                    case 5:{
                        //bitfield
                        int pNum = len -1;
                        my_peer->pieceInfo = (int*)malloc(pNum * sizeof(int));
                        int i = 1;
                        for(; i <= pNum; i ++){
                            my_peer->pieceInfo[i - 1] = buffer[i];
                        }
                        //other operation
                    break;}
                    case 6:{
                    //request
                    break;}
                    case 7:{
                    //piece
                    break;}
                    case 8:{
                    //cancel
                    break;}
                }
            }
        }
    }
    free(buffer);
    pthread_mutex_lock(&my_peer->sock_mutex);
    if(my_peer->sockfd > 0){
        close(sockfd);
        my_peer->sockfd = -1;
        my_peer->status = 0;
    }
    pthread_mutex_unlock(&my_peer->sock_mutex);
}
