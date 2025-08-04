/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Common definitions for simple audio formats and interface for audio loading
 */
#ifndef EC_SOUND_H
#define EC_SOUND_H

#include <stdint.h>
#include <stdio.h>
#include <ec/device.h>

/* wav header */
#define EC_WAV_FMT_PCM 1

typedef struct ec_wav_header {
	char riffsig[4]; /* "RIFF" signature */
	uint32_t rchsize; /* header chunk size */
	char wavesig[4]; /* "WAVE" signature */
	char fmtsig[4]; /* "fmt " signature */
	uint32_t fchsize; /* format chunk size */
	uint16_t format; /* audio format */
	uint16_t nchannels; /* number of channels */
	uint32_t srate; /* sample rate */
	uint32_t brate; /* byte rate */
	uint16_t balign; /* block align */
	uint16_t bitdepth; /* bits per sample */
	char datasig[4]; /* "data" signature */
	uint32_t dchsize; /* data chunk size */
	uint8_t data[]; /* audio data */
} __attribute__((packed)) ec_wav_header_t;

/* audio formats */
typedef enum ec_sound_format {
	EC_SOUND_FORMAT_WAV = 0,

	EC_SOUND_FORMAT_COUNT,
} ec_sound_format_t;

/* audio operations */
struct ec_sound;

typedef struct ec_sound_ops {
	int (*read_header)(struct ec_sound *); /* read header info */
	int (*read_sample)(struct ec_sound *, uint8_t *, uint32_t, uint32_t); /* read sample */
} ec_sound_ops_t;

/* generic audio info */
typedef struct ec_sound {
	FILE *fp; /* open file */
	size_t length; /* sample count */
	size_t rate; /* sample rate */
	size_t position; /* sample position */
	ec_sound_ops_t *ops; /* internal sound operations */
	uint32_t impl[4]; /* internal loader implementation details */
} ec_sound_t;

#define EC_SOUND_INIT ((ec_sound_t){.fp = NULL})

/*
 * Open a sound to read.
 *
 * 'format' is the file format, not the data format.
 *
 * Returns zero if successful, negative on error.
 */
extern int ec_sound_open(ec_sound_t *sound, const char *path, ec_sound_format_t format);

/*
 * Read one or more samples from a sound file.
 *
 * 'format' is the data format to read (see EC_SNDFMT, 'ec/device.h')
 *
 * Returns the number of samples read if successful, negative on error.
 */
extern int ec_sound_read_samples(ec_sound_t *sound, uint8_t *buffer, size_t count, uint32_t format, uint32_t channels);

/*
 * Close a sound.
 *
 * Returns zero if successful, negative on error.
 */
extern int ec_sound_close(ec_sound_t *sound);

#endif /* EC_SOUND_H */
