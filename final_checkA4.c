// #include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>


#define PAGE_SIZE 4096                                                                           // 4 KB
#define MAX_SEGMENTS 48

// Structure to hold segment details  , Is this structure accurate / necessary?
struct segment
{
  void * addr;
  size_t size;
  unsigned long offset;
};

Elf32_Ehdr * ehdr;
Elf32_Phdr * phdr;

int fd;

struct segment segments[MAX_SEGMENTS];  // Why is this needed ?
int num_segments = 0;
int page_faults = 0;
size_t total_mem = 0;
size_t int_frag = 0;

static int i = 0  ;
static int j = 0 ; 

void handle_buserror(int signum , siginfo_t *info, void *context){
    printf("Received SIGBUS (Bus Error)\n");
    printf("Faulting address: %p\n", info->si_addr);
    void * fault_addr = info->si_addr;
    void * start_addr = (void *)(((unsigned long)fault_addr & ~(PAGE_SIZE - 1)));
    printf("Start address = %p\n", start_addr);
    printf("Fault Address = %p\n", fault_addr);
    void * alloc_addr = mmap(start_addr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC , MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS , -1, 0);
    j++ ; 
    if(j == 5){
        kill(getpid(), SIGSTOP) ;
    }

}
// If the page is already allocated ,  then it wont enter the signal handler so we dont need to handle that .
void handle_segfault(int signum, siginfo_t *info, void *context) {
    printf("Faulting address: %p\n", info->si_addr);
    void * fault_addr = info->si_addr;
    int seg_index = -1;
    
    for (int i = 0; i < num_segments; i++) {
        if (fault_addr >= segments[i].addr && fault_addr < (void*)((uintptr_t)segments[i].addr + segments[i].size)) {
            seg_index = i;
            printf("Segment %d\n",i);
            printf("Address = %p\n",segments[i].addr);
            printf("Size = %d\n",segments[i].size);
            printf("Offset = %lu\n",segments[i].offset);
            printf("Bound Address = %p\n",segments[i].addr+segments[i].size);  // check not sure .
            break;
        }
    }
    
    if (seg_index == -1) {
        printf("Invalid page fault address %p\n", fault_addr);
        exit(1);
    }
    void *start_addr = (void *)(((unsigned long)fault_addr & ~(PAGE_SIZE - 1)));
    printf("Start address = %p\n", start_addr);
    printf("Segment address = %p\n", segments[seg_index].addr);
    printf("Fault Address = %p\n", fault_addr);
    unsigned long offset = ((unsigned long)fault_addr& ~(PAGE_SIZE-1)) - (unsigned long)segments[seg_index].addr; // check
    printf("Offset = %lu\n", offset);
    unsigned long offset_from_start_of_file = segments[seg_index].offset + offset;
    printf("Offset from start of file = %lu\n", offset_from_start_of_file);  //check

    void * alloc_addr = mmap(start_addr, PAGE_SIZE, PROT_READ|PROT_WRITE|PROT_EXEC , MAP_PRIVATE|MAP_FIXED , fd, offset_from_start_of_file);

    if (alloc_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    //void * pointer = memcpy(alloc_addr,segments[seg_index].offset + offset, PAGE_SIZE); 

    page_faults++;
    
    printf("Allocated page at %p\n", alloc_addr);
    printf("Page fault count = %d\n", page_faults);
    total_mem += PAGE_SIZE;
    int_frag += (PAGE_SIZE - (segments[seg_index].size % PAGE_SIZE));

    printf("Handled page fault\n\n");
}


void loader_cleanup()
{
  if(ehdr!=NULL){
    free(ehdr);
  }
  if(phdr!=NULL){
    free(phdr);
  }
}

int checkelf(Elf32_Ehdr *ehdr)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {return 1;}
}


void load_and_run_elf(char** exe){

    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
    printf("Not being able to open ELF file");
    return;
    }
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));                                          // read needs to store the first 52 bytes of the file in the memory location pointed by ehdr ,
                                                                                            // that is why we need to malloc
    if (!ehdr){
    printf("Could not allocate\n");
    return;
    }
    int f = read(fd, ehdr, sizeof(Elf32_Ehdr));
    if(f == -1){printf("Could not read file\n");}

    if(checkelf(ehdr)==1){printf("Invalid elf file\n");}

    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));
    int temp= lseek(fd, ehdr->e_phoff, SEEK_SET);
    if(temp==-1){printf("Coud not locate offset\n");}
    int f2 = read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
    if(f2 == -1){printf("Coulld not read file\n");}
    for(int i=0;i<ehdr->e_phnum;i++){
            segments[num_segments].addr = (void *)phdr[i].p_vaddr;
            segments[num_segments].size = phdr[i].p_memsz;
            segments[num_segments].offset = phdr[i].p_offset;
            num_segments++;
            printf("Segment %d\n",i);
            printf("Address = %p\n",segments[i].addr);
            printf("Size = %d\n",segments[i].size);
            printf("Offset = %lu\n",segments[i].offset);
            printf("Bound Address = %p\n", (void*)((uintptr_t)segments[i].addr + segments[i].size)); // use this instead
            printf("\n\n") ;

    }
    struct sigaction sa;
    sa.sa_sigaction = handle_segfault;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    struct sigaction sa2;
    sa2.sa_sigaction = handle_buserror;
    sa2.sa_flags = SA_SIGINFO;
    if (sigaction(SIGBUS, &sa2, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    
    printf("Entry point = %p\n", (void*)ehdr->e_entry);
    int (*_start)(void)=(void*)ehdr->e_entry; 
    int result = _start();
    printf("User _start return value = %d\n",result);

}


int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
