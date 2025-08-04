#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec.h>
#include <ec/device.h>
#include <ec/sound.h>

static const char *progname = "playsnd"; /* program name */
static const char *path = NULL; /* sound file path */
static int fd = -1; /* sound fd */
static ecio_sndattr_t attr; /* sound attributes */
static void *buf = NULL; /* data transfer buffer */
static void *readbuf = NULL; /* file read buffer */
static void *resbuf = NULL; /* resampled buffer */
static ec_sound_t sound; /* sound file */

/* run application */
static int run(int argc, const char **argv) {

	if (argv[0]) progname = argv[0];

	if (argc != 2) {

		fprintf(stderr, "Invalid arguments\nUsage: %s <path>\n", progname);
		return 1;
	}
	path = argv[1];

	/* connect to sound device */
	fd = ec_open("/dev/snd0", ECF_READ | ECF_WRITE, 0);
	if (fd < 0) {

		fprintf(stderr, "%s: Can't open '/dev/snd0': %s\n", progname, strerror(errno));
		return 1;
	}

	ec_ioctl(fd, ECIO_SND_SETATTR, (uintptr_t)&attr);

	/* open sound file */
	if (ec_sound_open(&sound, path, EC_SOUND_FORMAT_WAV) < 0) {

		fprintf(stderr, "%s: Can't open '%s': %s\n", progname, path, strerror(errno));
		return 1;
	}

	/* configure buffers */
	uint32_t samples = attr.bufsz / (attr.channels * (attr.format + 1));
	uint64_t ns = (1000000000 / (uint64_t)attr.rate) * (uint64_t)samples;
	buf = malloc((size_t)attr.bufsz);

	uint32_t fsamples = (samples * sound.rate) / attr.rate;

	size_t length = 0;
	if (sound.rate > attr.rate) length = sound.length * (sound.rate / attr.rate);
	else length = (uint32_t)(((uint64_t)sound.length * (((uint64_t)attr.rate * 100) / (uint64_t)sound.rate)) / 100);

	/* read audio data */
	readbuf = malloc((size_t)(attr.channels * (attr.format + 1)) * sound.length + (size_t)fsamples);
	if (ec_sound_read_samples(&sound, (uint8_t *)readbuf, sound.length, attr.format, attr.channels) < 0) {

		fprintf(stderr, "%s: Can't read '%s': %s\n", progname, path, strerror(errno));
		return 1;
	}

	/* resample audio */
	if (attr.rate != sound.rate) {

		resbuf = malloc((size_t)(attr.channels * (attr.format + 1)) * length + (size_t)samples);
		for (uint32_t i = 0; i < length * attr.channels; i++) {

			uint32_t offset = 0;
			if (sound.rate > attr.rate) offset = i * (sound.rate / attr.rate);
			else offset = (i * 100) / ((attr.rate * 100) / sound.rate);

			*(uint16_t *)(resbuf + i * 2) = *(uint16_t *)(readbuf + offset * 2);
		}
		free(readbuf);
		readbuf = NULL;
	}
	else {
		resbuf = readbuf;
		readbuf = NULL;
	}

	/* play sound */
	ec_ioctl(fd, ECIO_SND_START, 0);

	uint64_t timens = 0;
	uint32_t position = 0;

	while (position < length) {

		/* copy data */
		while (ec_write(fd, resbuf + position * 4, (size_t)attr.bufsz) == (size_t)attr.bufsz)
			position += samples;

		ec_timeval_t tv = {
			.sec = 0,
			.nsec = ns,
		};
		ec_sleepns(&tv);
		timens += ns;
	}

	ec_ioctl(fd, ECIO_SND_STOP, 0);
	return 0;
}

/* clean up resources */
static void cleanup(void) {

	ec_sound_close(&sound);
	if (resbuf) free(resbuf);
	if (readbuf) free(readbuf);
	if (buf) free(buf);
	if (fd >= 0) ec_close(fd);
}

int main(int argc, const char **argv) {

	int code = run(argc, argv);
	cleanup();
	return code;
}
