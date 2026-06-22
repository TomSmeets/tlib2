#include "os_main.h"
#include "time.h"
#include "fmt.h"
#include "fs.h"

static void os_main(void) {
    assert(os_argc >= 2);
    char *name = os_argv[1];
    char *dir  = os_argv[2];


    bool is_neg = dir && str_eq(dir, "-");
    bool is_pos = dir && str_eq(dir, "+");
    if(is_neg || is_pos) {
        File *f = fs_open(name, FileMode_Append);
        u64 bit = (u64)1 << 63;
        u64 data = ((u64) time_now()) & ~bit;
        if (is_neg) data |= bit;
        io_write(f, buf_from(&data, sizeof(data)));
        io_close(f);
    } else {
        Buffer data = fs_read(mem_tmp(), name);
        u64 *p = (u64*) data.data;

        File *f = fs_open(name, FileMode_Read);
        while(!error) {
            u64 data = 0;
            io_read(f, buf_from_struct(&data));
            print(data);
        }
        io_close(f);
    }

    os_exit();
}
