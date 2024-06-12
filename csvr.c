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

#include <ncurses.h>
#include <panel.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "data.h"

/* Macros */
#define LENGTH(X)         (sizeof X / sizeof X[0])

/* Enums and Structs */
enum { Top, Bottom };  /* Pivot Y */
enum { Left, Right };  /* Pivot X */
enum { Normal, NormalCell, Visual, Insert, Quit };  /* Program Mode */
enum { HeaderWindow, CellWindow, TextWindow, DebugWindow, EndWindow }; /* Windows */
enum { GotoPanel, EndPanel }; /* Panels */

typedef struct windowparam {
  WINDOW* windows[EndWindow];
  PANEL* panels[EndPanel];
  WINDOW* win_panels[EndPanel];
} WINDOWPARAM;

typedef struct sheetparam {
  int cellwinheight;
  int cellwinwidth;
  int cellwinupdate;
  int nrow, ncol;
  int begRow, lastRow;
  int begCol, lastCol;
  int vbegRow, vlastRow;
  int vbegCol, vlastCol;
  int currentRow, currentCol;
  int pad; /* Row number Padding: includes number and the bar char: '25|' */
  int pivoty, pivotx;
  int ycov, xcov; /* part of cell that is hidden */
} SHEETPARAM;

typedef struct {
  int width;
} Column;

typedef struct {
  int height;
} Row;

typedef union {
  int i;
  unsigned int ui;
  float f;
  const void *v;
} Arg;

typedef struct {
  int ch;
  int nextst;
  void (*func)(const Arg *);
  const Arg arg;
} Key;

/* Functions declarations */
static void calcdim(void);
static void cleanup(Data d);
static void csvr_resize(const Arg *arg);
/* static void gotopopup(const Arg *arg); */
static void handlesig(int signal);
static void headerupdate(void);
static void keypress(int ch);
static void movecell(int y, int x);
static void movex(const Arg *arg);
static void movey(const Arg *arg);
static void recreate_windows(void);
static void resizecell(int y, int x);
static void resizecellx(const Arg *arg);
static void resizecelly(const Arg *arg);
static void selectcell(int activate);
static void selectmultiplecell(int activate);
static void selectsinglecell(int activate);
static void setup(void);
static void usage(void);
static void v_begin(const Arg *arg);
static void v_end(const Arg *arg);
static void writecells(void);
static void writesinglecell(int y, int x, int height, int width, const char *str, size_t strlen);
static void writetextbox(void);
int get_digit(int n);
int getstrlength(char *s);
int strcicmp(char const *a, char const *b);

#include "config.h"

/* Variables */
static char *sep = "";
Column cols[MAX_COLUMN];
static int dbgboxheight;
static int csvr_state;
extern int errno;
int height, width;
static int textboxheight;
Row rows[MAX_ROW];
SHEETPARAM st;
WINDOWPARAM win_param_st;
static Data d;

/* Function implementations */
#ifdef CSVR_DEBUG
void debugprint()
{
  werase(win_param_st.windows[DebugWindow]);
  mvwprintw(win_param_st.windows[DebugWindow], 0, 0, "%d", st.pad);
  wrefresh(win_param_st.windows[DebugWindow]);
  refresh();
}
#endif /* CSVR_DEBUG */

void calcdim()
{
  int h, w;
  getmaxyx(stdscr, h, w);
  height = h; width = w;

  st.cellwinheight = height - textboxheight - dbgboxheight;
  if (st.pivoty == Top) {
    st.lastRow = st.begRow + st.cellwinheight - 2;
  } else if (st.pivoty == Bottom) {
    st.begRow = st.lastRow - st.cellwinheight + 2;
  }

  int new_pad = get_digit(st.lastRow) + 1;
  if (win_param_st.windows[CellWindow] && st.pad - new_pad) {
    st.pad = new_pad;
    int new_width = width - st.pad;
    if (st.cellwinwidth - new_width) {
      recreate_windows();
    }
  } else {
    st.pad = new_pad;
  }
  st.cellwinwidth = width - st.pad;

  int j, colswidth;
  if (st.pivotx == Right) {
    for (j = st.lastCol, colswidth = 0;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j--)
    {
      st.xcov = cols[j - 1].width - (st.cellwinwidth - colswidth);
    }
    if (j < 0)
      st.pivotx = Left;
    else
      st.begCol = j + 1;
  }
  if (st.pivotx == Left) {
    for (j = st.begCol, colswidth = 0;
        colswidth < st.cellwinwidth;
        colswidth += cols[j - 1].width, j++)
    {
      st.xcov = cols[j - 1].width - (st.cellwinwidth - colswidth);
    }
    st.lastCol = j - 1;
  }
}

