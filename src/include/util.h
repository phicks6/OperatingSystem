#pragma once

#include <stdint.h>
#include <stdbool.h>

int strcmp(const char *l, const char *r);
int atoi(const char *st);
int strncmp(const char *l, const char *r, int n);
int strfindchr(const char *r, char t);
int strlen(const char *s);
char *strcpy(char *dest, const char *s);
void *memset(void *dst, char data, unsigned long size);
void *memcpy(void *dst, const void *src, int64_t size);
bool memcmp(const void *haystack, const void *needle, int64_t size);