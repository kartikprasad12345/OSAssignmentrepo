// #include "loader.h"  // Include necessary headers
// struct calculation { // for storing segment_size,number of pages used and the total fragmentation in each segment
//   size_t segment_size;
//   int number_of_pages;
//   size_t fragmentation;
// };

// #define PAGE_SIZE 4096 // simply the page size
// Elf32_Ehdr *ehdr;    // ELF header pointer
// Elf32_Phdr *phdr;    // Program header pointer
// int fd;              // File descriptor for the ELF file
// Elf32_Phdr* arr[200]; // array containing all the segments pointer
// int count_arr=0; // count of the number of PT_LOADs
// void* arr11[200]={NULL}; // array containing all the virtual memory's 
// struct calculation arr2[100]; // array containing the struct for priting purposes at the last
// size_t calculating=0; // calcualting for mid way calculation of the segment size
// uintptr_t address; // contains which address have to be loaded
// bool flag=true; // used for settting the address and the offset for the first time
// int numbers=0; // number of times the program came into the signal handler
// int counttt=0; // for the for loop in the in the signal hangler
// int offset=0; // the address in the elf file from where we have to read the content
// /*
//  * Function to release memory and perform cleanup.

//  */

// // for the cleanup purposes like freeing phdr and ehdr pointers and unmapping all the mapped memorires
// void loader_cleanup() {
//   if (phdr != NULL) {
//     free(phdr);
//     phdr = NULL;
//   }

//   if (ehdr != NULL) {
//     free(ehdr);
//     ehdr = NULL;
//   }

  
//   for (int i=0;i<counttt;i++) {
//     if (munmap(arr11[i],arr[i]->p_memsz)==-1) {
//       fprintf(stderr,"Error in unmapping\n");
//       exit(1);
//     }
//   }
// }

// //signal handler for handling the segmentation fault, mapping the required segments and for calculating the number of pages etc.
// void signal_handler(int signal) {
//   if (signal==SIGSEGV) {
//     //printf("came hrere\n");
//     if (flag) {
//       address=arr[0]->p_vaddr;
//       offset=arr[0]->p_offset;
//       for (int i=0;i<count_arr;i++) {
//         struct calculation c;
//         c.fragmentation=0;
//         c.number_of_pages=0;
//         c.segment_size=0;
//         arr2[i]=c;
//       }
//       //printf("came in the flag part\n");
//       flag=false;
//     }
//     //int i=counttt;
    
//     while(1) {
//       //printf("counttt %d\n",counttt);
//       //printf("%u\n",arr[i]->p_memsz);
//       numbers++;
//       printf("offset %d\n",offset);
//       // printf("numbers %d\n",numbers);
//       // printf("address: %u\n",address);
//       // printf("offset: %d\n",offset);
//       // printf("size_of_segment: %u\n",arr[counttt]->p_memsz);
//       if (arr[counttt]->p_vaddr==ehdr->e_entry) {
//         // printf("first wale me hu %d\n",arr[counttt]->p_type);
//         // printf("first wale ka offset %d\n",arr[counttt]->p_vaddr);
//         // printf("entry point %d\n",ehdr->e_entry);
//         arr11[counttt]=mmap((void*)address,PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE  , fd,offset);
//         arr2[counttt].number_of_pages++;
//         arr2[counttt].segment_size=arr[counttt]->p_memsz;
//         calculating+=PAGE_SIZE;
//         address+=PAGE_SIZE;
//         offset+=PAGE_SIZE;
//         //size_t segment_size=arr[counttt]->p_memsz;
//         //printf("size of the segment %u\n",segment_size);
        
//         //printf("Total number of pages allocated %d\n",quotient+remainder);
        
//         if (calculating>=arr2[counttt].segment_size) {
//           calculating=0;
//           counttt++;
//           if (counttt<count_arr) {
//             printf("came in the if statement\n");
//             //offset=arr[counttt]->p_offset;
//             address=arr[counttt]->p_vaddr;
//           }
//         }
        
//       } else {
        
//         // if (flag) {
//         //   address=arr[i]->p_vaddr;
//         //   flag=false;
//         // }
//         arr11[counttt]=mmap((void*)address,PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS , fd,offset);
//         arr2[counttt].number_of_pages++;
//         arr2[counttt].segment_size=arr[counttt]->p_memsz;
//         //arr11[i]=mmap((void*)arr[i]->p_vaddr,PAGE_SIZE,PROT_READ | PROT_WRITE | PROT_EXEC, MAP_FIXED | MAP_PRIVATE , fd,arr[i]->p_offset);
//         //printf("came here\n");
//         calculating+=PAGE_SIZE;
//         address+=PAGE_SIZE;
//         offset+=PAGE_SIZE;
//         //size_t segment_size=arr[counttt]->p_memsz;
//         //printf("size of the segment %u\n",segment_size);
        
