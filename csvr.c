#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include "parser.h"
#include "config.h"

/* Macros */
#define LENGTH(X)               (sizeof X / sizeof X[0])

/* Enums and Structs */
enum { Top, Bottom };  /* Pivot Y */
enum { Left, Right }; /* Pivot X */

struct sheetparam {
  int cellwinheight;
  int cellwinwidth;
  int nrow, ncol;
  int begRow, lastRow;
  int begCol, lastCol;
  int activeRow, activeCol;
  int pad; /* Row number Padding: includes number and the bar char: '25|' */
  int pivoty, pivotx;
  int ycov, xcov; /* part of cell that is hidden */
};

typedef struct {
  int width;
} Column;

typedef struct Row {
  int height;
} Row;

/* Functions declarations */
//static Column* alloccol(int n);
//static Row* allocrow(int n);
static void cleanup(void);
static int get_digit(int n);
static void headerupdate(void);
static void refcell(void);
static void refstrl(void);
static void setup(void);
static void calcdim(void);
static void movecell(int y, int x);
static void selectcell(int activate);
static void usage(void);
static void writesinglecell(int y, int x, int height, int width, char *str);
static void writecells(void);

/* Variables */
static WINDOW *headwin, *cellwin, *strlwin;
static char sep = ',';
static Column cols[MAX_COLUMN];
static Row rows[MAX_ROW];
static struct sheetparam st;
static int height, width;
static int textboxheight;
static struct csv_t *csv = NULL;
extern int errno;

/* Function implementations */
void cleanup(void)
{
  delwin(cellwin);
  delwin(headwin);
  delwin(strlwin);
	endwin();
}

/* TODO: Reformat for more readability */
void printcolindex(int i, int b, int w)
{
  /* Column index is limited to ZZ. 3 digit not possible */
  if (i > MAX_COLUMN - 1)
    return;

  if (i / 26) {
    if ((b + w/2 - 1) >= 0)
      mvwprintw(headwin, 0, b + w/2 - 1, "%c", 64 + (i / 26));
  }
  if ((b + w/2) >= 0)
    mvwprintw(headwin, 0, b + w/2, "%c", 65 + (i % 26));
}

int get_digit(int n)
{
  unsigned short d = 0;
  while(n != 0){
    n = n / 10;
    d++;
  }
  if(d == 0){
    d = 1;
  }
  return d;
}

void headerupdate()
{
  int i, j;

  /* TODO: Row height != 1 */
  for (i = 0; i <= st.lastRow - st.begRow; i++){
    wmove(headwin, i + 1, 0);
    for (j = 0; j < st.pad - (1 + get_digit(st.begRow + i)); j++){
      wprintw(headwin, " ");
    }
    wprintw(headwin, "%d|", st.begRow + i);
  }

  wmove(headwin, 0, 0);
  wclrtoeol(headwin);
  int begin;
  if (st.pivotx == Left) {
    begin = st.pad;
    for (j = st.begCol; j <= st.lastCol; j++) {
      printcolindex(j - 1, begin, cols[j - 1].width);
      mvwprintw(headwin, 0, begin + cols[j - 1].width - 1, "|");
      begin += cols[j - 1].width;
    }
  } else if (st.pivotx == Right) {
    begin = width;
    for (j = st.lastCol; j >= st.begCol; j--) {
      begin -= cols[j - 1].width;
      /* TODO: Potential bug */
      //if (begin < 0) {
      //  mvwprintw(headwin, 0, begin + cols[j - 1].width - 1, "|");
      //  break;
      //}
      printcolindex(j - 1, begin, cols[j - 1].width);
      mvwprintw(headwin, 0, begin + cols[j - 1].width - 1, "|");
    }
  }

  wmove(headwin, 0, 0);
  for (j = 0; j < st.pad - 1; j++) {
    wprintw(headwin, " ");
  }
  wprintw(headwin, "|");

  wrefresh(headwin);
}

void refstrl()
{
  wrefresh(strlwin);
}

void refcell()
{
  wrefresh(cellwin);
}

