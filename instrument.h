#include <mach/mach_time.h>
#include <unistd.h>
#include <pthread.h>
#include <malloc/malloc.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdatomic.h>
#include <os/signpost.h>
#define LOGTYPE int
#define LOGGING 0


#if defined(__MACH__)

#undef LOGTYPE
#undef LOGGING
static mach_timebase_info_data_t info = {0};
/*
static pthread_mutex_t csMalloc = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csFree = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t csRealloc = PTHREAD_MUTEX_INITIALIZER;
#define LOCKM  pthread_mutex_lock(&csMalloc)
#define UNLOCKM pthread_mutex_unlock(&csMalloc)
#define LOCKF pthread_mutex_lock(&csFree)
*/
#define OSSIZEOFMALLOC(ptr) malloc_size(ptr)

#define LOGTYPE os_log_t

#define GETTHREAD ({ \
    uint64_t thread;\
    pthread_threadid_np(NULL, &thread);\
    thread;\
    })
#define UNLOCKF pthread_mutex_unlock(&csFree)
#define LOCKR pthread_mutex_lock(&csRealloc)
#define UNLOCKR pthread_mutex_unlock(&csRealloc)
__attribute__((constructor)) static void init_timebase_info() {
    
    mach_timebase_info(&info);    
}
uint64_t get_time_ns() {
    return mach_absolute_time();
}
static int rate=1000;
void setrate(int x){
    rate=x;
}
#define TIMETYPE uint64_t
#define THREADTYPE uint64_t
#define TIME_N get_time_ns()
uint64_t getTimeDif(TIMETYPE t1, TIMETYPE t2){
    uint64_t time1 =(t1*info.numer)/info.denom;
    uint64_t time2 =(t2*info.numer)/info.denom;
    return time2-time1;
}
#endif

#define CMALLOC(size) malloc(size)
#define CFREE(ptr) free(ptr)
#define CREALLOC(ptr,size) realloc(ptr,size)
#define CCALLOC(val,size) calloc(val,size)
#define CSIZEOFMALLOC(ptr) OSSIZEOFMALLOC(ptr)
#define MALLOCLOGGING "Default Malloc"
#ifdef USE_JEMALLOC
    #include <jemalloc/jemalloc.h>
    #undef CMALLOC
    #undef CFREE
    #undef CCALLOC
    #undef CREALLOC
    #undef MALLOCLOGGING
    #undef CSIZEOFMALLOC
    #define MALLOCLOGGING "JE Malloc"
    #define CMALLOC(size) je_malloc(size)
    #define CFREE(ptr) je_free(ptr)
    #define CSIZEOFMALLOC(ptr) je_malloc_usable_size(ptr)
    #define CREALLOC(ptr,size) je_realloc(ptr,size)
    #define CMALLOCX(size,flag) je_mallocx(size,flag)
    #define CCALLOC(info,size) je_calloc(info,size)
#elif USE_MIMALLOC
#include<mimalloc-2.2/mimalloc.h>
#undef CMALLOC
#undef CFREE
#undef CCALLOC
#undef CREALLOC
#undef MALLOCLOGGING
#undef CSIZEOFMALLOC
#define MALLOCLOGGING "MIMalloc"
#define CMALLOC(size) mi_malloc(size)
#define CFREE(ptr) mi_free(ptr)
#define CSIZEOFMALLOC(ptr) mi_malloc_usable_size(ptr)	
#define CREALLOC(ptr,size) mi_realloc(ptr,size)
#define CCALLOC(info,size) mi_calloc(info,size)
#endif
#define MALLOC(size) CMALLOC(size)
#define FREE(ptr) CFREE(ptr)
#define REALLOC(ptr,size) CREALLOC(ptr,size)
#define CALLOC(val,size) CCALLOC(val,size)
#define SIZEOFMALLOC(ptr) CSIZEOFMALLOC(ptr)

#define MAX_SIZE 1024

static __thread  os_log_t log =NULL;
size_t defineSize(size_t size){
    printf("%zu",size);
    size_t x=size;
    return x;
}
static __thread unsigned int tls_seed=-1;
static __thread unsigned int tls_seed2=-1;
static __thread unsigned int tls_seed3=-1;
static __thread unsigned int tls_seed4=-1;
char* generate_unique_log_name() {
    uint64_t timestamp = mach_absolute_time();
   
    char* log_name = malloc(100);
    snprintf(log_name, 100, "%llu", GETTHREAD);
    return log_name;
}
static inline void initializeThreadSeed(){
        if (tls_seed == rate) {
            tls_seed = 0;
        }
        if(tls_seed== -1||tls_seed2==-1||tls_seed3==-1||tls_seed4==-1)
        {
            int start=rand()%(rate);
            tls_seed=start;
            tls_seed2=start;
            tls_seed3=start;
            tls_seed4=start;
            
        }
        if (tls_seed2 == rate) {
            tls_seed2 = 0;
        }
        if (tls_seed3 == rate) {
            tls_seed3 = 0;
        }
        if (tls_seed4 == rate) {
            tls_seed4 = 0;
        }
        if(log ==NULL){
            log = os_log_create("com.yourapp.debug",generate_unique_log_name());
        }
}


