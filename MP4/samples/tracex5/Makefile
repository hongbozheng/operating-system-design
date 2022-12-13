LINUX ?= $(abspath ../../linux)

LINUX_INC = ${LINUX}/usr/include
LIBBPF_DIR = ${LINUX}/tools/lib/bpf
LIBIU_DIR = $(abspath ../../libiu)

DIST = $(abspath ../../rust-dist/bin)
CARGOBIN = $(abspath $(HOME)/.cargo/bin)

IUFLAGS = -Clink-arg=-nostartfiles -Ciu-playground

all: target/debug/tracex5 tracex5

target/debug/tracex5: Cargo.toml ./src/*.rs ${LINUX}/vmlinux
	PATH=${DIST}:${CARGOBIN}:${PATH} LINUX=${LINUX} cargo rustc -vv -- ${IUFLAGS}

tracex5: tracex5.c
	gcc -I${LINUX_INC} -I${LIBBPF_DIR} -I${LIBIU_DIR} -g -Wl,--as-needed $< \
	${LIBBPF_DIR}/libbpf.a ${LIBIU_DIR}/libiu.a -lelf -lstdc++ -lz -o $@

clean:
	rm -rf tracex5
	PATH=${DIST}:${CARGOBIN}:${PATH} cargo clean
