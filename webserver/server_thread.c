#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

void* thread_request(void* _sv);



struct node {
    int info;
    struct node* next;
};
struct queue {
    int curr_size;
    int max_size;
    struct node* elem;
};

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
        pthread_mutex_t* lock;
        pthread_cond_t* cv_empty;
        pthread_cond_t* cv_full;
        struct queue* pool;
        struct cache* s_cache;
};

/* cache */
struct entry{
    char* file_name;
    struct file_data* data;
    struct entry* next;
};
struct wc{
    struct entry* elem;
    //int hash_size;
};

struct cache {
    pthread_mutex_t* cache_lock;
    struct newQueue* page_list;
    struct wc* table;
    int table_size;
    int max_size;
    int curr_size;
};

struct newNode{
    struct file_data* data;
    struct newNode* next;
};

struct newQueue{

    struct newNode* elem;
};
//int cache_lookup(struct cache* , struct file_data* );
void queue_putbot(struct newQueue*, struct file_data*);
struct file_data* queue_outtop(struct newQueue* );
void cache_insert(struct cache* , struct file_data*);
void cache_evict(struct cache* );
int table_hashval(struct cache*, char*);
struct file_data* table_search(struct wc*, char*, int);
void table_insert(struct wc*, struct file_data*, int);
void table_remove(struct wc*, char*, int);



/* new queue function*/
// When current size less than max size
void queue_putbot(struct newQueue* queue, struct file_data* _data){
    if (queue->elem == NULL){
        queue->elem = malloc(sizeof(struct newNode));
        queue->elem->data = malloc(sizeof(struct file_data));
        queue->elem->data->file_name = malloc(sizeof(char)*(strlen(_data->file_name)+1));
        queue->elem->data->file_buf = malloc(sizeof(char)*(strlen(_data->file_buf)+1));
        strcpy(queue->elem->data->file_name, _data->file_name);
        strcpy(queue->elem->data->file_buf, _data->file_buf);
        
        queue->elem->data->file_size = _data->file_size;
        //queue->elem->data = _data;
        queue->elem->next = NULL;
      
        return;
    }
    struct newNode* newOne;
    newOne = malloc(sizeof(struct newNode));
    newOne->data = malloc(sizeof(struct file_data));
    newOne->data->file_name = malloc(sizeof(char)*(strlen(_data->file_name)+1));
    newOne->data->file_buf = malloc(sizeof(char)*(strlen(_data->file_buf)+1));
    strcpy(newOne->data->file_name, _data->file_name);
    strcpy(newOne->data->file_buf, _data->file_buf);
    
    
    newOne->data->file_size = _data->file_size;
    //newOne->data = _data;
    newOne->next = NULL;
    
    struct newNode* curr = queue->elem;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = newOne;

    return;
}
// When current size reaches the max size
struct file_data* queue_outtop(struct newQueue* queue){
    struct newNode* temp = queue->elem;
    struct file_data* target = temp->data;
    queue->elem = queue->elem->next;
    return target;
}

/* cache function */
void cache_init(struct cache* _cache, int cache_max_size){
    int i = 0;
    _cache->cache_lock = malloc(sizeof(pthread_mutex_t));
    _cache->curr_size = 0;
    _cache->max_size = cache_max_size;
    _cache->page_list = malloc(sizeof(struct newQueue));
    _cache->table_size = 100000;
    _cache->table = malloc(sizeof(struct wc)*(_cache->table_size));
    for (; i < _cache->table_size; i++){
        _cache->table[i].elem = NULL;
    }
}
int cache_lookup(struct cache* _cache, struct file_data* _data){
    pthread_mutex_lock(_cache->cache_lock);
    if (_cache == NULL || _data == NULL || _data->file_name == NULL){
        pthread_mutex_unlock(_cache->cache_lock);
        return 0;
    }
    // Need to look up from table
    int pos;
    pos = table_hashval(_cache,_data->file_name);
    struct file_data* target = table_search(_cache->table, _data->file_name, pos);
    if (target == NULL){
        pthread_mutex_unlock(_cache->cache_lock);
        return 0;
    }
    else{
        _data->file_buf = target->file_buf;
        _data->file_size = target->file_size;
        pthread_mutex_unlock(_cache->cache_lock);
        return 1;
    }
}

