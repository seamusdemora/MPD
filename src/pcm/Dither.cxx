// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The Music Player Daemon Project

#include "Dither.hxx"
#include "Prng.hxx"

template<typename T, T MIN, T MAX, unsigned scale_bits>
inline T
PcmDither::Dither(T sample) noexcept
{
	constexpr T round = 1 << (scale_bits - 1);
	constexpr T mask = (1 << scale_bits) - 1;

	sample += error[0] - error[1] + error[2];

	error[2] = error[1];
	error[1] = error[0] / 2;

	/* round */
	T output = sample + round;

	const T rnd = pcm_prng(random);
	output += (rnd & mask) - (random & mask);

	random = rnd;

	/* clip */
	if (output > MAX) {
		output = MAX;

		if (sample > MAX)
			sample = MAX;
	} else if (output < MIN) {
		output = MIN;

		if (sample < MIN)
			sample = MIN;
	}

	output &= ~mask;

	error[0] = sample - output;

	return output >> scale_bits;
}

template<typename ST, unsigned SBITS, unsigned DBITS>
inline ST
PcmDither::DitherShift(ST sample) noexcept
{
	static_assert(sizeof(ST) * 8 > SBITS, "Source type too small");
	static_assert(SBITS > DBITS, "Non-positive scale_bits");

	static constexpr ST MIN = -(ST(1) << (SBITS - 1));
	static constexpr ST MAX = (ST(1) << (SBITS - 1)) - 1;

	return Dither<ST, MIN, MAX, SBITS - DBITS>(sample);
}

template<IntegerSampleTraits ST, IntegerSampleTraits DT>
inline typename DT::value_type
PcmDither::DitherConvert(typename ST::value_type sample) noexcept
{
	static_assert(ST::BITS > DT::BITS,
		      "Sample formats cannot be dithered");

	constexpr unsigned scale_bits = ST::BITS - DT::BITS;

	return Dither<typename ST::sum_type, ST::MIN, ST::MAX,
		      scale_bits>(sample);
}

template<IntegerSampleTraits ST, IntegerSampleTraits DT>
inline void
PcmDither::DitherConvert(typename DT::pointer dest,
			 typename ST::const_pointer src,
			 typename ST::const_pointer src_end) noexcept
{
	while (src < src_end)
		*dest++ = DitherConvert<ST, DT>(*src++);
}

inline void
PcmDither::Dither24To16(int16_t *dest, const int32_t *src,
			const int32_t *src_end) noexcept
{
	using ST = SampleTraits<SampleFormat::S24_P32>;
	using DT = SampleTraits<SampleFormat::S16>;
	DitherConvert<ST, DT>(dest, src, src_end);
}

inline void
PcmDither::Dither32To16(int16_t *dest, const int32_t *src,
			const int32_t *src_end) noexcept
{
	using ST = SampleTraits<SampleFormat::S32>;
	using DT = SampleTraits<SampleFormat::S16>;
	DitherConvert<ST, DT>(dest, src, src_end);
}
