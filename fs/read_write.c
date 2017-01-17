#include "testfs.h"
#include "list.h"
#include "super.h"
#include "block.h"
#include "inode.h"

/* given logical block number, read the corresponding physical block into block.
 * return physical block number.
 * returns 0 if physical block does not exist.
 * returns negative value on other errors. */

static int testfs_read_block (struct inode* in, int log_block_nr, char* block){
    if (log_block_nr > NR_DIRECT_BLOCKS+NR_INDIRECT_BLOCKS+NR_INDIRECT_BLOCKS*NR_INDIRECT_BLOCKS){
        return -EFBIG;
    }
    
    int phy_block_nr = 0;
    int temp_log_block_nr = 0;
    // Variables for double-indirect blocks
    long block_nr = 0;
    long entry_nr = 0;
    
    
    assert(log_block_nr >= 0);
    
    // Access the direct block
    if (log_block_nr < NR_DIRECT_BLOCKS){
        phy_block_nr = (int)in->in.i_block_nr[log_block_nr];
        //return phy_block_nr;
    }
    // Access the indirect block 
    else if (log_block_nr >= NR_DIRECT_BLOCKS && log_block_nr < NR_INDIRECT_BLOCKS+NR_DIRECT_BLOCKS){
        temp_log_block_nr = log_block_nr - NR_DIRECT_BLOCKS;
        
        if (in->in.i_indirect > 0){
            read_blocks(in->sb, block, in->in.i_indirect, 1);
            phy_block_nr = ((int *)block)[temp_log_block_nr];
        }
        else {
            bzero(block, BLOCK_SIZE);
            //phy_block_nr = NONE;
            return phy_block_nr;
        }
      
    }
    // Access the double indirect block
    else if (log_block_nr >= NR_INDIRECT_BLOCKS+NR_DIRECT_BLOCKS){
        temp_log_block_nr = log_block_nr - NR_DIRECT_BLOCKS - NR_INDIRECT_BLOCKS;
        block_nr = temp_log_block_nr / NR_INDIRECT_BLOCKS;
        entry_nr = temp_log_block_nr % NR_INDIRECT_BLOCKS;
        
        if (in->in.i_dindirect > 0){
            read_blocks(in->sb, block, in->in.i_dindirect, 1);
            phy_block_nr = ((int *)block)[block_nr];
        }
        else {
            bzero(block, BLOCK_SIZE);
            return phy_block_nr;
        }
        if (phy_block_nr > 0){
            read_blocks(in->sb, block, phy_block_nr, 1);
            phy_block_nr = ((int *)block)[entry_nr];
        }
        else {
            bzero(block, BLOCK_SIZE);
            return phy_block_nr;
        }
        
        //return phy_block_nr;
    }
    
   if (phy_block_nr > 0) {
		read_blocks(in->sb, block, phy_block_nr, 1);
	} else {
		/* we support sparse files by zeroing out a block that is not
		 * allocated on disk. */
		bzero(block, BLOCK_SIZE);
	}
	return phy_block_nr;
}

int testfs_read_data (struct inode* in, char* buf, off_t start, size_t size){
    char block[BLOCK_SIZE];
    long block_nr = start / BLOCK_SIZE;
    long block_ix = start % BLOCK_SIZE;
    size_t _size = size;
    int ret;
    assert(buf);
    // Break the boundary
    if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS){
        return -EFBIG;
    }
    // What does this mean ???
    if (start + (off_t) _size > in->in.i_size){
        _size = in->in.i_size - start;
    }
    while (block_ix + _size > BLOCK_SIZE){
        // Check the potential error first, break the boundary or no valid physic block
        if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS)
            return -EFBIG;
       
        if ((ret = testfs_read_block(in, block_nr, block)) < 0)
	    return ret;
        
        memcpy(buf, block + block_ix, BLOCK_SIZE - block_ix);
        _size = _size - (BLOCK_SIZE - block_ix);
        // Move the pointer of buffer
        buf = buf + (BLOCK_SIZE - block_ix);
        // Move to next block !
        block_nr ++;
        block_ix = 0;
        
    }
    
    if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS)
            return -EFBIG;
    if ((ret = testfs_read_block(in, block_nr, block)) < 0)
	return ret;
    
    memcpy(buf, block + block_ix, _size);
    //return the number of bytes read or any error 
    return size;
    
    
}

