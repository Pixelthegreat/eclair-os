#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ec.h>

#define PATH "/bin"

#define LINEBUFSZ 128
static char linebuf[LINEBUFSZ];

static bool running = true;

#define ARGBUFSZ 128
static char argbuf[ARGBUFSZ];

static const char *progname = "sh";

/* argument data */
#define MAX_ARGS 16
#define ARG_VLEN 64
static struct arg {
	char value[ARG_VLEN];
	size_t len;
} args[MAX_ARGS];
static int nargs = 0;

static void finarg(void) {

	if (!nargs) return;

	struct arg *arg = &args[nargs-1];

	arg->value[arg->len] = 0;
}

static void newarg(void) {

	finarg();
	if (nargs < MAX_ARGS) {

		struct arg *arg = &args[nargs++];

		arg->len = 0;
	}
}

static void addarg(int ch) {

	if (!nargs) newarg();

	struct arg *arg = &args[nargs-1];
	if (arg->len < ARG_VLEN-1)
		arg->value[arg->len++] = (char)ch;
}

/* parse line of code */
#define ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
#define ISIDENT(c) (ISALPHA(c) || ISDIGIT(c) || ((c) == '_'))

#define NEXTCC ((int)*line++)

static void parse_line(const char *line, const char *file, int nline) {

	nargs = 0;
	int cc = NEXTCC;
	int lit = 0; /* literal character */
	bool exp = false; /* expecting new argument */
	while (cc) {

		if (cc == lit) {

			lit = 0;
			cc = NEXTCC;
		}

		/* comment */
		else if (cc == '#') break;

		/* newline */
		else if (cc == '\n') cc = NEXTCC;

		/* single quote */
		else if (lit == '\'') {

			addarg(cc);
			cc = NEXTCC;
		}

		/* variable */
		else if (cc == '$') {

			if (exp) {
				newarg();
				exp = false;
			}

			cc = NEXTCC;
			size_t len = 0;
			while (ISIDENT(cc)) {

				if (len < ARGBUFSZ-1)
					argbuf[len++] = (char)cc;
				cc = NEXTCC;
			}
			argbuf[len] = 0;

			/* get value */
			const char *value = getenv(argbuf);
			if (value) {
				while (*value) {
					addarg((int)*value);
					value++;
				}
			}
		}

		/* whitespace */
		else if (strchr(" \t", cc)) {

			if (lit) addarg(cc);
			else exp = true;

			cc = NEXTCC;
		}

		/* literal */
		else if (strchr("'\"", cc)) {

			if (exp) {
				newarg();
				exp = false;
			}

			lit = cc;
			cc = NEXTCC;
		}

		/* other */
		else {
			if (exp) {
				newarg();
				exp = false;
			}

			addarg(cc);
			cc = NEXTCC;
		}
	}
	finarg();
}

/* execute command */
static void cmd_exec(int argc, const char **argv) {

	const char *bin = argv[0];
	
	/* exit shell */
	if (!strcmp(bin, "exit"))
		running = false;

	/* print message */
	else if (!strcmp(bin, "echo")) {

		for (int i = 1; i < argc; i++) {
			if (i-1) fputc(' ', stdout);
			fputs(argv[i], stdout);
		}
		fputc('\n', stdout);
	}

	/* bad idea */
	else if (!strcmp(bin, "init"))
		fprintf(stderr, "%s: Bad idea\n", progname);

	/* other */
	else {

		snprintf(argbuf, ARGBUFSZ, "%s/%s", PATH, bin);
		int pid = ec_pexec(argbuf, argv, NULL);

		if (pid < 0) {

			fprintf(stderr, "%s: Command '%s' not found\n", progname, bin);
			return;
		}

		int status = 0;
		while (!ECW_ISEXITED(status))
			ec_pwait(pid, &status, NULL);
	}
}

/* evaluate line of code */
static void eval_line(const char *line, const char *file, int nline) {

	parse_line(line, file, nline);
	if (nargs) {

		/* construct argv array */
		const char *argv[MAX_ARGS+1] = {};
		int argc = 0;
		for (int i = 0; i < nargs; i++) {
			if (args[i].value)
				argv[argc++] = args[i].value;
		}
		argv[argc] = NULL;

		cmd_exec(argc, argv);
	}
}

/* evaluate file */
static void eval_file(const char *path) {

	int line = 1;
	FILE *fp = fopen(path, "r");
	if (!fp) {

		fprintf(stderr, "%s: Can't open '%s': %s\n", progname, path, strerror(errno));
		return;
	}

	const char *pstr = linebuf;
	while (pstr) {

		pstr = fgets(linebuf, LINEBUFSZ, fp);
		eval_line(linebuf, path, line++);
	}
	fclose(fp);
}

/* main repl */
int main(int argc, const char **argv) {

	if (argc) progname = argv[0];
	if (argc >= 2) eval_file(argv[1]);

	linebuf[0] = 0;
	while (running) {

		printf("$ ");
		fflush(stdout);

		fgets(linebuf, LINEBUFSZ, stdin);
		eval_line(linebuf, "<stdin>", 1);
	}
	return 123;
}
