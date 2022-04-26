#ifndef STRING_ST_H
#define STRING_ST_H

typedef struct string_t STRING_ST;
typedef struct vector_t VECTOR_ST;
typedef struct table_t  TABLE_ST;

TABLE_ST*  new_table();
TABLE_ST*  new_table_s(size_t sz);
TABLE_ST*  t_append(TABLE_ST *dst, VECTOR_ST *src);
TABLE_ST*  t_concat(int n, ...);
TABLE_ST*  t_copy(TABLE_ST *src);
TABLE_ST*  parse_delimited_f(FILE *s, char d); /* Not Yet implemented */
TABLE_ST*  transpose(TABLE_ST *t);

int del_table(TABLE_ST *t);
size_t t_get_len(TABLE_ST *t);
size_t t_get_mlen(TABLE_ST *t);
size_t t_get_max_vector_len(TABLE_ST *t);
const char* t_get_str_l(TABLE_ST *t, size_t i, size_t j);
STRING_ST* t_get_str(TABLE_ST *t, size_t i, size_t j);
VECTOR_ST* t_get_vector(TABLE_ST *t, size_t index);

VECTOR_ST* new_vector();
VECTOR_ST* new_vector_s(size_t sz);
VECTOR_ST* v_append(VECTOR_ST *dst, STRING_ST *src);
VECTOR_ST* v_concat(int n, ...);
VECTOR_ST* v_copy(VECTOR_ST *src);
VECTOR_ST* parse_delimited_c(STRING_ST *s, char d);
VECTOR_ST* parse_delimited_l(STRING_ST *s, const char *d);

int del_vector(VECTOR_ST *v);
size_t v_get_len(VECTOR_ST *v);
size_t v_get_mlen(VECTOR_ST *v);
const char* v_get_str_l(VECTOR_ST* v, size_t index);
STRING_ST* v_get_str(VECTOR_ST *v, size_t index);

STRING_ST* new_str(const char *l);
STRING_ST* new_str_s(const char *l, size_t sz);
STRING_ST* new_empty_str();
STRING_ST* new_empty_str_s(size_t sz);
STRING_ST* s_append_c(STRING_ST *dst, char ch);
STRING_ST* s_append_l(STRING_ST *dst, const char *src);
STRING_ST* s_concat(int n, ...);
STRING_ST* s_copy(STRING_ST *s);

int del_str(STRING_ST *s);
size_t s_get_len(STRING_ST *s);
size_t s_get_mlen(STRING_ST *s);
const char* s_get_str_l(STRING_ST *s);

#endif /* STRING_ST_H */
