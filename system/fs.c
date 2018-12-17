#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#ifdef FS
#include <fs.h>

static struct fsystem fsd;
int dev0_numblocks;
int dev0_blocksize;
char *dev0_blocks;

extern int dev0;

char block_cache[512];

#define SB_BLK 0
#define BM_BLK 1
#define RT_BLK 2

#define NUM_FD 16
struct filetable oft[NUM_FD];
int next_open_fd = 0;


#define INODES_PER_BLOCK (fsd.blocksz / sizeof(struct inode))
#define NUM_INODE_BLOCKS (( (fsd.ninodes % INODES_PER_BLOCK) == 0) ? fsd.ninodes / INODES_PER_BLOCK : (fsd.ninodes / INODES_PER_BLOCK) + 1)
#define FIRST_INODE_BLOCK 2

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock);
void fs_set_next_open_fd();
/* YOUR CODE GOES HERE */

int fs_fileblock_to_diskblock(int dev, int fd, int fileblock) {
  int diskblock;

  if (fileblock >= INODEBLOCKS - 2) {
    printf("No indirect block support\n");
    return SYSERR;
  }

  diskblock = oft[fd].in.blocks[fileblock]; //get the logical block address

  return diskblock;
}

/* read in an inode and fill in the pointer */
int
fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;
  int inode_off;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_get_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  inode_off = inn * sizeof(struct inode);

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  printf("inn*sizeof(struct inode): %d\n", inode_off);
  */
  bs_bread(dev0, bl, 0, &block_cache[0], fsd.blocksz);
  memcpy(in, &block_cache[inode_off], sizeof(struct inode));
	
  return OK;

}

int
fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
  int bl, inn;

  if (dev != 0) {
    printf("Unsupported device\n");
    return SYSERR;
  }
  if (inode_number > fsd.ninodes) {
    printf("fs_put_inode_by_num: inode %d out of range\n", inode_number);
    return SYSERR;
  }

  bl = inode_number / INODES_PER_BLOCK;
  inn = inode_number % INODES_PER_BLOCK;
  bl += FIRST_INODE_BLOCK;

  /*
  printf("in_no: %d = %d/%d\n", inode_number, bl, inn);
  */

  bs_bread(dev0, bl, 0, block_cache, fsd.blocksz);
  memcpy(&block_cache[(inn*sizeof(struct inode))], in, sizeof(struct inode));
  bs_bwrite(dev0, bl, 0, block_cache, fsd.blocksz);

  return OK;
}
     
int fs_mkfs(int dev, int num_inodes) {
  int i;
  
  if (dev == 0) {
    fsd.nblocks = dev0_numblocks;
    fsd.blocksz = dev0_blocksize;
  }
  else {
    printf("Unsupported device\n");
    return SYSERR;
  }

  if (num_inodes < 1) {
    fsd.ninodes = DEFAULT_NUM_INODES;
  }
  else {
    fsd.ninodes = num_inodes;
  }

  i = fsd.nblocks;
  while ( (i % 8) != 0) {i++;}
  fsd.freemaskbytes = i / 8; 
  
  if ((fsd.freemask = getmem(fsd.freemaskbytes)) == (void *)SYSERR) {
    printf("fs_mkfs memget failed.\n");
    return SYSERR;
  }
  
  /* zero the free mask */
  for(i=0;i<fsd.freemaskbytes;i++) {
    fsd.freemask[i] = '\0';
  }
  
  fsd.inodes_used = 0;
  
  /* write the fsystem block to SB_BLK, mark block used */
  fs_setmaskbit(SB_BLK);
  bs_bwrite(dev0, SB_BLK, 0, &fsd, sizeof(struct fsystem));
  
  /* write the free block bitmask in BM_BLK, mark block used */
  fs_setmaskbit(BM_BLK);
  bs_bwrite(dev0, BM_BLK, 0, fsd.freemask, fsd.freemaskbytes);

  return 1;
}

void
fs_print_fsd(void) {

  printf("fsd.ninodes: %d\n", fsd.ninodes);
  printf("sizeof(struct inode): %d\n", sizeof(struct inode));
  printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
  printf("NUM_INODE_BLOCKS: %d\n", NUM_INODE_BLOCKS);
}

/* specify the block number to be set in the mask */
int fs_setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
  return OK;
}

/* specify the block number to be read in the mask */
int fs_getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
  //return OK;

}

/* specify the block number to be unset in the mask */
int fs_clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
  return OK;
}

