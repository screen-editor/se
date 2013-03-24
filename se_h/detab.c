/* Detab - convert tabs to appropriate number of spaces */

/* transcribed from Kernighan and Plaguer (Software Tools) */
/* fixed up by Arnold Robbins */

#include <stdio.h>

#define MAXLINE         132

#define repeat          do
#define until(x)        while(!(x))

#define tabpos(col, tabs)       ( (col > MAXLINE) ? 1 : tabs[col - 1])

main()
{
        int c, i, tabs[MAXLINE], col = 1;

        settabs(tabs);

        while ((c = getchar()) != EOF)
                switch(c) {
                case '\t':
                        repeat
                        {
                                putchar(' ');
                                col++;
                        } until(tabpos(col, tabs));
                        break;

                case '\n':
                        putchar('\n');
                        col = 1;
                        break;

                default:
                        putchar(c);
                        col++;
                        break;
                }
}

settabs(tabs)
int tabs[];
{
        int i;

        for(i = 1; i <= MAXLINE; i++)
                tabs[i - 1] = ((i % 8) == 1);
                /* result is either 1 or 0 */
}
