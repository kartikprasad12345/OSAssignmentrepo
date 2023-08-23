#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
void *virtual_mem = NULL;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  free(phdr) ;
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char **exe)
{
    // Open the ELF file
    fd = open(exe[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening ELF file");
        return;
    }

    // Load ELF header
    ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) {
        printf("Memory allocation error\n");
        return;
    }

    ssize_t ehdr_read = read(fd, ehdr, sizeof(Elf32_Ehdr));
    if (ehdr_read != sizeof(Elf32_Ehdr)) {
        if (ehdr_read == -1) {
            perror("Error reading ELF header");
        } else {
            printf("Error: Could not read complete ELF header\n");
        }
        free(ehdr);
        return;
    }

    // Load program headers
    phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));


    // Seek to the program headers' offset in the file
    lseek(fd, ehdr->e_phoff, SEEK_SET);
    read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
    // Find and load the executable segment
    unsigned int temp;
    for (int i = 0; i < ehdr->e_phnum; i++) {
        printf("\n%d$%d\n",phdr[i].p_offset,phdr[i].p_memsz);
        if (phdr[i].p_type == PT_LOAD) {
            if((phdr[i].p_vaddr<=ehdr->e_entry)&&(ehdr->e_entry<phdr[i].p_vaddr+phdr[i].p_memsz)){
                virtual_mem=mmap((void*)phdr[i].p_vaddr,phdr[i].p_memsz,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_FIXED,fd,phdr[i].p_offset);
                temp=phdr[i].p_offset;
                break;
            }
        }
    }

    if(virtual_mem==NULL){
        printf("!");
    }

    int (*_start)(void)=(void*)ehdr->e_entry;
    int result = _start();
    printf("User _start return value = %d\n",result);
    // Navigate to the entrypoint address into the segment loaded in the memory in the above step
    
}