void writesinglecell(int y, int x, int height, int width, char *str)
{
  /* TODO: Wrap text when have row height > 1 */
  char temp[width];
  snprintf(temp, width, str);
  mvwprintw(cellwin, y, x, "%s", temp);
}

void writecells()
{
  if (!csv) {
    selectcell(1);
    return;
  }

  werase(cellwin);
  for (int i = 0, yt = 0; i < st.lastRow - st.begRow + 1 && i < csv->nlines - st.begRow + 1; i++) {
    int cellheight = rows[st.begCol + i - 1].height;
    for (int j = 0, xt = 0; j < st.lastCol - st.begCol + 1 && j < csv->lines[i]->nfields - st.begCol + 1; j++) {
      int cellwidth;
      if ((j == st.lastCol - st.begCol && st.pivotx == Left) || (j == 0 && st.pivotx == Right))
        cellwidth = cols[st.begCol + j - 1].width - st.xcov;
      else
        cellwidth = cols[st.begCol + j - 1].width;
      writesinglecell(yt, xt, cellheight, cellwidth,
          csv->lines[st.begRow + i - 1]->fields[st.begCol + j - 1]);
      xt += cellwidth;
    }
    yt += cellheight;
  }
  selectcell(1);
}

void calcdim()
{
  int h, w;
  getmaxyx(stdscr, h, w);
  height = h; width = w;

  st.cellwinheight = height - textboxheight;
  if (st.pivoty == Top) {
    st.lastRow = (st.cellwinheight - 1) + (st.begRow - 1);
  } else if (st.pivoty == Bottom) {
    st.begRow = (st.lastRow + 1) - (st.cellwinheight - 1);
  }

  if (cellwin && st.pad - (get_digit(st.lastRow) + 1)) {
    st.pad = get_digit(st.lastRow) + 1;
    int widthtemp = width - st.pad;
    if (st.cellwinwidth - widthtemp) {
      delwin(cellwin);
      cellwin = newwin(height - (textboxheight + 1), width - st.pad, textboxheight + 1, st.pad); /* 1 is the size of header */
      st.cellwinwidth = widthtemp;
    }
  } else {
    st.pad = get_digit(st.lastRow) + 1;
    st.cellwinwidth = width - st.pad;
  }

  if (st.pivotx == Left) {
    for (int j = st.begCol, colswidth = 0;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j++)
    {
      st.lastCol = j;
      st.xcov = cols[j - 1].width - (st.cellwinwidth - colswidth);
    }
  } else if (st.pivotx == Right) {
    for (int j = st.lastCol, colswidth = 0;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j--)
    {
      st.begCol = j;
      st.xcov = cols[j - 1].width - (st.cellwinwidth - colswidth);
    }
  }
}

void selectcell(int activate)
{
  int begin;
  int j;
  /* TODO: Row height != 1 */
  if (st.pivotx == Left) {
    begin = 0;
    for (j = st.begCol; j < st.activeCol; j++) {
      begin += cols[j - 1].width;
    }
    mvwchgat(cellwin, st.activeRow - st.begRow, 
        begin, cols[st.activeCol - 1].width - 1,
        A_NORMAL, activate + 1, NULL);
  }
  else if (st.pivotx == Right) {
    begin = st.cellwinwidth;
    for (j = st.lastCol; j >= st.activeCol; j--) {
      begin -= cols[j - 1].width;
    }
    mvwchgat(cellwin, st.activeRow - st.begRow, 
        begin, cols[st.activeCol - 1].width - 1,
        A_NORMAL, activate + 1, NULL);
  }
}

