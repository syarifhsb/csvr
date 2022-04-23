#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "string_st.h"

/* Macros */
#define LENGTH(X)         (sizeof X / sizeof X[0])
#define CTRL(X)           ((X) & 0x1f)
#define MAX_STRING_LENGTH 1024

/* Enums and Structs */
enum { Top, Bottom };  /* Pivot Y */
enum { Left, Right };  /* Pivot X */
enum { NoChange = 0, Normal, NormalCell, Insert, Command, Quit };  /* Program Mode */

struct sheetparam {
  int cellwinheight;
  int cellwinwidth;
  int cellwinupdate;
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
static void cleanup(void);
static void cmddel(const Arg *arg);
static void cmddo(const Arg *arg);
static void cmdput(int ch);
static void cmdstart(const Arg *arg);
static void csvr_resize(const Arg *arg);
static void handlesig(int signal);
static void headerupdate(void);
static void keypress(int ch);
static void movecell(int y, int x);
static void movex(const Arg *arg);
static void movey(const Arg *arg);
static void repaint(void);
static void resizecell(int y, int x);
static void resizecellx(const Arg *arg);
static void selectcell(int activate);
static void setup(void);
static void usage(void);
static void writecells(void);
static void writesinglecell(int y, int x, int height, int width, const char *str, size_t strlen);
static void writetextbox(void);
int get_digit(int n);
int getstrlength(char *s);
int strcicmp(char const *a, char const *b);

TABLE_ST* parse_csv(FILE *csv_file, char sep);

#include "config.h"

/* Variables */
static char cmdstr[CMD_MAX_CHAR];
static char sep = '\0';
static Column cols[MAX_COLUMN];
static int cmdboxheight;
static int cmdstrlen = 0;
static int csvr_state;
extern int errno;
static int height, width;
static int textboxheight;
static Row rows[MAX_ROW];
static struct sheetparam st;
static TABLE_ST *t = NULL;
static WINDOW *headwin, *cellwin, *strlwin, *cmdwin;

/* Function implementations */
void calcdim()
{
  int h, w;
  getmaxyx(stdscr, h, w);
  height = h; width = w;

  st.cellwinheight = height - textboxheight - cmdboxheight;
  if (st.pivoty == Top) {
    st.lastRow = (st.cellwinheight - 1) + (st.begRow - 1);
  } else if (st.pivoty == Bottom) {
    st.begRow = (st.lastRow + 1) - (st.cellwinheight - 1);
  }

  if (cellwin && st.pad - (get_digit(st.lastRow) + 1)) {
    st.pad = get_digit(st.lastRow) + 1;
    int widthtemp = width - st.pad;
    if (st.cellwinwidth - widthtemp) {
      repaint();
      st.cellwinwidth = widthtemp;
    }
  } else {
    st.pad = get_digit(st.lastRow) + 1;
    st.cellwinwidth = width - st.pad;
  }

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

void cleanup(void)
{
  delwin(cellwin);
  delwin(headwin);
  delwin(strlwin);
  delwin(cmdwin);
	endwin();

  if (t)
    del_table(t);
}

void cmddel(const Arg *arg)
{
  if (cmdstrlen <= 0) {
    cmdstrlen = 0;
    cmdstr[0] = '\0';
  } else {
    cmdstrlen--;
    mvwprintw(cmdwin, 0, cmdstrlen + 1, " ");
    wmove(cmdwin, 0, cmdstrlen + 1);
  }
}

void cmddo(const Arg *arg)
{
  /* Do Command */
  if (!strcicmp("q", cmdstr))
    csvr_state = Quit;
  curs_set(0);
}

void cmdput(int ch)
{
  if (cmdstrlen >= CMD_MAX_CHAR - 1) {
    cmdstrlen = CMD_MAX_CHAR - 1;
    cmdstr[CMD_MAX_CHAR - 1] = '\0';
  } else {
    cmdstrlen++;
    waddch(cmdwin, ch);
    cmdstr[cmdstrlen - 1] = ch;
    cmdstr[cmdstrlen] = '\0';
  }
}

void cmdstart(const Arg *arg)
{
  werase(cmdwin);
  curs_set(1);
  mvwprintw(cmdwin, 0, 0, ":");
  cmdstr[0] = '\0';
  cmdstrlen = 0;
}

void csvr_resize(const Arg *arg)
{
  repaint();
}

void printcolindex(int i, int b, int w)
{
  /* TODO: Reformat for more readability */
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

void handlesig(int signal)
{
  if (csvr_state == Command) {
    csvr_state = Normal;
    curs_set(0);
  }
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

void keypress(int ch)
{
  /* parse in master */
  for (int i = 0; i < LENGTH(master); i++) {
    if (ch == master[i].ch) {
      if (master[i].nextst)
        csvr_state = master[i].nextst;
      master[i].func(&(master[i].arg));
      return;
    }
  }

  /* parse in respective array */
  switch (csvr_state) {
    case Normal:
      for (int i = 0; i < LENGTH(normalkey); i++) {
        if (ch == normalkey[i].ch) {
          if (normalkey[i].nextst)
            csvr_state = normalkey[i].nextst;
          normalkey[i].func(&(normalkey[i].arg));
        }
      }
      break;
    case Command:
      int found = 0;
      for (int i = 0; i < LENGTH(commandkey); i++) {
        if (ch == commandkey[i].ch) {
          found = 1;
          if (commandkey[i].nextst)
            csvr_state = commandkey[i].nextst;
          commandkey[i].func(&(commandkey[i].arg));
          break;
        }
      }
      if (!found)
        cmdput(ch);
      break;
    default:
  }

}

void movecell(int y, int x)
{
  selectcell(0);

  st.activeRow += y;
  st.activeCol += x;

  if (st.activeRow <= 0) {
    st.activeRow = 1;
    st.begRow = 1;
    st.pivoty = Top;
  } else if (st.activeRow <= st.begRow && y != 0) {
    st.begRow = st.activeRow;
    st.pivoty = Top;
    st.cellwinupdate = 1;
  } else if (st.activeRow > MAX_ROW) {
    st.activeRow = MAX_ROW;
    st.lastRow = MAX_ROW;
    st.pivoty = Bottom;
  } else if (st.activeRow >= st.lastRow && y != 0) {
    st.lastRow = st.activeRow;
    st.pivoty = Bottom;
    st.cellwinupdate = 1;
  }

  if (st.activeCol <= 0) {
    st.activeCol = 1;
    st.lastCol = 1;
    st.pivotx = Left;
  } else if (st.activeCol <= st.begCol && x != 0) {
    st.begCol = st.activeCol;
    st.pivotx = Left;
    st.cellwinupdate = 1;
  } else if (st.activeCol > MAX_COLUMN) {
    st.activeCol = MAX_COLUMN;
    st.lastCol = MAX_COLUMN;
    st.pivotx = Right;
  } else if (st.activeCol >= st.lastCol && x != 0) {
    st.lastCol = st.activeCol;
    st.pivotx = Right;
    st.cellwinupdate = 1;
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

void repaint(void)
{
  delwin(cellwin);
  delwin(headwin);
  delwin(strlwin);
  delwin(cmdwin);

  calcdim();

  strlwin = newwin(textboxheight, width, 0, 0);
  cmdwin  = newwin(cmdboxheight, width, height - 1, 0);
  headwin = newwin(height - 2, width, textboxheight, 0);
  cellwin = newwin(height - (textboxheight + cmdboxheight + 1), width - st.pad, 
      textboxheight + 1, st.pad); /* 1 is the size of header */

	wbkgd(stdscr, COLOR_PAIR(1));
	wbkgd(stdscr, COLOR_PAIR(2));
	wbkgd(strlwin, COLOR_PAIR(3));
	wbkgd(headwin, COLOR_PAIR(4));
	wbkgd(cmdwin, COLOR_PAIR(3));

  writecells();
}

void resizecell(int y, int x)
{
  rows[st.activeRow - 1].height += y;
  cols[st.activeCol - 1].width += x;

  if (!y && !x) {
    if (t)
      if (t_get_len(t) > st.activeRow && v_get_len(t_get_vector(t, 0)) > st.activeCol) {
        int len = s_get_len(v_get_str(t_get_vector(t, st.activeRow - 1), st.activeCol - 1));
        cols[st.activeCol - 1].width = len + 1;
      }
  }

  if (rows[st.activeRow - 1].height < 1)
    rows[st.activeRow - 1].height = 1;

  if (cols[st.activeCol - 1].width < 3)
    cols[st.activeCol - 1].width = 2;

  calcdim();
}

void resizecellx(const Arg *arg)
{
  resizecell(0, arg->i);
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
        begin, cols[st.activeCol - 1].width,
        A_NORMAL, activate + 1, NULL);
  }
  else if (st.pivotx == Right) {
    begin = st.cellwinwidth;
    for (j = st.lastCol; j >= st.activeCol; j--) {
      begin -= cols[j - 1].width;
    }
    mvwchgat(cellwin, st.activeRow - st.begRow, 
        begin, cols[st.activeCol - 1].width,
        A_NORMAL, activate + 1, NULL);
  }
  writetextbox();
}

void setup()
{
  signal(SIGINT, handlesig);
  initscr();
  textboxheight = 1;
  cmdboxheight = 1;
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

  repaint();
}

void usage(void)
{
  fprintf(stdout, "Usage: csvr [OPTION] [FILE.csv]\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, "\t-t\tchoose separator\n");
  fprintf(stdout, "\t-h\tshow this help\n");
}

void writecells()
{
  const char *str;
  size_t strlen;

  if (!t) {
    selectcell(1);
    return;
  }

  if (st.cellwinupdate) {
    werase(cellwin);
    for (int i = 0, yt = 0; i < st.lastRow - st.begRow + 1 && i < t_get_len(t) - st.begRow + 1; i++) {
      int cellheight = rows[st.begCol + i - 1].height;
      for (int j = 0, xt = 0; j < st.lastCol - st.begCol + 1 && j < v_get_len(t_get_vector(t, i)) - st.begCol + 1; j++) {
        int cellwidth;
        if ((j == st.lastCol - st.begCol && st.pivotx == Left) || (j == 0 && st.pivotx == Right))
          cellwidth = cols[st.begCol + j - 1].width - st.xcov;
        else
          cellwidth = cols[st.begCol + j - 1].width;

        if (st.begRow + i - 1 < t_get_len(t) && st.begCol + j - 1 < v_get_len(t_get_vector(t, 0))) {
          str = v_get_str_l(t_get_vector(t, st.begRow + i - 1), st.begCol + j - 1);
          strlen = s_get_mlen(v_get_str(t_get_vector(t, st.begRow + i - 1), st.begCol + j - 1));
        }
        else {
          str = "";
          strlen = MAX_STRING_LENGTH;
        }

        writesinglecell(yt, xt, cellheight, cellwidth, str, strlen);
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
  mvwprintw(cellwin, y, x, "%s", temp);
}

void writetextbox(void)
{
  wmove(strlwin, 0, 0); wclrtoeol(strlwin);

  if (!t)
    return;

  if (t_get_len(t) < st.activeRow || v_get_len(t_get_vector(t, 0)) < st.activeCol)
    return;

  for (int i = 0; i < st.pad; i++)
    wprintw(strlwin, " ");
  wprintw(strlwin, "%s", v_get_str_l(t_get_vector(t, st.activeRow - 1), st.activeCol - 1));
}

int main(int argc, char **argv)
{
  FILE *csv_file;
  int chopt;

  opterr = 0;
  while ((chopt = getopt(argc, argv, "t:")) != -1)
    switch (chopt) {
      case 't':
        if (optarg[1] != '\0') {
          fprintf(stderr, "Multi-character separator currently not supported\n");
          exit(1);
        }
        sep = optarg[0];
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

    t = parse_csv(csv_file, sep);
    fclose(csv_file);
  }

  unsigned int c;
  setup();
  csvr_state = Normal;

  while (csvr_state != Quit) {
    writecells();
    refresh();
    headerupdate();
    wrefresh(cmdwin);
    wrefresh(cellwin);
    wrefresh(strlwin);

    c = getch();
    if (c != ERR)
      keypress(c);
  }

  cleanup();
  return 0;
}
