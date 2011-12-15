#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>

char buf[1024];

__attribute((noreturn)) void segv(void){
    void *tracePtrs[10];
    int i;

    int count = backtrace(tracePtrs, 10);

    char** funcNames = backtrace_symbols(tracePtrs, count);

    printf("BACKTRACE<br>\n");

    for (i = 0; i < count; i++){
        printf("%s ", funcNames[i]);
        sprintf(buf, "addr2line -e app.cgi %p -s", tracePtrs[i]);
        system(buf);
        printf("<br><b>");
        sprintf(buf, "awk \"NR==$(addr2line -e app.cgi %p -s | cut -d : -f 2)\" $(addr2line -e app.cgi %p -s | cut -d : -f 1)", tracePtrs[i], tracePtrs[i]);
        system(buf);
        printf("</b><br>\n");
    }

    free(funcNames);
    exit(0);
}
