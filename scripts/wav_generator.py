import numpy as np
import struct
import argparse

def generate_sine_wave(frequency, duration, amplitude, sampling_rate):
    """
    Generate a sine wave.

    :param frequency: Frequency of the sine wave in Hz
    :param duration: Duration of the sine wave in seconds
    :param amplitude: Amplitude of the sine wave (0.0 to 1.0)
    :param sampling_rate: Sampling rate in Hz
    :return: Numpy array of the sine wave
    """
    t = np.linspace(0, duration, int(sampling_rate * duration), endpoint=False)
    wave = amplitude * np.sin(2 * np.pi * frequency * t)
    return wave

def create_wav_header(num_channels, sample_rate, bits_per_sample, num_samples, is_float):
    """
    Create a WAV file header.

    :param num_channels: Number of audio channels (e.g., 2 for stereo)
    :param sample_rate: Sampling rate in Hz
    :param bits_per_sample: Number of bits per sample (e.g., 16 for 16-bit PCM)
    :param num_samples: Total number of samples per channel
    :param is_float: Whether the audio format is float (True for float, False for PCM)
    :return: Byte string representing the WAV file header
    """
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8
    subchunk2_size = num_samples * num_channels * bits_per_sample // 8
    chunk_size = 36 + subchunk2_size

    audio_format = 3 if is_float else 1

    header = struct.pack(
        '<4sI4s4sIHHIIHH4sI',
        b'RIFF',
        chunk_size,
        b'WAVE',
        b'fmt ',
        16,  # Subchunk1Size (16 for PCM)
        audio_format,  # AudioFormat (1 for PCM, 3 for IEEE float)
        num_channels,
        sample_rate,
        byte_rate,
        block_align,
        bits_per_sample,
        b'data',
        subchunk2_size
    )
    return header

def save_stereo_wav(filename, left_channel, right_channel, sampling_rate, audio_format):
    """
    Save stereo WAV file with the specified audio format.

    :param filename: Name of the output WAV file
    :param left_channel: Numpy array for the left channel
    :param right_channel: Numpy array for the right channel
    :param sampling_rate: Sampling rate in Hz
    :param audio_format: Audio format (e.g., S16, U16, F32)
    """
    num_samples = left_channel.size
    num_channels = 2

    # Determine scaling and data type based on audio format
    if audio_format == "S16":
        bits_per_sample = 16
        scale = 32767
        dtype = np.int16
        is_float = False
    elif audio_format == "U16":
        bits_per_sample = 16
        scale = 32767
        dtype = np.uint16
        is_float = False
    elif audio_format == "S8":
        bits_per_sample = 8
        scale = 127
        dtype = np.int8
        is_float = False
    elif audio_format == "U8":
        bits_per_sample = 8
        scale = 127
        dtype = np.uint8
        is_float = False
    elif audio_format == "S32":
        bits_per_sample = 32
        scale = 2147483647
        dtype = np.int32
        is_float = False
    elif audio_format == "U32":
        bits_per_sample = 32
        scale = 2147483647
        dtype = np.uint32
        is_float = False
    elif audio_format == "F32":
        bits_per_sample = 32
        scale = 1.0
        dtype = np.float32
        is_float = True
    else:
        raise ValueError(f"Unsupported audio format: {audio_format}")

    header = create_wav_header(num_channels, sampling_rate, bits_per_sample, num_samples, is_float)

    # Scale and convert to the appropriate dtype
    left_channel = (left_channel * scale).astype(dtype)
    right_channel = (right_channel * scale).astype(dtype)

    # Interleave left and right channels
    stereo_wave = np.empty((left_channel.size + right_channel.size,), dtype=dtype)
    stereo_wave[0::2] = left_channel
    stereo_wave[1::2] = right_channel

    with open(filename, 'wb') as wav_file:
        wav_file.write(header)
        wav_file.write(stereo_wave.tobytes())

def main():
    parser = argparse.ArgumentParser(description="Generate a WAV file from a sine wave with configurable audio format.")
    parser.add_argument('-f', '--frequency', type=float, default=440.0, help="Frequency of the sine wave in Hz (default: 440.0)")
    parser.add_argument('-d', '--duration', type=float, default=5.0, help="Duration of the sine wave in seconds (default: 5.0)")
    parser.add_argument('-a', '--amplitude', type=float, default=0.8, help="Amplitude of the sine wave (0.0 to 1.0, default: 0.8)")
    parser.add_argument('-sr', '--sampling-rate', type=int, default=44100, help="Sampling rate in Hz (default: 44100)")
    parser.add_argument('-F', '--format', type=str, default="S16", choices=["U8", "S8", "U16", "S16", "U32", "S32", "F32"], help="Audio format: U8, S8, U16, S16, U32, S32, F32 (default: S16)")
    parser.add_argument('-o', '--output', type=str, default="sine_wave.wav", help="Output WAV file name (default: sine_wave.wav)")

    args = parser.parse_args()

    # Generate left and right channel sine waves
    left_channel = generate_sine_wave(args.frequency, args.duration, args.amplitude, args.sampling_rate)
    right_channel = generate_sine_wave(args.frequency, args.duration, args.amplitude, args.sampling_rate)

    # Save to WAV file
    save_stereo_wav(args.output, left_channel, right_channel, args.sampling_rate, args.format)
    print(f"WAV file saved: {args.output}")

if __name__ == "__main__":
    main()
