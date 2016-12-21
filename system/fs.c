#include <xinu.h>
#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


#if FS
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

int fs_open(char *filename, int flags)
{
	// Checking if filename given is of length 0
	if(strlen(filename) == 0)
	{
		printf("fs_open :: Invalid File\n");
		return SYSERR;
	}
	
	// Checking if file is to be open in valid flag
	if(!(flags == O_RDONLY || flags == O_WRONLY || flags == O_RDWR))
	{
		printf("fs_open :: Invalid Flag\n");
		return SYSERR;
	}
	
	// Searching for given file name in directory
	int i = 0;
	for(; i < fsd.root_dir.numentries; i++)
	{
		if(strcmp(filename, fsd.root_dir.entry[i].name) == 0)
		{
			break;
		}
	}
	
	// Checking if file with given name doesnot exists
	if(i == fsd.root_dir.numentries)
	{
		printf("fs_open :: No File With Given Name Present\n");
		return SYSERR;
	}
	
	// Searching for corresponding file table entry
	int fd = -1;
	for(int j = 0; j < NUM_FD; j++)
	{
		if(strcmp(fsd.root_dir.entry[i].name, oft[j].de->name) == 0)
		{
			fd = j;
			break;
		}
	}
	
	// Checking if entry in file table exists for given file
	if(fd == -1)
	{
		printf("fs_open :: No Entry In File Table Found\n");
		return SYSERR;
	}
	
	// Checking if file is not closed
	if(oft[fd].state != FSTATE_CLOSED)
	{
		printf("fs_open :: File Already Open\n");
		return SYSERR;
	}
	
	// Getting iNode for the file
	struct inode in;
	int get_inode_status = fs_get_inode_by_num(0, oft[fd].in.id, &in);
	
	// Checking if error in completion of fs_get_inode_by_num
	if(get_inode_status == SYSERR)
	{
		printf("fs_open :: Error In fs_get_inode_by_num\n");
		return SYSERR;		
	}
	
	// Reinitializing file table entries
	oft[fd].state = FSTATE_OPEN;
	oft[fd].fileptr = 0;
	oft[fd].in = in;
	oft[fd].de = &fsd.root_dir.entry[i];
	oft[fd].mode = flags;
	
	// Returning file table index
	return fd;
}

int fs_close(int fd)
{
	// Checking if the file identifier is valid
	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_close :: Invalid File\n");
		return SYSERR;
	}
	
	// Checking if the file is already closed
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_close :: File Already Closed\n");
		return SYSERR;
	}
	
	// Setting file state to be close
	oft[fd].state = FSTATE_CLOSED;
	
	// Setting fileptr to 0
	oft[fd].fileptr = 0;
	
	// Returning operation status
	return OK;
}

int fs_create(char *filename, int mode)
{
	// Checking if filename given is of length 0
	if(strlen(filename) == 0 || strlen(filename) > FILENAMELEN)
	{
		printf("fs_create :: Invalid File Name\n");
		return SYSERR;
	}
	
	// Checking if mode is correct
	if(mode != O_CREAT)
	{
		printf("fs_create :: Invalid Mode\n");
		return SYSERR;
	}
	
	// Searching for given file name in directory
	int i = 0;
	for(; i < fsd.root_dir.numentries; i++)
	{
		if(strcmp(filename, fsd.root_dir.entry[i].name) == 0)
		{
			break;
		}
	}
	
	// Checking if file with given name exists
	if(i != fsd.root_dir.numentries)
	{
		printf("fs_create :: File With The Given Name Already Exists\n");
		return SYSERR;
	}
	
	// Checking if an inode is available
	if(fsd.inodes_used >= fsd.ninodes)
	{
		printf("fs_create :: No iNode Available Currently");
		return SYSERR;
	}
	
	// Getting iNode for the file
	struct inode in;
	int get_inode_status = fs_get_inode_by_num(0, ++fsd.inodes_used, &in);
	
	// Checking if error in completion of fs_get_inode_by_num
	if(get_inode_status == SYSERR)
	{
		printf("fs_create :: Error In fs_get_inode_by_num\n");
		return SYSERR;		
	}
	
	// Reinitializing inode data
	in.id = fsd.inodes_used;
	in.type = INODE_TYPE_FILE;
	in.nlink = 0;
	in.device = 0;
	in.size = 0;
	
	// Writing iNode for the file
	int put_inode_status = fs_put_inode_by_num(0, i, &in);
	
	// Checking if error in completion of fs_put_inode_by_num
	if(put_inode_status == SYSERR)
	{
		printf("fs_create :: Error In fs_put_inode_by_num\n");
		return SYSERR;
	}
	
	// Adding entries to directory
	strcpy(fsd.root_dir.entry[fsd.root_dir.numentries].name, filename);
	fsd.root_dir.entry[fsd.root_dir.numentries].inode_num = i;
	
	// Reinitializing file table entries
	oft[fsd.inodes_used].state = FSTATE_OPEN;
	oft[fsd.inodes_used].fileptr = 0;
	oft[fsd.inodes_used].in = in;
	oft[fsd.inodes_used].de = &fsd.root_dir.entry[fsd.root_dir.numentries];
	oft[fsd.inodes_used].mode = O_RDWR;
	
	// Incrementing count in directory
	fsd.root_dir.numentries++;
	
	// Returning file table index
	return fsd.inodes_used;
}

