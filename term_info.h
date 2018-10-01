#ifndef TERM_INFO_H
#define TERM_INFO_H

extern int ti_err_ret;

int term_info_init(void);
int ti_clr_eol(void);
/* Return value:
 *   -1: "cols" not in terminal description
 *   -2: "cols" is not a numeric capability
 *   else: Number of columns */
int ti_get_cols(void);

#endif
