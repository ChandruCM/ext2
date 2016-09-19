#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include"ext2_fs.h"
#define offset 1024
#define BLOCK_OFFSET(block) (offset + (block-1)*block_size)
static void read_dir(int fd ,  struct ext2_group_desc *group,int);
static void read_inode(int fd, int inode_no,struct ext2_group_desc  *group, struct ext2_inode *inode);
unsigned int block_size;
struct ext2_inode inode;
struct ext2_super_block super;
struct ext2_group_desc group;
int fd;
int main(int argc,char *argv[])
{
     unsigned short int magic=0;
     unsigned int noinodes,pos=0;
     int i;
     if(argc<2)
     {
	  printf("Pass the appropiriate ext2image file..\n");
	  exit(1);

     }

     if((fd=open(argv[1],O_RDONLY))<0)
     {
	  perror("open");
	  exit(1);
     }
     lseek(fd,offset,SEEK_SET);

     read(fd,&super,sizeof(super));

     if (super.s_magic != EXT2_SUPER_MAGIC) {
	  fprintf(stderr, "Not a Ext2 filesystem\n");
	  exit(1);
     }

     if(super.s_magic!=EXT2_SUPER_MAGIC )
     {
	  printf(" It is  not a  Ext2 file system \n");
	  exit(1);	
     }
     block_size = 1024 << super.s_log_block_size;

     /*	   printf("Reading super-block from device %s :\n\n"
	   "Inodes count            : %x\n"
	   "Blocks count            : %d\n"
	   "Reserved blocks count   : %x\n"
	   "Free blocks count       : %x\n"
	   "Free inodes count       : %x\n"
	   "First data block        : %x\n"
	   "Block size              : %d\n"
	   "Blocks per group        : %x\n"
	   "Inodes per group        : %x\n"
	   "Creator OS              : %x\n"
	   "First non-reserved inode: %x\n"
	   "Size of inode structure : %x\n"
	   "SIZE of super block ....%lu\n"
	   ,argv[1],
	   super.s_inodes_count,
	   super.s_blocks_count,
	   super.s_r_blocks_count,    
	   super.s_free_blocks_count,
	   super.s_free_inodes_count,
	   super.s_first_data_block,
	   block_size,
	   super.s_blocks_per_group,
	   super.s_inodes_per_group,
	   super.s_creator_os,
	   super.s_first_ino,         
	   super.s_inode_size,
	   sizeof(super));  */

     lseek(fd, offset + block_size, SEEK_SET);
     read(fd, &group, sizeof(group));
     printf("inode table entry %x\n",offset+block_size);     
     printf("\n\n");
     printf("Reading first group-descriptor from device %s :\n\n"
	       "Blocks bitmap block: %x\n"
	       "Inodes bitmap block: %x\n"
	       "Inodes table block : %x\n"
	       "Free blocks count  : %x\n"
	       "Free inodes count  : %x\n"
	       "Directories count  : %x\n"
	       ,
	       argv[1],
	       group.bg_block_bitmap,
	       group.bg_inode_bitmap,
	       group.bg_inode_table,
	       group.bg_free_blocks_count,
	       group.bg_free_inodes_count,
	       group.bg_used_dirs_count);     

     read_inode(fd, 2, &group, &inode);

     printf("Reading root inode\n"
	       "File mode: %hu\n"
	       "Owner UID: %hu\n"
	       "Size     : %u bytes\n"
	       "Blocks   : %u\n"
	       ,
	       inode.i_mode,
	       inode.i_uid,
	       inode.i_size,
	       inode.i_blocks);

     for(i=0; i<EXT2_N_BLOCKS; i++)
	  if (i < EXT2_NDIR_BLOCKS)         /* direct blocks */
	       printf("Block %2u : %u\n", i, inode.i_block[i]);
	  else if (i == EXT2_IND_BLOCK)     /* single indirect block */
	       printf("Single   : %u\n", inode.i_block[i]);
	  else if (i == EXT2_DIND_BLOCK)    /* double indirect block */
	       printf("Double   : %u\n", inode.i_block[i]);
	  else if (i == EXT2_TIND_BLOCK)    /* triple indirect block */
	       printf("Triple   : %u\n", inode.i_block[i]);


     read_dir(fd,&group,2);

     return 0;
}
static void read_inode(int fd, int inode_no,struct ext2_group_desc  *group, struct ext2_inode *inode)
{ 

     lseek(fd, BLOCK_OFFSET(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
     read(fd, inode, sizeof(struct ext2_inode));
}

static void  read_dir(int fd,struct ext2_group_desc *group,int N)
{

     lseek(fd,(group->bg_inode_table * block_size) + ((N - 1) * 0x80) + 4,SEEK_SET);
     int size = 0;
     unsigned int tsize;
     read(fd,&tsize,4);
     lseek(fd,32,SEEK_CUR);
     unsigned int block;
     read(fd,&block,4);
     lseek(fd,block * block_size,SEEK_SET);
     while(size < tsize)
     {
	  unsigned int inode;
	  unsigned short int entry_length;
	  unsigned char name_size,type;
	  int pos = lseek(fd,0,SEEK_CUR);
	  read(fd,&inode,4);
	  read(fd,&entry_length,2);
	  read(fd,&name_size,1);
	  read(fd,&type,1);
	  char *name = calloc(name_size + 1,sizeof(char));
	  read(fd,name,name_size);
	  printf("%s\n",name);
	  if(type == 2)
	  {
/*	       if((inode / super.s_inodes_per_group) == 0 && strcmp(name,".") != 0 && strcmp(name,"..") != 0)
	       {
		    read_dir(fd,group,inode);
	       }
	       else */if(strcmp(name,".") != 0 && strcmp(name,"..") != 0)
	       {
		    int pos = lseek(fd,0,SEEK_CUR);
		    lseek(fd,1024+ block_size + ((inode / super.s_inodes_per_group) * 32),SEEK_SET);
		    read(fd, group, sizeof(group));
	//	    printf("%X %X\n",inode / super.s_inodes_per_group,inode % super.s_inodes_per_group);
		    read_dir(fd,group,inode % super.s_inodes_per_group);
		    lseek(fd,pos,SEEK_SET);
	       }
	  }
	  lseek(fd,pos + entry_length,SEEK_SET);
	  size += entry_length;
	  if(inode == 0)
	  {
	       break;
	  }
     }
}
