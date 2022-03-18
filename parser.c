#include <stdio.h>
#include <stdlib.h>

#include "parser.h"

struct line_t* parse_csv_line_blk(FILE *csv_file, char sep, int blkfields)
{
  int n;
  char ch;

  long set = ftell(csv_file);
  for (n = 1; (ch = fgetc(csv_file)) != EOF && (ch != '\n'); )
    if (ch == sep)
      n++;

  fseek(csv_file, set, SEEK_SET);

  struct line_t *line = calloc(1, sizeof(struct line_t));
  line->blkfields = blkfields;
  if (blkfields == 0)
    line->fields = calloc(n, sizeof(char*));
  else
    line->fields = calloc(blkfields, sizeof(char*));
  line->nfields = n;

  for (int i = 0; i < line->nfields; i++)
    line->fields[i] = calloc(MAX_STRING_LENGTH, sizeof(char));

  for (int i = 0, j = 0; (ch = fgetc(csv_file)) != EOF && (ch != '\n'); ) {
    if (ch == sep) {
      line->fields[i][j] = '\0';
      j = 0; i++;
    } else {
      line->fields[i][j] = ch;
      j++;
    }
  }

  return line;
}

struct line_t* parse_csv_line(FILE *csv_file, char sep)
{
  struct line_t *line = parse_csv_line_blk(csv_file, sep, 0);

  return line;
}

struct csv_t* parse_csv_blk(FILE *csv_file, char sep, int blklines, int blkfields)
{
  int n;
  char ch;

  long set = ftell(csv_file);
  for (n = 0; (ch = fgetc(csv_file)) != EOF; )
    if (ch == '\n')
      n++;

  fseek(csv_file, set, SEEK_SET);

  struct csv_t *csv = calloc(1, sizeof(struct csv_t));
  csv->blklines = blklines;
  if (blklines == 0)
    csv->lines = calloc(n, sizeof(struct line_t*));
  else
    csv->lines = calloc(blklines, sizeof(struct line_t*));
  csv->nlines = n;

  for (int i = 0; i < csv->nlines; i++) {
    csv->lines[i] = parse_csv_line(csv_file, sep);
  }

  return csv;
}

struct csv_t* parse_csv(FILE *csv_file, char sep)
{
  struct csv_t *csv = parse_csv_blk(csv_file, sep, 0, 0);

  return csv;
}

int destroy_line(struct line_t *line)
{
  for (int i = 0; i < line->nfields; i++) {
    free(line->fields[i]);
  }
  free(line->fields);
  free(line);
  return 0;
}

int destroy_csv(struct csv_t *csv)
{
  for (int i = 0; i < csv->nlines; i++) {
    destroy_line(csv->lines[i]);
  }
  free(csv->lines);
  free(csv);
  return 0;
}