/* This is maybe a little overcomplicated since the lowest-numbered
   block is indicated in the high-order bit.  Shift the byte by j
   positions to make the match in bit7 (the 8th bit) and then shift
   that value 7 times to the low-order bit to print.  Yes, it could be
   the other way...  */
void fs_printfreemask(void) {
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

//Open a file. If file not present, create and open. Return file descriptor
int fs_open(char *filename, int flags) {
  	int total_files=fsd.root_dir.numentries;
  	int i;
  	int fd=-1;
  	int exists=0;
  	for(i=0;i<total_files;i++){
  		//Check if file is present in root dir
  		if(strncmp(filename,fsd.root_dir.entry[i].name,strlen(filename)+1)==0){
  			//File found.
  		  exists=1;
  			struct filetable ft;
		  	ft.state=FSTATE_OPEN;
		  	struct inode* fin=(struct inode*)getmem(sizeof(struct inode));
		  	int ret=fs_get_inode_by_num(dev0,fsd.root_dir.entry[i].inode_num,fin);
		  	if(ret==SYSERR){
		  		printf("Some error in getting inode\n");
		  	}
		  	ft.in=*fin;
		  	ft.de=&fsd.root_dir.entry[i];
		  	oft[next_open_fd]=ft;
		  	fd=next_open_fd;
		  	fs_set_next_open_fd();
		  	
  		}
  	}
  	if(exists==0)
  		fd=fs_create(filename,O_CREAT);
  	
  	if(fd==SYSERR||fd==-1){
  		return SYSERR;
  	}
  	return fd;
}
//Close a file. Return OK or SYSERR
int fs_close(int fd) {
  if (fd>=NUM_FD){
  	return SYSERR;
  }
  //filename of zero size means empty
  if(oft[fd].state==FSTATE_CLOSED){
  	return SYSERR;
  }
  else
  {
  	oft[fd].state=FSTATE_CLOSED;
  	//Mark as empty
  }
  return OK;
}
//Create a file. Return file descriptor of the file 
int fs_create(char *filename, int mode) {
  	if(mode!=O_CREAT){
  		return SYSERR;
  	}
 	//Create Inode
  	struct inode* file_inode=(struct inode*)getmem(sizeof(struct inode));
  	file_inode->id=++fsd.inodes_used;//+1 or 0 
  	file_inode->device=dev0;
  	file_inode->type=INODE_TYPE_FILE;
  	file_inode->size=0;
  	int ret=fs_put_inode_by_num(dev0, file_inode->id , file_inode);
  	if(ret==SYSERR){
  		printf("Error in inode\n");
  		return SYSERR;
  	}
	struct dirent* di =(struct dirent*)getmem(sizeof(struct dirent));
	di->inode_num=file_inode->id;
	if(fs_getmaskbit(2)!=1){
  		fs_setmaskbit(2);
  	}
 	int index=fsd.root_dir.numentries++;
 	fsd.root_dir.entry[index]=*di;
 	//Add file to file descriptor table
 	struct filetable* ft =(struct filetable*)getmem(sizeof(struct filetable));
	ft->state=FSTATE_OPEN;
	ft->in=*file_inode;
	ft->de=di;
  ft->fileptr=0;
	strncpy(ft->de->name,filename,FILENAMELEN);
  oft[next_open_fd]=*ft;
  int fd=next_open_fd;
  fs_set_next_open_fd();
  return fd;
}
//Seek File to passed offset. Returns OK or SYSERR
int fs_seek(int fd, int offset) {

 	if(strlen(oft[fd].de->name)==0){
 		return SYSERR;
 	}
	//Check file Size
  int inode_num=oft[fd].in.id;
  struct inode* fin=(struct inode*)getmem(sizeof(struct inode));
	int ret=fs_get_inode_by_num(dev0,inode_num,fin);
  if(ret==SYSERR){
  	return SYSERR;
	}
  if(oft[fd].in.size<offset){
  	return SYSERR;
  }
 
  if(offset<0){
  	oft[fd].fileptr-=offset*(-1);
  }
  else
    oft[fd].fileptr+=offset;
  return OK;
}

//Read a file and copy contents to buffer. Return Size of file.
int fs_read(int fd, void *buf, int nbytes) {
	int i;
  if(strlen(oft[fd].de->name)==0){
 		return SYSERR;
 	}
  int current_fp=oft[fd].fileptr;
  //Current offset from which to read
  int current_fp_offset=current_fp%fsd.blocksz;
  //Read first block
  int current_block=current_fp/MDEV_BLOCK_SIZE;
  char *tempbuf1=getmem(MDEV_BLOCK_SIZE);
  //Bytes to be read in first block
  int current_block_bytes=fsd.blocksz-current_fp_offset;
  int diskbl=fs_fileblock_to_diskblock(dev0,fd,current_block);
  bs_bread(dev0, diskbl, current_fp_offset, tempbuf1, current_block_bytes);  
  strncat(buf,tempbuf1,current_block_bytes);
  freemem(tempbuf1,MDEV_BLOCK_SIZE);
 	int blocks_required=nbytes/MDEV_BLOCK_SIZE;
 	if(nbytes%MDEV_BLOCK_SIZE!=0){
 		blocks_required++;
 	}
 	int last_block_offset=nbytes%MDEV_BLOCK_SIZE;
 	for(i=1;i<blocks_required;i++){
 		char *tempbuf=getmem(MDEV_BLOCK_SIZE);
 		diskbl=fs_fileblock_to_diskblock(dev0,fd,i);
 		if(i==blocks_required-1){
 			bs_bread(dev0, diskbl, 0, tempbuf, last_block_offset);	
 			strncat(buf,tempbuf,last_block_offset);
 		}
 		else{
 			bs_bread(dev0, diskbl, 0, tempbuf, MDEV_BLOCK_SIZE);
 			strncat(buf,tempbuf,MDEV_BLOCK_SIZE);
 		}
 		
 		freemem(tempbuf,MDEV_BLOCK_SIZE);
 	}
	oft[fd].fileptr+=nbytes;
 	return nbytes;

}
//Write into a file and return Size of file.
int fs_write(int fd, void *buf, int nbytes) {
  int i,j;
  
  char* tempbuf=getmem(fsd.blocksz);
  //Check if new file. if existing file, free previous data blocks
  int file_size=oft[fd].in.size;
  int existing_blocks=file_size/fsd.blocksz;
  if(file_size%fsd.blocksz>0){
    existing_blocks++;
  }
  if(file_size!=0){
    //Free the previous data blocks
    for(i=0;i<existing_blocks;i++){
      int datablk=fs_fileblock_to_diskblock(dev0,fd,i);
      fs_clearmaskbit(datablk);
    }
  }
  //Check if file is present in fd
  if(strlen(oft[fd].de->name)==0){
    return SYSERR;
  }
  //Update inode
  int inode_id=oft[fd].in.id;
  struct inode* file_inode=(struct inode*)getmem(sizeof(struct inode));
    file_inode->id=++fsd.inodes_used;
    file_inode->device=dev0;
    file_inode->type=INODE_TYPE_FILE;
    file_inode->size=nbytes;
    
  int blocks_required=nbytes/MDEV_BLOCK_SIZE;
  if(nbytes%MDEV_BLOCK_SIZE!=0){
    blocks_required++;
  }
  int last_block_offset=nbytes%MDEV_BLOCK_SIZE;
  //Loop from first data block to last block in device
  for (i=0;i<blocks_required;i++){
    void* current_offset=buf+i*MDEV_BLOCK_SIZE;
    for(j=NUM_INODE_BLOCKS+FIRST_INODE_BLOCK;j<MDEV_NUM_BLOCKS;j++){
      if(fs_getmaskbit(j)==0){
        fs_setmaskbit(j);
        if(i==blocks_required-1){
          memcpy((void*)tempbuf,current_offset,last_block_offset);
          bs_bwrite(dev0, j, 0, (void*)tempbuf, fsd.blocksz);
          file_inode->blocks[i]=j;
        }
        else{
          memcpy((void*)tempbuf,current_offset,MDEV_BLOCK_SIZE);
          bs_bwrite(dev0, j, 0, tempbuf, fsd.blocksz);  
          file_inode->blocks[i]=j;
        }
        break;
      }
    }
  }
  int ret=fs_put_inode_by_num(dev0, inode_id , file_inode);
    if(ret==SYSERR){
      printf("Error in putting inode:write file\n");
    }
    oft[fd].in=*file_inode;
    freemem(tempbuf,fsd.blocksz);
  oft[fd].fileptr+=nbytes;
  return nbytes;
}

//Set the next open FD available for use.
void fs_set_next_open_fd(){
	int i;
  for(i=0;i<NUM_FD;i++){
    if(oft[i].state==FSTATE_CLOSED){
     	next_open_fd=i;
 			return;
 		}
	}
}

#endif /* FS */
