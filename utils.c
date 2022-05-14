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

#include <ctype.h>

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

int strcicmp(char const *a, char const *b)
{
  for (;; a++, b++) {
    int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
    if (d != 0 || !*a)
      return d;
  }
}

int getstrlength(char *s)
{
  int count = 0;

  while (s[count] != '\0')
    count++;

  return count;
}