void cleanup(Data d)
{
  delwin(win_param_st.windows[CellWindow]);
  delwin(win_param_st.windows[HeaderWindow]);
  delwin(win_param_st.windows[TextWindow]);
#ifdef CSVR_DEBUG
  delwin(win_param_st.windows[DebugWindow]);
#endif /* CSVR_DEBUG */
  endwin();

  del_data(d);
}

void csvr_resize(const Arg *arg)
{
  st.cellwinupdate = 1;
  recreate_windows();
}

void printcolindex(int i, int b, int w)
{
  /* TODO: Reformat for more readability */
  /* Column index is limited to ZZ. 3 digit not possible */
  if (i > MAX_COLUMN - 1)
    return;

  if (i / 26) {
    if ((b + w/2 - 2) >= 0)
      mvwprintw(win_param_st.windows[HeaderWindow], 0, b + w/2 - 2, "%c", 64 + (i / 26));
  }
  if ((b + w/2 - 1) >= 0)
    mvwprintw(win_param_st.windows[HeaderWindow], 0, b + w/2 - 1, "%c", 65 + (i % 26));
}

/* void gotopopup(const Arg *arg) */
/* { */
/*   int lines = 12, cols = 18, y = height/2 - lines/2, x = width/2 - cols/2; */
/*   win_param_st.win_panels[GotoPanel] = newwin(lines, cols, y, x); */
/*   win_param_st.panels[GotoPanel] = new_panel(win_param_st.win_panels[GotoPanel]); */
/*   update_panels(); */
/*   doupdate(); */
/* } */

void handlesig(int signal)
{
  /* if (csvr_state == Command) { */
  /*   csvr_state = Normal; */
  /*   curs_set(0); */
  /* } */
}

void headerupdate()
{
  int i, j;

  /* TODO: Row height != 1 */
  for (i = 0; i <= st.lastRow - st.begRow; i++){
    wmove(win_param_st.windows[HeaderWindow], i + 1, 0);
    for (j = 0; j < st.pad - (1 + get_digit(st.begRow + i)); j++){
      wprintw(win_param_st.windows[HeaderWindow], " ");
    }
    wprintw(win_param_st.windows[HeaderWindow], "%d|", st.begRow + i);
  }

  wmove(win_param_st.windows[HeaderWindow], 0, 0);
  wclrtoeol(win_param_st.windows[HeaderWindow]);
  int begin;
  if (st.pivotx == Left) {
    begin = st.pad;
    for (j = st.begCol; j <= st.lastCol; j++) {
      printcolindex(j - 1, begin, cols[j - 1].width);
      mvwprintw(win_param_st.windows[HeaderWindow], 0, begin + cols[j - 1].width - 1, "|");
      begin += cols[j - 1].width;
    }
  } else if (st.pivotx == Right) {
    begin = width;
    for (j = st.lastCol; j >= st.begCol; j--) {
      begin -= cols[j - 1].width;
      printcolindex(j - 1, begin, cols[j - 1].width);
      mvwprintw(win_param_st.windows[HeaderWindow], 0, begin + cols[j - 1].width - 1, "|");
    }
  }

  wmove(win_param_st.windows[HeaderWindow], 0, 0);
  for (j = 0; j < st.pad - 1; j++) {
    wprintw(win_param_st.windows[HeaderWindow], " ");
  }
  wprintw(win_param_st.windows[HeaderWindow], "|");

  wrefresh(win_param_st.windows[HeaderWindow]);
}

void switchkeystate(int ch, int size, Key k[size])
{
  for (int i = 0; i < size; i++) {
    if (ch == k[i].ch) {
      csvr_state = k[i].nextst;
      if (k[i].func)
        k[i].func(&(k[i].arg));
    }
  }
}

void keypress(int ch)
{
  /* Master. supersedes all states */
  for (int i = 0; i < LENGTH(master); i++) {
    if (ch == master[i].ch) {
      if (master[i].nextst)
        csvr_state = master[i].nextst;
      master[i].func(&(master[i].arg));
      return;
    }
  }

  /* parse in respective key state array */
  switch (csvr_state) {
    case Normal:
      switchkeystate(ch, LENGTH(normalkey), normalkey);
      break;
    case Visual:
      switchkeystate(ch, LENGTH(visualkey), visualkey);
      break;
    default:
  }

}

