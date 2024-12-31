#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/perf_event.h>
#include <linux/unistd.h>

#include "libiu.h"
#include <libbpf.h>

#define EXE "./target/debug/hello"

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
	int trace_pipe_fd;
	struct bpf_object *obj;
	struct bpf_program *prog;
	struct bpf_link *link = NULL;

	// Parse and load the program
	obj = iu_object__open(EXE);
	if (!obj) {
		fprintf(stderr, "Object could not be opened\n");
		exit(1);
	}

	// Find the file descriptor referring to that program
	prog = bpf_object__find_program_by_name(obj, "iu_prog1");
	if (!prog) {
		fprintf(stderr, "Program iu_prog1 not found\n");
		exit(1);
	}

	// Attach the program to the hook point
	link = bpf_program__attach(prog);
	if (libbpf_get_error(link)) {
		fprintf(stderr, "ERROR: bpf_program__attach failed\n");
		link = NULL;
		goto cleanup;
	}

	// Trigger the program
	syscall(__NR_dup, 1);

	// Read from trace buffer
	read_trace_pipe();

	// You can safely ignore the part below this line -- the libiu has a bug in
	// its destructor so just use ctrl-c to interrupt the program
cleanup:
	bpf_link__destroy(link);
	bpf_object__close(obj);
	return 0;
}