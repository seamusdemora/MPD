/* the Music Player Daemon (MPD)
 * Copyright (C) 2003-2007 by Warren Dukes (warren.dukes@gmail.com)
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef MPD_DECODER_API_H
#define MPD_DECODER_API_H

/*
 * This is the public API which is used by decoder plugins to
 * communicate with the mpd core.
 *
 */

#include "decoder_plugin.h"
#include "input_stream.h"
#include "replay_gain.h"
#include "tag.h"
#include "audio_format.h"

#include <stdbool.h>

enum decoder_command {
	DECODE_COMMAND_NONE = 0,
	DECODE_COMMAND_START,
	DECODE_COMMAND_STOP,
	DECODE_COMMAND_SEEK
};


/**
 * Notify the player thread that it has finished initialization and
 * that it has read the song's meta data.
 */
void decoder_initialized(struct decoder * decoder,
			 const struct audio_format *audio_format,
			 bool seekable, float total_time);

/**
 * Returns the URI of the current song in UTF-8 encoding.
 *
 * The return value is allocated on the heap, and must be freed by the
 * caller.
 */
char *decoder_get_uri(struct decoder *decoder);

enum decoder_command decoder_get_command(struct decoder * decoder);

/**
 * Called by the decoder when it has performed the requested command
 * (dc->command).  This function resets dc->command and wakes up the
 * player thread.
 */
void decoder_command_finished(struct decoder * decoder);

double decoder_seek_where(struct decoder * decoder);

void decoder_seek_error(struct decoder * decoder);

/**
 * Blocking read from the input stream.  Returns the number of bytes
 * read, or 0 if one of the following occurs: end of file; error;
 * command (like SEEK or STOP).
 */
size_t decoder_read(struct decoder *decoder,
		    struct input_stream *inStream,
		    void *buffer, size_t length);

/**
 * This function is called by the decoder plugin when it has
 * successfully decoded block of input data.
 *
 * We send inStream for buffering the inputStream while waiting to
 * send the next chunk
 */
enum decoder_command
decoder_data(struct decoder *decoder,
	     struct input_stream *inStream,
	     const void *data, size_t datalen,
	     float data_time, uint16_t bitRate,
	     struct replay_gain_info *replay_gain_info);

/**
 * This function is called by the decoder plugin when it has
 * successfully decoded a tag.
 *
 * @param is an input stream which is buffering while we are waiting
 * for the player
 */
enum decoder_command
decoder_tag(struct decoder *decoder, struct input_stream *is,
	    const struct tag *tag);

#endif
