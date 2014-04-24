#include "util.h"
#include "btdata.h"
#include "assert.h"

void sendBitField(int sockfd){
    piecesInfo = parse_data_file(g_torrentmeta, &piecesNum);
    unsigned char *buffer = (unsigned char*)malloc(sizeof(int) + (1 + *piecesNum) * sizeof(unsigned char));
    memset(buffer, 0, sizeof(int) + (1 + *piecesNum) * sizeof(unsigned char));
    unsigned char *temp_buffer = buffer;

    int len = 1 + *piecesNum;
    strncpy(buffer, (char*)&len, 4);
    buffer += sizeof(int);

    *buffer ++ = 5;

    int i = 0;
    for(; i < *piecesNum; i ++){
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
        int i = 0;
        for(; i < MAXPEERS; i ++){
            if(i != k && peers_pool[i].used == 1 && peers_pool[i].status >= 2){
                pthread_mutex_lock(&peers_pool[i].alive_mutex);
                if(peers_pool[i].alive == 0){
                    pthread_mutex_lock(&peers_pool[i].sock_mutex);
                    if(peers_pool[i].sockfd > 0){
                        close(peers_pool[i].sockfd);
                        peers_pool[i].sockfd = -1;
                        peers_pool[i].status = 0;
                    }
                    pthread_mutex_unlock(&peers_pool[i].sock_mutex);
                }
                else{
                    int len = 0;
                    send(peers_pool[i].sockfd, &len, sizeof(int), 0);
                }
                peers_pool[i].alive = 0;
                pthread_mutex_unlock(&peers_pool[i].alive_mutex);
            }
        }
        sleep(120);
    }
}


