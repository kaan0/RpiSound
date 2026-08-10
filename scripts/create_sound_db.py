import os
import argparse
from enum import Enum
from pathlib import Path
from pydub import AudioSegment

class AudioFormat(Enum):
    F32LE = ("f32le", 32)
    U16LE = ("u16le", 16)
    S16LE = ("s16le", 16)

    def __init__(self, ffmpeg_name: str, bits: int):
        self._ffmpeg_name = ffmpeg_name
        self._bits = bits

    @property
    def ffmpeg_name(self) -> str:
        return self._ffmpeg_name

    @property
    def bits(self) -> int:
        return self._bits

    @property
    def sample_width(self) -> int:
        return self._bits // 8

def process_folder(folder, audio_format):
    parent = folder.name.lower().replace(" ", "_")
    idx = 0
    for file in folder.iterdir():
        if file.is_file() and file.suffix.lower() in ('.wav', '.mp3', '.flac'):
            outname_pcm = f"{parent}_{idx}.pcm"
            outpath_pcm = folder / outname_pcm
            outname_wav = f"{parent}_{idx}.wav"
            outpath_wav = folder / outname_wav

            audio = AudioSegment.from_file(file)
            audio = audio.set_channels(2).set_sample_width(audio_format.sample_width).set_frame_rate(44100)

            print(f"Processing {file} -> {outpath_pcm}")
            print(f"  Channels: {audio.channels}, Sample Width: {audio.sample_width}, Frame Rate: {audio.frame_rate}")

            pcm_data = audio.export(format=audio_format.ffmpeg_name).read()

            # Create metadata
            # 7.031 sec -> 310079 samples
            metadata = b"|".join([
                f"name:{outname_pcm}".encode(),
                b"samplerate:44100",
                f"channels:{audio.channels}".encode(),
                f"samplewidth:{audio.sample_width}".encode(),
                # b"sampleformat:float32",
                f"frames:{int(audio.frame_count())}".encode(),
            ]) + b"PCM DATA" + b"\n"

            with open(outpath_pcm, "wb") as f:
                f.write(metadata)
                f.write(pcm_data)
            with open(outpath_wav, "wb") as f:
                f.write(audio.export(format="wav").read())

            idx += 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create sound database with PCM files")
    parser.add_argument("--path", type=Path, default=Path("../sound"),
                        help="Root directory to process (default: ../sound)")
    parser.add_argument("--format", type=str, default="F32LE",
                        help="Audio format: F32LE or S16LE (default: F32LE)")
    parser.add_argument("--dest", type=Path, default=None,
                        help="Destination directory to copy PCM files (optional)")
    
    args = parser.parse_args()
    
    # Validate and get audio format
    try:
        audio_format = AudioFormat[args.format.upper()]
    except KeyError:
        print(f"Error: Invalid audio format '{args.format}'. Must be one of: {', '.join([f.name for f in AudioFormat])}")
        exit(1)
    
    root_dir = args.path
    
    for dirpath, dirnames, filenames in os.walk(root_dir):
        p = Path(dirpath)
        # Only process folders containing sound files, skip root
        if p != root_dir and any(f.suffix.lower() in ('.wav', '.mp3', '.flac') for f in p.iterdir()):
            process_folder(p, audio_format)
    
    # Copy PCM files to destination if specified
    if args.dest:
        args.dest.mkdir(parents=True, exist_ok=True)
        for pcm_file in root_dir.glob("**/*.pcm"):
            import shutil
            shutil.copy2(pcm_file, args.dest / pcm_file.name)
            print(f"Copied {pcm_file.name} to {args.dest}")