void movecell(int y, int x)
{
  selectcell(0);

  st.activeRow += y;
  st.activeCol += x;

  if (st.activeRow <= 0) {
    st.activeRow = 1;
    st.pivoty = Top;
  } else if (st.activeRow <= st.begRow) {
    st.begRow = st.activeRow;
    st.pivoty = Top;
  } else if (st.activeRow > MAX_ROW) {
    st.activeRow = MAX_ROW;
    st.pivoty = Bottom;
  } else if (st.activeRow >= st.lastRow) {
    st.lastRow = st.activeRow;
    st.pivoty = Bottom;
  }

  if (st.activeCol <= 0) {
    st.activeCol = 1;
    st.pivotx = Left;
  } else if (st.activeCol <= st.begCol) {
    st.begCol = st.activeCol;
    st.pivotx = Left;
  } else if (st.activeCol > MAX_COLUMN) {
    st.activeCol = MAX_COLUMN;
    st.pivotx = Right;
  } else if (st.activeCol >= st.lastCol) {
    st.lastCol = st.activeCol;
    st.pivotx = Right;
  }

  calcdim();
  selectcell(1);
}

void debug(void)
{
  cols[2].width = 10;
  cols[3].width = 14;
  cols[7].width = 5;

  mvwprintw(cellwin, 1, 25, "Debug. st.begCol: %d, st.lastCol: %d", st.begCol, st.lastCol); /* debug */
  mvwprintw(cellwin, 2, 25, "Debug. st.begRow: %d, st.lastRow: %d", st.begRow, st.lastRow); /* debug */
  mvwprintw(cellwin, 3, 25, "Debug. st.cellwinheight: %d, st.cellwinwidth: %d", st.cellwinheight, st.cellwinwidth); /* debug */
}

void setup()
{
  initscr();
  textboxheight = 1;
	start_color();
	init_pair(1, FG_COLOR, BG_COLOR);
	init_pair(2, BG_COLOR, FG_COLOR);
	init_pair(3, FG_COLOR_STRL, BG_COLOR_STRL);
	init_pair(4, FG_COLOR_HEAD, BG_COLOR_HEAD);
	keypad(stdscr, TRUE);
	curs_set(0);
  noecho();

  st.nrow = 1; st.ncol = 1;
  st.activeRow = 1; st.activeCol = 1;

  st.pivoty = Top; st.pivotx = Left;
  st.begRow = 1;
  st.begCol = 1;

  for (int i = 0; i < LENGTH(rows); i++) {
    rows[i].height = CELL_HEIGHT;
  }
  for (int i = 0; i < LENGTH(cols); i++) {
    cols[i].width = CELL_WIDTH;
  }

  //debug();
  calcdim();

  strlwin = newwin(textboxheight, width, 0, 0);
  headwin = newwin(height - 1, width, textboxheight, 0);
  cellwin = newwin(height - (textboxheight + 1), width - st.pad, textboxheight + 1, st.pad); /* 1 is the size of header */

	wbkgd(stdscr, COLOR_PAIR(1));
	wbkgd(stdscr, COLOR_PAIR(2));
	wbkgd(strlwin, COLOR_PAIR(3));
	wbkgd(headwin, COLOR_PAIR(4));


  writecells();

  refresh();
  headerupdate();
  refcell();
  refstrl();
}

void usage(void)
{
  fprintf(stdout, "Usage: csvr <file.csv>\n");
}

int main(int argc, char **argv)
{
  FILE *csv_file;

  if (argc > 2) {
    usage();
    exit(0);
  }
  if (argc == 2) {
    csv_file = fopen(*(argv + 1), "r");
    if (!csv_file) {
      fprintf(stderr, "ERROR. %s: \'%s\'\n", strerror(errno), *(argv + 1));
      exit(1);
    }
    csv = parse_csv(csv_file, sep);
    fclose(csv_file);
  }

  unsigned int c;
  setup();
  while ((c = getch()) != 'q') {
    switch (c)
    {
			case 'h':
			case KEY_LEFT:
				movecell(0, -1);
				break;
			case 'l':
			case KEY_RIGHT:
				movecell(0,  1);
				break;
			case 'k':
			case KEY_UP:
				movecell(-1,  0);
				break;
			case 'j':
			case KEY_DOWN:
				movecell(1,  0);
				break;
    }
    //debug();
    writecells();
    refresh();
    headerupdate();
    refcell();
    refstrl();
  }

  if (csv)
    destroy_csv(csv);
  cleanup();
  return 0;
}
