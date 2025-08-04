/*
 * Copyright 2025, Elliot Kohlmyer
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ec/sound.h>

#define SETERRNO(code) ({\
		errno = code;\
		return -1;\
	})

/* read wav header */
static int wav_read_header(ec_sound_t *sound) {

	ec_wav_header_t header;
	if (fread(&header, 1, sizeof(header), sound->fp) != sizeof(header) ||
	    !!memcmp(header.riffsig, "RIFF", 4) ||
	    !!memcmp(header.wavesig, "WAVE", 4) ||
	    !!memcmp(header.fmtsig, "fmt ", 4) ||
	    !!memcmp(header.datasig, "data", 4) ||
	    header.nchannels < 1 || header.nchannels > 2 ||
	    header.bitdepth % 8 != 0 || header.bitdepth > 16 ||
	    header.format != EC_WAV_FMT_PCM)
		SETERRNO(EBADF);

	sound->length = (size_t)(header.dchsize / (header.bitdepth / 8 * (uint32_t)header.nchannels));
	sound->rate = (size_t)header.srate;
	sound->impl[0] = (uint32_t)header.nchannels;
	sound->impl[1] = (uint32_t)header.bitdepth;
	return 0;
}

/* read wav sample */
static int wav_read_sample(ec_sound_t *sound, uint8_t *buf, uint32_t format, uint32_t channels) {

	uint8_t data[4];
	uint16_t samples[2];
	for (uint32_t i = 0; i < sound->impl[0]; i++) {

		if (fread(data, 1, (size_t)(sound->impl[1] / 8), sound->fp) != (size_t)(sound->impl[1] / 8))
			return -1;

		switch (sound->impl[1]) {
			/* 8-bit */
			case 8:
				samples[i] = (((uint16_t)*data) << 8) - 0x8000;
				break;
			case 16:
				samples[i] = *(uint16_t *)data;
				break;
		}
	}

	/* account for channels */
	if (channels < sound->impl[0])
		samples[0] += samples[1];

	else if (channels > sound->impl[0])
		samples[1] = samples[0];

	/* convert samples to requested format */
	for (uint32_t i = 0; i < channels; i++) {
		switch (format) {
			/* 16-bit */
			case ECIO_SNDFMT_I16:
				*(uint16_t *)(buf + i * 2) = samples[i];
				break;
			default:
				SETERRNO(EINVAL);
		}
	}
	return 0;
}

/* format operations */
static ec_sound_ops_t ops[EC_SOUND_FORMAT_COUNT] = {
	[EC_SOUND_FORMAT_WAV] = {
		.read_header = wav_read_header,
		.read_sample = wav_read_sample,
	},
};

/* open sound */
extern int ec_sound_open(ec_sound_t *sound, const char *path, ec_sound_format_t format) {

	if (sound->fp || !path || format < 0 || format >= EC_SOUND_FORMAT_COUNT)
		SETERRNO(EINVAL);

	/* open file and read sound header */
	sound->fp = fopen(path, "rb");
	if (!sound->fp) return -1;

	sound->ops = &ops[format];
	if (sound->ops->read_header(sound) < 0) {

		fclose(sound->fp);
		sound->fp = NULL;
		return -1;
	}
	sound->position = 0;

	return 0;
}

/* read samples from sound */
extern int ec_sound_read_samples(ec_sound_t *sound, uint8_t *buffer, size_t count, uint32_t format, uint32_t channels) {

	if (!sound || !sound->fp || !buffer ||
	    format < 1 || format > 1 ||
	    channels < 1 || channels > 2)
		SETERRNO(EINVAL);

	size_t i = 0;
	for (; i < count; i++) {

		if (sound->position >= sound->length)
			break;
		if (sound->ops->read_sample(sound, buffer + i * (format + 1) * channels, format, channels) < 0)
			return -1;
	}
	return (int)i;
}

/* close sound */
extern int ec_sound_close(ec_sound_t *sound) {

	if (!sound || !sound->fp) SETERRNO(EINVAL);

	fclose(sound->fp);
	sound->fp = NULL;

	return 0;
}
