#include "util.h"
#include "btdata.h"
#include <errno.h>

#define BUFSIZE 1500

extern int errno;

void sendshkhdmsg(int sockfd)
{
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
    for(; i < 5; i ++)
    {
        int j = 0;
        int part = reverse_byte_orderi(g_infohash[i]);
        unsigned char *p = (unsigned char*)&part;
        for(; j < 4; j ++)
        {
            *current++ = p[j];
        }
    }
    for(i = 0; i < 20; i ++)
    {
        *current = g_my_id[i];
        current ++;
    }

    msglen = current - shkhdmsg1;
    send(sockfd, shkhdmsg1, msglen, 0);
    free(shkhdmsg1);
    shkhdmsg1 = NULL;
    current = NULL;
}

void *recv_from_peer(void *p)
{
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
    int n;

    unsigned char *buffer = (unsigned char*)malloc(BUFSIZE);

    while(1)
    {
        memset(buffer, 0, BUFSIZE);
        printf("now I waiting recv\n");
        n = recv(sockfd, buffer, 4, 0);
        if(n <= 0)
        {
            printf("recv length error\n");
            break;
        }
        int len = *(int*)buffer;
        len = ntohl(len);
        if(len > BUFSIZE)
        {
            printf("\033[33m len is %d and ",len);
            printf("len is bigger than BUFSIZE\n \033[m");
            break;
        }
        /*
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
                        sendshkhdmsg(my_peer->sockfd);
                    }
                    my_peer->status = 2;
                }
                printf("shake hands succeed\n");
                sendBitField(my_peer->sockfd);
                pthread_t thread;
                pthread_create(&thread, NULL, check_and_keepalive, (void*)k);
            }
            else
            {
                perror("torrent file dismatched\n");
                exit(-1);
            }
        }*/
        if(len == 0)
        {
            //keepalive
            printf("Now I recv keepalive pack from %s:%d\n", my_peer->ip, my_peer->port);
            my_peer->alive = 1;
        }
        else
        {
            printf("len is %d before recv buf\n",len);
            n = recv(sockfd, buffer, len, 0);
            if(n<=0)
            {
                printf("recv buffer error\n");
                break;
            }

            unsigned char id = buffer[0];
            switch(id)
            {
            case 0:
            {
                //choke
                break;
            }
            case 1:
            {
                //unchoke
                break;
            }
            case 2:
            {
                //interested
                break;
            }
            case 3:
            {
                //not interested
                break;
            }
            case 4:
            {
                //have
                int index = *(int*)&buffer[1];
                my_peer->piecesInfo[index] = 1;
                break;
            }
            case 5:
            {
                //bitfield
                assert(len > 0);
                printf("Now I recv bitfield pack from %s:%d\n", my_peer->ip, my_peer->port);
                char *bit_8 = buffer+1;
                int *bit_array = (int *)malloc(sizeof(int)*8*(len-1));
                memset(bit_array,0,sizeof(int)*8*(len-1));
                int i;
                for(i=0; i<(len-1)*8; i++)
                {
                    if((*bit_8 & 0x80) != 0)
                    {
                        printf("hit ");
                        bit_array[i] = 1;
                    }
                    else
                        bit_array[i] = 0;
                    *bit_8 = *bit_8 << 1;
                    if((i+1) % 8 == 0)
                        bit_8++;
                }
                assert((len-1)*8 >= piecesNum);
                printf("piecesNum is %d\n",piecesNum);
                my_peer->piecesInfo = (int*)malloc(piecesNum * sizeof(int));
                memset(my_peer->piecesInfo,0,piecesNum*sizeof(int));
                for(i=0; i<piecesNum; i++)
                    my_peer->piecesInfo[i] = bit_array[i];
                free(bit_array);
                /*
                {
                    my_peer->piecesInfo[i - 1] = buffer[i];
                }
                */

                printf("%s:%d has pieces:", my_peer->ip, my_peer->port);
                for(i = 0; i < piecesNum; i ++)
                {
                    printf("%d ", my_peer->piecesInfo[i]);
                }
                printf("\n");
                //send interested
                //send request
                sendRequest(k);
                break;
            }
            case 6:
            {
                //request
                int index = *(int*)&buffer[1];
                index = ntohl(index);
                int begin = *(int*)&buffer[5];
                begin = ntohl(begin);
                int blocklen = *(int*)&buffer[9];
                blocklen = ntohl(blocklen);
                sendPiece(my_peer->sockfd, index, begin, blocklen);
                break;
            }
            case 7:
            {
                //piece
                int index = *(int*)&buffer[1];
                int begin = *(int*)&buffer[5];
                int blocklen = len - sizeof(char) - sizeof(int)*2;
                buffer2file(index, begin, blocklen, buffer + 9);
                int subPieceNo = begin / 65536;
                assert(piecesInfo[index] == 1);
                isSubpiecesReceived[index][subPieceNo] = 1;
                int flag = 1;
                int m = 0;
                for(; m < subpiecesNum[index]; m ++)
                {
                    if(isSubpiecesReceived[index][m] == 0)
                    {
                        flag = 0;
                    }
                }
                if(flag == 1)
                {
                    sendHave(my_peer->sockfd, index);
                    sendRequest(k);
                }
                break;
            }
            case 8:
            {
                //cancel
                break;
            }
            }
        }
    }
    printf("recv n is %d\n", n);
    if(n < 0)
    {
        printf("errno is %d:%s\n", errno, strerror(errno));
    }
    free(buffer);
    printf("connect broke\n");
    printf("sockfd is %d\n",sockfd);
    pthread_mutex_lock(&my_peer->sock_mutex);
    if(my_peer->sockfd > 0)
    {
        close(sockfd);
        destroy_peer(k);
    }
    pthread_mutex_unlock(&my_peer->sock_mutex);
}
