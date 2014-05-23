
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "btdata.h"
#include "bencode.h"

#ifndef UTIL_H
#define UTIL_H

#define MAXLINE 4096


#define MALLOC_PRO(ptr,type,size) {ptr = (type)malloc(size);memset(ptr,0,size);}

int is_bigendian();

// ��һ���������׽��ֽ������ݵĺ���
int recvline(int fd, char **line);
int recvlinef(int fd, char *format, ...);

// ���ӵ���һ̨����, ����sockfd
int connect_to_host(char* ip, int port);

// ����ָ���˿�, ���ؼ����׽���
int make_listen_port(int port);

// �����ļ��ĳ���, ��λΪ�ֽ�
int file_len(FILE* fname);

// ��torrent�ļ�����ȡ����
torrentmetadata_t* parsetorrentfile(char* filename);

// ��Tracker��Ӧ����ȡ���õ�����
tracker_response* preprocess_tracker_response(int sockfd);

// ��Tracker��Ӧ����ȡpeer������Ϣ
tracker_data* get_tracker_data(char* data, int len);
void get_peers(tracker_data* td, be_node* peer_list); // ���溯���ĸ�������
void get_peer_data(peerdata* peer, be_node* ben_res); // ���溯���ĸ�������

// ����һ�����͸�Tracker��HTTP����, ���ظ��ַ���
char* make_tracker_request(int event, int* mlen);

// ��������peer�������ĸ�������
int reverse_byte_orderi(int i);
int make_big_endian(int i);
int make_host_orderi(int i);

// ctrl-c�źŵĴ�����
void client_shutdown(int sig);

// ��announce url����ȡ�����Ͷ˿�����
announce_url_t* parse_announce_url(char* announce);

//��ÿ��peer��������
void *connect_to_peer(void *p);

//��������peer
void *listen_peers(void *p);

//������peer���ձ���
void *recv_from_peer(void *p);

//�������ӷ�Ƭ��Ϣ
int *parse_data_file(torrentmetadata_t *meta_tree,int *num_piece);

//����Bitfield����
void sendBitField(int sockfd);

//���ͱ���alive
void *check_and_keepalive(void *p);

int buffer2file(int index,int length,char *buf);
void file2buffer(int index,int begin,int length,char *buf);

void sendRequest(int k);
void sendPiece(int sockfd, int index, int begin, int len);
void sendHave(int sockfd, int index);
void sendInterested(int sockfd);
void sendUnchoked(int sockfd);
void destroy_peer(int pos);


void sendRequestForEnd(int sockfd,int index);
void sendCancel(int sockfd,int index);
#endif
