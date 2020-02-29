#include "tips.h"

/* The following two functions are defined in util.c */

/* finds the highest 1 bit, and returns its position, else 0xFFFFFFFF */
unsigned int uint_log2(word w); 

/* return random int from 0..x-1 */
int randomint( int x );

/*
  This function allows the lfu information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lfu information
 */
char* lfu_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lfu information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].accessCount);

  return buffer;
}

/*
  This function allows the lru information to be displayed

    assoc_index - the cache unit that contains the block to be modified
    block_index - the index of the block to be modified

  returns a string representation of the lru information
 */
char* lru_to_string(int assoc_index, int block_index)
{
  /* Buffer to print lru information -- increase size as needed. */
  static char buffer[9];
  sprintf(buffer, "%u", cache[assoc_index].block[block_index].lru.value);

  return buffer;
}

/*
  This function initializes the lfu information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lfu(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].accessCount = 0;
}

/*
  This function initializes the lru information

    assoc_index - the cache unit that contains the block to be modified
    block_number - the index of the block to be modified

*/
void init_lru(int assoc_index, int block_index)
{
  cache[assoc_index].block[block_index].lru.value = 0;
}

/*
  This is the primary function you are filling out,
  You are free to add helper functions if you need them

  @param addr 32-bit byte address
  @param data a pointer to a SINGLE word (32-bits of data)
  @param we   if we == READ, then data used to return
              information back to CPU

              if we == WRITE, then data used to
              update Cache/DRAM
*/
void accessMemory(address addr, word* data, WriteEnable we)
{
  /* Declare variables here */
  unsigned int tag_l, index_l, offset_l; //length of TIO
  unsigned int tag_v, index_v, offset_v; //values of TIO
  enum cache_hit {Hit, Miss}; 
  enum cache_hit cache_hit = Miss; //default it to miss
  address wb_addr;
  unsigned int block_location;
  unsigned int LRU_v, random_v = 0;

  /* handle the case of no cache at all - leave this in */
  if(assoc == 0) {
    accessDRAM(addr, (byte*)data, WORD_SIZE, we);
    return;
  }

  /*
  You need to read/write between memory (via the accessDRAM() function) and
  the cache (via the cache[] global structure defined in tips.h)

  Remember to read tips.h for all the global variables that tell you the
  cache parameters

  The same code should handle random, LFU, and LRU policies. Test the policy
  variable (see tips.h) to decide which policy to execute. The LRU policy
  should be written such that no two blocks (when their valid bit is VALID)
  will ever be a candidate for replacement. In the case of a tie in the
  least number of accesses for LFU, you use the LRU information to determine
  which block to replace.

  Your cache should be able to support write-through mode (any writes to
  the cache get immediately copied to main memory also) and write-back mode
  (and writes to the cache only gets copied to main memory when the block
  is kicked out of the cache.

  Also, cache should do allocate-on-write. This means, a write operation
  will bring in an entire block if the block is not already in the cache.

  To properly work with the GUI, the code needs to tell the GUI code
  when to redraw and when to flash things. Descriptions of the animation
  functions can be found in tips.h
  */

  /* Start adding code here */
  index_l = uint_log2(set_count); //find length of index
  offset_l = uint_log2(block_size); // find length of offset
  tag_l = 32 - (index_l + offset_l); //find length of tag, which is remaining bits
  //find tag_v
  tag_v = addr >> (index_l + offset_l);
  //find index_v
  unsigned int temp_addr;
  temp_addr = addr >> offset_l;
  int binary_rep_for_index_l = 1;
  for(int i = 0; i < offset_l; i++){
    binary_rep_for_index_l = 2 * binary_rep_for_index_l;
  }
  binary_rep_for_index_l = binary_rep_for_index_l - 1;
  index_v = temp_addr & binary_rep_for_index_l;
  //find offset_v
  int binary_rep_for_offset_l = 1;
  for(int j = 0; j < offset_l; j++){
    binary_rep_for_offset_l = 2 * binary_rep_for_offset_l;
  }
  binary_rep_for_offset_l = binary_rep_for_offset_l - 1 ;
  offset_v = addr & binary_rep_for_offset_l;
  
  switch (we){
    //handles Read
    case READ:
    {
      //go through all the block given the amount of assoc, and find if any block is the one we are looking for
      for(unsigned int a = 0; a < assoc; a++){
        if(cache[index_v].block[a].tag == tag_v && cache[index_v].block[a].valid == VALID){
          cache_hit = Hit;
          block_location = a;       
        }
      }
      switch (cache_hit)
      {
        //handles read hit
        case Hit:
        {
          cache[index_v].block[block_location].lru.value = 0;
          cache[index_v].block[block_location].valid = VALID;
          memcpy(data, (cache[index_v].block[block_location].data + offset_v), 4);
          highlight_offset(index_v, block_location, offset_v, HIT);
        }
        break;
        //handles read miss
        case Miss:
        {
          //handles LRU cache miss policy
          switch (policy)
          {
          case LRU:{
            for(unsigned int c = 0; c < assoc; c++){
              if(cache[index_v].block[c].lru.value > cache[index_v].block[LRU_v].lru.value){
                LRU_v = c;
              }
            }
            if(cache[index_v].block[LRU_v].dirty = DIRTY){
              if(memory_sync_policy == WRITE_BACK){
                wb_addr = cache[index_v].block[LRU_v].tag << (index_l + offset_l) + (index_v << offset_l);
                accessDRAM(wb_addr, cache[index_v].block[LRU_v].data, offset_l, WRITE);
              }
              else if(memory_sync_policy == WRITE_THROUGH){
                accessDRAM(addr, cache[index_v].block[LRU_v].data, offset_l, WRITE);
              }
            }
            accessDRAM(addr, cache[index_v].block[LRU_v].data, offset_l, READ); //get data from memory
            cache[index_v].block[LRU_v].tag = tag_v; //set all the values
			      cache[index_v].block[LRU_v].valid = 1;
            cache[index_v].block[LRU_v].lru.value = 0;
            cache[index_v].block[LRU_v].dirty = VIRGIN;
            memcpy(data, (cache[index_v].block[LRU_v].data + offset_v), 4); 
            highlight_block(index_v, LRU_v);
            highlight_offset(index_v, LRU_v, offset_v, MISS);
          }
          break;

          //handle random cache miss policy
          case RANDOM:
          {
            random_v = randomint(assoc);
            if(cache[index_v].block[random_v].dirty = DIRTY){
              if(memory_sync_policy == WRITE_BACK){
                wb_addr = cache[index_v].block[random_v].tag << (index_l + offset_l) + (index_v << offset_l);
                accessDRAM(wb_addr, cache[index_v].block[random_v].data, offset_l, WRITE);
              }
              else if(memory_sync_policy == WRITE_THROUGH){
                accessDRAM(addr, cache[index_v].block[random_v].data, offset_l, WRITE);
              }
            }
            accessDRAM(addr, cache[index_v].block[random_v].data, offset_l, READ);
            cache[index_v].block[random_v].valid = 1;
			      cache[index_v].block[random_v].lru.value = 0;
            cache[index_v].block[random_v].tag = tag_v;
            cache[index_v].block[random_v].dirty = VIRGIN;
            memcpy(data, (cache[index_v].block[random_v].data + offset_v), 4); 
            highlight_block(index_v, random_v);
            highlight_offset(index_v, random_v, offset_v, MISS);
          }
          break;

          default:
          break;
          }
        }
        break;
      }
    }
    break;
    //handles Write
    case WRITE:
      switch (memory_sync_policy)
      {
      case WRITE_BACK:
          for(unsigned int a = 0; a < assoc; a++){
           if(cache[index_v].block[a].tag == tag_v && cache[index_v].block[a].valid == VALID){
            cache_hit = Hit;
            block_location = a; 
            }
          }
          switch (cache_hit)
      {
        //handles read hit
        case Hit:
        {
          cache[index_v].block[block_location].lru.value = 0;
          cache[index_v].block[block_location].valid = VALID;
          cache[index_v].block[block_location].dirty = VIRGIN;
          memcpy((cache[index_v].block[block_location].data + offset_v), data, 4);
          accessDRAM(addr, cache[index_v].block[block_location].data, offset_l, WRITE);
          highlight_offset(index_v, block_location, offset_v, HIT);
        }
        break;
        //handles read miss
        case Miss:
        {
          //handles LRU cache miss policy
          switch (policy)
          {
          case LRU:
          {
            for(unsigned int c = 0; c < assoc; c++){
              if(cache[index_v].block[c].lru.value > cache[index_v].block[LRU_v].lru.value){
                LRU_v = c;
              }
            }
            accessDRAM(addr, cache[index_v].block[LRU_v].data, offset_l, READ); 
            cache[index_v].block[LRU_v].tag = tag_v;
			      cache[index_v].block[LRU_v].valid = 1;
            cache[index_v].block[LRU_v].lru.value = 0;
            cache[index_v].block[LRU_v].dirty = VIRGIN;
            memcpy(data, (cache[index_v].block[LRU_v].data + offset_v), 4); 
            highlight_block(index_v, LRU_v);
            highlight_offset(index_v, LRU_v, offset_v, MISS);
            
            if(cache[index_v].block[LRU_v].dirty = DIRTY){
              if(memory_sync_policy == WRITE_BACK){
                wb_addr = cache[index_v].block[LRU_v].tag << (index_l + offset_l) + (index_v << offset_l);
                accessDRAM(wb_addr, cache[index_v].block[LRU_v].data, offset_l, WRITE);
              }
            }
          }
          break;

          //handle random cache miss policy
          case RANDOM:
          {
            random_v = randomint(assoc);
            accessDRAM(addr, cache[index_v].block[random_v].data, offset_l, READ);
            cache[index_v].block[random_v].valid = 1;
			      cache[index_v].block[random_v].lru.value = 0;
            cache[index_v].block[random_v].tag = tag_v;
            cache[index_v].block[random_v].dirty = VIRGIN;
            memcpy(data, (cache[index_v].block[random_v].data + offset_v), 4); 
            highlight_block(index_v, random_v);
            highlight_offset(index_v, random_v, offset_v, MISS);

            if(cache[index_v].block[random_v].dirty = DIRTY){
              if(memory_sync_policy == WRITE_BACK){
                wb_addr = cache[index_v].block[random_v].tag << (index_l + offset_l) + (index_v << offset_l);
                accessDRAM(wb_addr, cache[index_v].block[random_v].data, offset_l, WRITE);
              }
            }
          }
          break;

          default:
          break;
          }
        }
        break;
      }
        break;
      case WRITE_THROUGH:
      //   for(unsigned int a = 0; a < assoc; a++){
      //      if(cache[index_v].block[a].tag == tag_v && cache[index_v].block[a].valid == VALID){
      //       cache_hit = Hit;
      //       block_location = a; 
      //       }
      //     }
      //     switch (cache_hit)
      // {
      //   //handles read hit
      //   case Hit:
      //   {
      //     cache[index_v].block[block_location].lru.value = 0;
      //     cache[index_v].block[block_location].valid = VALID;
      //     cache[index_v].block[block_location].dirty = VIRGIN;
      //     memcpy((cache[index_v].block[block_location].data + offset_v), data, 4);
      //     accessDRAM(addr, cache[index_v].block[block_location].data, offset_l, WRITE);
      //     highlight_offset(index_v, block_location, offset_v, HIT);
      //   }
      //   break;
      //   //handles read miss
      //   case Miss:
      //   {
      //     //handles LRU cache miss policy
      //     switch (policy)
      //     {
      //     case LRU:
      //     {
      //       for(unsigned int c = 0; c < assoc; c++){
      //         if(cache[index_v].block[c].lru.value > cache[index_v].block[LRU_v].lru.value){
      //           LRU_v = c;
      //         }
      //       }
      //       accessDRAM(addr, cache[index_v].block[LRU_v].data, offset_l, READ); 
      //       cache[index_v].block[LRU_v].tag = tag_v;
			//       cache[index_v].block[LRU_v].valid = 1;
      //       cache[index_v].block[LRU_v].lru.value = 0;
      //       cache[index_v].block[LRU_v].dirty = VIRGIN;
      //       memcpy(data, (cache[index_v].block[LRU_v].data + offset_v), 4); 
      //       highlight_block(index_v, LRU_v);
      //       highlight_offset(index_v, LRU_v, offset_v, MISS);

      //     }
      //     break;

      //     //handle random cache miss policy
      //     case RANDOM:
      //     {
      //       random_v = randomint(assoc);
      //       accessDRAM(addr, cache[index_v].block[random_v].data, offset_l, READ);
      //       cache[index_v].block[random_v].valid = 1;
			//       cache[index_v].block[random_v].lru.value = 0;
      //       cache[index_v].block[random_v].tag = tag_v;
      //       cache[index_v].block[random_v].dirty = VIRGIN;
      //       memcpy(data, (cache[index_v].block[random_v].data + offset_v), 4); 
      //       highlight_block(index_v, random_v);
      //       highlight_offset(index_v, random_v, offset_v, MISS);

      //     }
      //     break;

      //     default:
      //     break;
      //     }
      //   }
      //   break;//miss break
      // }
        break;//wt break
      }
    break; //write break
  }

  if(policy == LRU){
      for(unsigned int i = 0; i < assoc; i++){
        cache[index_v].block[i].lru.value = cache[index_v].block[i].lru.value + 1;
      }
  }
  /* This call to accessDRAM occurs when you modify any of the
     cache parameters. It is provided as a stop gap solution.
     At some point, ONCE YOU HAVE MORE OF YOUR CACHELOGIC IN PLACE,
     THIS LINE SHOULD BE REMOVED.
  */
}
