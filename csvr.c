#include <ncurses.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config.h"

/* Macros */
#define LENGTH(X)               (sizeof X / sizeof X[0])

/* Enums and Structs */
enum { Top, Bottom};  /* Pivot Y */
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

/* Variables */
static WINDOW *headwin, *cellwin, *strlwin;
//static char sep = ';';
static Column cols[MAX_COLUMN];
static Row rows[MAX_ROW];
static struct sheetparam st;
static int height, width;
static int textboxheight;

/* Function implementations */
void cleanup(void)
{
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
    st.cellwinwidth = width - st.pad;
    delwin(cellwin);
    cellwin = newwin(height - (textboxheight + 1), width - st.pad, textboxheight + 1, st.pad); /* 1 is the size of header */
  } else {
    st.pad = get_digit(st.lastRow) + 1;
    st.cellwinwidth = width - st.pad;
  }

  if (st.pivotx == Left) {
    for (int j = st.begCol, colswidth = st.pad;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j++)
    {
      st.lastCol = j;
    }
  } else if (st.pivotx == Right) {
    for (int j = st.lastCol, colswidth = st.pad;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j--)
    {
      st.begCol = j;
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

  calcdim();

  strlwin = newwin(textboxheight, width, 0, 0);
  headwin = newwin(height - 1, width, textboxheight, 0);
  cellwin = newwin(height - (textboxheight + 1), width - st.pad, textboxheight + 1, st.pad); /* 1 is the size of header */

	wbkgd(stdscr, COLOR_PAIR(1));
	wbkgd(stdscr, COLOR_PAIR(2));
	wbkgd(strlwin, COLOR_PAIR(3));
	wbkgd(headwin, COLOR_PAIR(4));

  int i;
  for (i = 0; i < LENGTH(rows); i++) {
    rows[i].height = CELL_HEIGHT;
  }
  for (i = 0; i < LENGTH(cols); i++) {
    cols[i].width = CELL_WIDTH;
  }

  //debug();

  selectcell(1);

  refresh();
  headerupdate();
  refcell();
  refstrl();
}

int main(int argc, char **argv)
{
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
    refresh();
    headerupdate();
    refcell();
    refstrl();
  }
  cleanup();
  return 0;
}
