#include <syscall.h>
#include <csr.h>
#include <printf.h>
#include <stdbool.h>
#include <stdint.h>
#include <scheduler.h>
#include <paint.h>
#include <input.h>
void syscall_handle(int hart, uint64_t sepc, int64_t *scratch){
    CSR_WRITE("sepc", sepc+4);

    switch(scratch[XREG_A7]){ 
        case EXIT: 
            printf("Exit sys call\n");
            scheduler_exit(hart);
            break;
        case PUT_CHAR:
            printf("%c",scratch[XREG_A0]);
            break;
        case 9:
            change_pixel(scratch[XREG_A0], scratch[XREG_A1], scratch[XREG_A2], scratch[XREG_A3], scratch[XREG_A4]);
            break;
        case 10:
            gpu_update_frame();
            break;
        case 11:{
            int x;
            x = get_mouse_x();
            int y;
            y = get_mouse_y();
            scratch[XREG_A0] = x;
            scratch[XREG_A7] = x;
            scratch[XREG_A1] = y;
        }
            break;
        case 12:
            scratch[XREG_A0] = get_mouse_state();
            break;
        default:
            printf("Unknown os system call %d on hart %d\n", scratch[XREG_A7], hart);
    }
}