//         //printf("Total number of pages allocated %d\n",quotient+remainder);
//         //printf("offset %d\n",offset);
//         if (calculating>=arr2[counttt].segment_size) {
          
//           calculating=0;
//           //flag=true;
//           counttt++;
          
//           if (counttt<count_arr) {
//             printf("came in the if statement\n");
//             address=arr[counttt]->p_vaddr;
//             //offset=arr[counttt]->p_offset;
            
//           }
//         }
//       }
//       if (arr11[counttt]==MAP_FAILED) {
//         fprintf(stderr,"Error in mmap");
//         exit(1);
//       }
//       break;
//     }
    
    
//   }
// }


// /*
//  * Load and run the ELF executable file.
//  */
// void load_and_run_elf(char** exe) {
//   fd = open(exe[1], O_RDONLY);

//   if (fd < 0) {
//     fprintf(stderr, "Error in opening the file");
//     exit(1);
//   }

//   // Allocate memory for the ELF header
//   ehdr = (Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));

//   if (ehdr == NULL) {
//     fprintf(stderr, "Memory allocation failed");
//     exit(1);
//   }

//   // Read the ELF header from the file
//   int d = read(fd, ehdr, sizeof(Elf32_Ehdr));

//   if (d != sizeof(Elf32_Ehdr)) {
//     fprintf(stderr, "Error in reading the file");
//     exit(1);
//   }

//   // Seek to the program header table
//   int e = lseek(fd, ehdr->e_phoff, SEEK_SET);

//   if (e < 0) {
//     fprintf(stderr, "Error in lseek");
//     exit(1);
//   }

//   // Allocate memory for the program header table
//   phdr = (Elf32_Phdr*)malloc(ehdr->e_phentsize * ehdr->e_phnum);

//   if (phdr == NULL) {
//     fprintf(stderr, "Memory allocation failed");
//     exit(1);
//   }

//   // Read the program header table from the file
//   int f = read(fd, phdr, ehdr->e_phentsize * ehdr->e_phnum);

//   if (f != ehdr->e_phentsize * ehdr->e_phnum) {
//     fprintf(stderr, "Error in read");
//     exit(1);
//   }

//   // Find the PT_LOAD segment containing the entry point
//   //int count=0;
//   for (int i = 0; i < ehdr->e_phnum; i++) {
//     Elf32_Phdr* temp = phdr + i;
//     arr[count_arr]=temp;
//     count_arr++;
//     //printf("%d %d\n",i,temp->p_type);
//     // if (temp->p_type==PT_LOAD) {
      
//     // }
    
//   }

//   // signal catcher that will call the signal handler for handling the segmentation fault
//   struct sigaction sa;
//   sa.sa_handler=signal_handler;
//   sa.sa_flags=0;
//   sigemptyset(&sa.sa_mask);
//   if (sigaction(SIGSEGV,&sa,NULL)==-1) {
//     fprintf(stderr,"Error in sigactions\n");
//     exit(1);
//   }
  
//   int (*_start)(void) = (int(*)(void))(arr[1]->p_vaddr);

//   int result = _start();

//   printf("User _start return value = %d\n", result);
//   loader_cleanup();
//   int total=0;
//   //printing all the reports that were asked
//   for (int i=0;i<count_arr;i++) {
//     if (arr2[i].number_of_pages>0) {
//       printf("Total memory required by this segment %u\n",arr2[i].segment_size);
//       printf("Total numbre of pages used by this segment %d\n",arr2[i].number_of_pages);
//       int num=arr2[i].number_of_pages*PAGE_SIZE-arr2[i].segment_size;
//       if (num<0) {
//         num=0;
//       }
//       total+=num;
//       printf("Total fragmentation for this segment is: %d\n",num);
//       printf("\n");
//     }
//   }
//   printf("Total fragmentation is: %d\n",total);
//   printf("Total number of page faults is: %d\n",numbers);
// }









//extern int _start();
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
  printf("came here\n");
  Elf32_Addr seg_fault_addr=(Elf32_Addr)info->si_addr;
  for(int i=0;i<total_phdr;i++){
    if((pghdrs_list[i].temp.p_vaddr<seg_fault_addr) && (pghdrs_list[i].temp.p_vaddr+pghdrs_list[i].temp.p_memsz>=seg_fault_addr) && pghdrs_list[i].ifloaded==false){
      if(i==1){
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
