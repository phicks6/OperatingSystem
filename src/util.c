#include <util.h>

//Returns 0 on equal or -1 when string1 is less than string2
int strcmp(const char *str1, const char *str2){
    unsigned char c1, c2;
	while(1){
		c1 = *str1++;
		c2 = *str2++;
		if(c1 != c2){
           if(c1 < c2){
               return -1;
           }else{
               return 1;
           }
        }else if (!c1 && !c2){
            return 0;
        }
	}
	return 0;
}

int atoi(const char *str){
	int r = 0;
	int p = 1;
	int i;
	int l = 0;
	int n = 0;

	if(str[0] == '-'){
		str++;
		n = 1;
	}
	while(str[l] >= '0' && str[l] <= '9'){
        l++;
    }
	for(i = l - 1; i >= 0; i--){
		r += p * (str[i] - '0');
		p *= 10;
	}

	if(n){
		return -r;
	}else{
		return r;
	}
}



int strncmp(const char *str1, const char *str2, int n){
	unsigned char c1 = '\0';
	unsigned char c2 = '\0';
	while(n > 0){
		c1 = *str1;
		c2 = *str2;
		if(c1 == '\0' || c1 != c2){
            return c1 - c2;
        }	
		n--;
		str1++;
		str2++;
	}

	return c1 - c2;
}

int strfindchr(const char *str, char c){
	int i = 0;
	while(str[i] != c){
		if(str[i] == '\0'){
			return -1;
		}
		i++;
	}
	return i;
}

int strlen(const char *str){
	int len = 0;
	while(str[len]){
        len++;
    } 
	return len;
}

char *strcpy(char *dest, const char *s)
{
    char *o = dest;
    while (*s) {
        *dest++ = *s++;
    }
    *dest = '\0';
    return o;
}


void *memset(void *dst, char data, unsigned long size){
    unsigned long i;
    char *cdst = (char *)dst;

    for(i = 0;i < size;i++){
        cdst[i] = data;
    }

    return dst;
}

void *memcpy(void *dst, const void *src, int64_t size){
	int64_t i;
	char *cdst = (char *)dst;
	const char *const csrc = (char *)src;

	for(i = 0; i < size; i++){
		cdst[i] = csrc[i];
	}

	return dst;
}

bool memcmp(const void *haystack, const void *needle, int64_t size){
	const char *hay = (char *)haystack;
	const char *need = (char *)needle;
	int64_t i;

	for(i = 0; i < size; i++){
		if(hay[i] != need[i]){
			return false;
		}
	}
	return true;
}
