/* 
 * File:   vdf.h
 * Author: emma
 *
 * Created on 2016年3月7日, 下午8:54
 */

#ifndef VDF_H
#define	VDF_H

#ifdef	__cplusplus
extern "C" {
#endif
    
#include <inttypes.h>
    
#define __VDF_VERSION__ '1'
#define __VDF_FLAG__ "vdf"

#pragma pack(1)
typedef struct {
    char version;      /*文件版本标志，此为版本"1"*/
    char flag[3];      /*文件标志位，此版本为第一版，填充为"vd",意为Virtual Disk*/
    uint64_t fsize;    /*文件长度*/
    uint8_t bsizeflag; /*block size flag*/
    uint64_t bcount;   /*block 数量*/
    uint64_t tstart;   /*block分配表起始位置*/
    uint64_t tend;     /*block分配表结束位置*/
    uint64_t bstart;   /*block起始位置*/
    uint64_t bend;     /*block结束位置*/
} VDF_HEADER;

typedef struct vdf_descriptor {
    VDF_HEADER header;
    int blocksize;
    FILE * vdfp;
} VDF;

typedef struct bat_node_descriptor{
    size_t id;
    size_t offset;//相对于VDF_HEADER.tstart的偏移值,单位是byte
    uint8_t umask;//掩码，bat中1 byte表示8个block,通过掩码才能获取到是否有数据
    uint8_t value;//1=block已有数据，0=block无数据
} BAT_NODE;

typedef struct block_descriptor {
    VDF vdf;
    BAT_NODE batnode;
    size_t offset; //相对于VDF_HEADER.bstart的偏移值,单位是blocksize
    char *data; //block的数据
} BLOCK;

typedef enum {
    bs256b, bs512b, bs1k, bs2k, bs4k, bs8k, bs16k, bs32k, bs64k
} VDF_BLOCKSIZE_FLAG;

#define BLOCKSIZE(bsflag) (0x00000001<<(bsflag))<<8; /*N<<8 means N*256*/
#define BLOCK_USE_FLAG(byte,n) byte&( 0x01 << ( (n)-1 ) )
#define SET_BLOCK_USE_FLAG(byte,n) byte|( 0x01 << ( (n)-1 ) )
#define UNSET_BLOCK_USE_FLAG(byte,n) byte&(~( 0x01 << ( (n)-1 ) ) )

#define DEFAULT_BLOCK_SIZE BLOCKSIZE_4K
#define MAX_FILE_SIZE 1024*1024*1024*1024 //2的40次方 =1T 最大文件大小

#define HEADER_SIZE sizeof(VDF_HEADER) //常量 文件头大小

int vdf_create(const uint64_t filesize, const VDF_BLOCKSIZE_FLAG blocksize_flag, const char *filepath);
int vdf_getheader(VDF_HEADER * const header, FILE * const fp);
VDF* vdf_open(const char *filepath);
void vdf_close(VDF *vdf);
int vdf_bread(void *mp, size_t size, uint64_t blockid, FILE *fp);
int vdf_getnodedesc(BAT_NODE * const batnode, size_t blockid, FILE *fp);

#ifdef	__cplusplus
}
#endif

#endif	/* VDF_H */

