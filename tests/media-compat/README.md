# Media compatibility smoke test

This directory contains a small FFmpeg-based smoke test for movie and audio
formats that are useful for DJV review workflows. The test generates tiny
synthetic media files, probes their streams with `ffprobe`, then decodes one
video frame and a short audio slice with `ffmpeg`.

The test is dependency-focused: it verifies that the FFmpeg toolchain can
create, identify, and decode candidate formats before they are promoted as DJV
compatibility targets. It does not replace an end-to-end DJV playback test.

Run it directly:

```sh
python tests/media-compat/media_compat.py --output-dir tests/_media_compat
```

Run a subset:

```sh
python tests/media-compat/media_compat.py --case mov_prores_pcm --case mxf_dnxhr_pcm
```

When DJV is configured with CMake and `ffmpeg`, `ffprobe`, and Python are
available, the same check is registered as:

```sh
ctest -R media_compat_ffmpeg_smoke
```

The generated files are intentionally not committed. The smoke matrix currently
covers:

* MP4: H.264 video with AAC audio
* M4V and MOV: H.264/AAC
* MOV: ProRes/PCM
* MXF: DNxHR/PCM
* MKV: H.264/AAC and AV1/Opus
* WebM: VP9/Opus
* MP4: HEVC/AAC
* Audio-only: M4A/AAC, ADTS AAC, FLAC, Opus, MP3, WAV/PCM, AIFF/PCM
