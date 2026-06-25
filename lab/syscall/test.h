static long sys_write(uint fd, const char *buf, size_t count) {
    linux_syscall3(1, (i64)fd, (i64)buf, (i64)count)
}
static long sys_read(uint fd, char *buf, size_t count) {
    linux_syscall3(0, (i64)fd, (i64)buf, (i64)count)
}
static long sys_mmap(ulong addr, ulong len, ulong prot, ulong flags, ulong fd, ulong pgoff) {
    linux_syscall6(9, (i64)addr, (i64)len, (i64)prot, (i64)flags, (i64)fd, (i64)pgoff)
}
static long sys_open(const char *filename, int flags, umode_t mode) {
    linux_syscall3(2, (i64)filename, (i64)flags, (i64)mode)
}
static long sys_close(uint fd) {
    linux_syscall1(3, (i64)fd)
}