/*Invokes a Malloc as well as sampling information about a malloc invocation
RATE is what determines the 1/RATE ratio so it will give you log 1 every RATE iteration
seed for the start is randomly generated to avoid getting threads lining up on the same exact mallocs/ frees 
*/
void* sampleMallocLogToInsturments(size_t size){
    initializeThreadSeed();
    if(tls_seed==1)
    {
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"Sample Malloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= MALLOC(size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid,"Sample Malloc","PTRID:%lu,  THREADNUM:%{threadID}llu,  TIME:%{Time}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f", address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    tls_seed++;
    return ptr;
    }
    tls_seed++;
    void *ptr= MALLOC(size); 
    return ptr;
    
}



void sampleFreeLogToInsturments(void* ptr){
    initializeThreadSeed();
    if(tls_seed2==1){
    os_signpost_id_t spid = os_signpost_id_generate(log);
    uintptr_t address_value = (uintptr_t)ptr;
    uint64_t pts=CSIZEOFMALLOC(ptr);
    os_signpost_interval_begin(log, spid, "Sample Free");
    TIMETYPE ts1=TIME_N;
    FREE(ptr); 
    TIMETYPE ts2=TIME_N;
    os_signpost_interval_end(log, spid, "Sample Free","PTRIDL %lu  THREADID:%llu  TIME:%llu ns  SIZE:%llu",  address_value,GETTHREAD,getTimeDif(ts1,ts2),pts);


    }else{

FREE(ptr);
    }
    tls_seed2++;
}



void allFreeLogToInsturments(void* ptr){
    initializeThreadSeed();
    os_signpost_id_t spid = os_signpost_id_generate(log);
    uintptr_t address_value = (uintptr_t)ptr;
    uint64_t pts=CSIZEOFMALLOC(ptr);
    os_signpost_interval_begin(log, spid, "ALL Free");
    TIMETYPE ts1=TIME_N;
    FREE(ptr); 
    TIMETYPE ts2=TIME_N;
    os_signpost_interval_end(log, spid, "ALL Free","PTRIDL %lu  THREADID:%llu  TIME:%llu ns  SIZE:%llu",  address_value,GETTHREAD,getTimeDif(ts1,ts2),pts);
}

/*
Will Log All Mallocs You Invoke its not sampling so be careful if you have a high level of mallocs os_signpost might not be able to catch them all and will just drop the data
*/
void* allMallocLogToInsturments(size_t size){
    initializeThreadSeed();
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"ALL Malloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= MALLOC(size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid, "ALL Malloc","PTRID:%lu,  THREADNUM:%{public}llu,  TIME:%{public}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f",address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    tls_seed++;
    return ptr;
       
}


/*
Will Log All Callocs You Invoke its not sampling so be careful if you have a high level of mallocs os_signpost might not be able to catch them all and will just drop the data
*/
void* allCallocLogToInsturments(size_t val,size_t size){
    initializeThreadSeed();
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"ALL Calloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= CALLOC(val,size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid, "ALL Calloc","PTRID:%lu,  THREADNUM:%{public}llu,  TIME:%{public}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f",address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    tls_seed3++;
    return ptr;
       
}



/*Invokes a Calloc as well as sampling information about a malloc invocation
RATE is what determines the 1/RATE ratio so it will give you log 1 every RATE iteration
seed for the start is randomly generated to avoid getting threads lining up on the same exact mallocs/ frees 
*/
void* sampleCallocLogToInsturments(size_t val,size_t size){
    initializeThreadSeed();
    if(tls_seed3==1)
    {
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"Sample Calloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= CALLOC(val,size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid, "Sample Calloc","PTRID:%lu,  THREADNUM:%{public}llu,  TIME:%{public}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f",address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    tls_seed3++;
    return ptr;
    }
    tls_seed3++;
    void *ptr= CALLOC(val,size); 
    return ptr;
    
}

void* sampleReallocLogToInsturments(void* val,size_t size){
    initializeThreadSeed();
    if(tls_seed4==1)
    {
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"Sample Realloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= REALLOC(val,size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid, "Sample Realloc","PTRID:%lu,  THREADNUM:%{public}llu,  TIME:%{public}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f",address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    tls_seed4++;
    return ptr;
    }
    tls_seed4++;
    void *ptr= REALLOC(val,size); 
    return ptr;
    
}
void* allReallocLogToInsturments(void* val,size_t size){
    initializeThreadSeed();
    os_signpost_id_t spid = os_signpost_id_generate(log);
    os_signpost_interval_begin(log,spid,"ALL Realloc");
    TIMETYPE ts1=TIME_N;
    void *ptr= REALLOC(val,size); 
    TIMETYPE ts2=TIME_N;
    uintptr_t address_value = (uintptr_t)ptr;
    os_signpost_interval_end(log, spid, "ALL Realloc","PTRID:%lu,  THREADNUM:%{public}llu,  TIME:%{public}llu ns, SIZE:%{public}zu, REALSIZE:%{public}zu, UsageOfSpace:%{public}.2f",address_value,GETTHREAD,getTimeDif(ts1,ts2),size,CSIZEOFMALLOC(ptr),(double)((double)size/(double)CSIZEOFMALLOC(ptr))*100);
    return ptr;
}
