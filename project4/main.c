#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define R 0
#define W 1


typedef struct
{
	int read;
	int write;
	int writeback;
	int readhit;
	int writehit;
	int readmiss;
	int writemiss;
} cache_stat;

int get_bit(int n)
{
	if (n < 0)
	{
		return 0;
	}

	int cnt = 0;

	while (n)
	{
		n = n >> 1;
		++cnt;
	}

	return cnt - 1;
}

/***************************************************************/
/*                                                             */
/*                       Implement below                       */

typedef struct
{
	uint32_t address;
	long freq;
	int valid;
	int dirty;
	int index;

} block;

typedef struct
{
	// design your own structure
	int block_size, num_sets, num_ways;	
	block **sets;

} cache_struct;

cache_struct* build_cache(int num_set, int num_way, int block_size)
{
	// generate an instance of cache_struct and return the instance
	// you can change the type of return value as a pointer if you want
	cache_struct *cache = malloc(sizeof(cache_struct));
	if(cache == NULL){
		exit(EXIT_FAILURE);
	}

    cache->sets = malloc(num_set * sizeof(block*));  
	cache->block_size = block_size;
	cache->num_sets = num_set;
	cache->num_ways = num_way;

    for(int i = 0; i < num_set; i++) {
        cache->sets[i] = malloc(num_way * sizeof(block));  

		// malloc failure
		if (cache->sets[i] == NULL) {
			for (int k = 0; k < i; k++) {
				free(cache->sets[k]);
			}
			free(cache->sets);
			free(cache);
			fprintf(stderr, "Failed to allocate memory for set %d\n", i);
			exit(EXIT_FAILURE);
		}

        for(int j = 0; j < num_way; j++) {
            cache->sets[i][j].valid = 0;  
            cache->sets[i][j].address = 0;
            cache->sets[i][j].freq = 0;
			cache->sets[i][j].dirty = 0;
			cache->sets[i][j].index = INT32_MAX;
        }
    }

	return cache;
}

void search_cache(uint32_t address, cache_struct *cache, int index[]) {
	int set_index, valid;
	uint32_t cur_address;

	int block_bit = get_bit(cache->block_size);
	int index_bit = get_bit(cache->num_sets);

	unsigned long new_index = address >> block_bit;
    unsigned int index_mask = (1 << index_bit) - 1; 


	set_index = new_index & index_mask;

	for(int j = 0; j < cache->num_ways; j++) {
		valid = cache->sets[set_index][j].valid;
		cur_address = cache->sets[set_index][j].address;

		if (valid == 1 && cur_address == address){
			index[0] = set_index;
			index[1] = j;

			cache->sets[set_index][j].freq += 1; 
			return;
		}
	}

	index[0] = -1;
	index[1] = -1;
}

void insert_to_cache(uint32_t address, cache_struct *cache, cache_stat *stat, int lru, int op, int global_index){
	// insert new block to cache
	// evict if necessary
	int set_index, valid;
	
	int block_bit = get_bit(cache->block_size);
	int index_bit = get_bit(cache->num_sets);

	unsigned long new_index = address >> block_bit;
    unsigned int index_mask = (1 << index_bit) - 1; 

	set_index = new_index & index_mask;

	for(int j = 0; j < cache->num_ways; j++) {
		valid = cache->sets[set_index][j].valid;

		if (valid == 0) { // found a spot
			cache->sets[set_index][j].address = address;
			cache->sets[set_index][j].valid = 1;
			cache->sets[set_index][j].freq = 1;
			cache->sets[set_index][j].dirty = 0;
			cache->sets[set_index][j].index = global_index;

			if(op == W) cache->sets[set_index][j].dirty = 1;
			
			return;
		}
	}

	// cacheline is full, need to evict
	long min_frequency = LONG_MAX;
	int min_index = INT32_MAX;
	int way;

	if(lru){
		for(int j = 0; j < cache->num_ways; j++) {
			if (cache->sets[set_index][j].index < min_index && cache->sets[set_index][j].freq > 0) {
				min_index = cache->sets[set_index][j].index;
				way = j;
			}
		}
	}else{ // need to eliminate using LFLRU
		for(int j = 0; j < cache->num_ways; j++) {
			if (cache->sets[set_index][j].freq < min_frequency && cache->sets[set_index][j].freq > 0) { // found a spot
				min_frequency = cache->sets[set_index][j].freq;
			}
		}

		for(int j = 0; j < cache->num_ways; j++) {
			if (cache->sets[set_index][j].index < min_index && cache->sets[set_index][j].freq == min_frequency) {
				min_index = cache->sets[set_index][j].index;
				way = j;
			}
		}	
	}

	if(cache->sets[set_index][way].dirty == 1){
		stat->writeback += 1;
	}

	cache->sets[set_index][way].address = address;
	cache->sets[set_index][way].valid = 1;
	cache->sets[set_index][way].freq = 1;
	cache->sets[set_index][way].dirty = 1 ? op == W : 0;
	cache->sets[set_index][way].index = global_index;
}