static int
testfs_allocate_block(struct inode *in, int log_block_nr, char *block)
{
	int phy_block_nr;
	char indirect[BLOCK_SIZE];
	int indirect_allocated = 0;
        // For double indirect blocks
        char dindirect[BLOCK_SIZE];
        int dindirect_allocated = 0;
        //int block_nr = 0;
        //int block_offset = 0;
        bzero(dindirect, BLOCK_SIZE);
        bzero(indirect, BLOCK_SIZE);
        
	assert(log_block_nr >= 0);
	phy_block_nr = testfs_read_block(in, log_block_nr, block);

	/* phy_block_nr > 0: block exists, so we don't need to allocate it, 
	   phy_block_nr < 0: some error */
        
	if (phy_block_nr != 0)
		return phy_block_nr;

	/* allocate a direct block */
	if (log_block_nr < NR_DIRECT_BLOCKS) {
		assert(in->in.i_block_nr[log_block_nr] == 0);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr >= 0) {
			in->in.i_block_nr[log_block_nr] = phy_block_nr;
		}
		return phy_block_nr;
	}
        // Modified part
        
	log_block_nr -= NR_DIRECT_BLOCKS;
        
	if (log_block_nr >= NR_INDIRECT_BLOCKS){
            log_block_nr -= NR_INDIRECT_BLOCKS;
            if (in->in.i_dindirect == 0){ // allocate a double indirect block
                phy_block_nr = testfs_alloc_block_for_inode(in);
                
                if (phy_block_nr < 0)
		    return phy_block_nr;
                
                in->in.i_dindirect = phy_block_nr;
                dindirect_allocated = 1;
                
            }
            // above is good !!!!
            
            else {
                read_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
            }
     
            if (((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] == 0){
                
               // Which block is the indirect block ????
                phy_block_nr = testfs_alloc_block_for_inode(in);
                // If the physical block is invalid
                if(phy_block_nr < 0){
                    if (dindirect_allocated){
                        testfs_free_block_from_inode(in, in->in.i_dindirect);
                        in->in.i_dindirect = 0;
                        dindirect_allocated = 0;
                       
                    }
                    return phy_block_nr;
                }
                
                ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] = phy_block_nr;
                write_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
                indirect_allocated = 1;
                //in->in.i_indirect = phy_block_nr;
                
            }
            else {
                read_blocks(in->sb, indirect, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS], 1);
            }
            
            // Allocate a data block           
            //if (((int *)indirect)[log_block_nr % NR_INDIRECT_BLOCKS] == 0){
                phy_block_nr = testfs_alloc_block_for_inode(in);
                
                if (phy_block_nr < 0){
                    if (indirect_allocated){
                        
                        testfs_free_block_from_inode(in, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS]);
                        ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS] = 0;
                        write_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
                      
                        indirect_allocated = 0;
                        
                    }
                    if (dindirect_allocated){
                        //testfs_free_block_from_inode(in, ((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS]);
                        testfs_free_block_from_inode(in, in->in.i_dindirect);
                        in->in.i_dindirect = 0;
                    
                        dindirect_allocated = 0;
                    }
                    return phy_block_nr;
                }
                
                ((int *)indirect)[log_block_nr % NR_INDIRECT_BLOCKS] = phy_block_nr;
                write_blocks(in->sb,indirect,((int *)dindirect)[log_block_nr/NR_INDIRECT_BLOCKS],1);
               
           // }
            return phy_block_nr;
            
        }
		
        else{
	if (in->in.i_indirect == 0) {	/* allocate an indirect block */
		bzero(indirect, BLOCK_SIZE);
		phy_block_nr = testfs_alloc_block_for_inode(in);
		if (phy_block_nr < 0)
			return phy_block_nr;
		indirect_allocated = 1;
		in->in.i_indirect = phy_block_nr;
	} else {	/* read indirect block */
		read_blocks(in->sb, indirect, in->in.i_indirect, 1);
	}

	/* allocate direct block */
	assert(((int *)indirect)[log_block_nr] == 0);	
	phy_block_nr = testfs_alloc_block_for_inode(in);

	if (phy_block_nr >= 0) {
		/* update indirect block */
		((int *)indirect)[log_block_nr] = phy_block_nr;
		write_blocks(in->sb, indirect, in->in.i_indirect, 1);
	} else if (indirect_allocated) {
		/* free the indirect block that was allocated */
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}
	return phy_block_nr;
        }
}



