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

#ifndef DATA_H
#define DATA_H

#define MAX_CHAR 1024
#define ROW_ALLOC 256
#define COL_ALLOC 256

typedef struct data_t {
  int nrow;
  int ncol;
  size_t len_alloc;
  char *texts;
} Data;

Data parse_csv(FILE *csv_file, const char *delim);

void del_data(Data d);

int get_col(Data d);
int get_row(Data d);
ssize_t get_str_length(Data d, int row, int col);
char * get_str(Data d, int row, int col);
int text_is_not_empty(Data d);

#endif /* DATA_H */