int fs_seek(int fd, int offset)
{
	// Checking if the file identifier is valid
	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_seek :: Invalid File\n");
		return SYSERR;
	}
	
	// Checking if the file is closed
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_seek :: File Not Open\n");
		return SYSERR;
	}
	
	// Checking if new fileptr will go less than 0
	if((oft[fd].fileptr + offset) < 0)
	{
		printf("fs_seek :: Cannot Set Position To Negative Value\n");
		return SYSERR;
	}
	
	// Checking if new fileptr will go greater than file size
	if((oft[fd].fileptr + offset) > oft[fd].filesize)
	{
		printf("fs_seek :: Cannot Set Position To A Value Greater Than File Size\n");
		return SYSERR;
	}
	
	// Returning & setting fileptr to new position
	return oft[fd].fileptr += offset;
}

int fs_read(int fd, void *buf, int nbytes)
{	
	// Checking if the file identifier is valid
	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_read :: Invalid File\n");
		return SYSERR;
	}
	
	// Checking if the file is closed
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_read :: File Not Open\n");
		return SYSERR;
	}
	
	// Checking if file is open in correct mode
	if(!(oft[fd].mode == O_RDONLY || oft[fd].mode == O_RDWR))
	{
		printf("fs_read :: File Open In Read Only Mode\n");
		return SYSERR;
	}
	
	// Checking if length of given buffer is not zero
	if(nbytes <=0)
	{
		printf("fs_read :: Invalid Length Of Read Buffer\n");
		return SYSERR;	
	}
	
	// Checking if file is not empty
	if(oft[fd].in.size == 0)
	{
		printf("fs_read :: File To Read Is Empty\n");
		return SYSERR;
	}
	
	// Incremeting nbytes in order to accommodate with fileptr
	nbytes += oft[fd].fileptr;
		
	// Calculating how many blocks to read
	int blocksToRead = nbytes / MDEV_BLOCK_SIZE;
	
	// If nbytes is not exact divisor then add one more block
	if((nbytes % MDEV_BLOCK_SIZE) != 0)
	{
		blocksToRead++;
	}
	
	// Actual blocks in file is less set number of blocks to read to actual number of blocks
	blocksToRead = oft[fd].in.size < blocksToRead ? oft[fd].in.size : blocksToRead;
	
	// Finding first block to read
	int blockNum = (oft[fd].fileptr / MDEV_BLOCK_SIZE);
		
	// Clearing the given buffer in case it is not empty	
	memset(buf, NULL, (MDEV_BLOCK_SIZE * MDEV_NUM_BLOCKS));
	
	// Variable to count number of bytes read
	int bytesRead = 0;
	
	// Setting file offset
	int offset = (oft[fd].fileptr % MDEV_BLOCK_SIZE);
	
	// Reading blocks till blocks to read
	for(; blockNum < blocksToRead; blockNum++, offset = 0)
	{
		// Clearing the block cache
		memset(block_cache, NULL, MDEV_BLOCK_SIZE+1);
		
		// Reading the given block
		if(bs_bread(0, oft[fd].in.blocks[blockNum], offset, block_cache, MDEV_BLOCK_SIZE - offset) == SYSERR)
		{
			printf("fs_read :: Error In Reading File\n");
			return SYSERR;
		}

		// Copy the bytes read in buffer
		strcpy((buf+bytesRead), block_cache);
		
		// Resetting number of bytes read to length of buffer
		bytesRead = strlen(buf);
	}
	
	// Resetting fileptr to new value
	oft[fd].fileptr = bytesRead;
	
	// Returning number of bytes read
	return bytesRead;
}

