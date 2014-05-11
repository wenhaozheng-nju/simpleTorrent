
#include "util.h"
#include "btdata.h"

void *connect_to_peer(void *p){
    int k = (int)p;
    peer_t *mypeer = &peers_pool[k];
    pthread_mutex_lock(&mypeer->sock_mutex);
    if(mypeer->sockfd < 0){
        printf("\033[32m""I connect to somebody\n""\033[m");
        mypeer->sockfd = connect_to_host(mypeer->ip, mypeer->port);
        pthread_t thread;
        int rc = pthread_create(&thread, NULL, recv_from_peer, (void *)k);
        if(rc){
            printf("ERROR, return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    pthread_mutex_unlock(&mypeer->sock_mutex);
    unsigned char *shkhdmsg;
    unsigned char *current;
    int msglen = 0;

    shkhdmsg = (char*)malloc((HANDSHAKE_LEN +1) * sizeof(char));
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
        *current = g_my_id[i];
        current ++;
    }

    msglen = current - shkhdmsg;
    if(mypeer->status == 0){
        int n_a = send(mypeer->sockfd, shkhdmsg, msglen, 0);
        mypeer->status = 1;
    } 
    free(shkhdmsg);
    shkhdmsg = NULL;
    current = NULL;

}
