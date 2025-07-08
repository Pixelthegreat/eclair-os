/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ec.h>

#define BUFBIN 0
#define BUFTEXT 1

struct __LIBC_FILE {
	int fd; /* file descriptor */
	int buftype; /* buffer type */
	int errno; /* error number */
	size_t rpos, wpos; /* positions in buffers */
	size_t rcnt; /* amount read */
	uint8_t *rbuf; /* read buffer */
	uint8_t *wbuf; /* write buffer */
	bool eof; /* reached end of file */
};
FILE *stdin, *stdout, *stderr;

static FILE files[FOPEN_MAX];
static FILE *openfdfile(int fd, int buftype);

/* initialize file subsystem */
extern int __libc_init_file(void) {

	for (int i = 0; i < FOPEN_MAX; i++)
		files[i].fd = -1;

	if (!(stdin = openfdfile(0, BUFTEXT))) return -1;
	if (!(stdout = openfdfile(1, BUFTEXT))) return -1;
	if (!(stderr = openfdfile(2, BUFTEXT))) return -1;

	return 0;
}

/* clean up file subsystem */
extern void __libc_fini_file(void) {

	for (int i = 0; i < FOPEN_MAX; i++) {

		if (files[i].fd >= 3)
			fclose(&files[i]);
	}
}

/* locate a free FILE pointer */
static FILE *locatefile(void) {

	for (int i = 0; i < FOPEN_MAX; i++) {
		if (files[i].fd < 0) return &files[i];
	}
	return NULL;
}

/* open a file descriptor as a FILE pointer */
static FILE *openfdfile(int fd, int buftype) {

	FILE *fp = locatefile();
	if (!fp) return NULL;

	fp->fd = fd;
	fp->buftype = buftype;
	fp->errno = 0;
	fp->rpos = 0;
	fp->wpos = 0;
	fp->rcnt = 0;
	fp->rbuf = (uint8_t *)malloc(BUFSIZ);
	fp->wbuf = (uint8_t *)malloc(BUFSIZ);
	fp->eof = false;

	return fp;
}

/* parse mode string */
static int parsemode(const char *mode, int *buftype) {

	*buftype = BUFTEXT;

	int rdwr = 0;
	int other = 0;

	char c;
	while ((c = *mode++) != 0) {

		if (c == 'r') {

			if (rdwr) continue;
			rdwr = ECF_READ;
		}
		else if (c == 'w') {

			if (rdwr) continue;
			rdwr = ECF_WRITE;
			other = other | ECF_TRUNCATE | ECF_CREATE;
		}
		else if (c == 'b')
			*buftype = BUFBIN;
		else if (c == '+')
			rdwr = ECF_READ | ECF_WRITE;
	}
	return rdwr | other;
}

/* close file */
extern int fclose(FILE *stream) {

	fflush(stream);

	int fd = stream->fd;
	stream->fd = -1;
	free(stream->rbuf);
	free(stream->wbuf);

	if (ec_close(fd) < 0) return EOF;
	return 0;
}

/* flush write buffer */
extern int fflush(FILE *stream) {

	if (stream->errno) return EOF;
	if (!stream->wpos) return 0;

	ec_ssize_t nwrite = ec_write(stream->fd, stream->wbuf, stream->wpos);
	stream->wpos = 0;

	if (nwrite < 0) {

		stream->errno = -((int)nwrite);
		return EOF;
	}
	return 0;
}

/* open file */
extern FILE *fopen(const char *restrict filename, const char *restrict mode) {

	int buftype;
	int mdint = parsemode(mode, &buftype);

	int fd = ec_open(filename, mdint, 0644);
	if (fd < 0) return NULL;

	FILE *fp = openfdfile(fd, buftype);
	if (!fp) {

		ec_close(fd);
		return fp;
	}
	return fp;
}

/* get character from stream */
extern int fgetc(FILE *stream) {

	if (stream->errno || stream->eof) return EOF;

	/* read more */
	if (stream->rpos >= stream->rcnt) {

		stream->rcnt = 0;
		stream->rpos = 0;
		ec_ssize_t nread = ec_read(stream->fd, stream->rbuf, BUFSIZ);

		/* end of file or error */
		if (nread == EOF) {

			stream->eof = true;
			return EOF;
		}
		else if (nread < 0) {

			stream->errno = -((int)nread);
			return EOF;
		}
		stream->rcnt = (size_t)nread;
		if (!nread) return EOF;
	}
	return (int)stream->rbuf[stream->rpos++];
}

/* read string from stream */
extern char *fgets(char *restrict s, int n, FILE *restrict stream) {

	int ch = 0;
	int p = 0;
	while (ch != EOF && ch != '\n') {

		ch = fgetc(stream);
		if (ch != EOF && p < n-1)
			s[p++] = (char)ch;
	}
	s[p] = 0;

	if (ch == EOF) return NULL;
	return s;
}

/* write character to stream */
extern int fputc(int c, FILE *stream) {

	stream->wbuf[stream->wpos++] = (uint8_t)c;

	if (stream->wpos >= BUFSIZ || (stream->buftype == BUFTEXT && c == '\n')) {
		if (fflush(stream) == EOF)
			return EOF;
	}
	return c;
}

/* write string to stream */
extern int fputs(const char *restrict s, FILE *restrict stream) {

	while (*s) {

		int ch = (int)*s++;
		if (fputc(ch, stream) != ch)
			return EOF;
	}
	return 0;
}

/* read from stream */
extern size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {

	size_t total = size * nmemb;
	size_t nread = 0;
	while (nread < total) {

		int ch = fgetc(stream);
		if (ch == EOF) break;

		((uint8_t *)ptr)[nread++] = (uint8_t)ch;
	}
	return nread;
}

/* write to stream */
extern size_t fwrite(const void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream) {

	size_t total = size * nmemb;
	size_t nwrite = 0;
	while (nwrite < total) {

		int ch = ((uint8_t *)ptr)[nwrite];
		if (fputc(ch, stream) != ch) break;

		nwrite++;
	}
	return nwrite;
}

/* seek to position in file */
extern int fseek(FILE *stream, long int offset, int whence) {

	if (stream->wpos) return -1;

	(void)ec_lseek(stream->fd, (ec_off_t)offset, whence);

	stream->rpos = 0;
	stream->rcnt = 0;
	stream->eof = false;

	return 0;
}

/* get file position */
extern long int ftell(FILE *stream) {

	ec_off_t pos = ec_lseek(stream->fd, 0, SEEK_CUR);
	if (pos < 0) return -1L;
	return (long int)pos;
}

/* get error */
extern int ferror(FILE *stream) {

	return stream->errno;
}
