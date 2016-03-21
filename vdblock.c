#include <stdio.h>
#include "vdf.h"
#include "vdferr.h"

/*
 * return VALUE: 1-可写  0-不可写
 */
int vdf_bwriteable(uint64_t blockid, FILE * const fp){
    VDF_HEADER header;
    uint64_t offset = blockid>>3;
    int nbit = blockid%8+1;
    char flagbyte;
    
    if(vdf_getheader(&header,fp)!=E_SUCCESS)return 0;
    fseek(fp, header.tstart+offset, SEEK_SET);
    flagbyte=fgetc(fp);
    if(BLOCK_USE_FLAG(flagbyte,nbit)>0)
        return 0;
    else 
        return 1;
}

/*
 * @params: mp-读出数据存放的内存指针
 *      size-读出的字节数
 *      blockid-读出的块编号
 *      fp-读出的文件指针
 * @return: 读出的字节数
 */
int vdf_bread(void *mp, size_t size, uint64_t blockid, FILE *fp){
    VDF_HEADER header;
    size_t realsize=size;
    uint64_t blocksize;
    uint64_t boffset;
    
    if(vdf_getheader(&header,fp)!=E_SUCCESS)return 0;//获取不到文件头，返回0
    //判断需要读出的字节数
    blocksize=BLOCKSIZE(header.bsizeflag);
    if(size>blocksize)realsize=blocksize;
    
    boffset=blockid*blocksize;
    fseek(fp, header.bstart+boffset, SEEK_SET); //定位到块区
    fread(mp,realsize,1,fp);
    return realsize;
}

/*
 * @params: mp-写入数据存放的内存指针
 *      size-写入的字节数
 *      blockid-写入的块编号
 *      fp-写入的文件指针
 * @return: 写入的字节数
 */
int vdf_bwrite(void *mp, size_t size, uint64_t blockid, FILE *fp){
    VDF_HEADER header;
    size_t realsize=size;
    uint64_t blocksize;
    uint64_t boffset;
    
    if(vdf_getheader(&header,fp)!=E_SUCCESS)return 0;//获取不到文件头，返回0
    //判断需要写入的字节数
    blocksize=BLOCKSIZE(header.bsizeflag);
    if(size>blocksize)realsize=blocksize;
    
    _vdf_bset(blockid,header,fp);//设置标志位
    
    boffset=blockid*blocksize;
    fseek(fp, header.bstart+boffset, SEEK_SET); //定位到块区
    fwrite(mp,realsize,1,fp);
    fflush(fp);
    return realsize;
}

/*
 * @params: 
 *      blockid-擦除的块编号
 *      fp-文件指针
 * @return: 失败 0，成功 1
 */
int vdf_berase(uint64_t blockid, FILE *fp){
    VDF_HEADER header;
    if(vdf_getheader(&header,fp)!=E_SUCCESS)return 0;//获取不到文件头，返回0
    return _vdf_bunset(blockid,header,fp);//设置标志位
}

int _vdf_bset(uint64_t blockid,const VDF_HEADER header,FILE *fp){
    uint64_t offset = blockid/8;
    int nbit = blockid%8+1;
    char flagbyte;
    
    fseek(fp, header.tstart+offset, SEEK_SET); //定位到标志位
    flagbyte=fgetc(fp); //获取字节
    flagbyte=SET_BLOCK_USE_FLAG(flagbyte,nbit);
    fseek(fp, -1, SEEK_CUR); //定位到标志位
    fputc(flagbyte,fp);
    fflush(fp);
    return 1;
}

int _vdf_bunset(uint64_t blockid,const VDF_HEADER header,FILE *fp){
    uint64_t offset = blockid/8;
    int nbit = blockid%8+1;
    char flagbyte;
    
    fseek(fp, header.tstart+offset, SEEK_SET); //定位到标志位
    flagbyte=fgetc(fp); //获取字节
    flagbyte=UNSET_BLOCK_USE_FLAG(flagbyte,nbit);
    fseek(fp, -1, SEEK_CUR); //定位到标志位
    fputc(flagbyte,fp);
    fflush(fp);
    return 1;
}

int vdf_getnodedesc(BAT_NODE * const batnode, size_t blockid, FILE *fp){
    uint64_t offset=blockid/8;
    int nbit = blockid%8;
    char flagbyte;
    VDF_HEADER header;
    
    batnode->id=blockid;
    batnode->offset=offset;
    batnode->umask=0x01<<nbit;
    
    if(fp==NULL)return 1;
    vdf_getheader(&header,fp);
    fseek(fp, header.tstart+offset, SEEK_SET); //定位到标志位
    flagbyte=fgetc(fp); //获取字节
    batnode->value=( BLOCK_USE_FLAG(flagbyte,nbit+1) ) >> nbit;
    
    return 0;
}

int vdf_getdesc(VDF * const vdf,  FILE *fp){
    VDF_HEADER header;
    
    if(fp==NULL)return 1;
    vdf_getheader(&header,fp);
    vdf->header=header;
    vdf->blocksize=BLOCKSIZE(header.bsizeflag);
    vdf->vdfp=fp;
    
    return 0;
}
