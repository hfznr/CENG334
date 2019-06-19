#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ext21.h"
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#define BASE_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define IMAGE "image.img"

typedef unsigned char bmap;
#define __NBITS (8 * (int) sizeof (bmap))
#define __BMELT(d) ((d) / __NBITS)
#define __BMMASK(d) ((bmap) 1 << ((d) % __NBITS))
#define BM_SET(d, set) ((set[__BMELT (d)] |= __BMMASK (d)))
#define BM_CLR(d, set) ((set[__BMELT (d)] &= ~__BMMASK (d)))
#define BM_ISSET(d, set) ((set[__BMELT (d)] & __BMMASK (d)) != 0)

unsigned int block_size = 0;
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*block_size)

#define BLOCK_OFFSET2(block) (block*block_size)

static void read_inode(int fd, int inode_no,struct ext2_super_block *super, struct ext2_group_desc *group, struct ext2_inode *inode){ //TODO GROUP HESAPLAMASI
	if(block_size == 1024){
		lseek(fd, BLOCK_OFFSET(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
	}else{
		lseek(fd, BLOCK_OFFSET2(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
	}
	read(fd, inode, sizeof(struct ext2_inode));
}

static void write_inode(int fd, int inode_no, struct ext2_super_block *super, struct ext2_group_desc *group, struct ext2_inode *inode){
	if(block_size==1024){
		lseek(fd, BLOCK_OFFSET(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
	}else{
		lseek(fd, BLOCK_OFFSET2(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);	
	}
	write(fd, inode, sizeof(struct ext2_inode));	
}

int isNumeric(char* string){
   for(int i=0 ; i<strlen(string) ; i++){
      if (string[i] < 48 || string[i] > 57)
         return 0;
   }
   return 1;
}

int round4(int num){
	int i=0;
	while(1){
		if(i*4>=num){
			return i*4;
		}
		i++;
	}
}

int parseNumber(char* string){
	int j=0;
	int res=0;
	for(int i=strlen(string)-1 ; i>=0 ; i--){
		if(j==0){
			res = res + (string[i] - '0');
		}else{
			res = res + pow(10, j)*(string[i] - '0');
		}
		j++;
	}
	return res;
}

int countEntries(struct ext2_dir_entry *block){
	int i=0;
	struct ext2_dir_entry* entryPointer = block;
	while(entryPointer->file_type){
		entryPointer=(void*)entryPointer + entryPointer->rec_len;
		i++;
	}
	return i;
}

int main(int argc, char **argv){
	
    struct ext2_super_block super;
    struct ext2_group_desc group;
	int fd,sourceFile;
	char* sourceFileName = malloc(sizeof(char)*(strlen(argv[2])+1));
	for(int i=0 ; i<strlen(argv[2]) ; i++){
		sourceFileName[i]=argv[2][i];
	}
	sourceFileName[strlen(argv[2])]=0;


    if((fd = open(argv[1], O_RDWR)) < 0){ //open filesystem
    	perror("Image cannot opened");
    }

    if((sourceFile = open(argv[2], O_RDONLY)) < 0){ //open source file
    	perror("Source File cannot opened");
    }

    lseek(fd, BASE_OFFSET, SEEK_SET);  //read super
    read(fd, &super, sizeof(super));

    if(super.s_magic != EXT2_SUPER_MAGIC){
    	fprintf(stderr, "Not a ext2 filesystem");
    	exit(1);
    }

    block_size = EXT2_BLOCK_SIZE << super.s_log_block_size;


	lseek(fd, BASE_OFFSET + 1024, SEEK_SET); //read group
	read(fd, &group, sizeof(group));
	
	// get targetInode
	int targetInode;
	if(strlen(argv[3])==1 && argv[3][0]=='/'){
		targetInode = 2;
	}
    else if(isNumeric(argv[3])){
    	targetInode = parseNumber(argv[3]);
    }else{

    	struct ext2_inode root;
		read_inode(fd, 2, &super, &group, &root);

		char delim[] = "/";

		char *ptr = strtok(argv[3], delim);
		int inode_no;

		while(ptr != NULL){
			unsigned int size = 0 ;
			struct ext2_dir_entry *entry;
			unsigned char block[block_size];
			if(block_size==1024){
				lseek(fd, BLOCK_OFFSET(root.i_block[0]), SEEK_SET);
			}else{
				lseek(fd, BLOCK_OFFSET2(root.i_block[0]), SEEK_SET);				
			}
	    	read(fd, block, block_size);    
	    	entry = (struct ext2_dir_entry *) block;
			while(size < root.i_size){
				char fileName[EXT2_NAME_LEN+1];
				memcpy(fileName, entry->name, entry->name_len);
				fileName[entry->name_len] = 0;
				if(!strcmp(ptr, fileName)){
					int inodeNo = entry->inode;
					int groupCount = inodeNo/super.s_inodes_per_group;
					inodeNo = inodeNo%super.s_inodes_per_group;
					lseek(fd, BASE_OFFSET + 1024 + groupCount*sizeof(group), SEEK_SET);
					read(fd, &group, sizeof(group));
					read_inode(fd, inodeNo, &super, &group, &root);
					inode_no = entry->inode;
					break;
				}
				entry = (void*) entry + entry->rec_len;
				size += entry->rec_len;
			}
			ptr = strtok(NULL, delim);

		}

		targetInode = inode_no;
    }
    

    int groupCount = super.s_inodes_count / super.s_inodes_per_group;
	int freeInode; /// FIND FREE INODE FIRST
    bmap *inodeBitmap;
    inodeBitmap = malloc(block_size);
    int foundNode = 0;
    for(int i=0 ; i<groupCount && !foundNode; i++){
	    lseek(fd, BASE_OFFSET + 1024 + i*sizeof(group), SEEK_SET); //read group
		read(fd, &group, sizeof(group));   	
    	if(block_size==1024){
		    lseek(fd, BLOCK_OFFSET(group.bg_inode_bitmap), SEEK_SET);
    	}else{
		    lseek(fd, BLOCK_OFFSET2(group.bg_inode_bitmap), SEEK_SET);    		
    	}
	    read(fd, inodeBitmap, block_size);
	    for(int j=0 ; j<super.s_inodes_per_group ; j++){
	    	if(!BM_ISSET(j, inodeBitmap)){
	    		freeInode = i*super.s_inodes_per_group + j+1;
	    		foundNode = 1;
	    		BM_SET(j, inodeBitmap);
	    		group.bg_free_inodes_count--;
	    		if(block_size==1024){
	    			lseek(fd, BLOCK_OFFSET(group.bg_inode_bitmap), SEEK_SET);	    			
	    		}else{
	    			lseek(fd, BLOCK_OFFSET2(group.bg_inode_bitmap), SEEK_SET);
	    		}
	    		write(fd, inodeBitmap, block_size);
	    		lseek(fd, BASE_OFFSET + 1024 + i*sizeof(group) , SEEK_SET);
	    		write(fd, &group, sizeof(group));
	    		break;
	    	}
	    }
    }
    
    lseek(fd, BASE_OFFSET + 1024, SEEK_SET);
    read(fd, &group, sizeof(group));

    //get empty data blocks
	unsigned int *emptyBlocks = malloc(sizeof(unsigned int)*super.s_blocks_count);
	bmap *blockBitmap;
	blockBitmap = malloc(block_size);
    int j=0;
    for(int k=0; k<=super.s_blocks_count/super.s_blocks_per_group ; k++){
    	lseek(fd, BASE_OFFSET + 1024 + k*sizeof(group), SEEK_SET);
    	read(fd, &group, sizeof(group));
    	if(block_size==1024){
		    lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
    	}else{
		    lseek(fd, BLOCK_OFFSET2(group.bg_block_bitmap), SEEK_SET);    		
    	}
	    read(fd, blockBitmap, block_size);	
	    for(int i=1; i< super.s_blocks_per_group+1; i++){
	        if (!BM_ISSET(i,blockBitmap)){
	        	if(block_size==1024){
	    	    	emptyBlocks[j]=k*super.s_blocks_per_group+i+1;
	        	}else{
	    	    	emptyBlocks[j]=k*super.s_blocks_per_group+i;	        		
	        	}
		        j++;
	        }
	    }	
    }
    int blocksReserved = 0;
    for(int i=0 ; i<12 ; i++){
    	void *block;
	    block = malloc(block_size);
    	lseek(sourceFile, i*block_size, SEEK_SET);
    	if(read(sourceFile, block, block_size)==0){
    		blocksReserved = i;
    		break;
    	}
    	blocksReserved = i;


    	if(block_size==1024){
	    	lseek(fd, BASE_OFFSET + (emptyBlocks[i]-1)*block_size, SEEK_SET);
    	}else{
	    	lseek(fd, BASE_OFFSET + (emptyBlocks[i]-1)*block_size, SEEK_SET);    		
    	}
    	write(fd, block, block_size);
    	free(block);
    }
    struct ext2_inode target, dataInode;
    int groupCountTargetInode = targetInode/super.s_inodes_per_group;
    int indexCountTargetInode = targetInode%super.s_inodes_per_group;
    lseek(fd, BASE_OFFSET + 1024 + groupCountTargetInode*sizeof(group), SEEK_SET);
    read(fd, &group, sizeof(group));
    read_inode(fd, indexCountTargetInode, &super, &group, &target);

    int groupCountFreeInode = freeInode/super.s_inodes_per_group;
    int indexCountFreeInode = freeInode%super.s_inodes_per_group;
    lseek(fd, BASE_OFFSET + 1024 + groupCountFreeInode*sizeof(group), SEEK_SET);
    read(fd, &group, sizeof(group));
    read_inode(fd, indexCountFreeInode, &super, &group, &dataInode);
   	dataInode.i_blocks = 0;

    for(int i=0 ; i<blocksReserved ; i++){
    	dataInode.i_block[i]=emptyBlocks[i];
    }
    struct stat ss;
    stat(argv[2], &ss);
    dataInode.i_blocks = blocksReserved*(block_size/512);
    dataInode.i_links_count = ss.st_nlink;
    dataInode.i_mode = ss.st_mode;
    dataInode.i_uid = ss.st_uid;
    dataInode.i_size = ss.st_size;
    dataInode.i_atime = ss.st_atime;
    dataInode.i_mtime = ss.st_mtime;
    dataInode.i_ctime = ss.st_ctime;
    dataInode.i_gid = ss.st_gid;

    lseek(fd, BASE_OFFSET + 1024 + groupCountFreeInode*sizeof(group), SEEK_SET);
    read(fd, &group, sizeof(group));
    write_inode(fd, indexCountFreeInode, &super, &group, &dataInode);

    //FIND PLACE FOR DIR ENTRY
    struct ext2_dir_entry entry;

    entry.inode = freeInode;
    entry.file_type = EXT2_FT_REG_FILE;
    entry.name_len = strlen(argv[2]);
    entry.rec_len = sizeof(unsigned int) + sizeof(unsigned short) + 2*sizeof(unsigned char)
    + (entry.name_len)*sizeof(char);
    memcpy(&entry.name, argv[2], (entry.name_len)*sizeof(char));
    entry.name[entry.name_len] = 0;

    char block[block_size];
    if(block_size==1024){
		lseek(fd, BLOCK_OFFSET(target.i_block[0]), SEEK_SET);
    }else{
		lseek(fd, BLOCK_OFFSET2(target.i_block[0]), SEEK_SET);    	
    }
	read(fd, block, block_size);    

	struct ext2_dir_entry* entryPointer = (struct ext2_dir_entry*) block;

	struct ext2_dir_entry* lastEntry;
	while(entryPointer->file_type){
		lastEntry = entryPointer;
		entryPointer=(void*)entryPointer + entryPointer->rec_len;
	}
	lastEntry->rec_len = round4(sizeof(unsigned int)+sizeof(unsigned short)+sizeof(unsigned char)*2 + sizeof(char)*(lastEntry->name_len+1));
	entryPointer = (struct ext2_dir_entry* )block;
	int size=0;
	int lastSize=0;
	while(entryPointer->file_type){
		lastEntry = entryPointer;
		lastSize = size;
		size += entryPointer->rec_len;
		entryPointer=(void*)entryPointer + entryPointer->rec_len;
	}
	if(block_size==1024){
		lseek(fd, BLOCK_OFFSET(target.i_block[0])+lastSize + sizeof(unsigned int), SEEK_SET);
	}else{
		lseek(fd, BLOCK_OFFSET2(target.i_block[0])+lastSize + sizeof(unsigned int), SEEK_SET);		
	}
	write(fd, &lastEntry->rec_len, sizeof(unsigned short));

	lastEntry = (void*)lastEntry + lastEntry->rec_len;
	*lastEntry = entry;
	memcpy(lastEntry->name, entry.name, lastEntry->name_len+1);
	lastEntry->rec_len = block_size-size;

	if(block_size==1024){
		lseek(fd, BLOCK_OFFSET(target.i_block[0])+size,SEEK_SET);
	}else{
		lseek(fd, BLOCK_OFFSET2(target.i_block[0])+size,SEEK_SET);
	}
	write(fd, &lastEntry->inode, sizeof(unsigned int));
	write(fd, &lastEntry->rec_len, sizeof(unsigned short));
	write(fd, &lastEntry->name_len, sizeof(unsigned char));
	write(fd, &lastEntry->file_type, sizeof(unsigned char));
	write(fd, &lastEntry->name, sizeof(char)*(lastEntry->name_len+1));
	

   	/* TESTING PURPOSES*/

	lseek(fd, BASE_OFFSET + 1024, SEEK_SET);
	read(fd, &group, sizeof(group));
	struct ext2_inode testInode2;
	read_inode(fd, 12, &super, &group, &testInode2);

	lseek(fd, BASE_OFFSET + 1024, SEEK_SET);
	read(fd, &group, sizeof(group));

	lseek(fd, BASE_OFFSET + 1024 + sizeof(group)*2, SEEK_SET);
	read(fd, &group, sizeof(group));
	struct ext2_inode testInode3;
	read_inode(fd, 30, &super, &group, &testInode3);

    super.s_free_blocks_count -=blocksReserved;
    super.s_free_inodes_count--;
    lseek(fd, BASE_OFFSET, SEEK_SET);  //read super
    write(fd, &super, sizeof(super));


    for(int i=0 ; i<blocksReserved ; i++){
    	int blockGroup = emptyBlocks[i]/super.s_blocks_per_group;
    	lseek(fd, BASE_OFFSET + 1024 + blockGroup*sizeof(group), SEEK_SET);
    	read(fd, &group, sizeof(group));
    	group.bg_free_blocks_count--;
    	lseek(fd, BASE_OFFSET + 1024 + blockGroup*sizeof(group), SEEK_SET);
    	write(fd, &group, sizeof(group));
    }

    printf("%d ", freeInode);
    for(int i=0 ; i<blocksReserved ; i++){
    	if(i==blocksReserved-1){
    		printf("%d\n", emptyBlocks[i]);
    	}else{
    		printf("%d, ", emptyBlocks[i]);
    	}
    	int blockGroupNum = emptyBlocks[i]/super.s_blocks_per_group;
    	int blockIndex = emptyBlocks[i]%super.s_blocks_per_group;
    	lseek(fd, BASE_OFFSET + 1024 + blockGroupNum*sizeof(group), SEEK_SET);
    	read(fd, &group, sizeof(group));

    	if(block_size==1024){
	    	lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
    	}else{
    		lseek(fd, BLOCK_OFFSET2(group.bg_block_bitmap), SEEK_SET);
    	}
	    read(fd, blockBitmap, block_size);	
	    if(block_size == 1024){
    	   	BM_SET(blockIndex-1,blockBitmap);
	    }else{
    	   	BM_SET(blockIndex,blockBitmap);
	    }
		
		lseek(fd, BASE_OFFSET + 1024 + blockGroupNum*sizeof(group), SEEK_SET);
    	read(fd, &group, sizeof(group));
    	if(block_size==1024){
       		lseek(fd, BLOCK_OFFSET(group.bg_block_bitmap), SEEK_SET);
    	}else{
       		lseek(fd, BLOCK_OFFSET2(group.bg_block_bitmap), SEEK_SET);
    	}
       	write(fd, blockBitmap, block_size);

    }

    free(emptyBlocks);
    free(inodeBitmap);
    free(blockBitmap);
	close(fd);
	close(sourceFile);
	exit(0);
    return 0;
}

