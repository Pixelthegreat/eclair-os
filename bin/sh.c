/*
 * Copyright 2025-2026, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <ec/hmap.h>
#include <ec.h>

#define PATH "/bin"

static bool running = true;

static const char *progname = "sh";

static ec_uinfo_t uinfo; /* user info */

/* options */
enum {
	OPT_HELP_BIT = 0x1,
	OPT_REPL_BIT = 0x2,
};
static int opt_flags = 0;
static const char *runpath = NULL;

/* miscellaneous buffers */
#define LASTRETSZ 4
static char lastret[LASTRETSZ] = "0";

static char cwdbuf[EC_PATHSZ];

#define LINEBUFSZ 128
static char linebuf[LINEBUFSZ];

#define ARGBUFSZ 128
static char argbuf[ARGBUFSZ];
static char argbuf2[ARGBUFSZ];

/* variable hashmap */
struct ec_hmap_var;
static void ec_hmap_var_set(struct ec_hmap_var *hmap, const char *val);

ec_hmap_define_type(var, char val[256]);

static void ec_hmap_var_set(struct ec_hmap_var *hmap, const char *val) {

	strncpy(hmap->val, val, sizeof(hmap->val));
}

static struct ec_hmap_var *varmap = NULL;

/* raw getenv */
extern const char **environ;

static const char *raw_getenv(const char *name) {

	size_t i = 0;
	while (environ[i]) {

		const char *env = environ[i++];

		const char *end = strchr(env, '=');
		if (!end) continue;

		size_t len = (size_t)(end - env);
		if (!strncmp(env, name, len))
			return env;
	}
	return NULL;
}

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

/* get variable value */
static const char *getvar(const char *name) {

	struct ec_hmap_var *item = ec_hmap_get(varmap, name);
	if (item) return item->val;

	/* last error code */
	else if (!strcmp(name, "?"))
		return lastret;

	/* current working directory */
	else if (!strcmp(name, "PWD"))
		return cwdbuf;

	/* user */
	else if (!strcmp(name, "USER"))
		return uinfo.uname;

	/* group */
	else if (!strcmp(name, "GROUP"))
		return uinfo.gname;

	/* other */
	else return getenv(name);
}

/* parse line of code */
#define ISALPHA(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define ISDIGIT(c) ((c) >= '0' && (c) <= '9')
#define ISIDENT(c) (ISALPHA(c) || ISDIGIT(c) || ((c) == '_') || ((c) == '?'))

#define NEXTCC ((int)*++line)

