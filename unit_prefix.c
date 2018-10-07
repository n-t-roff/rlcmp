#include <stdlib.h> /* abs() */
#include "unit_prefix.h"

static int unit_prefix(char *const buf, const size_t bufsiz,
                       FILE *file_ptr, const intmax_t value,
                       const unsigned mode);

const struct unit_prefix UnitPrefix = {
    .dont_scale = 1,
    .dont_group = 2,
    .decimal    = 4,
    .space      = 8,
    .unit_prefix = unit_prefix
};

static int unit_prefix(char *const buf, const size_t bufsiz,
                       FILE *file_ptr, const intmax_t value,
                       const unsigned mode)
{
    const char *unit;
    const char *const space = (mode & UnitPrefix.space) ? " " : "";
    float f;
    if (!file_ptr)
        file_ptr = stdout;
    const float pw = UnitPrefix.decimal ? 1000 : 1024;
    const float thr = 999.9;

    if (mode == UnitPrefix.dont_scale) {
        if (buf)
            return snprintf(buf, bufsiz, "%'jd", value);
        else
            return fprintf(file_ptr, "%'jd", value);

    } else if ((mode & UnitPrefix.dont_group) || abs(value) < 1024) {
        if (buf)
            return snprintf(buf, bufsiz, "%jd", value);
        else
            return fprintf(file_ptr, "%jd", value);

    } else {
        f = value / pw;
        unit = "K";

        if (f < -thr || f > thr) {
            f /= pw;
            unit = "M";
        }
        if (f < -thr || f > thr) {
            f /= pw;
            unit = "G";
        }
        if (f < -thr || f > thr) {
            f /= pw;
            unit = "T";
        }
    }
    if (f > -10 && f < 10) {
        if (buf)
            return snprintf(buf, bufsiz, "%.1f%s%s", f, space, unit);
        else
            return fprintf(file_ptr, "%.1f%s%s", f, space, unit);
    } else {
        if (f < 0)
            f -= .5;
        else
            f += .5;
        if (buf)
            return snprintf(buf, bufsiz, "%.0f%s%s", f+.5, space, unit);
        else
            return fprintf(file_ptr, "%.0f%s%s", f+.5, space, unit);
    }
}
