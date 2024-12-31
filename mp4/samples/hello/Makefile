LINUX ?= $(abspath ../../linux)

LINUX_INC = ${LINUX}/usr/include
LIBBPF_DIR = ${LINUX}/tools/lib/bpf
LIBIU_DIR = $(abspath ../../libiu)

DIST = $(abspath ../../rust-dist/bin)
CARGOBIN = $(abspath $(HOME)/.cargo/bin)

IUFLAGS = -Clink-arg=-nostartfiles -Ciu-playground

all: target/debug/hello loader

target/debug/hello: Cargo.toml ./src/*.rs ${LINUX}/vmlinux
	PATH=${DIST}:${CARGOBIN}:${PATH} LINUX=${LINUX} cargo rustc -vv -- ${IUFLAGS}

loader: loader.c
	gcc -I${LINUX_INC} -I${LIBBPF_DIR} -I${LIBIU_DIR} -g -Wl,--as-needed $< \
	${LIBBPF_DIR}/libbpf.a ${LIBIU_DIR}/libiu.a -lelf -lstdc++ -lz -o $@

clean:
	rm -rf loader
	PATH=${DIST}:${CARGOBIN}:${PATH} cargo clean
