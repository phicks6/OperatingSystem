#include "os.h"

void yield(void) {
    asm volatile("mv a7, %0\necall" : : "r"(3) : "a7");
}
void sleep(int tm) {
    asm volatile("mv a7, %0\nmv a0, %1\necall" : : "r"(4), "r"(tm) : "a0", "a7");
}
void change_pixel(int x, int y, int r, int g, int b) {
    asm volatile(
        "mv a7, %0\nmv a0, %1\nmv a1, %2\nmv a2, %3\nmv a3, %4\nmv a4, %5\necall"
        : :
        "r"(9), "r"(x), "r"(y), "r"(r), "r"(g), "r"(b)
        : "a0", "a1", "a2", "a3", "a4", "a7"
    );
}
void updateFB(void) {
    asm volatile("mv a7, %0\necall" : : "r"(10) : "a7");
}
void getMousePos(int *x, int *y){
    int xv = -1;
    int yv = -1;
    asm volatile("mv a7, %2\necall\nmv %0, a0\nmv %1, a1"
                 : "=r"(xv), "=r"(yv)
                 : "r"(11)
                 : "a0", "a1", "a7");
    *x=xv;
    *y=yv;
}
int getMouseState(){
    int state;
    asm volatile("mv a7, %1\necall\nmv %0, a0"
                 : "=r"(state)
                 : "r"(12)
                 : "a0", "a7");
   
    return state;
}
void fill(int x, int y, int r, int g, int b) {
    asm volatile(
        "mv a7, %0\nmv a0, %1\nmv a1, %2\nmv a2, %3\nmv a3, %4\nmv a4, %5\necall"
        : :
        "r"(13), "r"(x), "r"(y), "r"(r), "r"(g), "r"(b)
        : "a0", "a1", "a2", "a3", "a4", "a7"
    );
}