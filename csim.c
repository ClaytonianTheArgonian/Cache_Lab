
/*
 * Author: Clayton James cathey
 *
 * single-signon ID: cc996142
 *
 */

#include "cachelab.h"
#include <strings.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#define ADDRESS_LENGTH 64

/*
 * This struct will be used to handle info about whatever cache is currently being worked with.
 * We handle it as a struct since their are no objects in C
 */
struct cache_details{
    int s;
    int b;
    int E;
    int S;
    int B;
    int hit;
    int miss;
    int evict;
    int pos;
};

/*
Type: Memory address
*/
typedef unsigned long long int mem_addr_t;

/*
Type: Cache line
LRU is a counter used to implement LRU replacement policy
*/
typedef struct cache_line {
char valid;
mem_addr_t tag;
unsigned long long int lru;
} cache_line_t;

/*
 * This function is used to print out the help menu. When the help menu is called the program exits
 */
void help() {
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n-h         Print this help message.\n");
    printf("-v         Optional verbose flag.\n-s <num>   Number of set index bits.\n");
    printf("-E <num>   Number of lines per set.\n-b <num>   Number of block offset bits.\n-t <file>  Trace file.\n");
}

void print_summary(int hit,int miss,int evict) {
    printf("hits:%d misses:%d evictions:%d\n",hit,miss,evict);
}

void age_up(struct cache_line** cache, int set, int E) {
    for (int i = 0; i<E; i++) {
        cache[set][i].lru++;
    }
    return;
}


/*Search given set for hit or miss, return 1 for hit -1 for miss*/
int set_search(int set_num, mem_addr_t tag,struct cache_line** cache,struct cache_details* result) {
    int len = result->E;
    for (int i=0;i<len;i++) {
        if ((cache[set_num][i].valid) && (cache[set_num][i].tag == tag)) {
            if (len == 1) {
                result->pos=0;
                return 1;
            }
            result->pos = i;
            return 1;
        }
    }
    return 0;
}

void cache_hit(struct cache_line** cache,int set, int pos, int E) {
    age_up(cache,set,E);
    cache[set][pos].lru=0;
}

void cache_miss(struct cache_line** cache, int E, int set, mem_addr_t tag,struct cache_details* result) {
    age_up(cache,set,E);
    int high = cache[set][0].lru;
    int pos = 0;
    int found = 0;
    for (int i=0; i<E;i++) {
        if (cache[set][i].lru > high){
            high = cache[set][i].lru;
            pos = i;
        }

        if (!cache[set][i].valid) {
            cache[set][i].tag = tag;
            cache[set][i].lru = 0;
            cache[set][i].valid = 1;
            found = 1;
            break;
        }
    }

    if (found) {
        return;
    }

    result->evict++;
    cache[set][pos].tag = tag;
    cache[set][pos].lru = 0;
    cache[set][pos].valid = 1;    
    return;
}

void find(struct cache_line** cache ,mem_addr_t address,struct cache_details* result) {
    int found;
    mem_addr_t in_tag = address >> (result->s + result->b); // Since tag is the first amount of bits
    int set_num = (address >> result->b) & ~(~0 << result->s); // Mask Relevant Set Number from Address
    found = set_search(set_num,in_tag,cache,result);

    if (found) {
        cache_hit(cache,set_num,result->pos,result->E);
        result->hit++;
        return;
    }

    else {
        result->miss++;
        cache_miss(cache,result->E,set_num,in_tag,result);
        return;
    }
}

// Function to free the allocated memory
void free_cache(struct cache_line** cache, struct cache_details* result) {
    for (int i=0; i<result->S; i++) {
        free(cache[i]);
    }    
    free(cache);
}

// argc is to count arguments argv to creata array of arguments
int main(int argc, char** argv) { 
    struct cache_details result;
    int size;
    char inst_usd;
    mem_addr_t address;
    char* trace_file;
    FILE* in_file;
    char c;

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1) {
        switch(c) {
            case 's':
                result.s = atoi(optarg);
                break;
            case 'E':
                result.E = atoi(optarg);
                break;
            case 'b':
                result.b = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'h':
                help();
                exit(0);
            default:
                exit(1);
        }
    }
    /*Compute the needed values to utilize the cache*/
    result.S = pow(2.0,result.s);
    /*This is init the cache*/
    struct cache_line** cache =(struct cache_line**) malloc(sizeof(struct cache_line*) * result.S);
    for (int i=0; i<result.S; i++) {
        cache[i]=(struct cache_line*) malloc(sizeof(struct cache_line)*result.E);
        for (int j=0; j<result.E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = 0;
            cache[i][j].lru = 0;
        }
    }

    in_file = fopen(trace_file, "r");
    // Check if opened file is empty
    if (in_file != NULL) {
        // Read while we get three arguments
        while (fscanf(in_file, " %c %llx,%d", &inst_usd, &address, &size) == 3) {
            switch(inst_usd) {
                case 'I':
                    break;
                case 'L':
                    find(cache,address,&result);
                    break;
                case 'S':
                    find(cache,address,&result);                    
                    break;
                case 'M':
                    find(cache,address,&result);
                    find(cache,address,&result);                    
                    break;
                default:
                    break;
            }            
        }
    }

    printSummary(result.hit, result.miss, result.evict);
    // free All used memory
    free_cache(cache,&result);
    return 0;
}
