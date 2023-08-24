#include "../loader/loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  if(ehdr!=NULL){
    free(ehdr);
  }
  if(phdr!=NULL){
    free(phdr);
  }
}
void *traversingphdr(int cur,int max,Elf32_Phdr *phdr){
  if(cur==max-1){
    return NULL;
    }
  if (phdr[cur].p_type == PT_LOAD){
    if((phdr[cur].p_vaddr<=ehdr->e_entry)&&(ehdr->e_entry<phdr[cur].p_vaddr+phdr[cur].p_memsz)){
      return mmap((void*)phdr[cur].p_vaddr,phdr[cur].p_memsz,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_FIXED,fd,phdr[cur].p_offset);
      }
  }
  return traversingphdr(cur+1,max,phdr);
}
int checkelf(Elf32_Ehdr *ehdr){
  if(ehdr->e_ident[0]!=0x7f){
    return 1;
  }
  else if(ehdr->e_ident[1]!='E'){
    return 1;
  }
  else if(ehdr->e_ident[2]!='L'){
    return 1;
  }
  else if (ehdr->e_ident[3]!='F'){
    return 1;
  }else{
    return 0;
  }
}
/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);
  if (fd == -1) {
    printf("Not being able to open ELF file");
    return;
  }
  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr));
  if (!ehdr) {
    printf("Could not allocate\n");
    return;
  }
  int fileread = read(fd, ehdr, sizeof(Elf32_Ehdr));
  if(fileread==-1){
    printf("Could not read file\n");
  }
  if(checkelf(ehdr)==1){
    printf("Invalid elf file\n");
  }
  phdr = (Elf32_Phdr *)malloc(ehdr->e_phnum * sizeof(Elf32_Phdr));
  int temp= lseek(fd, ehdr->e_phoff, SEEK_SET);
  if(temp==-1){
    printf("Coud not locate offset\n");
  }
  int fileread2=read(fd, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr));
  if(fileread2==-1){
    printf("Coulld not read file\n");
  }
  int a=0;
  int lenph=ehdr->e_phnum;
  void *virtual_mem=traversingphdr(0,lenph,phdr);
  if(virtual_mem==NULL){
    printf("No entry point found\n");
  }
  int (*_start)(void)=(void*)ehdr->e_entry;
  int result = _start();
  printf("User _start return value = %d\n",result);
}
