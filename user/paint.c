#include <os.h>

typedef struct Pixel{
    /* This pixel structure must match the format! */
    char r;
    char g;
    char b;
    char a;
}Pixel;

Pixel color = { 0, 0, 0, 255 };

int mouse_x = 0;
int mouse_y = 0;
int prev_mouse_x = 0;
int prev_mouse_y = 0;

int getMouse(){
    prev_mouse_x = mouse_x;
    prev_mouse_y = mouse_y;
	getMousePos(&mouse_x,&mouse_y);
    return getMouseState();
}

void drawline(int x1, int y1, int x2, int y2, int r, int g, int b){
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
        change_pixel((int)x,(int)y,r,g,b);
        x += dxdt;
        y += dydt;
    }
}

void main(void) {
	while(1){
		int state = getMouse();
        if(state == 1){
            if(mouse_x < 50){
                if(mouse_y < 100){
                    Pixel black = { 0, 0, 0, 255 };
                    color = black;
                }else if(mouse_y < 200){
                    Pixel grey = { 88, 89, 91, 255 };
                    color = grey;
                }
                else if(mouse_y < 300){
                    Pixel orange = { 255, 130, 0, 255 };
                    color = orange;
                }
                else if(mouse_y < 400){
                    Pixel red = { 255, 0, 0, 255 };
                    color = red;
                }
                else if(mouse_y < 500){
                    Pixel green = { 0, 255, 0, 255 };
                    color = green;
                }
                else if(mouse_y < 600){
                    Pixel blue = { 0, 0, 255, 255 };
                    color = blue;
                }
                else if(mouse_y < 700){
                    Pixel pink = { 255, 0, 230, 100 };
                    color = pink;
                }else{
                    Pixel darkerpink = { 255, 0, 145, 255 };
                    color = darkerpink;
                }
            }else{
                drawline(prev_mouse_x,prev_mouse_y,mouse_x,mouse_y,color.r,color.g,color.b);
                drawline(prev_mouse_x+1,prev_mouse_y,mouse_x+1,mouse_y,color.r,color.g,color.b);
                drawline(prev_mouse_x,prev_mouse_y+1,mouse_x,mouse_y+1,color.r,color.g,color.b);
                drawline(prev_mouse_x-1,prev_mouse_y,mouse_x-1,mouse_y,color.r,color.g,color.b);
                drawline(prev_mouse_x,prev_mouse_y-1,mouse_x,mouse_y-1,color.r,color.g,color.b);

                updateFB();
            }
            
        }else if(state == 2){
            drawline(prev_mouse_x,prev_mouse_y,mouse_x,mouse_y,255,255,255);
            drawline(prev_mouse_x+1,prev_mouse_y,mouse_x+1,mouse_y,255,255,255);
            drawline(prev_mouse_x,prev_mouse_y+1,mouse_x,mouse_y+1,255,255,255);
            drawline(prev_mouse_x-1,prev_mouse_y,mouse_x-1,mouse_y,255,255,255);
            drawline(prev_mouse_x,prev_mouse_y-1,mouse_x,mouse_y-1,255,255,255);
            updateFB();
        }
	}
	return;
}



