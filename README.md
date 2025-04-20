Created a Simple Malloc Microbenchmark where one can look at fragmentation of a specific malloc implementation as well as the time it takes 
for the implementation to actually allocate the memory.

-DUSE_JEMALLOC

  will run JE Malloc implementation of malloc if you have it dynamically linked
  
-DUSE_MIMALLOC 

  will run MIMALLOC if you have the implementation dynamically linked
  
default:

  uses Mac OS malloc


functions and macros:

  MACROS:
  
    MALLOC(size_t x)
    
    CALLOC(size_t x, size_t y)
    
    REALLOC(void* ptr, size_t size)
    FREE(void *ptr)
    
    These macros corrospond to the implementation of malloc you put in compolation argument
    
  FUNCTIONS:
  
    setrate(int x) --sets rate of how many mallocs/callocs/etc.. you want to sample default rate is set to 1000
    
    sampleMallocLogToInstruments(size_t size)  --returns allocates and returns a pointer of where the memory you allocated using malloc is also will log if its time to sample
    
    sampleCallocLogToInstruments(size_t x, size_t y)  --returns allocates and returns a pointer of where the memory you allocated using calloc is also will log if its time to sample
    
    sampleReallocLogToInstruments(void* ptr, size_t size)  --returns a reallocation of memory is also will log if its time to sample
    
    sampleCallocLogToInstruments(void *ptr)  --frees the memory you allocated using free is also will log if its time to sample
    
    allMallocLogToInstruments(size_t size)  --returns allocates and returns a pointer of where the memory you allocated using malloc is also will log
    
    allCallocLogToInstruments(size_t x, size_t y)  --returns allocates and returns a pointer of where the memory you allocated using calloc is also will log 
    
    allReallocLogToInstruments(void* ptr, size_t size)  --returns a reallocation of memory is also will log 
    
    allCallocLogToInstruments(void *ptr)  --frees the memory you allocated using free is also will log 
    
  
  
