#include "util.h"
#include "btdata.h"

void *recv_from_peer(void *p){
    int k =  (int)p;
    peer_t *my_peer = &peers_pool[k];
    int sockfd = my_peer->sockfd;
    char *ip = my_peer->ip;
    int port = my_peer->port;
}
