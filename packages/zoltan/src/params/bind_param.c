/*****************************************************************************
 * Zoltan Dynamic Load-Balancing Library for Parallel Applications           *
 * Copyright (c) 2000, Sandia National Laboratories.                         *
 * For more info, see the README file in the top-level Zoltan directory.     *  
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#ifdef __STDC__
#include <string.h>
#else
#include <strings.h>
#endif  /* __STDC__ */
#include "lb_const.h"
#include "lb_util_const.h"
#include "params_const.h"

int       LB_Bind_Param(
PARAM_VARS *params,		/* parameter structure */
char *name,			/* parameter name */
void *var)			/* pointer to variable to be associated with the parameter name */
{
/*
 *  Function to bind a parameter name to a variable.
 *  On output:
 *    ZOLTAN_OK indicates success.
 *    ZOLTAN_WARN indicates that parameter name was not found (misspelled?).
 *            No binding took place. A warning message is printed in this case.
 *    ZOLTAN_FATAL signals something more serious.
 */

    char     *yo = "LB_Bind_Param";
    char      msg[256];
    char     *name2;		/* clean version of name */
    int       flag;		/* return value from function */
    PARAM_VARS *ptr;		/* pointer to a parameter */

    /* First convert to upper case & remove leading white space. */
    flag = LB_clean_string(name, &name2);
    if (flag)
	return (flag);

    /* Search through parameter array to find name2 */

    for (ptr = params; ptr->name != NULL; ptr++) {
	if (!strcmp(name2, ptr->name)) {	/* string match */
	    ptr->ptr = var;
            LB_FREE(&name2);
	    return (ZOLTAN_OK);
	}
    }

    /* If we reach this point, the parameter name must be invalid */
    sprintf(msg, "Parameter name %s not found; it will"
                 "not be bound to any variable.", name2);
    ZOLTAN_PRINT_WARN(-1, yo, msg);
    LB_FREE(&name2);
    return (ZOLTAN_WARN);
}
