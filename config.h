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
  /* { ':',         Command,   cmdstart,     {0}        }, */
  { '=',         Normal,    resizecellx,  {.i =  0}  },
  { '+',         Normal,    resizecellx,  {.i = +1}  },
  { '-',         Normal,    resizecellx,  {.i = -1}  },
  { 'h',         Normal,    movex,        {.i = -1}  },
  { 'j',         Normal,    movey,        {.i = +1}  },
  { 'k',         Normal,    movey,        {.i = -1}  },
  { 'l',         Normal,    movex,        {.i = +1}  },
  { KEY_LEFT,    Normal,    movex,        {.i = -1}  },
  { KEY_DOWN,    Normal,    movey,        {.i = +1}  },
  { KEY_UP,      Normal,    movey,        {.i = -1}  },
  { KEY_RIGHT,   Normal,    movex,        {.i = +1}  },
  { 'B',         Normal,    movex,        {.i = -5}  },
  { 'W',         Normal,    movex,        {.i = +5}  },
  { 'Q',         Quit,      NULL,         {0}        },
  { 'v',         Visual,    v_begin,      {0}        },
  { CTRL('D'),   Normal,    movey,        {.i = +10} },
  { CTRL('U'),   Normal,    movey,        {.i = -10} },
  { KEY_NPAGE,   Normal,    movey,        {.i = +10} },
  { KEY_PPAGE,   Normal,    movey,        {.i = -10} },
};

static Key visualkey[] = {
  /* ch          NextState  Function  Arg*/
  { 27,          Normal,    v_end,        {0}        },
  { 'q',         Normal,    v_end,        {0}        },
  { 'h',         Visual,    movex,        {.i = -1}  },
  { 'j',         Visual,    movey,        {.i = +1}  },
  { 'k',         Visual,    movey,        {.i = -1}  },
  { 'l',         Visual,    movex,        {.i = +1}  },
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
