#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "vdf.h"
#include "vdferr.h"

/* 计算BAT(Block Allocation Table)的大小 */
uint64_t _vdf_batsize(const uint64_t filesize, const uint64_t blocksize){
     return (filesize - HEADER_SIZE)/(blocksize*8+1);
}

/* 计算给定大小中block 的数量 */
uint64_t _vdf_blockcount(const uint64_t filesize, const uint64_t blocksize){
     return 8 * _vdf_batsize(filesize, blocksize);
}

/* 计算最小文件的大小 */
uint64_t _vdf_minfilesize(const uint64_t blocksize){
    uint64_t batsize = 1; //block分配表大小 至少1字节
    return HEADER_SIZE + batsize + 8*blocksize; //最小的文件大小
}

/* 计算block size */
uint64_t _vdf_blocksize(const VDF_BLOCKSIZE_FLAG blocksize_flag){
    return (0x00000001<<blocksize_flag)*256;
}

/* 初始化文件头 */
void _vdf_fillheader(const uint64_t filesize, const VDF_BLOCKSIZE_FLAG blocksize_flag, VDF_HEADER* const header){
    char vdf_flag[3] = __VDF_FLAG__;
    uint64_t blocksize=_vdf_blocksize(blocksize_flag); // per block size
    uint64_t batsize=_vdf_batsize(filesize,blocksize); // Block Allocation Table size
    uint64_t blockcount = _vdf_blockcount(filesize,blocksize); //block count
    header->version = __VDF_VERSION__;
    header->flag[0] = vdf_flag[0];
    header->flag[1] = vdf_flag[1];
    header->flag[2] = vdf_flag[2];
    header->fsize=filesize;
    header->bsizeflag = blocksize_flag;
    header->bcount = _vdf_blockcount(filesize,blocksize);
    header->tstart = HEADER_SIZE;//文件中的位置，相对于文件头的偏移量
    header->tend = HEADER_SIZE + batsize -1;//文件中的位置，相对于文件头的偏移量
    header->bstart = HEADER_SIZE + batsize;//文件中的位置，相对于文件头的偏移量
    header->bend = HEADER_SIZE + batsize + blockcount * blocksize -1;//文件中的位置，相对于文件头的偏移量
}

/* 创建文件 */
int vdf_create(const uint64_t filesize, const VDF_BLOCKSIZE_FLAG blocksize_flag, const char *filepath){
    VDF_HEADER fheader;
    FILE *fp;
    _vdf_fillheader(filesize, blocksize_flag, &fheader);
    uint64_t blocksize=_vdf_blocksize(fheader.bsizeflag);
    uint64_t batsize=_vdf_batsize(filesize,blocksize);
    uint8_t c=0;
    
    if( filesize>(uint64_t)MAX_FILE_SIZE )return E_FMAXLT;
    if( filesize<_vdf_minfilesize(blocksize) )return E_FMINLT;
    if(access(filepath,F_OK)==0)return E_FEXIST;
    fp = fopen(filepath,"wb");
    if(NULL==fp)
        return E_FBADIO;
    fwrite(&fheader , HEADER_SIZE, 1, fp);//写入文件头
    fwrite(&c, 1, batsize, fp); //初始化bat
    fputc(c,fp); //快速初始化block区域，只在区域开始写入一个字节'\0'
    fseek(fp, fheader.bend, SEEK_SET); //移动到block区域末尾
    fputc(c,fp); //快速初始化block区域，只在区域末尾写入一个字节'\0'
    fseek(fp, fheader.fsize-1, SEEK_SET); //移动到文件末尾
    fputc(c,fp); //快速初始化文件剩余区域，只在区域末尾写入一个字节'\0'
    fflush(fp);  //写入文件
    fclose(fp);
    return E_SUCCESS;
}

/* 获取VDF文件头 */
int vdf_getheader(VDF_HEADER * const header,FILE * const fp){
    if(NULL==fp)return E_FBADIO;
    fseek(fp, 0, SEEK_SET);//回到文件头
    fread(header,HEADER_SIZE,1,fp);
    if(header->version!=__VDF_VERSION__ || strcmp(header->flag,__VDF_FLAG__))return E_FINVAL;
    return E_SUCCESS;
}

/* 打开VDF文件，会检验是否为VDF文件，返回文件指针，如不是VDF文件，返回NULL */
FILE *vdf_open(const char *filepath){
    FILE *fp=fopen(filepath,"r+b");
    
    if(NULL==fp)return fp;
    if(vdf_check(fp)==1)
        return fp;
    else{
        fclose(fp);
        return NULL;
    } 
}

void vdf_close(FILE *fp){
    fclose(fp);
}

/* 检验是否为VDF文件，如获取到VDF文件头，则返回1，否则返回0*/
int vdf_check(FILE *fp){
    VDF_HEADER header;
    if(vdf_getheader(&header,fp)==E_SUCCESS)
        return 1;
    else
        return 0;
}

/*@Unperfect*/
/*向系统配置文件vdf.conf中添加文件*/
int vdf_addvdf(const char *filepath){
    FILE *configfp=fopen("vdf.conf","ab");/*以添加方式打开*/
    if(configfp==NULL)return E_FBADIO;
    if(access(filepath,F_OK))return E_FBADIO; /*文件不存在*/
    fwrite(filepath,strlen(filepath),1,configfp);
    fputc(0xAA,configfp);
    fclose(configfp);
    return E_SUCCESS;
}

/*@Unperfect*/
/*从系统配置文件vdf.conf中删除文件*/
int vdf_rmvdf(const char *filepath){
    
}

/*@Unperfect*/
/*从系统配置文件中读取一个vdf文件,并返回一个可读写的文件描述符*/
FILE * vdf_getvdf(FILE *conf){
    char f[1000];
    int i;
    if(feof(conf))return NULL;
    for(i=0;i<1000;i++){
        f[i]=fgetc(conf);
        if(f[i]==0xAA){
            f[i]='\0';
            break;
        }
    }
    return fopen(f,"r+b");
}