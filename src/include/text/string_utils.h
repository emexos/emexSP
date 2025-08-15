#ifndef STRING_UTILS_H
#define STRING_UTILS_H

// String utility functions for the shell
int str_equals(const char* a, const char* b);
int str_starts_with(const char* str, const char* prefix);
int str_length(const char* str);
void str_copy(char* dest, const char* src, int max_len);

#endif
