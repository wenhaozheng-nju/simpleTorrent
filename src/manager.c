#include "util.h"
#include "btdata.h"
#include <errno.h>

extern int errno;

void sendBitField(int sockfd)
{
    //piecesInfo = parse_data_file(g_torrentmeta, &piecesNum);
    int bit_num = piecesNum / 8;
    if(piecesNum % 8 != 0)
        bit_num ++;
    unsigned char *buffer = (unsigned char*)malloc(sizeof(int) + (1 + bit_num) * sizeof(unsigned char));
    memset(buffer, 0, sizeof(int) + (1 + bit_num) * sizeof(unsigned char));
    unsigned char *temp_buffer = buffer;

    int len = 1 + bit_num;     //1位type值
    len = htonl(len);
    printf("len is %x in sendBitField\n",len);
    memcpy(buffer, (char*)&len, 4);
    buffer += sizeof(int);

    *buffer ++ = 5;

    int i = 0;
    char bit_8 = 0x80;
    for(; i < piecesNum; i ++)
    {
        if(piecesInfo[i] == 1)
        {
            (*buffer) = (*buffer) | bit_8;
        }
        bit_8 = bit_8 >> 1;
        if((i+1) % 8 == 0)
        {
            buffer++;
            bit_8 = 0x80;
        }
    }
    printf("temp buffer is %x %x %x %x\n",temp_buffer[0],temp_buffer[1],temp_buffer[2],temp_buffer[3]);

    //从这里开始我就看不懂在干什么了。。。
    subpiecesNum = (int *)malloc(sizeof(int) * piecesNum);
    isSubpiecesReceived = (int **)malloc(sizeof(int *) * piecesNum);
    printf("piece_len is %d\n", g_torrentmeta->piece_len);
    for(i= 0; i < piecesNum; i ++)
    {
        int temp;
        if(i != piecesNum -1)
        {
            temp = g_torrentmeta->piece_len / SUB_PIECE_LEN;
            if(g_torrentmeta->piece_len % SUB_PIECE_LEN != 0)
            {
                temp ++;
            }
        }
        else
        {
            int piece_len = g_filelen % g_torrentmeta->piece_len;
            if(piece_len == 0)
            {
                piece_len = g_torrentmeta->piece_len;
            }
            temp = piece_len / SUB_PIECE_LEN;
            if(piece_len % SUB_PIECE_LEN != 0)
            {
                temp ++;
            }
        }
        subpiecesNum[i] = temp;
        isSubpiecesReceived[i] = (int *)malloc(sizeof(int) * temp);
        int j = 0;
        for(; j < temp; j ++)
        {
            isSubpiecesReceived[i][j] = piecesInfo[i];
        }
    }

    printf("Now I will send BitField pack\n");
    send(sockfd, temp_buffer, sizeof(int) + ntohl(len) * sizeof(unsigned char), 0);
    free(temp_buffer);
}

void *check_and_keepalive(void *p)
{

    int k = (int)p;
    while(1)
    {
        if(peers_pool[k].used == 1 && peers_pool[k].status >= 2)
        {
            pthread_mutex_lock(&peers_pool[k].alive_mutex);
            if(peers_pool[k].alive == 0)
            {
                pthread_mutex_lock(&peers_pool[k].sock_mutex);
                if(peers_pool[k].sockfd > 0)
                {
                    printf("check_and_keepalive close %d\n",peers_pool[k].sockfd);
                    close(peers_pool[k].sockfd);
                    peers_pool[k].sockfd = -1;
                    peers_pool[k].status = 0;
                }
                pthread_mutex_unlock(&peers_pool[k].sock_mutex);
                break;
            }
            else
            {
                int len = 0;
                printf("Now I will send keepalive pack to %s:%d\n", peers_pool[k].ip, peers_pool[k].port);
                send(peers_pool[k].sockfd, (char *)&len, sizeof(int), 0);
            }
            peers_pool[k].alive = 0;
            pthread_mutex_unlock(&peers_pool[k].alive_mutex);
        }
        sleep(120);
    }

}

