#pragma once

void *pmalloc(unsigned int size);
void *pzalloc(unsigned int size);
void *cont(unsigned int size);
void *prealloc(void *ptr, unsigned int size);
void pfree(void *ptr);


