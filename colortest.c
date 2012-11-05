/**
 * colortest.c
 */

#include <stdio.h>

// 前景
#define FBK "\x1b[30m" // black
#define FRD "\x1b[31m" // red
#define FGR "\x1b[32m" // green
#define FYL "\x1b[33m" // yellow
#define FBL "\x1b[34m" // blue
#define FMG "\x1b[35m" // magenta
#define FCY "\x1b[36m" // cyan
#define FWT "\x1b[37m" // white
#define FDF "\x1b[39m" // default

// 背景
#define BBK "\x1b[40m"
#define BRD "\x1b[41m"
#define BGR "\x1b[42m"
#define BYL "\x1b[43m"
#define BBL "\x1b[44m"
#define BMG "\x1b[45m"
#define BCY "\x1b[46m"
#define BWT "\x1b[47m"
#define BDF "\x1b[49m"

// その他
#define DF "\x1b[0m" // default
#define BD "\x1b[1m" // bold
#define LI "\x1b[2m" // low intensity
#define UL "\x1b[4m" // underline
#define BL "\x1b[5m" // blink
#define RV "\x1b[7m" // reverse video
#define IT "\x1b[9m" // invisible text

int main() {
    // black
    printf("%sab%scd%sef%s%s%s\n", BBK, FBK, BD, DF, FDF, BDF);
    // red
    printf("%sab%scd%sef%s%s%s\n", BRD, FRD, BD, DF, FDF, BDF);
    // green
    printf("%sab%scd%sef%s%s%s\n", BGR, FGR, BD, DF, FDF, BDF);
    // yellow
    printf("%sab%scd%sef%s%s%s\n", BYL, FYL, BD, DF, FDF, BDF);
    // blue
    printf("%sab%scd%sef%s%s%s\n", BBL, FBL, BD, DF, FDF, BDF);
    // magenta
    printf("%sab%scd%sef%s%s%s\n", BMG, FMG, BD, DF, FDF, BDF);
    // cyan
    printf("%sab%scd%sef%s%s%s\n", BCY, FCY, BD, DF, FDF, BDF);
    // white
    printf("%sab%scd%sef%s%s%s\n", BWT, FWT, BD, DF, FDF, BDF);

    return 0;
}