int fs_write(int fd, void *buf, int nbytes)
{
	// Checking if the file identifier is valid
	if(fd < 0 || fd > NUM_FD)
	{
		printf("fs_write :: Invalid File\n");
		return SYSERR;
	}
	
	// Checking if the file is closed
	if(oft[fd].state == FSTATE_CLOSED)
	{
		printf("fs_write :: File Not Open\n");
		return SYSERR;
	}
	
	// Checking if file is open in correct mode
	if(!(oft[fd].mode == O_WRONLY || oft[fd].mode == O_RDWR))
	{
		printf("fs_write :: File Open In Read Only Mode\n");
		return SYSERR;
	}
	
	// Checking if length of given buffer is not zero or not equal to nbytes
	if(nbytes <= 0 || (strlen((char*)buf) == 0) || strlen((char*)buf) != nbytes)
	{
		printf("fs_write :: Invalid Length Of Write Buffer\n");
		return SYSERR;
	}
	
	// Loop over and clear till iNode size is not zero
	struct inode tempiNode;
	if((oft[fd].in.size) > 0)
	{
		tempiNode = oft[fd].in;
		for(; (oft[fd].in.size) > 0; (oft[fd].in.size)--)
		{
			// Clearing previously written blocks if not empty
			if(fs_clearmaskbit(tempiNode.blocks[(oft[fd].in.size)-1]) != OK)
			{
				printf("fs_write :: Error In Clearing Block %d\n",(oft[fd].in.size)-1);
				return SYSERR;
			}
		}
	}
	
	// Calculating How Many Blocks To Write
	int blocksToWrite = nbytes / MDEV_BLOCK_SIZE;
	
	// If nbytes is not exact divisor then add one more block
	if((nbytes % MDEV_BLOCK_SIZE) != 0)
	{
		blocksToWrite++;
	}
		
	// New variable to store number of bytes
	int bytesToWrite = nbytes;
	
	// Finding first block to write
	int blockNum = FIRST_INODE_BLOCK + NUM_INODE_BLOCKS; 
	for(int i = 0; ((i < blocksToWrite) && (blockNum < MDEV_BLOCK_SIZE)); blockNum++)
	{
		if(fs_getmaskbit(blockNum) == 0)
		{
			// Filling block cache with NULLs
			memset(block_cache, NULL, MDEV_BLOCK_SIZE);
			
			// Writing blocks from block cache
			if(bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
			{
				printf("fs_write :: Error In Clearing Block %d\n", blockNum);
				return SYSERR;
			}
			
			// Getting the minimum bytes to write
			int minBytes = MDEV_BLOCK_SIZE < bytesToWrite ? MDEV_BLOCK_SIZE : bytesToWrite;
			
			// Filling block cache with actual bytes to write
			memcpy(block_cache, buf, minBytes);
			
			// Writing memory and checking if operation produced some error
			if(bs_bwrite(0, blockNum, 0, block_cache, MDEV_BLOCK_SIZE) == SYSERR)
			{
				printf("fs_write :: Error In Writing Block %d\n", blockNum);
				return SYSERR;
			}
			
			// Resetting buf with new value
			buf = (char*) buf + minBytes;
			
			// Subtracting the number of bytes already written
			bytesToWrite = bytesToWrite - minBytes;
			
			// Setting but mask for block num
			fs_setmaskbit(blockNum);
			
			// Allocating blocks with the block num value
			oft[fd].in.blocks[i++] = blockNum;
		}
	}	
	
	// Resetting iNode size to new value
	oft[fd].in.size = blocksToWrite;
	
	// Writing iNode for the file
	int put_inode_status = fs_put_inode_by_num(0, oft[fd].in.id, &oft[fd].in);
	
	// Checking if error in completion of fs_put_inode_by_num
	if(put_inode_status == SYSERR)
	{
		printf("fs_create :: Error In fs_put_inode_by_num\n");
		return SYSERR;
	}

	// Resetting filesize to new value
	oft[fd].filesize = nbytes;
	
	// Resetting fileptr to new value
	oft[fd].fileptr = nbytes;
	
	// Returning the number of bytes just wrote
	return nbytes;
}

int fs_mount(int dev)
{
	// Disable and save interrupt mask
    intmask mask = disable();

	// Check if device is bad or number of device names reached to maximum
	if ((isbaddev(dev)) || (nnames >= NNAMES))
	{
		restore(mask);
		return SYSERR;
	}

	// Get the reference of entry in list
	struct nmentry *namptr = &nametab[nnames];	

	// Set device number in the list
	namptr->ndevice = dev;	

	// Increment number of device names 
    nnames++;			

	// Restore saved mask
	restore(mask);

	// Return the OK status
	return OK;
}

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
int fs_get_inode_by_num(int dev, int inode_number, struct inode *in) {
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

int fs_put_inode_by_num(int dev, int inode_number, struct inode *in) {
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

void fs_print_fsd(void) {

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
  return OK;

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

#endif /* FS */
