#ifndef PARSER_H
#define PARSER_H

#define MAX_STRING_LENGTH 1024

#include "string_st.h"

struct vec_t {
  VECTOR_ST **vs;
  size_t n;
};

struct vec_t* parse_csv_v(FILE *csv_file, char sep);
int destroy_strings(struct vec_t *v);

#endif /* PARSER_H */
