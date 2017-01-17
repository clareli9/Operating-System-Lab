#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

/* This is the thread control block */
struct thread {
    // The status: 0 is blocked, 1 is running, 2 is exited
    unsigned int status;
    // The signal: 0 is enabled, 1 is disabled
    unsigned int sigenable;
    Tid id;
    ucontext_t context;
    void* stack;
    
    //struct thread* next;
};
struct node {
    Tid id;
    struct node* next;
};
// The queue to store the ready thread ID
struct node* readyQueue;
struct node* destroyQueue;

// The thread which is currently running

struct thread* currentThread;
// The array which stores the status of each thread
// 1: current working, 0: ready, -1: to be deleted
int TidList[THREAD_MAX_THREADS] = {0};
// The array which stores all threads
struct thread threadList[THREAD_MAX_THREADS];

// Helper functions of ready queue
struct node* queue_insert(struct node* head, Tid target){
    if (head == NULL){
        head = malloc(sizeof(struct node));
        head->id = target;
        head->next = NULL;
        return head;
    }
    struct node* newNode;
    newNode = malloc(sizeof(struct node));
    newNode->id = target;
    newNode->next = NULL;
    struct node* curr = head;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = newNode;
    return head;
}

struct node* queue_delete (struct node* head, Tid target){
    if (head == NULL){
        return NULL;
    }
    if (target == head->id){
        struct node* temp = head;
        head = head->next;
        free(temp);
        return head;
    }
    struct node* curr = head;
    struct node* prev = NULL;
    while (target != curr->id ){
        prev = curr;
        curr = curr->next;
    }
    prev->next = curr->next;
    curr->next = NULL;
    free(curr);
    return head;
}
// Helper function to delete one thread
void thread_free(struct thread* target){
    free(target->stack);
    target->stack = NULL;
    // How about other registers?
    target->context.uc_mcontext.gregs[15] = 0x0;
    
    // Remove it from the ready queue
    readyQueue = queue_delete(readyQueue,target->id);
    TidList[target->id] = 0;
}

void thread_id_free(Tid tid){
    struct thread* target = &(threadList[tid]);
    
    free(target->stack);
    target->stack = NULL;
    target->context.uc_mcontext.gregs[15] = 0x0;
    readyQueue = queue_delete(readyQueue,tid);
    TidList[tid] = 0;
    target = NULL;
}

void
thread_init(void)
{
    readyQueue = NULL;
    destroyQueue = NULL;
    threadList[0].id = 0;
    // Change the signal state?
    threadList[0].sigenable = 0;
    TidList[0] = 1;
    currentThread = &threadList[0];
    // Implement the signal
    //register_interrupt_handler(1);
    //int enable = interrupts_on();
    //assert(enable);
}

void
thread_stub(void (*thread_main)(void *), void *arg)
{
	Tid ret;
        if(!interrupts_enabled()){
                   //printf("hahaha yield any %d", enabled);
            interrupts_on();
        }
	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	// all threads are done, so process should exit
	exit(0);
}

Tid
thread_id()
{
	
    return currentThread->id;
}
static void
call_setcontext(ucontext_t * context)
{
	int err = setcontext(context);
	assert(!err);
}
Tid
thread_create(void (*fn) (void *), void *parg)
{
    // Decide which Tid to use
    //printf("create in\n");
    int i;
    int check, enabled;
    for (i = 0; i < THREAD_MAX_THREADS; i++){
        if (TidList[i] == 0){
            break;
        }
    }
    if (i == THREAD_MAX_THREADS){
        return THREAD_NOMORE;
    }
   
    struct thread* newThread = &(threadList[i]);
    //newThread.status = 0;
    enabled = interrupts_off();
    assert(enabled);
    check = getcontext(&(newThread->context));
    assert(!check);
    // Allocate a new stack
    newThread->id = (Tid)i;
    newThread->stack = (void*)malloc(THREAD_MIN_STACK + 8);
    // Change the stack pointer
    if (newThread->stack == NULL){
        return THREAD_NOMEMORY;
    }
    newThread->context.uc_mcontext.gregs[15] = (unsigned long)newThread->stack + THREAD_MIN_STACK + 8;
    // Change the arguments
    newThread->context.uc_mcontext.gregs[REG_RDI] = (unsigned long)fn;
    newThread->context.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;
    // Change the PC
    newThread->context.uc_mcontext.gregs[16] = (unsigned long)((thread_stub));
    // Put the element into ready queue 
    readyQueue = queue_insert(readyQueue,(Tid)i);
    TidList[i] = 1;
    
    enabled = interrupts_on();
    assert(!enabled);
    return (Tid)i;
}

