keep = [
    'sys_write',
    'sys_read',
    'sys_mmap',
    'sys_open',
    'sys_close',

    'sys_accept',
    'sys_bind',
    'sys_socket',
]

nums = {}
with open('syscall_64.tbl', 'r') as f:
    for l in f.readlines():
        l = l.strip()
        if l.startswith('#'):
            continue
        if l == '':
            continue
        cols = l.split()
        if len(cols) < 4:
            continue
        num = cols[0]
        arch = cols[1]
        name = cols[2]
        name2 = cols[3]
        if arch == 'x32':
            continue
        nums[name2] = int(num)

def split_arg(arg: str) -> (str, str):
    for i in range(len(arg)):
        j = len(arg) - i - 1
        if arg[j] == ' ' or arg[j] == '*':
            return (arg[0:j+1].strip(), arg[j+1:].strip())
funs = {}
with open('syscalls.h', 'r') as f:
    for l in f.readlines():
        l = l.strip()
        sig = l[:-1]
        [ret, l] = l.split(' ', 1)
        [name, l] = l.split('(', 1)
        l = l[:-2]
        args = [split_arg(c.strip()) for c in l.split(',')]
        if name not in keep:
            continue
        funs[name] = [ret, name, sig, args]

for name in keep:
    [ret, _, sig, args] = funs[name]
    num = nums[name]
    print(f'static {sig}{{linux_syscall{len(args)}({num}{"".join([", (i64)" + a[1] for a in args])})}}')
