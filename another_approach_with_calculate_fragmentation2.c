#include <stdint.h>
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

#define PAGE_SIZE 4096 // 4 KB
#define MAX_SEGMENTS 48
#define MAX_TIMES_SEGMENT_CAN_BE_LOADED 48
int cnt1 = 0;
int cnt2 = 0;

struct segment {
    uintptr_t addr;
    size_t size;
    Elf32_Off offset;
    int ifloaded;
    void *all_mapped_addresses[MAX_TIMES_SEGMENT_CAN_BE_LOADED];
};

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;

int fd;

struct segment segments[MAX_SEGMENTS];
int num_segments = 0;
int page_faults = 0;
size_t total_mem = 0;
size_t int_frag = 0;

void handle_segfault(int signum, siginfo_t *info, void *context) {
    printf("*******************\n");
    printf("Faulting address: %p\n", info->si_addr);
    void *fault_addr = info->si_addr;
    int seg_index = -1;

    for (int i = 0; i < num_segments; i++) {
        if ((fault_addr >= segments[i].addr) && (fault_addr < (void *)((uintptr_t)segments[i].addr + segments[i].size))) {
            seg_index = i;

            break;
        }
    }

    if (seg_index == -1) {
        printf("Invalid page fault address %p\n", fault_addr);
        exit(1);
    }
    printf("Segment %d\n",seg_index);
    printf("Virtual address %x\n",phdr[seg_index].p_vaddr);
    printf("Base offset %lu\n",segments[seg_index].offset );
    printf("Address %d\n",phdr[seg_index].p_vaddr+segments[seg_index].ifloaded*PAGE_SIZE);
//    void *start_addr = (void *)(((unsigned long)phdr[seg_index].p_vaddr+segments[seg_index].ifloaded*PAGE_SIZE));
    // unsigned long offset = (unsigned long)fault_addr - (unsigned long)segments[seg_index].addr;
    // unsigned long offset_from_start_of_file = segments[seg_index].offset +segments[seg_index].ifloaded*PAGE_SIZE;
    printf("Ifloaded %d\n",segments[seg_index].ifloaded);
    printf("Real offset %d\n",phdr[seg_index].p_vaddr + segments[seg_index].ifloaded * PAGE_SIZE);
    void *alloc_addr;
    if (seg_index == 1) {
        cnt1++;
        alloc_addr = mmap((void *)(uintptr_t)phdr[seg_index].p_vaddr + segments[seg_index].ifloaded * PAGE_SIZE, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED, fd, phdr[seg_index].p_offset + segments[seg_index].ifloaded * PAGE_SIZE);
        segments[seg_index].all_mapped_addresses[segments[seg_index].ifloaded]=alloc_addr;
        segments[seg_index].ifloaded++;
    } else {
        cnt1++;
        alloc_addr = mmap((void *)(uintptr_t)phdr[seg_index].p_vaddr + segments[seg_index].ifloaded * PAGE_SIZE, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_FIXED|MAP_ANONYMOUS, fd, phdr[seg_index].p_offset + segments[seg_index].ifloaded * PAGE_SIZE);
        segments[seg_index].all_mapped_addresses[segments[seg_index].ifloaded]=alloc_addr;
        
        segments[seg_index].ifloaded++;
    }

    if (alloc_addr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("Allocation address %d\n",alloc_addr);

    page_faults++;
    total_mem += PAGE_SIZE;
    int_frag += (PAGE_SIZE - (segments[seg_index].size % PAGE_SIZE));

    printf("Allocated page at %p\n", alloc_addr);
    printf("Page fault count = %d\n", page_faults);
    printf("Size %zu\n",segments[seg_index].size);
    printf("Index %d\n",seg_index);

    printf("Handled page fault\n\n");
}
long int calculate_internal_fragmentation(){
    long int total_fragmentation=0;
    for(int i=0;i<num_segments;i++){
        if(segments[i].ifloaded>0){
           long int total_page_size=(segments[i].ifloaded)*PAGE_SIZE;
            long int segment_size=segments[i].size;
            printf("Segment size %ld\n",segment_size);
            printf("Total page size %ld\n",total_page_size);
            if(segment_size>=total_page_size){
                continue;
            }else{
                printf("Fragmentation of segment %ld\n",(total_page_size-segment_size));
                total_fragmentation=total_fragmentation+(total_page_size-segment_size);
            } 
        }
        // long int total_page_size=(segments[i].ifloaded)*PAGE_SIZE;
        // long int segment_size=segments[i].size;
        // printf("Segment size %ld\n",segment_size);
        // printf("Total page size %ld\n",total_page_size);
        // if(segment_size>=total_page_size){
        //     continue;
        // }else{
        //     total_fragmentation=total_fragmentation+(total_page_size-segment_size);
        // }

    }
    return total_fragmentation;
}
void loader_cleanup() {
    long int tot_frag=calculate_internal_fragmentation();
    float tot_kb=(float)tot_frag/1000;
    float tot_kib=(float)tot_frag/1024;
    printf("Total internal fragmentation (in bytes) %ld\n",calculate_internal_fragmentation());
    printf("Total internal fragmentation (in KB) %f\n",tot_kb);
    printf("Total internal fragmentation (in KiB) %f\n",tot_kib);
    if (ehdr != NULL) {
        free(ehdr);
    }
    if (phdr != NULL) {
        free(phdr);
    }
    printf("cnt1 %d\n", cnt1);
    // printf("Total internal fragmentation %ld\n",calculate_internal_fragmentation());
    for (int i = 0; i < num_segments; i++) {
        if (segments[i].ifloaded > 0) {
            for (int j = 0; j < segments[i].ifloaded; j++) {
                printf("segment status %d\n", segments[i].ifloaded);
                printf("segment munmapped\n");
                cnt2++;
                munmap((void *)segments[i].all_mapped_addresses[j], PAGE_SIZE);
            }
        }
    }
    printf("page faults %d\n", page_faults);
    printf("cnt2 %d\n", cnt2);
}

int checkelf(Elf32_Ehdr *ehdr) {
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1] != ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) {
        return 1;
    }
}