void cache_insert(struct cache* _cache, struct file_data* _data){
    pthread_mutex_lock(_cache->cache_lock);
    if (_cache == NULL || _data == NULL || _data->file_name == NULL){
        pthread_mutex_unlock(_cache->cache_lock);
        return;
    }
    if (_data->file_size > _cache->max_size){
       // printf("too big! ,%s\n", _data->file_name);
        pthread_mutex_unlock(_cache->cache_lock);
        return;
    }
    // Need to insert
    int pos;
    _cache->curr_size += _data->file_size ;
    
    // Modify the LRU queue and insert the element into table
    
    while (_cache->curr_size > _cache->max_size){
        cache_evict(_cache);
    }
    
    pos = table_hashval(_cache, _data->file_name);
    table_insert(_cache->table, _data, pos);
    queue_putbot(_cache->page_list, _data);
    pthread_mutex_unlock(_cache->cache_lock);
    return;
}

void cache_evict(struct cache* _cache){
    
   // pthread_mutex_lock(_cache->cache_lock);
    if (_cache == NULL){
       // pthread_mutex_unlock(_cache->cache_lock);
        return;
    }
    // Modify the LRU queue and curr_size
    int pos;
 
    struct file_data* target = queue_outtop(_cache->page_list);
    pos = table_hashval(_cache, target->file_name);
    table_remove(_cache->table, target->file_name, pos);

    _cache->curr_size -= target->file_size;
    //pthread_mutex_unlock(_cache->cache_lock);
    return;
}

/* hashtable function */
// Do I need the key?
int table_hashval(struct cache* _cache, char* name){
    // for testing
  
    unsigned long long hashval = 0;
    int i, pos;
    for (i = 0; i < strlen(name); i++){
        hashval = hashval*31 + (int)(name[i]);
    }
    pos = (int)(hashval % _cache->table_size);
    return pos;
}

struct file_data* table_search(struct wc* table, char* name, int pos){
   
    if (table[pos].elem == NULL){
        //printf("NOt found!!, %s\n", name);
        return NULL;
    }
    
    struct entry* curr = table[pos].elem;
    struct file_data* target = NULL;
    while(curr != NULL){
        if (strcmp(name, curr->file_name) == 0){
            target = curr->data;
            break;
        }
        else{
            curr = curr->next;
        }
    }

    return target;
}

void table_insert(struct wc* table, struct file_data* _data, int pos){
 
    
    if (table[pos].elem == NULL){
        table[pos].elem = malloc(sizeof(struct entry));
        table[pos].elem->file_name = malloc(sizeof(char)*(strlen(_data->file_name)+1));
        table[pos].elem->data = malloc(sizeof(struct file_data));
        table[pos].elem->data->file_buf = malloc(sizeof(char)*(strlen(_data->file_buf)+1));
        
        strcpy(table[pos].elem->file_name, _data->file_name);
        //table[pos].elem->data = _data;
        strcpy(table[pos].elem->data->file_buf, _data->file_buf);
        table[pos].elem->data->file_size = _data->file_size;
        table[pos].elem->next = NULL;
        
    }
    else{
        struct entry* newEntry = malloc(sizeof(struct entry));
        newEntry->file_name = malloc(sizeof(char)*(strlen(_data->file_name)+1));
        newEntry->data = malloc(sizeof(struct file_data));
        newEntry->data->file_buf = malloc(sizeof(char)*(strlen(_data->file_buf)+1));
        strcpy(newEntry->file_name,_data->file_name);
        //newEntry->data = _data;
        strcpy(newEntry->data->file_buf, _data->file_buf);
        newEntry->data->file_size = _data->file_size;
        newEntry->next = NULL;
        struct entry* curr = table[pos].elem;
        while (curr->next != NULL){
            curr = curr->next; 
        }
        curr->next = newEntry;   
    }
    return;
}
// May have Memory leak problem
void table_remove(struct wc* table, char* name, int pos){
 
    struct entry* curr = table[pos].elem;
    struct entry* prev = NULL;
    // If located at the head
    if (table[pos].elem == NULL){
        return;
    }
    if (strcmp(curr->file_name, name) == 0){
        struct entry* temp = curr;
        table[pos].elem =curr->next;
        free(temp->file_name);
        free(temp);
        return;
    }
    // If not
    prev = curr;
    curr = curr->next;
    while(curr != NULL){
        if (strcmp(curr->file_name, name) == 0){
            prev->next = curr->next;
            curr->next = NULL;
            free(curr->file_name);
            free(curr);
        }
        else{
            prev = curr;
            curr = curr->next;
        }
    }
}

