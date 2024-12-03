#ifndef ERROR_HANDLING_UTILS
#define ERROR_HANDLING_UTILS
#include <stdlib.h>
#include <stdio.h>

#define ERR_PRINT_EXIT(cond, msg) 											   \
	if ((cond)) { 															   \
		fprintf(stderr, msg); 												   \
		exit(EXIT_FAILURE); 												   \
	}

#define ERR_RET(cond, retval) 												   \
	if ((cond)) return retval;


#define ERR_FREE_EXIT(cond, ptr, msg)                                          \
    if ((cond)) {                                                              \
        perror((msg));                                                         \
        free((ptr));                                                           \
        exit(EXIT_FAILURE);                                                    \
    }

#define ERR_PERROR_EXIT(cond, msg)                                             \
    if ((cond)) {                                                              \
        perror((msg));                                                         \
        exit(EXIT_FAILURE);                                                    \
    }

#endif