void sendRequest(int k)
{
    printf("k is %d\n", k);
    peer_t* my_peer = &peers_pool[k];
    int i, requestPiece = -1;
    for(i = 0; i < piecesNum; i ++)
    {
        if(piecesInfo[i] == 0 && my_peer->piecesInfo[i] == 1)
        {
            requestPiece = i;
            break;
        }
    }
    printf("requestPiece is %d\n", requestPiece);
    if(requestPiece >= 0)
    {
        my_peer->isRequest = 1;
        piecesInfo[requestPiece] = 1;
        int j;
        printf("subpiecesNum is %d\n", subpiecesNum[requestPiece]);
        for(j = 0; j < subpiecesNum[requestPiece]; j ++)
        {
            unsigned char *buffer = (char*)malloc(sizeof(int)*4 + sizeof(unsigned char));
            memset(buffer, 0, sizeof(int)*4 + sizeof(unsigned char));
            unsigned char *temp_buffer = buffer;

            int len = 13;
            len = htonl(len);
            memcpy(buffer, (char*)&len, sizeof(int));
            buffer += sizeof(int);
            len = ntohl(len);

            *buffer ++ = (unsigned char)6;

            int index = htonl(requestPiece);
            memcpy(buffer, (char*)&index, sizeof(int));
            buffer += sizeof(int);
            int begin = j * SUB_PIECE_LEN;
            begin = htonl(begin);
            memcpy(buffer, (char*)&begin, sizeof(int));
            buffer += sizeof(int);
            int len1;
            if(j != subpiecesNum[requestPiece] - 1)
            {
                len1 = SUB_PIECE_LEN;
            }
            else
            {
                if(requestPiece != piecesNum - 1)
                {
                    printf("piece_len is %d and filelen is %d\n",g_torrentmeta->piece_len,g_filelen);
                    len1 = g_torrentmeta->piece_len % SUB_PIECE_LEN;
                    if(len1 == 0)
                    {
                        len1 = SUB_PIECE_LEN;
                    }
                }
                else                           //最后一个分片？
                {
                    int piece_len = g_filelen % g_torrentmeta->piece_len;
                    if(piece_len == 0)
                    {
                        piece_len = g_torrentmeta->piece_len;
                    }
                    len1 = piece_len % SUB_PIECE_LEN;
                    if(len1 == 0)
                    {
                        len1 = SUB_PIECE_LEN;
                    }
                }
            }
            len1 = htonl(len1);
            memcpy(buffer, (char*)&len1, sizeof(int));
            //printf("Now I will send Request pack to %s:%d\n", peers_pool[k].ip, peers_pool[k].port);
            //printf("index is %d, begin is %d, len is %d\n",ntohl(index), ntohl(begin), ntohl(len1));
           // printf("send to %d in sendRequest\n",my_peer->sockfd);
            int n = send(my_peer->sockfd, temp_buffer, sizeof(int)*4 + sizeof(char), 0);
            //printf("n is %d\n", n);
            free(temp_buffer);
        }
    }
    printf("now will return from sendRequest\n");
}

void sendPiece(int sockfd, int index, int begin, int len)
{
    unsigned char* send_buff = (unsigned char*)malloc(sizeof(int) * 3 + sizeof(unsigned char) * (1 + len));
    unsigned char* temp_buff = send_buff;

    int send_len = sizeof(int) * 2 + sizeof(char) * (1 + len);
    int send_len_n = htonl(send_len);
    printf("piece pack_len is %x,",send_len_n);
    memcpy(send_buff, (char*)&send_len_n, 4);
    send_buff += 4;
    *send_buff ++ = 7;

    int index_n = htonl(index);
    printf("index is %x,",index_n);
    memcpy(send_buff, (char*)&index_n, 4);
    send_buff += 4;

    int begin_n = htonl(begin);
    printf("begin is %x\n",begin_n);
    memcpy(send_buff, (char*)&begin_n, 4);
    send_buff += 4;

    file2buffer(index, begin, len, send_buff);
    printf("Now I will send piece pack\n");
    send(sockfd, temp_buff, sizeof(int) * 3 + sizeof(unsigned char) * (1 + len), 0);
    g_uploaded += len;
    free(temp_buff);
}

void sendHave(int sockfd, int index)
{
    unsigned char* send_buff = (unsigned char*)malloc(sizeof(int) * 2 + sizeof(unsigned char));
    unsigned char* temp_buff = send_buff;

    int send_len = sizeof(int) + sizeof(char);
    int send_len_n = htonl(send_len);
    memcpy(send_buff, (char*)&send_len_n, 4);
    send_buff += 4;
    *send_buff ++ = 4;

    int index_n = htonl(index);
    strncpy(send_buff, (char*)&index_n, 4);
    printf("Now I will send have pack\n");
    send(sockfd, temp_buff, sizeof(int) * 2 + sizeof(unsigned char), 0);
    free(temp_buff);
}
void sendInterested(int sockfd)
{
    unsigned char *send_buff = (unsigned char *)malloc(sizeof(int) + sizeof(unsigned char));
    unsigned char *temp_buffer = send_buff;

    int send_len = 1;
    send_len = htonl(send_len);
    memcpy(send_buff,(char *)&send_len,4);
    send_buff +=4;
    *send_buff ++ = 2;
    printf("now I will send interested pack\n");
    send(sockfd,temp_buffer,sizeof(int)+sizeof(unsigned char),0);
    free(temp_buffer);
}
void sendUnchoked(int sockfd)
{
    unsigned char *send_buff = (unsigned char *)malloc(sizeof(int) + sizeof(unsigned char));
    unsigned char *temp_buffer = send_buff;

    int send_len = 1;
    send_len = htonl(send_len);
    memcpy(send_buff,(char *)&send_len,4);
    send_buff +=4;
    *send_buff ++ = 1;
    printf("now I will send unchoked pack\n");
    send(sockfd,temp_buffer,sizeof(int)+sizeof(unsigned char),0);
    free(temp_buffer);
    
}

