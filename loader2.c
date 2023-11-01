extern int _start();
#include "loader.h"
# define PAGE_SIZE 4096
Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int count_page_fault=0;
int total_phdr;
struct segment_store{
  Elf32_Phdr temp;
  bool ifloaded;
};
struct segment_store *pghdrs_list;
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
void storehdrs(int cur,int max,Elf32_Phdr *phdr){
  if(cur==max-1){
    return ;
  }
  pghdrs_list[cur].temp=phdr[cur];
  pghdrs_list[cur].ifloaded=false;

  storehdrs(cur+1,max,phdr);
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
void sig_handler(int sig,siginfo_t * info,void *ucontext){
  // info->si_addr
  Elf32_Addr seg_fault_addr=(Elf32_Addr)info->si_addr;
  for(int i=0;i<total_phdr;i++){
    if((pghdrs_list[i].temp.p_vaddr<seg_fault_addr) && (pghdrs_list[i].temp.p_vaddr+pghdrs_list[i].temp.p_memsz>=seg_fault_addr) && pghdrs_list[i].ifloaded==false){
      if(count_page_fault==0){
        mmap((void *)pghdrs_list[i].temp.p_vaddr,((pghdrs_list[i].temp.p_memsz)/PAGE_SIZE)+1, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_FIXED,fd,pghdrs_list[i].temp.p_offset);
        pghdrs_list[i].ifloaded=true;
        count_page_fault++;
        break;
      }else{
        mmap((void *)pghdrs_list[i].temp.p_vaddr, ((pghdrs_list[i].temp.p_memsz)/PAGE_SIZE)+1, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_FIXED|MAP_ANONYMOUS,fd,pghdrs_list[i].temp.p_offset);
        pghdrs_list[i].ifloaded=true;
        break;
      }
    }
  }
}
/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  struct sigaction signl_handlr;
  signl_handlr.sa_sigaction=sig_handler;
  signl_handlr.sa_flags=SA_SIGINFO;
  sigaction(SIGSEGV,&signl_handlr,NULL);


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
  pghdrs_list=(struct segment_store*)malloc(ehdr->e_phnum);
  total_phdr=ehdr->e_phnum;
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
  // void *virtual_mem=traversingphdr(0,lenph,phdr);
  // if(virtual_mem==NULL){
  //   printf("No entry point found\n");
  // }
  storehdrs(0, total_phdr, phdr);
  int (*_start)(void)=(void*)ehdr->e_entry;
  int result = _start();
  printf("User _start return value = %d\n",result);
}