void movecell(int y, int x)
{
  selectcell(0);

  st.currentRow += y;
  st.currentCol += x;

  if (st.currentRow <= 0) {
    st.currentRow = 1;
    st.begRow = 1;
    st.pivoty = Top;
    st.cellwinupdate = 1;
  } else if (st.currentRow <= st.begRow && y != 0) {
    st.begRow = st.currentRow;
    st.pivoty = Top;
    st.cellwinupdate = 1;
  } else if (st.currentRow > MAX_ROW) {
    st.currentRow = MAX_ROW;
    st.lastRow = MAX_ROW;
    st.pivoty = Bottom;
    st.cellwinupdate = 1;
  } else if (st.currentRow >= st.lastRow && y != 0) {
    st.lastRow = st.currentRow;
    st.pivoty = Bottom;
    st.cellwinupdate = 1;
  }

  if (st.currentCol <= 0) {
    st.currentCol = 1;
    st.lastCol = 1;
    st.pivotx = Left;
    st.cellwinupdate = 1;
  } else if (st.currentCol <= st.begCol && x != 0) {
    st.begCol = st.currentCol;
    st.pivotx = Left;
    st.cellwinupdate = 1;
  } else if (st.currentCol > MAX_COLUMN) {
    st.currentCol = MAX_COLUMN;
    st.lastCol = MAX_COLUMN;
    st.pivotx = Right;
    st.cellwinupdate = 1;
  } else if (st.currentCol >= st.lastCol && x != 0) {
    st.lastCol = st.currentCol;
    st.pivotx = Right;
    st.cellwinupdate = 1;
  }

  if (csvr_state == Visual) {
    st.vlastRow = st.currentRow;
    st.vlastCol = st.currentCol;
  }

  calcdim();
  selectcell(1);
}

void movex(const Arg *arg)
{
  movecell(0, arg->i);
}

void movey(const Arg *arg)
{
  movecell(arg->i, 0);
}

void recreate_windows(void)
{
  delwin(win_param_st.windows[CellWindow]);
  delwin(win_param_st.windows[HeaderWindow]);
  delwin(win_param_st.windows[TextWindow]);
#ifdef CSVR_DEBUG
  delwin(win_param_st.windows[DebugWindow]);
#endif /* CSVR_DEBUG */

  calcdim();

  win_param_st.windows[TextWindow] = newwin(textboxheight, width, 0, 0);
#ifdef CSVR_DEBUG
  win_param_st.windows[DebugWindow]  = newwin(dbgboxheight, width, height - 1, 0);
#endif /* CSVR_DEBUG */
  win_param_st.windows[HeaderWindow] = newwin(height - (textboxheight + dbgboxheight), width, textboxheight, 0);
  win_param_st.windows[CellWindow] = newwin(height - (textboxheight + dbgboxheight + 1), width - st.pad,
      textboxheight + 1, st.pad); /* 1 is the size of header */

  wbkgd(stdscr,  COLOR_PAIR(1));
  wbkgd(stdscr,  COLOR_PAIR(2));
  wbkgd(win_param_st.windows[TextWindow], COLOR_PAIR(3));
  wbkgd(win_param_st.windows[HeaderWindow], COLOR_PAIR(4));
#ifdef CSVR_DEBUG
  wbkgd(win_param_st.windows[DebugWindow], COLOR_PAIR(3));
#endif /* CSVR_DEBUG */

  writecells();
}

void resizecell(int y, int x)
{
  rows[st.currentRow - 1].height += y;
  cols[st.currentCol - 1].width += x;

  if (!y && !x) {
    if (text_is_not_empty(d))
      if (get_row(d) > st.currentRow && get_col(d) > st.currentCol) {
        size_t len = get_str_length(d, st.currentRow, st.currentCol);
        cols[st.currentCol - 1].width = len + 1;
      }
  }

  if (rows[st.currentRow - 1].height < 1)
    rows[st.currentRow - 1].height = 1;

  if (cols[st.currentCol - 1].width < 3)
    cols[st.currentCol - 1].width = 2;

  calcdim();
  st.cellwinupdate = 1;
  writecells();
}

void resizecellx(const Arg *arg)
{
  resizecell(0, arg->i);
}

void resizecelly(const Arg *arg)
{
  resizecell(arg->i, 0);
}

void selectcell(int activate)
{
  if (st.vbegRow + st.vbegCol)
    selectmultiplecell(activate);
  else
    selectsinglecell(activate);
}

