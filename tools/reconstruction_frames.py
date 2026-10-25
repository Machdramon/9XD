"""Create a bounded frame-sampling job for later COLMAP reconstruction."""
import argparse
import json
import pathlib
import subprocess

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('video', type=pathlib.Path)
    parser.add_argument('output', type=pathlib.Path)
    parser.add_argument('--ffmpeg', required=True, type=pathlib.Path)
    parser.add_argument('--ffprobe', required=True, type=pathlib.Path)
    parser.add_argument('--fps', type=float, default=1)
    parser.add_argument('--max-frames', type=int, default=300)
    parser.add_argument('--max-size', type=int, default=1600)
    args = parser.parse_args()
    if not args.video.is_file():
        parser.error('Video does not exist')
    if not 0.05 <= args.fps <= 10 or not 2 <= args.max_frames <= 10000 or not 256 <= args.max_size <= 4096:
        parser.error('Invalid sampling budget')
    # Refuse existing output to preserve user work and avoid mixing stale frames.
    args.output.mkdir(parents=True, exist_ok=False)
    frames = args.output / 'images'
    frames.mkdir()
    manifest = {'schema': 1, 'state': 'inspecting', 'source': str(args.video.resolve()),
                'parameters': {'fps': args.fps, 'max_frames': args.max_frames, 'max_size': args.max_size},
                'commands': []}
    def save():
        (args.output / 'job.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    save()
    try:
        probe = [str(args.ffprobe), '-v', 'error', '-show_format', '-show_streams', '-of', 'json', str(args.video.resolve())]
        result = subprocess.run(probe, check=True, capture_output=True, text=True, timeout=60)
        manifest['input'] = json.loads(result.stdout)
        manifest['ffmpeg_version'] = subprocess.run([str(args.ffmpeg), '-version'], check=True, capture_output=True, text=True, timeout=15).stdout.splitlines()[0]
        manifest['state'] = 'extracting'
        command = [str(args.ffmpeg), '-nostdin', '-v', 'error', '-n', '-i', str(args.video.resolve()),
                   '-map', '0:v:0', '-vf', f'fps={args.fps},scale={args.max_size}:{args.max_size}:force_original_aspect_ratio=decrease:force_divisible_by=2',
                   '-frames:v', str(args.max_frames), str(frames / 'frame-%06d.png')]
        manifest['commands'] = [probe, command]
        save()
        with (args.output / 'extraction.log').open('w', encoding='utf-8') as log:
            subprocess.run(command, check=True, stdout=log, stderr=log, timeout=3600)
        manifest['frame_count'] = len(list(frames.glob('frame-*.png')))
        if manifest['frame_count'] < 2:
            raise RuntimeError('At least two frames are required')
        manifest['state'] = 'frames_ready'
    except (Exception, KeyboardInterrupt) as error:
        manifest['state'] = 'canceled' if isinstance(error, KeyboardInterrupt) else 'failed'
        manifest['error'] = str(error)
        raise
    finally:
        save()
    print(json.dumps({'state': manifest['state'], 'frames': manifest['frame_count']}))

if __name__ == '__main__':
    main()
