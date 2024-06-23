#ifndef FUNCTION_H
#define FUNCTION_H

/* Sometimes we want to keep parameters to a function for later use
 * and this is a solution to avoid the 'unused parameter' compiler warning */
#define UNUSED(x) (void)(x)

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a < b ? b : a)

#endif
