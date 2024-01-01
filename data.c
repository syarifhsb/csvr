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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "data.h"

static size_t
get_buffer_length(const char *buf)
{
  size_t len = 0;
  while (*buf++ != '\0')
    len++;

  if (len > MAX_CHAR)
    return MAX_CHAR;
  return len;
}

static size_t
get_char_position(int row, int col, int max_row, int max_col)
{
  return ((row - 1) * max_col + (col - 1)) * MAX_CHAR;
}

static void
get_max_row_col(FILE *fd, const char *delim, int *max_row, int *max_col)
{
  ssize_t nread;
  size_t len = 0;
  char *buf = NULL;
  *max_row = 1;
  *max_col = 1;

  while ((nread = getline(&buf, &len, fd)) != -1) {
    char *str, *token;
    int col;
    for (col = 1, str = buf; ; col++, str = NULL) {
      token = strtok(str, delim);
      if (token == NULL)
        break;
    }

    (*max_row)++;
    if (col > *max_col)
      *max_col = col;
  }

  free(buf);
  rewind(fd);
}

const char *
get_str_by_pos(Data d, int row, int col)
{
  if (row > d.nrow || col > d.ncol)
    return NULL;

  return d.texts + get_char_position(row, col, d.nrow, d.ncol);
}

char *
get_str(Data d, int row, int col)
{
  if (d.texts == NULL)
    return NULL;

  return d.texts + get_char_position(row, col, d.nrow, d.ncol);
}

ssize_t
get_str_length(Data d, int row, int col)
{
  ssize_t len;

  if (d.texts == NULL)
    return -1;

  return (ssize_t)get_buffer_length(get_str(d, row, col));
}

int
get_row(Data d)
{
  return d.nrow;
}

int
get_col(Data d)
{
  return d.ncol;
}

void
del_data(Data d)
{
  if (d.texts != NULL)
    free(d.texts);
}

int text_is_not_empty(Data d)
{
  if (d.texts == NULL)
    return 0;

  return 1;
}

Data
parse_csv(FILE *csv_file, const char *delim)
{
  size_t len = get_buffer_length(delim) + 2;
  char delim_newline[len];
  memset(delim_newline, 0, sizeof delim_newline);

  ssize_t nread;
  char *buffer = NULL;
  int max_row, max_col;

  /* Add new line to the delimiter and */
  /* null byte char protection */
  int i;
  for (i = 0; delim[i] != '\0' && i < len - 1; i++)
    delim_newline[i] = delim[i];
  delim_newline[i++] = '\n';
  delim_newline[i] = '\0';

  get_max_row_col(csv_file, delim_newline, &max_row, &max_col);

  /* Data *d = malloc(sizeof(Data)); */
  Data d;
  d.nrow = max_row;
  d.ncol = max_col;
  d.len_alloc = MAX_CHAR * ROW_ALLOC * COL_ALLOC;
  d.texts = calloc(d.len_alloc, sizeof (char));

  len = 0;
  int row = 1;
  int col = 1;
  while ((nread = getline(&buffer, &len, csv_file)) != -1) {
    char *str, *token;
    for (col = 1, str = buffer; ; col++, str = NULL) {
      token = strtok(str, delim_newline);
      if (token == NULL)
        break;

      strncpy(
          d.texts+get_char_position(row, col, max_row, max_col),
          token,
          get_buffer_length(token) );
    }
    row++;
  }

  return d;
}
