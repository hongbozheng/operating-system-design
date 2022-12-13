#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <unistd.h>

#include "libiu.h"
#include <libbpf.h>

#define EXE "./target/debug/tracex5"

/* install fake seccomp program to enable seccomp code path inside the kernel,
 * so that our kprobe attached to seccomp_phase1() can be triggered
 */
static inline void install_accept_all_seccomp(void)
{
	struct sock_filter filter[] = {
		BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog prog = {
		.len = (unsigned short)(sizeof(filter)/sizeof(filter[0])),
		.filter = filter,
	};
	if (prctl(PR_SET_SECCOMP, 2, &prog))
		perror("prctl");
}

#define DEBUGFS "/sys/kernel/debug/tracing/"

static void read_trace_pipe(void)
{
	int trace_fd;

	trace_fd = open(DEBUGFS "trace_pipe", O_RDONLY, 0);
	if (trace_fd < 0)
		return;

	while (1) {
		static char buf[4096];
		ssize_t sz;

		sz = read(trace_fd, buf, sizeof(buf) - 1);
		if (sz > 0) {
			buf[sz] = 0;
			puts(buf);
		}
	}
}

int main(void)
{
	FILE *f;

	// TODO add your loader code here, you should refer to samples/hello and
	// https://elixir.bootlin.com/linux/v5.15.79/source/samples/bpf/tracex5_user.c
    struct bpf_link *link = NULL;
    struct bpf_program *prog;
    struct bpf_object *obj;

    obj = iu_object__open(EXE);
    if (!obj) {
        fprintf(stderr, "ERROR: failed to open obj\n");
        exit(1);
    }

    prog = bpf_object__find_program_by_name(obj, "iu_prog1");
    if (!prog) {
        fprintf(stderr, "ERROR: finding a prog in obj file failed\n");
        exit(1);
    }

    link = bpf_program__attach(prog);
    if (libbpf_get_error(link)) {
        fprintf(stderr, "ERROR: bpf_program__attach failed\n");
        link = NULL;
        return 0;
    }

	// Do not change after this line, no need to do cleanup as in the original
	// loader -- read_trace_pipe() is an infinite loop anyway, just use ctrl-c
	// to interrupt the program
	install_accept_all_seccomp();

	f = popen("dd if=/dev/zero of=/dev/null count=5", "r");
	(void) f;

	read_trace_pipe();

	return 0;
}