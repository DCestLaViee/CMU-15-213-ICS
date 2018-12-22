#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include "cachelab.h"

typedef struct {
	bool valid;
	bool dirty;
	unsigned long tag;
	long time;
} Node;

bool help;
bool verbose;
int s;
int e;
int b;

int set_size;
int line_size;
unsigned long set_index_mask;
Node *cache;

int hit_count;
int miss_count;
int evict_count;

void replay(char *file_path);
unsigned long getSetIndex(unsigned long addr);
unsigned long getTag(unsigned long addr);
void execInstruction(char ins, unsigned long addr);
Node* getNode(unsigned long addr);

int main(int argc, char *argv[]) {
	int ch;
	char file_path[100];

	while((ch = getopt(argc, argv, ":hvs:E:b:t:")) != -1) {
		switch(ch) {
			case 'h':
				help = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				e = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(file_path, optarg);
				break;
			case ':':
				printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>");
				exit(1);
	    	case '?':
               printf("Unknown option: %c\n",(char)optopt);
               exit(1);
		}
	}

	set_size = 1 << s;
	line_size = e;
	set_index_mask = ~(-1 << s);
	cache = (Node *) malloc(sizeof(Node) * set_size * line_size);
	for(int i = 0; i < set_size * line_size; ++i) {
		cache[i].valid = false;
	}
	replay(file_path);
	printSummary(hit_count, miss_count, evict_count);
}

void replay(char *file_path) {
	FILE *fp;

	if((fp = fopen(file_path,"r")) == NULL) {
	    printf("The file can not be opened.\n");
	    exit(1);
	}

	char ins;
	unsigned long addr;
	int size;
	int count;
	while((count = fscanf(fp, "%c %lx,%d", &ins, &addr, &size)) != EOF) {
		if(count == 3) {
			printf("%c %lx,%d", ins, addr, size);
			execInstruction(ins, addr);
			printf("\n");
		}
	}
	fclose(fp);
}

unsigned long getSetIndex(unsigned long addr) {
	return (addr >> b) & set_index_mask;
}

unsigned long getTag(unsigned long addr) {
	return addr >> (s + b);
}

void execInstruction(char ins, unsigned long addr) {
	Node *node = NULL;
	if(ins == 'M') {
		// simulate load
		node = getNode(addr);
		// simulate write
		node = getNode(addr);
		node->dirty = true;
	} else if(ins == 'L') {
		// simulate load
		node = getNode(addr);
	} else if(ins == 'S') {
		// simulate write
		node = getNode(addr);
		node->dirty = true;
	}
}

Node* getNode(unsigned long addr) {
	static long time = 0;

	unsigned long tag = getTag(addr);
	unsigned long set_index = getSetIndex(addr);

	Node *start = cache + set_index * line_size;
	Node *replace = start;
	for(int i = 0; i < line_size; ++i) {
		Node *cur = start + i;
		if(cur->valid) {
			if(cur->tag == tag) {
				++hit_count;
				cur->time = ++time;
				if(verbose) {
					printf(" hit");
				}
				return cur;
			} else if(replace->valid && cur->time < replace->time) {
				replace = cur;
			}
		} else if(replace->valid) {
			replace = cur;
		}
	}

	++miss_count;
	if(verbose) {
		printf(" miss");
	}
	if(replace->valid) {
		++evict_count;
		if(verbose) {
			printf(" eviction");
		}
	}

	replace->valid = true;
	replace->dirty = false;
	replace->tag = tag;
	replace->time = ++time;
	return replace;
}