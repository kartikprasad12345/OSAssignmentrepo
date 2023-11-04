#include <stdio.h>
#include <stdint.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>

void load_and_run_elf(char** exe);
void loader_cleanup();