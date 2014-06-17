#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED



#define To32(x, y, z) ((x) << 16 | (y) << 8 | (z))
#define To32ex(arr) (To32((arr[0]), (arr[1]), (arr[2])))



void split_32to8(uint32_t val, uint8_t *arr);
int str2uint8arr(char *str, uint8_t *arr);
int str_cmp(const char *p1, const char *p2);
char *pad_str(char *buf, const char *str);
int get32and8arr(char *buf, char *str, uint32_t *val, uint8_t *arr);



#endif /* UTIL_H_INCLUDED */