/* new queue for LRU */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret, hit;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fills data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* reads file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */
        // Now with cache
        /*
	ret = request_readfile(rq);
	if (!ret)
		goto out;
	
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);*/
    //   printf("%s , tid: %d, requesting \n", data->file_name, (int)pthread_self());
        hit = cache_lookup(sv->s_cache, data);
        if (hit){
         //  printf("%s, hit!\n", data->file_name);
            request_sendfile(rq);
            request_destroy(rq);
        }
        else{
            ret = request_readfile(rq);
            if (!ret){
		goto out;
            
            }
          //  printf("%s, miss!\n", data->file_name);
           cache_insert(sv->s_cache, data);
           //printf("%s, have inserted!\n", data->file_name);
	    request_sendfile(rq);
out:
	   request_destroy(rq);
	    file_data_free(data);
        }
    //   printf("tid: %d, finishing\n", (int)pthread_self());
        
        
}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;

	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
            int i;
            sv->pool = malloc(sizeof(struct queue));
            sv->pool->elem = NULL;
            sv->pool->curr_size = 0;
            sv->pool->max_size = max_requests;
            sv->lock = malloc(sizeof(pthread_mutex_t));
            sv->cv_empty = malloc(sizeof(pthread_cond_t));
            sv->cv_full = malloc(sizeof(pthread_cond_t));
            sv->s_cache = malloc(sizeof(struct cache));
            
            // initialize the cache
            cache_init(sv->s_cache, max_cache_size);
            
            pthread_t* threadList = malloc(nr_threads*sizeof(pthread_t));
            for (i = 0; i < nr_threads; i++){
                pthread_create(threadList+i,NULL,thread_request,(void*)sv);
            }
	}
        
	/* Lab 4: create queue of max_request size when max_requests > 0 */
        
	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}
// queue function 

// Push one connfd into the queue
void queue_push(struct server* sv, int connfd){
    if (sv->pool->curr_size == 0){
        sv->pool->elem = malloc(sizeof(struct node));
        sv->pool->elem->info = connfd;
        sv->pool->elem->next = NULL;
        sv->pool->curr_size ++;
        return;
    }
    struct node* newNode;
    newNode = malloc(sizeof(struct node));
    newNode->info = connfd;
    newNode->next = NULL;
    struct node* curr = sv->pool->elem;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = newNode;
    sv->pool->curr_size ++;
    return;
}

// Pop returns the connfd
int queue_pop(struct server* sv){
    struct node* temp = sv->pool->elem;
    sv->pool->elem = sv->pool->elem->next;
    int target = temp->info;
    free(temp);
    sv->pool->curr_size --;
    return target;
}

int queue_empty(struct server* sv){
    if (sv->pool->curr_size == 0){
        return 1;
    }
    return 0;
}

int queue_full(struct server* sv){
    if (sv->pool->curr_size == sv->pool->max_size){
        return 1;
    }
    return 0;
}

// Consumer, worker threads
void *thread_request(void* _sv){
    struct server* sv = (struct server*) _sv;
    while (1){
    pthread_mutex_lock(sv->lock);
    while (queue_empty(sv)){
        pthread_cond_wait(sv->cv_empty,sv->lock);
    }
    int target = queue_pop(sv);
    if (sv->pool->curr_size < sv->pool->max_size){
        pthread_cond_signal(sv->cv_full);
    }
    pthread_mutex_unlock(sv->lock);
    do_server_request(sv,target);
    
    }
}
// Producer, master thread
void server_request(struct server *sv, int connfd)
{
    if (sv->nr_threads == 0) { /* no worker threads */
        do_server_request(sv, connfd);
    } 
    else {
        int temp = connfd;
        pthread_mutex_lock(sv->lock);
        // Queue is full
        while (queue_full(sv)){
            pthread_cond_wait(sv->cv_full,sv->lock);
        }
        queue_push(sv,temp);
        // New element exists
        if (sv->pool->curr_size > 0){
            pthread_cond_signal(sv->cv_empty);
        }
        pthread_mutex_unlock(sv->lock);
    }
}