static const char *parse_line(const char *line, const char *file, int nline) {

	nargs = 0;
	int cc = *line;
	int lit = 0; /* literal character */
	bool exp = false; /* expecting new argument */
	while (cc) {

		if (cc == lit) {

			lit = 0;
			cc = NEXTCC;
		}

		/* newline */
		else if (cc == '\n') cc = NEXTCC;

		/* single quote */
		else if (lit == '\'') {

			addarg(cc);
			cc = NEXTCC;
		}

		/* escape charactere */
		else if (cc == '\\') {

			cc = NEXTCC;
			if (!cc) break;

			switch (cc) {
				case 'n': cc = '\n';
				case 'r': cc = '\r';
				case 't': cc = '\t';
				case 'e': cc = '\x1b';
			}
			addarg(cc);
			cc = NEXTCC;
		}

		/* break */
		else if (!lit && cc == ';') {

			line++;
			break;
		}

		/* comment */
		else if (!lit && cc == '#') {

			while (cc) cc = NEXTCC;
			break;
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
			const char *value = getvar(argbuf);
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
		else if (!lit && strchr("'\"", cc)) {

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
	return line;
}

/* execute command */
#define FLAG_BG 0x1

static int if_block = 0;
static bool if_skip = false;

static int cmd_exec(int argc, const char **argv, int flags) {

	const char *varset = strchr(argv[0], '=');
	if (varset && !if_skip) {

		char key[256];
		size_t len = (size_t)(varset-argv[0]);
		if (len > sizeof(key)-1) len = sizeof(key)-1;

		memcpy(key, argv[0], len);
		key[len] = 0;

		ec_hmap_set(varmap, key, ++varset);
		argv++; argc--;
	}

	if (!argc) return 0;

	const char *bin = argv[0];

	/* inside if block */
	if (if_block && !strcmp(bin, "fi")) {

		if_block--;
		if (if_skip && !if_block)
			if_skip = false;
	}

	else if (if_block && !strcmp(bin, "else"))
		if_skip = !if_skip;

	else if (if_skip || (if_block && !strcmp(bin, "then")));

	/* exit shell */
	else if (!strcmp(bin, "exit"))
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
	else if (!strcmp(bin, "init")) {

		fprintf(stderr, "%s: Bad idea\n", progname);
		return 0xff;
	}

	/* change directory */
	else if (!strcmp(bin, "cd")) {

		if (argc != 2) {
			
			fprintf(stderr, "%s: cd: Invalid arguments\n", progname);
			return 0xff;
		}
		if (ec_chdir(argv[1]) < 0) {

			fprintf(stderr, "%s: cd: %s\n", progname, strerror(errno));
			return 0xff;
		}
		ec_getcwd(cwdbuf, EC_PATHSZ);
	}

	/* test condition */
	else if (!strcmp(bin, "test") || !strcmp(bin, "[")) {

		int expa = 3 + (*bin == '[');
		int expb = expa+1;

		if ((argc != expa && argc != expb) ||
		    (*bin == '[' && !!strcmp(argv[argc-1], "]"))) {

			fprintf(stderr, "%s: %s: Invalid arguments\n", progname, bin);
			return 0xff;
		}
		int result = 1;

		int index = 1;
		const char *left = argc == expa? NULL: argv[index++];
		const char *op = argv[index++];
		const char *right = argv[index++];

		/* binary operation */
		if (left) {

			if (!strcmp(op, "="))
				result = !strcmp(left, right);
			else {
				int ileft = atoi(left);
				int iright = atoi(right);

				if (!strcmp(op, "-eq"))
					result = (ileft == iright);
				else if (!strcmp(op, "-ne"))
					result = (ileft != iright);
				else if (!strcmp(op, "-lt"))
					result = (ileft < iright);
				else if (!strcmp(op, "-gt"))
					result = (ileft > iright);
				else {
					fprintf(stderr, "%s: %s: Invalid operation '%s'\n",
						progname, bin, op);
					return 0xff;
				}
			}
		}

		/* unary operation */
		else {
			ec_stat_t stat;
			int stat_res = ec_stat(right, &stat);

			if (!strcmp(op, "-e"))
				result = (stat_res >= 0);
			else if (!strcmp(op, "-f"))
				result = (stat_res >= 0 && stat.flags & ECS_REG);
			else if (!strcmp(op, "-d"))
				result = (stat_res >= 0 && stat.flags & ECS_DIR);
			else if (!strcmp(op, "-x"))
				result = (stat_res >= 0 && stat.mode & 0111);
			else {
				fprintf(stderr, "%s: %s: Invalid operation '%s'\n",
					progname, bin, op);
				return 0xff;
			}
		}
		return !result;
	}

	/* if statement */
	else if (!strcmp(bin, "if")) {

		int result = cmd_exec(argc-1, argv+1, flags);

		if_block++;
		if (result >= 1) if_skip = true;
	}

	/* not */
	else if (!strcmp(bin, "!")) {

		int result = cmd_exec(argc-1, argv+1, flags);
		return !result;
	}

	/* other */
	else {

		snprintf(argbuf, ARGBUFSZ, "%s/%s", PATH, bin);
		snprintf(argbuf2, ARGBUFSZ, "PWD=%s", cwdbuf);

		const char *envp[] = {
			raw_getenv("EC_STDIN"),
			raw_getenv("EC_STDOUT"),
			raw_getenv("EC_STDERR"),
			argbuf2,
			NULL,
		};
		int pid = ec_pexec(argbuf, argv, envp);

		if (pid < 0) {

			fprintf(stderr, "%s: Command '%s' not found\n", progname, bin);
			return 0xff;
		}

		/* wait for process */
		if (!(flags & FLAG_BG)) {

			int status = 0;
			while (!ECW_ISEXITED(status))
				ec_pwait(pid, &status, NULL);

			return ECW_TOEXITCODE(status);
		}
	}
	return 0;
}

/* evaluate line of code */
static int eval_line(const char *line, const char *file, int nline) {

	do {
		line = parse_line(line, file, nline);

		if (nargs) {

			/* construct argv array */
			const char *argv[MAX_ARGS+1] = {};
			int argc = 0;
			for (int i = 0; i < nargs; i++) {
				if (args[i].value)
					argv[argc++] = args[i].value;
			}
			argv[argc] = NULL;

			int flags = 0;
			if (argc && !strcmp(argv[argc-1], "&")) {

				argv[--argc] = NULL;
				flags |= FLAG_BG;
			}
			int result = cmd_exec(argc, argv, flags);
			snprintf(lastret, LASTRETSZ, "%d", result);
		}
	} while(*line);
	return 0;
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
		int ret = eval_line(linebuf, path, line++);
	}
	fclose(fp);
}

/* print prompt */
static void print_prompt(void) {

	const char *prompt;
	struct ec_hmap_var *var = ec_hmap_get(varmap, "PS1");

	if (var) prompt = var->val;
	else prompt = !strcmp(uinfo.uname, "root")? "'# '": "'$ '";

	parse_line(prompt, "<prompt>", 1);

	int n = 0;
	for (int i = 0; i < nargs; i++) {

		struct arg *arg = &args[i];

		if (arg->len) {

			if (n++) fputc(' ', stdout);
			fputs(arg->value, stdout);
		}
	}
}

/* parse arguments */
static int parse_args(int argc, const char **argv) {

	int opt;
	while ((opt = getopt(argc, argv, "hr")) != -1) {
		switch (opt) {
			case 'r': /* repl */
				opt_flags |= OPT_REPL_BIT;
				break;
			case 'h': /* help */
				opt_flags |= OPT_HELP_BIT;
			default:
				fprintf(stderr, "Usage: %s [-h] [-r] [path]\n", progname);
				return opt_flags & OPT_HELP_BIT? 0: -1;
		}
	}
	if (optind < argc) runpath = argv[optind];
	return 0;
}

/* run application */
static int run(int argc, const char **argv) {

	ec_hmap_init(varmap, var);

	progname = argv[0];
	if (parse_args(argc, argv) < 0)
		return 1;
	if (opt_flags & OPT_HELP_BIT)
		return 0;

	ec_getuser(&uinfo);
	ec_getcwd(cwdbuf, EC_PATHSZ);

	if (runpath) {
		
		eval_file(runpath);
		if (!(opt_flags & OPT_REPL_BIT)) return 0;
	}

	/* main repl */
	linebuf[0] = 0;
	while (running) {

		print_prompt();
		fflush(stdout);

		fgets(linebuf, LINEBUFSZ, stdin);
		int ret = eval_line(linebuf, "<stdin>", 1);

		if_block = 0;
		if_skip = false;
	}
	return ec_getpid() == 2? 123: 0;
}

/* clean up resources */
static void cleanup(void) {

	ec_hmap_free(varmap);
}

int main(int argc, const char **argv) {

	int code = run(argc, argv);
	cleanup();
	return code;
}
