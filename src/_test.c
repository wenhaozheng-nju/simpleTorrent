/*
 * =====================================================================================
 *
 *       Filename:  _test.c
 *
 *    Description:  测试函数
 *
 *        Version:  1.0
 *        Created:  2014年05月11日 20时42分45秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zwh (), 1209681393@qq.com
 *   Organization:  
 *
 * =====================================================================================
 */


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
#include	<stdlib.h>

/* 
* ===  FUNCTION  ======================================================================
*         Name:  main
*  Description:  
* =====================================================================================
*/
int
main ( int argc, char **argv )
{
    g_torrentmeta = parsetorrentfile(argv[1]);
    
    int num;
    printf("hehe\n");
    int *flag_piece = parse_data_file(g_torrentmeta,&num);
    int i;
    printf("xixi\n");
    for(i=0;i<num;i++)
    {
        printf("the %d piece is %d\n",i,flag_piece[i]);
    }
    return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */
