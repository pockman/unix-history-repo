/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)tunefs.c	8.3 (Berkeley) %G%";
#endif /* not lint */

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/param.h>
#include <sys/stat.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>

/* the optimization warning string template */
#define	OPTWARN	"should optimize for %s with minfree %s %d%%"

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi;
long dev_bsize = 1;

void bwrite(daddr_t, char *, int);
int bread(daddr_t, char *, int);
void getsb(struct fs *, char *);
void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp, *special, *name;
	struct stat st;
	int i;
	int Aflag = 0, Nflag = 0;
	struct fstab *fs;
	char *chg[2], device[MAXPATHLEN];

	argc--, argv++; 
	if (argc < 2)
		usage();
	special = argv[argc - 1];
	fs = getfsfile(special);
	if (fs)
		special = fs->fs_spec;
again:
	if (stat(special, &st) < 0) {
		if (*special != '/') {
			if (*special == 'r')
				special++;
			(void)sprintf(device, "%s/%s", _PATH_DEV, special);
			special = device;
			goto again;
		}
		err(1, "%s", special);
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK &&
	    (st.st_mode & S_IFMT) != S_IFCHR)
		errx(10, "%s: not a block or character device", special);
	getsb(&sblock, special);
	chg[FS_OPTSPACE] = "space";
	chg[FS_OPTTIME] = "time";
	for (; argc > 0 && argv[0][0] == '-'; argc--, argv++) {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {

			case 'A':
				Aflag++;
				continue;

			case 'N':
				Nflag++;
				continue;

			case 'a':
				name = "maximum contiguous block count";
				if (argc < 1)
					errx(10, "-a: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_maxcontig, i);
				sblock.fs_maxcontig = i;
				continue;

			case 'd':
				name =
				   "rotational delay between contiguous blocks";
				if (argc < 1)
					errx(10, "-d: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				warnx("%s changes from %dms to %dms",
				    name, sblock.fs_rotdelay, i);
				sblock.fs_rotdelay = i;
				continue;

			case 'e':
				name =
				  "maximum blocks per file in a cylinder group";
				if (argc < 1)
					errx(10, "-e: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 1)
					errx(10, "%s must be >= 1 (was %s)",
					    name, *argv);
				warnx("%s changes from %d to %d",
				    name, sblock.fs_maxbpg, i);
				sblock.fs_maxbpg = i;
				continue;

			case 'm':
				name = "minimum percentage of free space";
				if (argc < 1)
					errx(10, "-m: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0 || i > 99)
					errx(10, "bad %s (%s)", name, *argv);
				warnx("%s changes from %d%% to %d%%",
				    name, sblock.fs_minfree, i);
				sblock.fs_minfree = i;
				if (i >= MINFREE &&
				    sblock.fs_optim == FS_OPTSPACE)
					warnx(OPTWARN, "time", ">=", MINFREE);
				if (i < MINFREE &&
				    sblock.fs_optim == FS_OPTTIME)
					warnx(OPTWARN, "space", "<", MINFREE);
				continue;

			case 'o':
				name = "optimization preference";
				if (argc < 1)
					errx(10, "-o: missing %s", name);
				argc--, argv++;
				if (strcmp(*argv, chg[FS_OPTSPACE]) == 0)
					i = FS_OPTSPACE;
				else if (strcmp(*argv, chg[FS_OPTTIME]) == 0)
					i = FS_OPTTIME;
				else
					errx(10, "bad %s (options are `space' or `time')",
					    name);
				if (sblock.fs_optim == i) {
					warnx("%s remains unchanged as %s",
					    name, chg[i]);
					continue;
				}
				warnx("%s changes from %s to %s",
				    name, chg[sblock.fs_optim], chg[i]);
				sblock.fs_optim = i;
				if (sblock.fs_minfree >= MINFREE &&
				    i == FS_OPTSPACE)
					warnx(OPTWARN, "time", ">=", MINFREE);
				if (sblock.fs_minfree < MINFREE &&
				    i == FS_OPTTIME)
					warnx(OPTWARN, "space", "<", MINFREE);
				continue;

			case 't':
				name = "track skew in sectors";
				if (argc < 1)
					errx(10, "-t: missing %s", name);
				argc--, argv++;
				i = atoi(*argv);
				if (i < 0)
					errx(10, "%s: %s must be >= 0",
						*argv, name);
				warnx("%s changes from %d to %d",
					name, sblock.fs_trackskew, i);
				sblock.fs_trackskew = i;
				continue;

			default:
				usage();
			}
	}
	if (argc != 1)
		usage();
	if (Nflag) {
		fprintf(stdout, "tunefs: current settings\n");
		fprintf(stdout, "\tmaximum contiguous block count %d\n",
		    sblock.fs_maxcontig);
		fprintf(stdout,
		    "\trotational delay between contiguous blocks %dms\n",
		    sblock.fs_rotdelay);
		fprintf(stdout,
		    "\tmaximum blocks per file in a cylinder group %d\n",
		    sblock.fs_maxbpg);
		fprintf(stdout, "\tminimum percentage of free space %d%%\n",
		    sblock.fs_minfree);
		fprintf(stdout, "\toptimization preference: %s\n",
		    chg[sblock.fs_optim]);
		fprintf(stdout, "\ttrack skew %d sectors\n",
			sblock.fs_trackskew);
		fprintf(stdout, "tunefs: no changes made\n");
		exit(0);
	}
	fi = open(special, 1);
	if (fi < 0)
		err(3, "cannot open %s for writing", special);
	bwrite((daddr_t)SBOFF / dev_bsize, (char *)&sblock, SBSIZE);
	if (Aflag)
		for (i = 0; i < sblock.fs_ncg; i++)
			bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)),
			    (char *)&sblock, SBSIZE);
	close(fi);
	exit(0);
}

void
usage()
{

	fprintf(stderr, "Usage: tunefs [-AN] tuneup-options special-device\n");
	fprintf(stderr, "where tuneup-options are:\n");
	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-m minimum percentage of free space\n");
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	fprintf(stderr, "\t-t track skew in sectors\n");
	exit(2);
}

void
getsb(fs, file)
	register struct fs *fs;
	char *file;
{

	fi = open(file, 0);
	if (fi < 0)
		err(3, "cannot open %s for reading", file);
	if (bread((daddr_t)SBOFF, (char *)fs, SBSIZE))
		err(4, "%s: bad super block", file);
	if (fs->fs_magic != FS_MAGIC)
		err(5, "%s: bad magic number", file);
	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	close(fi);
}

void
bwrite(blk, buf, size)
	daddr_t blk;
	char *buf;
	int size;
{

	if (lseek(fi, (off_t)blk * dev_bsize, SEEK_SET) < 0)
		err(6, "FS SEEK");
	if (write(fi, buf, size) != size)
		err(7, "FS WRITE");
}

int
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{
	int i;

	if (lseek(fi, (off_t)bno * dev_bsize, SEEK_SET) < 0)
		return(1);
	if ((i = read(fi, buf, cnt)) != cnt) {
		for(i=0; i<sblock.fs_bsize; i++)
			buf[i] = 0;
		return (1);
	}
	return (0);
}
