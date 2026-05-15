# Media compatibility smoke matrix

DJV uses FFmpeg for movie and audio playback. The smoke test in
`tests/media-compat` generates small synthetic files, probes their streams, and
decodes a video frame or short audio slice. This validates the FFmpeg media path
before a format is promoted as a DJV compatibility target.

The smoke test does not replace end-to-end playback validation in DJV. It is a
fast dependency check for the containers and codecs most relevant to review
workflows.

| Case | Extension | Video | Audio | Notes |
| --- | --- | --- | --- | --- |
| `mp4_h264_aac` | `.mp4` | H.264 | AAC | Common camera, phone, and web export format. |
| `mov_h264_aac` | `.mov` | H.264 | AAC | Common QuickTime-style review export. |
| `m4v_h264_aac` | `.m4v` | H.264 | AAC | MP4-family delivery variant. |
| `mov_prores_pcm` | `.mov` | ProRes | PCM | Common editorial and finishing interchange. |
| `mxf_dnxhr_pcm` | `.mxf` | DNxHR | PCM | Common post-production and broadcast container. |
| `webm_vp9_opus` | `.webm` | VP9 | Opus | Common open web media combination. |
| `mkv_h264_aac` | `.mkv` | H.264 | AAC | Common general-purpose container. |
| `mp4_hevc_aac` | `.mp4` | HEVC | AAC | Build-dependent codec; useful for modern cameras and phones. |
| `mkv_av1_opus` | `.mkv` | AV1 | Opus | Build-dependent codec; useful for open modern delivery. |
| `m4a_aac` | `.m4a` | - | AAC | Common audio-only MPEG-4 container. |
| `aac_adts` | `.aac` | - | AAC | Raw ADTS AAC stream. |
| `flac_audio` | `.flac` | - | FLAC | Lossless audio. |
| `opus_audio` | `.opus` | - | Opus | Open low-latency audio. |
| `mp3_audio` | `.mp3` | - | MP3 | Common legacy audio. |
| `wav_pcm` | `.wav` | - | PCM | Common uncompressed audio. |
| `aiff_pcm` | `.aiff` | - | PCM | Common uncompressed audio. |

Run the full smoke test:

```sh
python tests/media-compat/media_compat.py --output-dir tests/_media_compat
```

Run selected cases:

```sh
python tests/media-compat/media_compat.py --case mov_prores_pcm --case mxf_dnxhr_pcm
```