Tid
thread_yield(Tid want_tid)
{   

    volatile int setcontext_called = 0;
    int check;
    struct thread* nextThread;
    // Destroy all threads in destroyQueue
    interrupts_off();
    
    struct node* curr = destroyQueue;
    while (curr != NULL){
        Tid target = curr->id;
        curr = curr->next;
        if ((int)target != currentThread->id){
            thread_id_free(target);
            destroyQueue = queue_delete(destroyQueue,target);
        }
    }
    //enabled = interrupts_on();
    //assert(!enabled);
    if (want_tid == THREAD_SELF){
        return currentThread->id;
        
    }
    if (want_tid == THREAD_ANY){
        if (readyQueue == NULL){
            return THREAD_NONE;
        }
        else{
            
            nextThread = &threadList[readyQueue->id];
            // Check the whether the main thread is exit or not

            if (currentThread->id == 0 && TidList[0] == -1){
                readyQueue = queue_delete(readyQueue, readyQueue->id);
            }
            else{
                readyQueue = queue_insert(readyQueue, currentThread->id);
                readyQueue = queue_delete(readyQueue, readyQueue->id);
            }
     

            check = getcontext(&(currentThread->context));
           
            assert(!check);
            if (setcontext_called == 1){
                if(!interrupts_enabled()){
                  // printf("hahaha yield any %d", enabled);
                   interrupts_on();
               }
                return currentThread->id;
            }
            currentThread = nextThread;
            setcontext_called = 1;

            call_setcontext(&(nextThread->context));
        }
    }
    if (want_tid < 0 || want_tid >= THREAD_MAX_THREADS || TidList[(int)want_tid] == 0){
        return THREAD_INVALID;
    }
    // Discuss the situation about specific Tid
    interrupts_off();
    
    nextThread = &(threadList[want_tid]);
    readyQueue = queue_insert(readyQueue, currentThread->id);
    readyQueue = queue_delete(readyQueue, want_tid);
    
   
    
    check = getcontext(&(currentThread->context));
    assert(!check);

    if (setcontext_called == 1){
        if (!interrupts_enabled()){
           
           interrupts_on();
        }
        return nextThread->id;
    }
    currentThread = nextThread;
    setcontext_called = 1;
    
    call_setcontext(&(nextThread->context));
    
    return THREAD_FAILED;
}

Tid
thread_exit()
{   
    // Exit the current running thread
    // Check the thread to exit is main thread or not
    // If the main thread is to be exited, continue to check the remain threads 
    // in ready queue.
    int enabled;
    enabled = interrupts_off();
    
    if (currentThread->id == 0){

        if (readyQueue != NULL) {
            TidList[currentThread->id] = -1;
            thread_yield(THREAD_ANY);
       }
        return THREAD_NONE;
    }
    TidList[currentThread->id] = -1;
    destroyQueue = queue_insert(destroyQueue,currentThread->id);
    
    enabled = interrupts_on();
    assert(!enabled);
    thread_yield(THREAD_ANY);
    
    return THREAD_NONE;
}

Tid
thread_kill(Tid tid)
{   
    if (TidList[tid] == 0  || currentThread->id == tid || tid < 0 || tid >= THREAD_MAX_THREADS){
        return THREAD_INVALID;
    }
    
    TidList[tid] = -1;
    struct thread *killedThread = &(threadList[tid]);
    killedThread->context.uc_mcontext.gregs[16] = (unsigned long)(thread_exit);
    
    return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
    Tid wait;
    struct wait_queue* next;
};

struct wait_queue* insert (struct wait_queue* head, Tid target){
    if (head == NULL){
        head = malloc(sizeof(struct wait_queue));
        head->wait = target;
        head->next = NULL;
        return head;
    }
    struct wait_queue* newNode;
    newNode = malloc(sizeof(struct wait_queue));
    newNode->wait = target;
    newNode->next = NULL;
    struct wait_queue* curr = head;
    while (curr->next != NULL){
        curr = curr->next;
    }
    curr->next = newNode;
    return head;
}

struct wait_queue* queue_remove (struct wait_queue* head, Tid target){
    if (head == NULL){
        return NULL;
    }
    if (target == head->wait){
        struct wait_queue* temp = head;
        head = head->next;
        free(temp);
        return head;
    }
    struct wait_queue* curr = head;
    struct wait_queue* prev = NULL;
    while (target != curr->wait ){
        prev = curr;
        curr = curr->next;
    }
    prev->next = curr->next;
    curr->next = NULL;
    free(curr);
    return head;
}

struct wait_queue* remove_all (struct wait_queue* head){
    if (head == NULL){
        return NULL;
    }
    if (head->next == NULL){
        return head;
    }
    
    return remove_all(queue_remove(head,head->next->wait));
}