void access_cache(cache_struct *cache, cache_stat *stat, int op, uint32_t addr, int lru, int global_index)
{	
	int index[2];
	search_cache(addr, cache, index);

	if (index[0] == -1){ // doesn't exist in cache, insert it
		if (op == R) stat->readmiss += 1;
		else if(op == W) stat->writemiss += 1;

		insert_to_cache(addr, cache, stat, lru, op, global_index);
	}else{
		if (op == R) stat->readhit += 1;
		else if(op == W){
			stat->writehit += 1;
			cache->sets[index[0]][index[1]].dirty = 1;
		} 
		cache->sets[index[0]][index[1]].index = global_index;
		// update_cache(index, cache); // make sure the LRU is in the front of the array
	}

	if (op == R){
		stat->read += 1;
	}
	else if(op == W){
		stat->write += 1;
	}
}

/*                       Implement above                       */
/*                                                             */
/***************************************************************/

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int num_way, int block_size, int lru_only_flag)
{
	printf("Cache Configuration:\n");
	printf("-------------------------------------\n");
	printf("Capacity: %dB\n", capacity);
	printf("Associativity: %dway\n", num_way);
	printf("Block Size: %dB\n", block_size);
	if (lru_only_flag)
		printf("Replacement Policy: LRU\n");
	else
		printf("Replacement Policy: LFLRU\n");
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat                                 */
/*                                                             */
/***************************************************************/
void sdump(cache_stat *stat)
{
	printf("Cache Stat:\n");
	printf("-------------------------------------\n");
	printf("Total reads: %d\n", stat->read);
	printf("Total writes: %d\n", stat->write);
	printf("Write-backs: %d\n", stat->writeback);
	printf("Read hits: %d\n", stat->readhit);
	printf("Write hits: %d\n", stat->writehit);
	printf("Read misses: %d\n", stat->readmiss);
	printf("Write misses: %d\n", stat->writemiss);
	printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/*                                                             */
/***************************************************************/
void xdump(cache_struct *cache, int num_set, int num_way, int block_size)
{
	printf("Cache Content:\n");
	printf("-------------------------------------\n");
	for (int i = 0; i < num_way; i++)
	{
		if (i == 0)
		{
			printf("    ");
		}
		printf("      WAY[%d]", i);
	}
	printf("\n");

	for (int i = 0; i < num_set; i++)
	{
		printf("SET[%d]:   ", i);

		for (int j = 0; j < num_way; j++)
		{
			uint32_t cache_block_addr = 0; // include only the information of tag and index (block offset must be 0)

			/***************************************************************/
			/*                                                             */
			/*                       Implement here                        */
			/*                                                             */
			/***************************************************************/

			cache_block_addr = cache->sets[i][j].address;
			printf("0x%08x  ", cache_block_addr);
		}

		printf("\n");
	}

	printf("\n");
}

int main(int argc, char *argv[])
{
	if (argc < 4)
	{
		printf("Usage: %s -c cap:assoc:block_size [-x] [-r] input_trace \n", argv[0]);
		exit(1);
	}

	int capacity;
	int num_way;
	int block_size;
	int xflag = 0;
	int lru_only_flag = 0;

	{
		char *token;
		int option_flag = 0;

		while ((option_flag = getopt(argc, argv, "c:xr")) != -1)
		{
			switch (option_flag)
			{
			case 'c':
				token = strtok(optarg, ":");
				capacity = atoi(token);
				token = strtok(NULL, ":");
				num_way = atoi(token);
				token = strtok(NULL, ":");
				block_size = atoi(token);
				break;
			case 'x':
				xflag = 1;
				break;
			case 'r':
				lru_only_flag = 1;
				break;
			default:
				printf("Usage: %s -c cap:assoc:block_size [-x] [-r] input_trace \n", argv[0]);
				exit(1);
			}
		}
	}

	char *trace_name;
	trace_name = argv[argc - 1];

	FILE *fp;
	fp = fopen(trace_name, "r"); // read trace file

	if (fp == NULL)
	{
		printf("\nInvalid trace file: %s\n", trace_name);
		return 1;
	}

	cdump(capacity, num_way, block_size, lru_only_flag);

	// cache statistics initialization
	cache_stat stat;
	stat.read = 0;
	stat.write = 0;
	stat.writeback = 0;
	stat.readhit = 0;
	stat.writehit = 0;
	stat.readmiss = 0;
	stat.writemiss = 0;

	// initialize following variables
	cache_struct *cache;
	int num_set = ((capacity / block_size) / num_way);  // changed

	/***************************************************************/
	/*                                                             */
	/*                       Implement here                        */
	/*                                                             */
	/***************************************************************/
	
	cache = build_cache(num_set, num_way, block_size);

	char buffer[16];
	char cmd;
	uint32_t address;
	int op, global_index = 0;

	while(fgets(buffer, 16, fp) != NULL){
		cmd = buffer[0];
		sscanf(buffer+2, "%x", &address);

		if(cmd == 'R') op = R;
		else op = W;

    	uint32_t mask = ~((1L << get_bit(block_size)) - 1);
		access_cache(cache, &stat, op, address & mask, lru_only_flag, global_index);

		global_index += 1;
	}

	sdump(&stat);
	if (xflag)
	{
		xdump(cache, num_set, num_way, block_size);
	}
 
	return 0;
}