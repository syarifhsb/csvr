/* This file is part of csvr.
 *
 * csvr is free software: you can redistribute it and/or modify it under the terms of the 
 * GNU General Public License as published by the Free Software Foundation, either version 
 * 2 of the License, or (at your option) any later version.
 *
 * csvr is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with csvr. 
 * If not, see <https://www.gnu.org/licenses/>. 
 */

#include <stdio.h>
#include <stdlib.h>

#include "string_st.h"

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
  
  t = transpose(t);

  return t;
}

