import os
import subprocess
import sys
import toml

# TODO: this is not configured automatically, fix this on your machine first
types = {
    '::std::os::raw::c_char': 'i8',
    '::std::os::raw::c_double': 'f64',
    '::std::os::raw::c_float': 'f32',
    '::std::os::raw::c_int': 'i32',
    '::std::os::raw::c_longlong': 'i64',
    '::std::os::raw::c_long': 'i64',
    '::std::os::raw::c_schar': 'i8',
    '::std::os::raw::c_short': 'i16',
    '::std::os::raw::c_uchar': 'u8',
    '::std::os::raw::c_uint': 'u32',
    '::std::os::raw::c_ulonglong': 'u64',
    '::std::os::raw::c_ulong': 'u64',
    '::std::os::raw::c_ushort': 'u16'
}

# https://github.com/rust-lang/rust-bindgen
bindgen_cmd = 'bindgen --size_t-is-usize --use-core --no-doc-comments '\
        '--translate-enum-integer-types --no-layout-tests '\
        '--no-prepend-enum-name'.split()

# These 2 functions are from the old fixup_addrs.py
def filter_text(nm_line):
    # T/t means text(code) symbol
    return len(nm_line) == 3 and nm_line[1].lower() == 't'

def get_symbols(vmlinux):
    result = subprocess.run(['nm', vmlinux], check=True, capture_output=True)
    return map(lambda l: l.strip().split(),
            result.stdout.decode('utf-8').split('\n'))

def gen_stubs(vmlinux, helpers, out_dir):
    if len(helpers) == 0:
        return

    # Construct a func -> addr map
    text_syms = dict(map(lambda l: (l[2], l[0]),
                         filter(filter_text, get_symbols(vmlinux))))
    stubs = '\n'.join(map(lambda h: 'pub const STUB_%s: u64 = 0x%%s;' %\
            h.upper(), helpers))
    output = stubs % tuple(map(lambda f: text_syms[f], helpers))
    with open(os.path.join(out_dir, 'stub.rs'), 'w') as stub_f:
        stub_f.write(output)

# Generates Rust binding from C/C++ header
def bindgen(header):
    p = subprocess.run([*bindgen_cmd, header], check=True, capture_output=True)
    output = p.stdout.decode('utf-8')
    for ctype, rtype in types.items():
        output = output.replace(ctype, rtype)
    assert 'std::' not in output # sanity check
    return output

def prep_headers(usr_include, headers, out_dir):
    if len(headers) == 0:
        return

    for h in headers:
        output = bindgen(os.path.join(usr_include, h))

        subdir, file = os.path.split(h)
        subdir = os.path.join(out_dir, subdir)
        if not os.path.exists(subdir):
            os.makedirs(subdir)

        with open(os.path.join(subdir, '%s.rs' % file[:-2]), 'w') as bind_f:
            bind_f.write(output)

def parse_config(cargo_toml):
    config = toml.load(cargo_toml)
    
    if not 'inner_unikernel' in config:
        return [], []

    helpers = config['inner_unikernel'].get('helpers', [])
    headers = config['inner_unikernel'].get('headers', [])

    return helpers, headers

def main(argv):
    linux_path = argv[1]
    out_dir = argv[2]
    target_path = os.getcwd()
    helpers, headers = parse_config(os.path.join(target_path, 'Cargo.toml'))
    gen_stubs(os.path.join(linux_path, 'vmlinux'), helpers, out_dir)
    prep_headers(os.path.join(linux_path, 'usr/include'), headers, out_dir)
    return 0

if __name__ == '__main__':
    exit(main(sys.argv))