#include <stdio.h>
#include <string.h>

char * basename (const char *filename) {
	const char *p = filename + strlen (filename);
	while (p != filename) {
		p--;
		if (*p == '/' || *p == '\\')
			return ((char *) p + 1);
	}
	return ((char *) filename);
}

char *dirname(const char *path) {
    static char bname[1024];
    register const char *endp;

    /* Empty or NULL string gets treated as "." */
    if (path == NULL || *path == '\0') {
        (void)strncpy(bname, ".", sizeof bname);
        return(bname);
    }

    /* Strip trailing slashes */
    endp = path + strlen(path) - 1;
    while (endp > path && *endp == '\\')
        endp--;

        /* Find the start of the dir */
        while (endp > path && *endp != '\\')
        	endp--;

        /* Either the dir is "/" or there are no slashes */
        if (endp == path) {
            (void)strncpy(bname, *endp == '\\' ? "\\": ".", sizeof bname);
            return(bname);
        } else {
            do {
            	endp--;
            } while (endp > path && *endp == '\\');
        }

        if (endp - path + 2 > sizeof(bname)) {
            return(NULL);
        }
        strncpy(bname, path, endp - path + 2);
        return(bname);
}

int	opterr = 1, /* if error message should be printed */
	optind = 1,	/* index into parent argv vector */
	optopt,		/* character checked for validity */
	optreset;	/* reset getopt */
char *optarg;	/* argument associated with option */

#define	BADCH	(int)'?'
#define	BADARG	(int)':'
#define	EMSG	""

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int getopt(nargc, nargv, ostr)
	int nargc;
	char * const *nargv;
	const char *ostr;
{
	#define __progname nargv[0]
	static char *place = EMSG;	/* option letter processing */
	char *oli;				    /* option letter list index */

	if (optreset || !*place) {	/* update scanning pointer */
		optreset = 0;
		if (optind >= nargc || *(place = nargv[optind]) != '-') {
			place = EMSG;
			return (-1);
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++optind;
			place = EMSG;
			return (-1);
		}
	}	/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' ||
	    !(oli = strchr(ostr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means -1.
		 */
		if (optopt == (int)'-')
			return (-1);
		if (!*place)
			++optind;
		if (opterr && *ostr != ':')
			(void)printf("%s: illegal option -- %c\n", __progname, optopt);
		return (BADCH);
	}
	if (*++oli != ':') {	/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else {			/* need an argument */
		if (*place)	/* no white space */
			optarg = place;
		else if (nargc <= ++optind) {	/* no arg */
			place = EMSG;
			if (*ostr == ':')
				return (BADARG);
			if (opterr)
				(void)printf(
				    "%s: option requires an argument -- %c\n",
				    __progname, optopt);
			return (BADCH);
		} else	/* white space */
			optarg = nargv[optind];
		place = EMSG;
		++optind;
	}
	return (optopt); /* dump back option letter */
}

