#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "util.h"
#include "btdata.h"
#include "bencode.h"

//#define MAXLINE 4096 
// pthread数据

void init()
{
  g_done = 0;
  g_tracker_response = NULL;
}

int main(int argc, char **argv) 
{
  int sockfd = -1;
  char rcvline[MAXLINE];
  char tmp[MAXLINE];
  FILE* f;
  int rc;
  int i;
 
// 注意: 你可以根据自己的需要修改这个程序的使用方法
  if(argc < 3)
  {
    printf("Usage: SimpleTorrent <torrent file> <ip of this machine (XXX.XXX.XXX.XXX form)> [isseed]\n");
    printf("\t isseed is optional, 1 indicates this is a seed and won't contact other clients\n");
    printf("\t 0 indicates a downloading client and will connect to other clients and receive connections\n");
    exit(-1);
  }

  init();
  srand(time(NULL));

  int val[5];
  for(i=0; i<5; i++)
  {
    val[i] = rand();
  }
    g_peerport = rand() % (65535 - 1024) + 1025;   //分配监听peer的端口号
  memcpy(g_my_id,(char*)val,20);       //把五个随机int值拷贝到g_my_id中
  strncpy(g_my_ip,argv[2],strlen(argv[2]));
  g_my_ip[strlen(argv[2])] = '\0';

  g_torrentmeta = parsetorrentfile(argv[1]);
  memcpy(g_infohash,g_torrentmeta->info_hash,20);

  g_filelen = g_torrentmeta->length;
  g_num_pieces = g_torrentmeta->num_pieces;
  g_filedata = (char*)malloc(g_filelen*sizeof(char));

  announce_url_t* announce_info;
  announce_info = parse_announce_url(g_torrentmeta->announce);
  // 提取tracker url中的IP地址
  printf("HOSTNAME: %s\n",announce_info->hostname);
  struct hostent *record;
  //int i_j = strcmp(announce_info->hostname,"114.212.190.188");
  //printf("HOSTNAME is %x\n",announce_info->hostname[15]);
  record = gethostbyname(announce_info->hostname);
  //record = gethostbyname("114.212.190.188");
  if (record==NULL)
  { 
    printf("gethostbyname failed");
    exit(1);
  }
  struct in_addr* address;
  address =(struct in_addr * )record->h_addr_list[0];
  printf("Tracker IP Address: %s\n", inet_ntoa(* address));
  strcpy(g_tracker_ip,inet_ntoa(*address));
  g_tracker_port = announce_info->port;

  free(announce_info);
  announce_info = NULL;

  // 初始化
  // 设置信号句柄
  signal(SIGINT,client_shutdown);

  // 设置监听peer的线程

  // 定期联系Tracker服务器
  int firsttime = 1;
  int mlen;
  char* MESG;
  MESG = make_tracker_request(BT_STARTED,&mlen);
  while(!g_done)
  {
    if(sockfd <= 0)
    {
      //创建套接字发送报文给Tracker
      printf("Creating socket to tracker...\n");
      sockfd = connect_to_host(g_tracker_ip, g_tracker_port);
    }
    
    printf("Sending request to tracker...\n");
    
    if(!firsttime)
    {
      free(MESG);
      // -1 指定不发送event参数
      MESG = make_tracker_request(-1,&mlen);
      printf("MESG: ");
      for(i=0; i<mlen; i++)
        printf("%c",MESG[i]);
      printf("\n");
    }
    send(sockfd, MESG, mlen, 0);
    firsttime = 0;
    
    memset(rcvline,0x0,MAXLINE);
    memset(tmp,0x0,MAXLINE);
    
    // 读取并处理来自Tracker的响应
    tracker_response* tr;
    tr = preprocess_tracker_response(sockfd); 
   
    // 关闭套接字, 以便再次使用
    shutdown(sockfd,SHUT_RDWR);
    close(sockfd);
    sockfd = 0;

    printf("Decoding response...\n");
    char* tmp2 = (char*)malloc(tr->size*sizeof(char)+1);
    memset(tmp2,0,tr->size*sizeof(char)+1);
    memcpy(tmp2,tr->data,tr->size*sizeof(char));

    printf("Parsing tracker data\n");
    g_tracker_response = get_tracker_data(tmp2,tr->size);
    
    if(tmp)
    {
      free(tmp2);
      tmp2 = NULL;
    }

    printf("Num Peers: %d\n",g_tracker_response->numpeers);
    for(i=0; i<g_tracker_response->numpeers; i++)
    {
      //printf("Peer id: %d\n",g_tracker_response->peers[i].id);
      printf("Peer id: ");
      int idl;
      for(idl=0; idl<20; idl++)
        printf("%02X ",(unsigned char)g_tracker_response->peers[i].id[idl]);
      printf("\n");
      printf("Peer ip: %s\n",g_tracker_response->peers[i].ip);
      printf("Peer port: %d\n",g_tracker_response->peers[i].port);
        //为每个新增的peer创建线程
    }
  
    // 必须等待td->interval秒, 然后再发出下一个GET请求
    sleep(g_tracker_response->interval);

  }
 
  // 睡眠以等待其他线程关闭它们的套接字, 只有在用户按下ctrl-c时才会到达这里
  sleep(2);

  exit(0);
}
