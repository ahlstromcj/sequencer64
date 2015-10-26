/*
 * A small program to dump the extended ASCII character set to the screen, with
 * separators suitable for import into a spreadsheet.
 *
 * Build:         gcc ascii-matrix.c -o ascii-matrix
 * Use:           ./ascii-matrix > ascii-matrix.txt
 *
 * Unfortunately, the full set is visible only in a console that uses
 * a primitive X font.  For importing into Sequencer64, we need to
 * individually add the characters above 0x7f.  And we still can't show
 * the sub-0x1f characters.
 */

#include <ctype.h>
#include <stdio.h>

int
main (int argc, char * argv [])
{
   int ch = 0;
   int row, column;
   for (row = 0; row < 16; row++)
   {
      for (column = 0; column < 16; column++)
      {
         // char output = (char)(isprint(ch) ? ch : '|');
         // char output = (char) ch;
         char output = (char)(ch > 0x1f ? ch : '_');
         printf("%c", output);
         if (column < 15)
            printf(" ");

         ch++;
      }
      printf("\n");
   }
}