void selectmultiplecell(int activate)
{
  int begin, end;
  int selectbegRow, selectbegCol;
  int selectlastRow, selectlastCol;

  if (st.vbegRow < st.begRow)
    selectbegRow = st.begRow;
  else if (st.vbegRow > st.lastRow)
    selectbegRow = st.lastRow;
  else
    selectbegRow = st.vbegRow;

  if (st.vbegCol < st.begCol)
    selectbegCol = st.begCol;
  else if (st.vbegCol > st.lastCol)
    selectbegCol = st.lastCol;
  else
    selectbegCol = st.vbegCol;

  if (st.vlastRow > st.lastRow)
    selectlastRow = st.lastRow;
  else if (st.vlastRow < st.begRow)
    selectlastRow = st.begRow;
  else
    selectlastRow = st.vlastRow;

  if (st.vlastCol > st.lastCol)
    selectlastCol = st.lastCol;
  else if (st.vlastCol < st.begCol)
    selectlastCol = st.begCol;
  else
    selectlastCol = st.vlastCol;

  /* selectsinglecell(activate); */

  int minRow, maxRow;
  if (selectlastRow < selectbegRow) {
    minRow = selectlastRow;
    maxRow = selectbegRow;
  } else {
    minRow = selectbegRow;
    maxRow = selectlastRow;
  }

  int minCol, maxCol;
  if (selectlastCol < selectbegCol) {
    minCol = selectlastCol;
    maxCol = selectbegCol;
  } else {
    minCol = selectbegCol;
    maxCol = selectlastCol;
  }

  if (st.pivotx == Left) {
    for (int i = minRow; i < maxRow + 1; i++) {
      begin = 0;
      end = 0;
      for (int j = st.begCol; j < maxCol + 1; j++) {
        end += cols[j - 1].width;
        if (j < minCol)
          begin += cols[j - 1].width;
      }
      mvwchgat(win_param_st.windows[CellWindow], i - st.begRow, begin,
          end - begin, A_NORMAL, activate + 1, NULL);
    }
  } else if (st.pivotx == Right) {
    for (int i = minRow; i < maxRow + 1; i++) {
      begin = st.cellwinwidth;
      end = st.cellwinwidth;
      for (int j = st.lastCol; j >= minCol; j--) {
        begin -= cols[j - 1].width;
        if (j > maxCol)
          end -= cols[j - 1].width;
      }
      if (begin < 0)
        begin = 0;
      mvwchgat(win_param_st.windows[CellWindow], i - st.begRow, begin,
          end - begin, A_NORMAL, activate + 1, NULL);
    }
  }
}

void selectsinglecell(int activate)
{
  int begin;
  /* int j; */
  /* TODO: Row height != 1 */
  if (st.pivotx == Left) {
    begin = 0;
    for (int j = st.begCol; j < st.currentCol; j++) {
      begin += cols[j - 1].width;
    }
    mvwchgat(win_param_st.windows[CellWindow], st.currentRow - st.begRow, begin,
        cols[st.currentCol - 1].width,
        A_NORMAL, activate + 1, NULL);
  }
  else if (st.pivotx == Right) {
    begin = st.cellwinwidth;
    for (int j = st.lastCol; j >= st.currentCol; j--) {
      begin -= cols[j - 1].width;
    }
    if (begin < 0)
      begin = 0;
    mvwchgat(win_param_st.windows[CellWindow], st.currentRow - st.begRow,
        begin, cols[st.currentCol - 1].width,
        A_NORMAL, activate + 1, NULL);
  }
  writetextbox();
}

void setup()
{
  signal(SIGINT, handlesig);
  initscr();
  textboxheight = 1;
#ifdef CSVR_DEBUG
  dbgboxheight = 1;
#else
  dbgboxheight = 0;
#endif /* CSVR_DEBUG */
  start_color();
  init_pair(1, FG_COLOR, BG_COLOR);
  init_pair(2, BG_COLOR, FG_COLOR);
  init_pair(3, FG_COLOR_STRL, BG_COLOR_STRL);
  init_pair(4, FG_COLOR_HEAD, BG_COLOR_HEAD);
  keypad(stdscr, TRUE);
  curs_set(0);
  noecho();
  halfdelay(255);

  st.cellwinupdate = 1;

  st.nrow = 1; st.ncol = 1;
  st.currentRow = 1; st.currentCol = 1;

  st.pivoty = Top; st.pivotx = Left;
  st.begRow = 1; st.begCol = 1;
  st.vbegRow = 0; st.vbegCol = 0;

  for (int i = 0; i < LENGTH(rows); i++) {
    rows[i].height = CELL_HEIGHT;
  }
  for (int i = 0; i < LENGTH(cols); i++) {
    cols[i].width = CELL_WIDTH;
  }

  recreate_windows();
}

