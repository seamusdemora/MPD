// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Convert.hxx"
#include "ConfiguredResampler.hxx"
#include "util/SpanCast.hxx"

#include <cassert>
#include <stdexcept>

void
pcm_convert_global_init(const ConfigData &config)
{
	pcm_resampler_global_init(config);
}

PcmConvert::PcmConvert(const AudioFormat _src_format,
		       const AudioFormat dest_format)
	:src_format(_src_format)
{
	assert(src_format.IsValid());
	assert(dest_format.IsValid());

	AudioFormat format = _src_format;
	if (format.format == SampleFormat::DSD &&
	    (dest_format.format != SampleFormat::DSD ||
	     format.sample_rate != dest_format.sample_rate)) {
#ifdef ENABLE_DSD
		dsd2pcm_float = dest_format.format == SampleFormat::FLOAT;
		format.format = dsd2pcm_float
			? SampleFormat::FLOAT
			: SampleFormat::S24_P32;
#else
		throw std::runtime_error("DSD support is disabled");
#endif
	}

	enable_resampler = format.sample_rate != dest_format.sample_rate;
	if (enable_resampler) {
		resampler.Open(format, dest_format.sample_rate);

		format.format = resampler.GetOutputSampleFormat();
		format.sample_rate = dest_format.sample_rate;
	}

	enable_format = format.format != dest_format.format;
	if (enable_format) {
		try {
			format_converter.Open(format.format,
					      dest_format.format);
		} catch (...) {
			if (enable_resampler)
				resampler.Close();
			throw;
		}
	}

	format.format = dest_format.format;

	enable_channels = format.channels != dest_format.channels;
	if (enable_channels) {
		try {
			channels_converter.Open(format.format, format.channels,
						dest_format.channels);
		} catch (...) {
			if (enable_format)
				format_converter.Close();
			if (enable_resampler)
				resampler.Close();
			throw;
		}
	}
}

PcmConvert::~PcmConvert() noexcept
{
	if (enable_channels)
		channels_converter.Close();
	if (enable_format)
		format_converter.Close();
	if (enable_resampler)
		resampler.Close();

#ifdef ENABLE_DSD
	dsd.Reset();
#endif
}

void
PcmConvert::Reset() noexcept
{
	if (enable_resampler)
		resampler.Reset();

#ifdef ENABLE_DSD
	dsd.Reset();
#endif
}

std::span<const std::byte>
PcmConvert::Convert(std::span<const std::byte> buffer)
{
#ifdef ENABLE_DSD
	if (src_format.format == SampleFormat::DSD) {
		auto d = dsd2pcm_float
			? std::as_bytes(dsd.ToFloat(src_format.channels, buffer))
			: std::as_bytes(dsd.ToS24(src_format.channels, buffer));
		if (d.data() == nullptr)
			throw std::runtime_error("DSD to PCM conversion failed");

		buffer = d;
	}
#endif

	if (enable_resampler)
		buffer = resampler.Resample(buffer);

	if (enable_format)
		buffer = format_converter.Convert(buffer);

	if (enable_channels)
		buffer = channels_converter.Convert(buffer);

	return buffer;
}

std::span<const std::byte>
PcmConvert::Flush()
{
	if (enable_resampler) {
		auto buffer = resampler.Flush();
		if (buffer.data() != nullptr) {
			if (enable_format)
				buffer = format_converter.Convert(buffer);

			if (enable_channels)
				buffer = channels_converter.Convert(buffer);

			return buffer;
		}
	}

	return {};
}
