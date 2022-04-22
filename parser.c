#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "string_st.h"

struct vec_t* parse_csv_v(FILE *csv_file, char sep)
{
  int n;
  char ch;

  long set = ftell(csv_file);
  for (n = 0; (ch = fgetc(csv_file)) != EOF; )
    if (ch == '\n')
      n++;

  struct vec_t *pv;
  pv = calloc(1, sizeof(struct vec_t));
  if (!pv)
    return NULL;
  pv->vs = calloc(n, sizeof(VECTOR_ST*));
  if (!pv->vs) {
    free(pv);
    return NULL;
  }

  fseek(csv_file, set, SEEK_SET);
  for (int i = 0; i < n; i++) {
    STRING_ST* line = new_empty_str();

    while ((ch = fgetc(csv_file)) != '\n')
      s_append_c(line, ch);

    VECTOR_ST *line_v;
    line_v = parse_delimited_c(line, sep);
    pv->vs[i] = line_v;
    pv->n += 1;
    del_str(line);
  }

  return pv;
}

int destroy_strings(struct vec_t *v)
{
  for (int i = 0; i < v->n; i++) {
    del_vector(v->vs[i]);
  }
  free(v->vs);
  free(v);
  return 0;
}
