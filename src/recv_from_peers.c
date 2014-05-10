#include "util.h"
#include "btdata.h"

#define BUFSIZE 1500

void sendshkhdmsg(int sockfd){
    char *shkhdmsg;
    char *current;
    int msglen = 0;

    shkhdmsg = (char*)malloc(HANDSHAKE_LEN * sizeof(char));
    current = shkhdmsg;

    int pstrlen = strlen(BT_PROTOCOL);
    memcpy(current, (char*)&pstrlen, sizeof(int));
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
        current += sprintf(current, "%02x", (unsigned char)g_my_id[i]);
    }

    msglen = current - shkhdmsg;
    send(sockfd, shkhdmsg, msglen, 0);
    free(shkhdmsg);
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

    char *buffer = (char*)malloc(BUFSIZE);
    memset(buffer, 0, BUFSIZE);

    while(1){
        int n = recv(sockfd, buffer, 4, 0);
        if(n <= 0)
            break;
        int len = *(int*)buffer;
        printf("\033[31m""recv peer wire proto len is %d\n""\033[m" , len);
        
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
            if(flag == 1){
                memset(buffer, 0, BUFSIZE);
                n = recv(sockfd, buffer, 20, 0);
                if(n<=0)
                    break;
                strncpy(my_peer->id, buffer, 20);
                if(my_peer->status != 2){
                    if(my_peer->status == 0){
                        sendshkhdmsg(my_peer->sockfd);
                    }
                    my_peer->status = 2;
                }

                printf("shake hands succeed\n");
            }
        }
        else{
            //其他类型
        }
    }
}