struct wait_queue* wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);
        wq->next = NULL;
        wq->wait = -1;
	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
    /*
    struct wait_queue* curr = wq;
    struct wait_queue* temp = NULL;
    while (curr != NULL){
        //thread_id_free(temp->wait);
        temp = curr;
        curr = curr-> next;
        temp->next = NULL;
        free(temp);
    }*/
    free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
    volatile int setcontext_called = 0;
    struct thread* nextThread;
    
    if (queue == NULL){
        return THREAD_INVALID;
    }
    if (readyQueue == NULL){
        return THREAD_NONE;
    }
    
       int en =  interrupts_off();
    
    nextThread = &threadList[readyQueue->id];
    readyQueue = queue_delete(readyQueue,nextThread->id);
    assert(queue != NULL);
    queue = insert(queue,currentThread->id);
    getcontext(&(currentThread->context));
    if (setcontext_called == 1){
        interrupts_set(en);
            return currentThread->id;
        }
        currentThread = nextThread;
        setcontext_called = 1;

        call_setcontext(&(nextThread->context));
    
    return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    int num = 0;
    if (queue == NULL || queue->next == NULL){
	return 0;
    }
    if (all == 0){
        if (interrupts_enabled()){
            interrupts_off();
        }
        Tid temp = queue->next->wait;
        queue = queue_remove(queue,temp);
        
        readyQueue = queue_insert(readyQueue,temp);
        if (!interrupts_enabled()){
            interrupts_on();
        }
        //printf("%d\n",queue->wait);
        return 1;
    }
    
    if (all == 1){
        int en = interrupts_off();
        struct wait_queue* curr = queue->next;
        while (curr != NULL){
            Tid temp = curr->wait;
            readyQueue = queue_insert(readyQueue,temp);
            num ++;
            curr = curr->next;
        }
        queue = remove_all(queue);
       // assert(queue != NULL);
        interrupts_set(en);
        return num;
    }
    return num;
}

struct lock {
	/* ... Fill this in ... */
    // 0: available 1: occupied
    //unsigned int status;
    // 0: not 1: acquired
    //unsigned int called;
    struct thread* holder;
    struct wait_queue* queue;
};

struct lock *
lock_create()
{
	struct lock *lock;
        
	lock = malloc(sizeof(struct lock));
        assert(lock);
        lock->queue = wait_queue_create();
        //lock->status = 0;
        lock->holder = NULL;
        //lock->called = 0;
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL); 
        lock->holder = NULL;
        wait_queue_destroy(lock->queue);
	free(lock);
}

void
lock_acquire(struct lock *lock)
{      int en =  interrupts_off();
	assert(lock != NULL);
        
        // When the lock is avaliable
        if (lock->holder == NULL){
            //lock->status = 1;
            lock->holder = currentThread;
        }
        // When the lock is occupied by other thread
        while (lock->holder != currentThread){
            thread_sleep(lock->queue); 
            //assert(!interrupts_enabled());
            if (lock->holder == NULL){
                lock->holder = currentThread;
            }
        }    
        interrupts_set(en);
        return;
}
void
lock_release(struct lock *lock)

{       int en = interrupts_off();
	assert(lock != NULL);
        
        // Disable the interrupt
        

        //assert(lock->holder == currentThread);
        
        if (lock->holder == currentThread){
            lock->holder = NULL;
            thread_wakeup(lock->queue,1);  
        }
        interrupts_set(en);
        /*
        if (lock->queue->next == NULL){
            lock->status = 0;
            interrupts_on();
            return;
        }
        target = lock->queue->next->wait;
        nextThread = &threadList[target];
        lock->queue = queue_remove(lock->queue,target);
        
        getcontext(&(currentThread->context));
        if (setcontext_called == 1){
            if(!interrupts_enabled()){
                interrupts_on();
            }
                return ;
        }
        currentThread = nextThread;
        setcontext_called = 1;

        call_setcontext(&(nextThread->context));
         */
        
}
struct cv {
    // 0:default, 1:wait, 2:signal
    // unsigned int signal;
    struct wait_queue* queue;
    
};

struct cv *
cv_create()
{
	struct cv *cv;
	cv = malloc(sizeof(struct cv));
	assert(cv);
        cv->queue = wait_queue_create();
        //cv->signal = 0;
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

        wait_queue_destroy(cv->queue);
	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{       int en =interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
        
        lock_release(lock);
        thread_sleep(cv->queue);
        lock_acquire(lock);
	interrupts_set(en);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{       int en =interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
        
        if (currentThread == lock->holder){
	    thread_wakeup(cv->queue,0);
        }
        interrupts_set(en);
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{       
       int en =interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
        //struct wait_queue* temp = cv->queue
        if (currentThread == lock->holder){
	    thread_wakeup(cv->queue,1);
        }
        interrupts_set(en);
}
