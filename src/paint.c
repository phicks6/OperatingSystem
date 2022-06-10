#include <paint.h>
#include <gpu.h>
#include <malloc.h>
#include <mmu.h>
#include <csr.h>
#include <util.h>
#include <input.h>
#include <input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>

int screen_width;
int screen_height;
Pixel *FB;
Pixel color = { 255, 0, 0, 255 };
Pixel background = { 255, 255, 255, 255 };
//int mouse_x = 0;
//int mouse_y = 0;
//int prev_mouse_x = 0;
//int prev_mouse_y = 0;

void make_blank_canvas(void){
    int * width = pzalloc(sizeof(int));
    int * height = pzalloc(sizeof(int));
    FB = get_frame_buffer(width,height);
    screen_width = *width;
    screen_height = *height;
    //printf("creating blank canvas with width %d and heigth %d\n",screen_width, screen_height);
    Rectangle r = {0, 0, screen_width, screen_height};
    
    fill_rect(screen_width, screen_height, FB, &r, &background);
    //printf("filled screen with white\n");
    Pixel border = { 30, 30, 30, 255 };

    Pixel black = { 0, 0, 0, 255 };
    Pixel grey = { 88, 89, 91, 255 };
    Pixel orange = { 255, 130, 0, 255 };
    Pixel red = { 255, 0, 0, 255 };
    Pixel green = { 0, 255, 0, 255 };
    Pixel blue = { 0, 0, 255, 255 };
    Pixel pink = { 255, 0, 230, 100 };
    Pixel darkerpink = { 255, 0, 145, 255 };

    r.x = 0;
    r.y = 0;
    r.width = 50;
    r.height = 800;
    fill_rect(screen_width, screen_height, FB, &r, &border);

    r.x = 5;
    r.y = 5;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &black);

    r.x = 5;
    r.y = 105;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &grey);

    r.x = 5;
    r.y = 205;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &orange);

    r.x = 5;
    r.y = 305;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &red);

    r.x = 5;
    r.y = 405;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &green);

    r.x = 5;
    r.y = 505;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &blue);

    r.x = 5;
    r.y = 605;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &pink);

    r.x = 5;
    r.y = 705;
    r.width = 40;
    r.height = 90;
    fill_rect(screen_width, screen_height, FB, &r, &darkerpink);

    gpu_update_frame();
}

void change_pixel(int x, int y, int r, int g, int b){
    Pixel color = { r, g, b, 255 };
    FB[(y*screen_width)+x] = color;
}

void draw_line(int x1, int y1, int x2, int y2, Pixel color){
    int temp = 0;
    

    float x,y;
    int dx = x2-x1;
    int dy = y2-y1;
    int n;

    int posx = dx;
    if(dx < 0){
        posx = dx*-1;
    }
    int posy = dy;
    if(dy < 0){
        posy = dy*-1;
    }

    if(posx > posy){
        n = posx;
    }else{
        n = posy;
    }
    
    float dt = n;
    float dxdt = dx/dt;
    float dydt = dy/dt;
    x = x1;
    y = y1;
    while( n-- ) {
        change_pixel((int)x,(int)y,color.r,color.g,color.b);
        //point(round(x),round(y));
        x += dxdt;
        y += dydt;
    }
}