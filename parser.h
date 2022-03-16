#ifndef PARSER_H
#define PARSER_H

#define MAX_STRING_LENGTH 1024

struct line_t {
  int nfields;
  char **fields;
};

struct csv_t {
  int nlines;
  struct line_t **lines;
};

struct line_t* parse_csv_line(FILE *csv, char sep);
struct csv_t* parse_csv(FILE *csv, char sep);
int destroy_line(struct line_t *line);
int destroy_csv(struct csv_t *csv);

#endif /* PARSER_H */
