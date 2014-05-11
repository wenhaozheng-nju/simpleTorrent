/*
 * =====================================================================================
 *
 *       Filename:  parse_data_file.c
 *
 *    Description:  解析数据文件
 *
 *        Version:  1.0
 *        Created:  2014年05月10日 16时04分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  zwh (), 1209681393@qq.com
 *   Organization:
 *
 * =====================================================================================
 */
#include "btdata.h"
#include "util.h"
#include "sha1.h"
#include <sys/stat.h>  

#define DATA_PATH "./data/"

file_array *my_file_array;

int *new_file(torrentmetadata_t *meta_tree,char *name)
{
    FILE *filename = fopen(name,"wb");
    ftruncate(fileno(filename),meta_tree->length);
    fclose(filename);
    int *ret = (int *)malloc(sizeof(int) * meta_tree->num_pieces);
    int i;
    for(i=0;i<meta_tree->num_pieces;i++)
    {
        ret[i] = 0;
    }
    return ret;
}

int *parse_data_file(torrentmetadata_t *meta_tree,int *num_piece)
{
    meta_tree->single_or_muti = 0;       //单文件解析
    if(meta_tree->single_or_muti == 0)
    {
        printf("file name is %s\n",meta_tree->name);
        char *file_path = (char *)malloc(strlen(meta_tree->name) + 1);
        memset(file_path,0,strlen(meta_tree->name) + 1);
        //strcat(file_path,DATA_PATH);
        strcpy(file_path,meta_tree->name);
        struct stat statbuf;
        if(stat(file_path,&statbuf) < 0)
        {
            *num_piece = meta_tree->num_pieces;
            return new_file(meta_tree,file_path);
        }
        int n_file_len = statbuf.st_size;
            printf("n_file_len is %d and length is %d\n",n_file_len,meta_tree->length);  
        if(n_file_len != meta_tree->length)
        {
            printf("n_file_len is %d and length is %d\n",n_file_len,meta_tree->length);  
            *num_piece = meta_tree->num_pieces;
            return new_file(meta_tree,file_path);
        }
        FILE *data_file = fopen(file_path,"rb");
        //fseek(data_file,0,SEEK_END);
        //printf("11\n");
        my_file_array = (file_array *)malloc(1*sizeof(file_array));
        my_file_array->offset = 0;
        my_file_array->name = file_path;
        *num_piece = meta_tree->num_pieces;
        int len = meta_tree->length;
        int i;
        char *buf = (char *)malloc(sizeof(char)*meta_tree->piece_len);
        int *ret = (int *)malloc(sizeof(int) * meta_tree->num_pieces);
        char *tmp_pieces = meta_tree->pieces;
        printf("piece_len is %d\n",meta_tree->piece_len);
        for(i=0;i<*num_piece;i++)
        {
            memset(buf,0,meta_tree->piece_len);
            ret[i] = 0;               //初始化分片信息
            if(len < meta_tree->piece_len)
            {
                fread(buf,1,len,data_file);
                SHA1Context sha;
                SHA1Reset(&sha);
                SHA1Input(&sha,(const unsigned char *)buf,len);
                if(strncmp((char *)sha.Message_Digest,tmp_pieces,20) == 0)
                {
                    ret[i] = 1;
                }
                break;
            }
            else
            {
                fread(buf,1,meta_tree->piece_len,data_file);
                SHA1Context sha;
                SHA1Reset(&sha);
                SHA1Input(&sha,(const unsigned char *)buf,meta_tree->piece_len);         
                int k;
                for(k=0;k<5;k++)
                {
                    sha.Message_Digest[k] = htonl(sha.Message_Digest[k]);
                }
                printf("SHA1:");
                for(k=0;k<5;k++)
                {
                    printf("%X ",sha.Message_Digest[k]);
                }
                printf("and tmp_pieces is :");
                for(k=0;k<20;k++)
                {
                    printf("%X ",(unsigned char)tmp_pieces[k]);
                }
                printf("\n");
                if(strncmp((char *)sha.Message_Digest,tmp_pieces,20) == 0 )
                {
                    ret[i] = 1;
                }
            }
            len -= meta_tree->piece_len;
            tmp_pieces += 20;
            //fseek(data_file,meta_tree->piece_len,SEEK_CUR);
        }
        free(buf);
        fclose(data_file);
        return ret;
    }
    else
    {
        
    }
    return NULL;
}
