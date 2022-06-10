#pragma once

void sleep(int tm);
void yield(void);
void change_pixel(int x, int y, int r, int g, int b);
void updateFB(void);
void getMousePos(int *x, int *y);
int getMouseState(void);
void fill(int x, int y, int r, int g, int b);