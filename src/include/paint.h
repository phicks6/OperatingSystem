#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <gpu.h>

void make_blank_canvas(void);
void change_pixel(int x, int y, int r, int g, int b);

void draw(void);