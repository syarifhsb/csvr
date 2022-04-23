#include <stdio.h>
#include <stdlib.h>

#include "string_st.h"

#define MAX_STRING_LENGTH 1024

TABLE_ST* parse_csv(FILE *csv_file, char sep)
{
  int n;
  char ch;

  long set = ftell(csv_file);
  for (n = 0; (ch = fgetc(csv_file)) != EOF; )
    if (ch == '\n')
      n++;

  TABLE_ST *t;
  t = new_table();
  if (!t)
    return NULL;

  fseek(csv_file, set, SEEK_SET);
  for (int i = 0; i < n; i++) {
    STRING_ST* line = new_empty_str();

    while ((ch = fgetc(csv_file)) != '\n')
      s_append_c(line, ch);

    VECTOR_ST *line_v;
    line_v = parse_delimited_c(line, sep);
    del_str(line);
    t = t_append(t, line_v);
  }

  return t;
}

