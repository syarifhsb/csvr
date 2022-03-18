#ifndef PARSER_H
#define PARSER_H

#define MAX_STRING_LENGTH 1024

struct line_t {
  int nfields;
  int blkfields;
  char **fields;
};

struct csv_t {
  int nlines;
  int blklines;
  struct line_t **lines;
};

struct line_t* parse_csv_line_blk(FILE *csv, char sep, int blkfields);
struct line_t* parse_csv_line(FILE *csv, char sep);
struct csv_t* parse_csv_blk(FILE *csv, char sep, int blklines, int blkfields);
struct csv_t* parse_csv(FILE *csv, char sep);
int destroy_line(struct line_t *line);
int destroy_csv(struct csv_t *csv);

#endif /* PARSER_H */
