#include "util.h"
#include "btdata.h"

void sendBitField(int sockfd){
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
            buffer[i] = 1;
        }
        else{
            buffer[i] = 0;
        }
    }

    subpiecesNum = (int *)malloc(sizeof(int) * piecesNum);
    isSubpiecesReceived = (int **)malloc(sizeof(int *) * piecesNum);
    printf("piece_len is %d\n", g_torrentmeta->piece_len);
    for(i= 0; i < piecesNum; i ++){
        int temp;
        if(i != piecesNum -1){
            temp = g_torrentmeta->piece_len / 65536;
            if(g_torrentmeta->piece_len % 65536 == 0){
                temp ++;
            }
        }
        else{
            int piece_len = g_filelen % g_torrentmeta->piece_len;
            if(piece_len == 0){
                piece_len = g_torrentmeta->piece_len;
            }
            temp = piece_len / 65536;
            if(piece_len % 65536 == 0){
                temp ++;
            }
        }
        subpiecesNum[i] = temp;
        isSubpiecesReceived[i] = (int *)malloc(sizeof(int) * temp);
        int j = 0;
        for(; j < temp; j ++){
            isSubpiecesReceived[i][j] = piecesInfo[i];
        }
    }

    printf("Now I will send BitField pack\n");
    send(sockfd, temp_buffer, sizeof(int) + len * sizeof(char), 0);
    free(temp_buffer);
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
                break;
            }
            else{
                int len = 0;
                printf("Now I will send keepalive pack to %s:%d\n", peers_pool[k].ip, peers_pool[k].port);
                send(peers_pool[k].sockfd, &len, sizeof(int), 0);
            }
            peers_pool[k].alive = 0;
            pthread_mutex_unlock(&peers_pool[k].alive_mutex);
        }
        sleep(120);
    }
}

void sendRequest(int k){
    printf("k is %d\n", k);
    peer_t* my_peer = &peers_pool[k];
    int i, requestPiece = -1;
    for(i = 0; i < piecesNum; i ++){
        if(piecesInfo[i] == 0 && my_peer->piecesInfo[i] == 1){
            requestPiece = i;
            break;
        }
    }
    printf("requestPiece is %d\n", requestPiece);
    if(requestPiece >= 0){
        piecesInfo[requestPiece] = 1;
        int j;
        printf("subpiecesNum is %d\n", subpiecesNum[requestPiece]);
        for(j = 0; j < subpiecesNum[requestPiece]; j ++){
            unsigned char *buffer = (char*)malloc(sizeof(int)*4 + sizeof(unsigned char));
            memset(buffer, 0, sizeof(int)*4 + sizeof(unsigned char));
            unsigned char *temp_buffer = buffer;

            int len = 13;
            strncpy(buffer, (char*)&len, 4);
            buffer += sizeof(int);
            
            *buffer ++ = 6;

            int index = requestPiece;
            strncpy(buffer, (char*)&index, 4);
            buffer += sizeof(int);
            int begin = j * 65536;
            strncpy(buffer, (char*)&begin, 4);
            buffer += sizeof(int);  
            int len1;
            if(j != subpiecesNum[requestPiece] - 1){
                len1 = 65536;
            }
            else{
                if(requestPiece != piecesNum - 1){
                    len1 = g_torrentmeta->piece_len % 65536;
                    if(len1 == 0){
                        len1 = 65536;
                    }
                }
                else{
                    int piece_len = g_filelen % g_torrentmeta->piece_len;
                    len1 = piece_len % 65536;
                    if(len1 == 0){
                        len1 = 65536;
                    }
                }
            }
            strncpy(buffer, (char*)&len1, 4);
                
            printf("Now I will send Request pack to %s:%d\n", peers_pool[k].ip, peers_pool[k].port);
            printf("index is %d, begin is %d, len is %d\n",index, begin, len1);

            send(my_peer->sockfd, temp_buffer, sizeof(int)*4 + sizeof(char), 0);
            free(temp_buffer);
        }
        printf("now will return from sendRequest\n");
    }
}

void sendPiece(int sockfd, int index, int begin, int len){
    unsigned char* send_buff = (unsigned char*)malloc(sizeof(int) * 3 + sizeof(unsigned char) * (1 + len));
    unsigned char* temp_buff = send_buff;

    int send_len = sizeof(int) * 2 + sizeof(char) * (1 + len);
    strncpy(send_buff, (char*)&send_len, 4);
    send_buff += 4;
    *send_buff ++ = 7;

    strncpy(send_buff, (char*)&index, 4);
    send_buff += 4;

    strncpy(send_buff, (char*)&begin, 4);
    send_buff += 4;

    file2buffer(index, begin, len, send_buff);
    printf("Now I will send piece pack\n");
    send(sockfd, temp_buff, sizeof(int) * 3 + sizeof(unsigned char) * (1 + len), 0);
    free(temp_buff);
}

void sendHave(int sockfd, int index){
    unsigned char* send_buff = (unsigned char*)malloc(sizeof(int) * 2 + sizeof(unsigned char));
    unsigned char* temp_buff = send_buff;
    
    int send_len = sizeof(int) + sizeof(char);
    strncpy(send_buff, (char*)&send_len, 4);
    send_buff += 4;
    *send_buff ++ = 5;

    strncpy(send_buff, (char*)&index, 4);
    printf("Now I will send have pack\n");
    send(sockfd, temp_buff, sizeof(int) * 2 + sizeof(unsigned char), 0);
    free(temp_buff);
}
