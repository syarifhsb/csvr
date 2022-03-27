#ifndef CONFIG_H
#define CONFIG_H

#define BG_COLOR      COLOR_BLACK
#define FG_COLOR      COLOR_WHITE
#define BG_COLOR_STRL COLOR_BLUE
#define FG_COLOR_STRL COLOR_BLACK
#define BG_COLOR_HEAD COLOR_GREEN
#define FG_COLOR_HEAD COLOR_BLACK
#define CELL_WIDTH    20
#define CELL_HEIGHT   1
#define MAX_COLUMN    702  /* Limited to ZZ index */
#define MAX_ROW       65535 
#define CMD_MAX_CHAR  10

static Key master[] = {
  /* ch          NextState  Function      Arg*/
  { KEY_RESIZE,  0,         csvr_resize,  {0} },
};

static Key normalkey[] = {
  /* ch          NextState  Function      Arg*/
  { ':',         Command,   cmdstart,     {0}        },
  { '=',         Normal,    resizecellx,  {.i =  0}  },
  { '+',         Normal,    resizecellx,  {.i = +1}  },
  { '-',         Normal,    resizecellx,  {.i = -1}  },
  { 'h',         Normal,    movex,        {.i = -1}  },
  { 'j',         Normal,    movey,        {.i = +1}  },
  { 'k',         Normal,    movey,        {.i = -1}  },
  { 'l',         Normal,    movex,        {.i = +1}  },
  { 'B',         Normal,    movex,        {.i = -5}  },
  { 'W',         Normal,    movex,        {.i = +5}  },
  { CTRL('D'),   Normal,    movey,        {.i = +10} },
  { CTRL('U'),   Normal,    movey,        {.i = -10} },
};

static Key commandkey[] = {
  /* ch             NextState  Function  Arg*/
  { KEY_ENTER,      Normal,    cmddo,    {0} },
  { 10,             Normal,    cmddo,    {0} },
  { KEY_BACKSPACE,  Command,   cmddel,   {0} },
  { CTRL('?'),      Command,   cmddel,   {0} },
  { 127,            Command,   cmddel,   {0} },
  { 27,             Command,   cmddel,   {0} },
};

#endif /* CONFIG_H */
