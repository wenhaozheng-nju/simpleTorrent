#include "util.h"
#include "btdata.h"

void *connect_to_peer(void *p){
    int k = (int)p;
    peer_t *mypeer = &peers_pool[k];
    mypeer->sockfd = connect_to_host(mypeer->ip, mypeer->port);
    char *shkhdmsg;
    char *current;
    int msglen = 0;

    shkhdmsg = (char*)malloc(HANDSHAKE_LEN * sizeof(char));
    current = shkhdmsg;

    char pstrlen = (char)strlen(BT_PROTOCOL);
    *current++ = pstrlen;
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

    int times = 0;
    while(times < 5){
        if(mypeer->status == 0){
            send(mypeer->sockfd, shkhdmsg, msglen, 0);
        }
        else{
            break;
        }
        sleep(1);
        times ++;
    }
    if(mypeer->status == 1){
        
    }
    else{
        printf("Fail to connect to peer at %s:%d", mypeer->ip, mypeer->port);
    }

}