int testfs_write_data(struct inode *in, const char *buf, off_t start, size_t size){
    char block[BLOCK_SIZE];
    long block_nr = start / BLOCK_SIZE;
    long block_ix = start % BLOCK_SIZE;
    int ret;
    size_t _size = size;
    
    if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS){
        in->in.i_size = block_nr * BLOCK_SIZE;
        in->i_flags |= I_FLAGS_DIRTY;
        return -EFBIG;
    }
    
    while (block_ix + _size > BLOCK_SIZE){
        // Check whether break the boundary or some other errors
        if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS){
            in->in.i_size = block_nr * BLOCK_SIZE;
            in->i_flags |= I_FLAGS_DIRTY;
            return -EFBIG;
        }
        ret = testfs_allocate_block(in, block_nr, block);
        if (ret < 0){
            return ret;
        }
        
        memcpy(block + block_ix, buf, BLOCK_SIZE - block_ix);
        write_blocks(in->sb, block, ret, 1);
        _size = _size - (BLOCK_SIZE - block_ix);
        buf = buf + (BLOCK_SIZE - block_ix);
        
        block_nr ++;
        block_ix = 0;
        
    }
    /* ret is the newly allocated physical block number */
        if (block_nr >= NR_DIRECT_BLOCKS + NR_INDIRECT_BLOCKS + NR_INDIRECT_BLOCKS * NR_INDIRECT_BLOCKS){
            in->in.i_size = block_nr * BLOCK_SIZE;
            in->i_flags |= I_FLAGS_DIRTY;
            return -EFBIG;
        }
    
	ret = testfs_allocate_block(in, block_nr, block);
	if (ret < 0)
		return ret;
	memcpy(block + block_ix, buf, _size);
	write_blocks(in->sb, block, ret, 1);
        
	/* increment i_size by the number of bytes written. */
	if (size > 0)
		in->in.i_size = MAX(in->in.i_size, start + (off_t) size);
	in->i_flags |= I_FLAGS_DIRTY;
	/* return the number of bytes written or any error */
	return size;
}

int
testfs_free_blocks(struct inode *in)
{
	int i;
        int j;
	int e_block_nr;
        
        char dindirect[BLOCK_SIZE];
        char indirect[BLOCK_SIZE];
	/* last block number */
	e_block_nr = DIVROUNDUP(in->in.i_size, BLOCK_SIZE);

	/* remove direct blocks */
	for (i = 0; i < e_block_nr && i < NR_DIRECT_BLOCKS; i++) {
		if (in->in.i_block_nr[i] == 0)
			continue;
		testfs_free_block_from_inode(in, in->in.i_block_nr[i]);
		in->in.i_block_nr[i] = 0;
	}
	e_block_nr -= NR_DIRECT_BLOCKS;

	/* remove indirect blocks */
	if (in->in.i_indirect > 0) {
		char block[BLOCK_SIZE];
		read_blocks(in->sb, block, in->in.i_indirect, 1);
		for (i = 0; i < e_block_nr && i < NR_INDIRECT_BLOCKS; i++) {
			testfs_free_block_from_inode(in, ((int *)block)[i]);
			((int *)block)[i] = 0;
		}
		testfs_free_block_from_inode(in, in->in.i_indirect);
		in->in.i_indirect = 0;
	}

	e_block_nr -= NR_INDIRECT_BLOCKS;
        
        // remove double indirect blocks
	if (in->in.i_dindirect > 0){
            
            read_blocks(in->sb, dindirect, in->in.i_dindirect, 1);
            
            for (i = 0; i < NR_INDIRECT_BLOCKS; i++){
                 if (((int *)dindirect)[i] <= 0){
                     continue;
                 }
                 else {
                     read_blocks(in->sb, indirect, ((int *)dindirect)[i], 1);
                     for (j = 0; j < NR_INDIRECT_BLOCKS; j++){
                        if (((int *)indirect)[j] > 0){
                            testfs_free_block_from_inode(in, ((int *)indirect)[j]);
			    ((int *)indirect)[j] = 0;
                        }
                     }
                }
                // Do I need to check?
                testfs_free_block_from_inode(in, ((int *)dindirect)[i]);
                ((int *)dindirect)[i] = 0;
            }
            // remove the last entry
            /*
            if (((int *)dindirect)[e_block_nr / NR_INDIRECT_BLOCKS] > 0){
                read_blocks(in->sb, indirect, ((int *)dindirect)[e_block_nr / NR_INDIRECT_BLOCKS], 1);
                for (j = 0; j < e_block_nr % NR_INDIRECT_BLOCKS; j++){
                    if (((int *)indirect)[j] > 0){
                       testfs_free_block_from_inode(in, ((int *)indirect)[j]);
                    }
                }
            }*/
            testfs_free_block_from_inode(in, in->in.i_dindirect);
            in->in.i_dindirect = 0;
            
        }

	in->in.i_size = 0;
	in->i_flags |= I_FLAGS_DIRTY;
	return 0;
}
