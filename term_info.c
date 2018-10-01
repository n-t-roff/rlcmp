#include <term.h>
#include <unistd.h>
#include "compat.h"
#include "term_info.h"

int ti_err_ret;

int term_info_init(void) {
    if (setupterm(NULL, STDOUT_FILENO, &ti_err_ret) == ERR)
        return -1;
    return 0;
}

int ti_clr_eol(void) {
    if (putp(clr_eol) == ERR)
        return -1;
    return 0;
}

int ti_get_cols(void) {
    return tigetnum("cols");
}
