#include "loader.h"

#include <signal.h>
extern int _start();
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;



#define MAX_SEGMENTS 32
#define PAGE_SIZE 4096

struct segment {
    unsigned long start;
    unsigned long end;
    unsigned char *data;
    int number_of_pages;
    int loaded;
    int iscalculated;
};

struct segment segments[MAX_SEGMENTS];
int page_faults = 0;
int page_allocations = 0;
float internal_fragmentation = 0;
float total_size_of_process = 0;


/*
 * release memory and other cleanups
 */
void loader_cleanup()
{
    // free the value of ehdr and phdr if they are not null
    free(ehdr);
    free(phdr);
    // close the file if it is still open
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    // all the cleaning has been done
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **argv)
{
    // Open the ELF file
    fd = open(argv[1], O_RDONLY);

    // error handling - checking if the file is opened correctly
    if (fd < 0)
    {
        printf("Error in opening of ELF file\n");
        return;
    }

    // assigning memory to the ehdr
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));

    // assigning memory for program headers
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phentsize * ehdr->e_phnum);

    // reading the ehdr using the read function
    size_t check1 = read(fd, ehdr, sizeof(Elf32_Ehdr));

    // error handling -- checking if the elf file has been read correctly
    if (check1 != (size_t)sizeof(Elf32_Ehdr))
    {
        printf("Error reading ELF header\n");
        return;
    }

    //  ehdr->e_phoff gives the offset from where the array of segments start
    lseek(fd, ehdr->e_phoff , SEEK_SET);

    // reading the program headers
    ssize_t f = read(fd, phdr, ehdr->e_phnum*sizeof(Elf32_Phdr));

    // error handling - if the size read is not equal to Elf32_Phdr then throw error
    if (f!=(ssize_t)(ehdr->e_phnum*sizeof(Elf32_Phdr)))
    {
        printf("error reading the elf file\n");
        return;
    }

    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            segments[i].loaded = 1;
            segments[i].start = phdr[i].p_vaddr;
            segments[i].end = phdr[i].p_vaddr + phdr[i].p_memsz;
        }
        else
        {
            segments[i].loaded = 0;
        }
    }
    int (*entry_point)() = (int (*)())((char *)ehdr->e_entry);
//    entry_point();
    int result = entry_point();
    printf("User _start return value = %d\n", result);



}

int fgh = 0;
void segfault_handler(int signo, siginfo_t *siginfo, void *context) {
    page_faults++;
    page_allocations++;
    fgh++;
    if (siginfo == NULL) {
        printf("siginfo is NULL\n");
        exit(1);
    }

    unsigned long  fault_addr = (unsigned long)siginfo->si_addr;
    int ind_of_seg = -1;

    for (int i = 0; i < MAX_SEGMENTS; i++)
    {
        if (segments[i].start <= fault_addr && fault_addr < segments[i].end) {
            ind_of_seg = i;
            break;
        }
    }

    int dtu = segments[ind_of_seg].number_of_pages;
    Elf32_Phdr *current_phdr = &phdr[ind_of_seg];
    int nsut = current_phdr->p_memsz;
    if (segments[ind_of_seg].iscalculated == 0)
    {
        total_size_of_process+=current_phdr->p_memsz;
        segments[ind_of_seg].iscalculated = 1;
    }
    
//    internal_fragmentation=current_phdr->p_vaddr%PAGE_SIZE;

    if (fgh == 1)
    {
        current_phdr = &phdr[ind_of_seg];
        long double s1 = current_phdr->p_vaddr;
        long double s2 = current_phdr->p_vaddr+current_phdr->p_memsz;
        int noa = ((s2-s1)/PAGE_SIZE)+1;
        int len = noa*PAGE_SIZE;
//        internal_fragmentation += len - current_phdr->p_memsz;



//        printf("if page_allocations %d \n", page_allocations);
//        printf("if size of phdr %Lf\n",(s2-s1));
//        printf("if size of PAGE_SIZE%d\n",PAGE_SIZE);
//        printf("if len: %d\n",len);
//        printf("if noa %d\n",noa);


        void *segment_addr = mmap((void *)current_phdr->p_vaddr+dtu*PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_FIXED,fd, current_phdr->p_offset+dtu*PAGE_SIZE);

        // error handling -- checking if the coying has been done right
        if (segment_addr == MAP_FAILED)
        {
            printf("segment not being able to get copied");
            loader_cleanup();
            return;
        }


//        if (noa == 1)
//        {
//            internal_fragmentation+=
//        }
//        internal_fragmentation += len - current_phdr->p_memsz;
//        printf("%f\n",internal_fragmentation);
//        printf("%d\n",len);
//        printf("%d\n",current_phdr->p_memsz);

    }
    else
    {
        long double s1 = current_phdr->p_vaddr;
        long double s2 = current_phdr->p_vaddr+current_phdr->p_memsz;
        int noa = ((s2-s1)/PAGE_SIZE)+1;
        int len = noa*PAGE_SIZE;
        Elf32_Phdr *current_phdr = &phdr[ind_of_seg];

//        printf("else page_allocations %d \n", page_allocations);
//        printf("else size of phdr%Lf\n",(s2-s1));
//        printf("else size of PAGE_SIZE%d\n",(s2-s1));
//        printf("else len: %d\n",len);
//        printf("else noa %d\n",noa);



        void *segment_addr = mmap((void *)current_phdr->p_vaddr+dtu*PAGE_SIZE,PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS,fd, current_phdr->p_offset+dtu*PAGE_SIZE);

        // error handling -- checking if the coying has been done right
        if (segment_addr == MAP_FAILED)
        {
            printf("segment not being able to get copied");
            loader_cleanup();
            return;
        }


//        internal_fragmentation += len - current_phdr->p_memsz;
//        printf("%f\n",internal_fragmentation);
//        printf("%d\n",len);
//        printf("%d\n",current_phdr->p_memsz);
    }
    segments[ind_of_seg].number_of_pages++;
    internal_fragmentation = page_allocations*PAGE_SIZE-total_size_of_process;



}



int main(int argc, char **argv)
{
    struct sigaction seg_fault;
    seg_fault.sa_sigaction = segfault_handler;
    seg_fault.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &seg_fault, NULL);


    for (int i = 0; i < MAX_SEGMENTS; i++) {
        segments[i].data = NULL;
        segments[i].loaded = 0;
        segments[i].number_of_pages = 0;
        segments[i].iscalculated = 0;
    }
    printf("The Execution of the main function has started\n");
    if (argc != 2)
    {
        printf("Usage: %s <ELF Executable>\n", argv[0]);
        exit(1);
    }
    printf("Exexcuting file\n");

    // Load and run the ELF executable
    load_and_run_elf(argv);

    printf("Page faults: %d\n", page_faults);
    printf("Page allocations: %d\n", page_allocations);
    printf("Internal fragmentation (KB): %f\n", internal_fragmentation / 1024);

    // Clean up allocated memory and resources
    // closing the file descriptor
    loader_cleanup();
    return 0;
}