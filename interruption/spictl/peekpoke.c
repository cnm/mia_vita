#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h> /* For getpagesize  */


void *map_phys(off_t addr,int *fd) {
  off_t page;
  unsigned char *start;

  if (*fd == -1)
    *fd = open("/dev/mem", O_RDWR|O_SYNC);
  if (*fd == -1) {
/*    perror("open(/dev/mem):");*/
    return 0;
  }
  page = addr & 0xfffff000;
  start = mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, *fd, page);
  if (start == MAP_FAILED) {
/*    perror("mmap:");*/
    return 0;
  }
  start = start + (addr & 0xfff);
  return start;
}
