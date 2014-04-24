#include "util.h"
#include "btdata.h"

void sendBitField(int sockfd){
    printf("Now I will send BitField pack\n");
    piecesInfo = parse_data_file(g_torrentmeta, &piecesNum);
    unsigned char *buffer = (unsigned char*)malloc(sizeof(int) + (1 + piecesNum) * sizeof(unsigned char));
    memset(buffer, 0, sizeof(int) + (1 + piecesNum) * sizeof(unsigned char));
    unsigned char *temp_buffer = buffer;

    int len = 1 + piecesNum;
    strncpy(buffer, (char*)&len, 4);
    buffer += sizeof(int);

    *buffer ++ = 5;

    int i = 0;
    for(; i < piecesNum; i ++){
        if(piecesInfo[i] == 1){
            buffer[i] = 0x01;
        }
        else{
            buffer[i] = 0x00;
        }
    }

    send(sockfd, temp_buffer, sizeof(int) + len * sizeof(char), 0);
}

void *check_and_keepalive(void *p){
    int k = (int)p;
    while(1){
        if(peers_pool[k].used == 1 && peers_pool[k].status >= 2) {
            pthread_mutex_lock(&peers_pool[k].alive_mutex);
            if(peers_pool[k].alive == 0){
                pthread_mutex_lock(&peers_pool[k].sock_mutex);
                if(peers_pool[k].sockfd > 0){
                    close(peers_pool[k].sockfd);
                    peers_pool[k].sockfd = -1;
                    peers_pool[k].status = 0;
                }
                pthread_mutex_unlock(&peers_pool[k].sock_mutex);
            }
            else{
                int len = 0;
                printf("Now I will send to %s:%d\n", peers_pool[k].ip, peers_pool[k].port);
                send(peers_pool[k].sockfd, &len, sizeof(int), 0);
            }
            peers_pool[k].alive = 0;
            pthread_mutex_unlock(&peers_pool[k].alive_mutex);
        }
        sleep(120);
    }
}