void load_and_run_elf(char **exe) {
    fd = open(exe[1], O_RDONLY|O_RDWR);
    if (fd == -1) {
        printf("Not being able to open ELF file");
        return;
    }

    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        printf("Could not allocate\n");
        return;
    }

    int f = read(fd, ehdr, sizeof(Elf32_Ehdr));
    if (f == -1) {
        printf("Could not read file\n");
    }

    if (checkelf(ehdr) == 1) {
        printf("Invalid ELF file\n");
    }

    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));
    int temp = lseek(fd, ehdr->e_phoff, SEEK_SET);
    if (temp == -1) {
        printf("Could not locate offset\n");
    }

    int f2 = read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
    if (f2 == -1) {
        printf("Could not read file\n");
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        segments[num_segments].addr = phdr[i].p_vaddr;
        segments[num_segments].size = phdr[i].p_memsz;
        segments[num_segments].offset = phdr[i].p_offset;
        segments[num_segments].ifloaded = 0;
        num_segments++;

        printf("Segment %d\n", i);
        printf("Address = %p\n", segments[i].addr);
        printf("Size = %zu\n", segments[i].size);
        printf("Offset = %u\n", segments[i].offset);
        printf("Bound Address = %p\n", (void *)((uintptr_t)segments[i].addr + segments[i].size));
        printf("\n\n");
    }

    struct sigaction sa;
    sa.sa_sigaction = handle_segfault;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Entry point = %p\n", (void *)ehdr->e_entry);
    int (*_start)(void) = (void *)ehdr->e_entry;
    int result = _start();
    printf("User _start return value = %d\n", result);
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