void usage(void)
{
  fprintf(stdout, "Usage: csvr [OPTION] [FILE.csv]\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, "\t-t\tchoose separator\n");
  fprintf(stdout, "\t-h\tshow this help\n");
}

void v_begin(const Arg *arg)
{
  st.vbegRow = st.vlastRow = st.currentRow;
  st.vbegCol = st.vlastCol = st.currentCol;
}

void v_end(const Arg *arg)
{
  selectmultiplecell(0);
  st.vbegRow = st.vlastRow = 0;
  st.vbegCol = st.vlastCol = 0;
  selectcell(1);
}

void writecells()
{
  const char *str;
  size_t strlen;

  if (text_is_not_empty(d) == 0) {
    selectcell(1);
    return;
  }

  if (st.cellwinupdate) {
    werase(win_param_st.windows[CellWindow]);
    for (int i = 0, yt = 0; i < st.lastRow - st.begRow + 1 && i < get_row(d) - st.begRow + 1; i++) {
      int cellheight = rows[st.begCol + i - 1].height;
      for (int j = 0, xt = 0; j < st.lastCol - st.begCol + 1 && j < get_col(d) - st.begCol + 1; j++) {
        int cellwidth;
        int startch = 0;
        if (j == st.lastCol - st.begCol && st.pivotx == Left) {
          cellwidth = cols[st.begCol + j - 1].width - st.xcov;
        } else if (j == 0 && st.pivotx == Right) {
          cellwidth = cols[st.begCol + j - 1].width - st.xcov;
          startch = st.xcov;
        } else {
          cellwidth = cols[st.begCol + j - 1].width;
        }

        if (st.begRow + i <= get_row(d) && st.begCol + j <= get_col(d)) {
          str = get_str(d, st.begRow + i, st.begCol + j);
          strlen = get_str_length(d, st.begRow + i, st.begCol + j);
        }
        else {
          str = "";
          strlen = 0;
        }

        writesinglecell(yt, xt, cellheight, cellwidth, str + startch, strlen);
        xt += cellwidth;
      }
      yt += cellheight;

    }
    st.cellwinupdate = 0;
    selectcell(1);
  }
}

void writesinglecell(int y, int x, int height, int width, const char *str, size_t strlen)
{
  /* TODO: Wrap text when have row height > 1 */
  char temp[strlen];
  snprintf(temp, width, str);
  mvwprintw(win_param_st.windows[CellWindow], y, x, "%s", temp);
}

void writetextbox(void)
{
  wmove(win_param_st.windows[TextWindow], 0, 0); wclrtoeol(win_param_st.windows[TextWindow]);

  if (text_is_not_empty(d) == 0)
    return;

  if (get_row(d) < st.currentRow || get_col(d) < st.currentCol)
    return;

  for (int i = 0; i < st.pad; i++)
    wprintw(win_param_st.windows[TextWindow], " ");
  wprintw(win_param_st.windows[TextWindow], "%s", get_str(d, st.currentRow, st.currentCol));
}

int main(int argc, char **argv)
{
  FILE *csv_file;
  int chopt;

  opterr = 0;
  while ((chopt = getopt(argc, argv, "t:")) != -1)
    switch (chopt) {
      case 't':
        sep = optarg;
        break;
      case '?':
        if (optopt == 't')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
              "Unknown option character `\\x%x'.\n",
              optopt);
        usage();
        exit(1);
      default:
        exit(0);
    }

  if ((argc - optind) > 0) {
    if (!sep) {
      printf("Warning. The separator is not set.\n");
      printf("Please press ENTER to continue\n");
      int ch;
      while ((ch = fgetc(stdin)) != 10) {}
    }
    csv_file = fopen(*(argv + optind), "r");
    if (!csv_file) {
      fprintf(stderr, "Error. %s: \'%s\'\n", strerror(errno), *(argv + 1));
      exit(1);
    }

    d = parse_csv(csv_file, sep);
    fclose(csv_file);
  }

  unsigned int c;
  setup();
  csvr_state = Normal;

  while (csvr_state != Quit) {
    writecells();
    refresh();
    headerupdate();
#ifdef CSVR_DEBUG
    wrefresh(win_param_st.windows[DebugWindow]);
#endif /* CSVR_DEBUG */
    wrefresh(win_param_st.windows[CellWindow]);
    wrefresh(win_param_st.windows[TextWindow]);

    c = getch();
    if (c != ERR)
      keypress(c);
  }

  cleanup(d);
  return 0;
}
