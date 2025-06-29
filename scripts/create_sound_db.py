import os
from pathlib import Path
from pydub import AudioSegment

# --- Settings ---
root_dir = Path("../sound")

def process_folder(folder):
    parent = folder.name.lower().replace(" ", "_")
    idx = 0
    for file in folder.iterdir():
        if file.is_file() and file.suffix.lower() in ('.wav', '.mp3', '.flac'):
            outname_pcm = f"{parent}_{idx}.pcm"
            outpath_pcm = folder / outname_pcm
            outname_wav = f"{parent}_{idx}.wav"
            outpath_wav = folder / outname_wav

            audio = AudioSegment.from_file(file)
            audio = audio.set_channels(2).set_sample_width(2).set_frame_rate(44100)

            print(f"Processing {file} -> {outpath_pcm}")
            print(f"  Channels: {audio.channels}, Sample Width: {audio.sample_width}, Frame Rate: {audio.frame_rate}")

            # Create metadata
            metadata = b"|".join([
                f"name:{outname_pcm}".encode(),
                b"samplerate:44100",
                f"channels:{audio.channels}".encode(),
                f"samplewidth:{audio.sample_width}".encode(),
                f"frames:{int(audio.frame_count())}".encode(),
            ]) + b"PCM DATA" + b"\n"

            with open(outpath_pcm, "wb") as f:
                f.write(metadata)
                f.write(audio.raw_data)
            with open(outpath_wav, "wb") as f:
                f.write(audio.export(format="wav").read())

            idx += 1


for dirpath, dirnames, filenames in os.walk(root_dir):
    p = Path(dirpath)
    # Only process folders containing sound files, skip root
    if p != root_dir and any(f.suffix.lower() in ('.wav', '.mp3', '.flac') for f in p.iterdir()):
        process_folder(p)
