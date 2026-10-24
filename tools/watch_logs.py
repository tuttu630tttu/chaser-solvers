#!/usr/bin/env python3
"""Rename completed battle JSON downloads. Stop with Ctrl+C."""
import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import re
import time
from zoneinfo import ZoneInfo

ROOT = Path(__file__).resolve().parents[1]
NAMED = re.compile(r'^\d{8}_\d{6}_[^_]+_[^_]+_[^_]+_\d+_\d+\.json$')


def label(value):
    return re.sub(r'[^\w.\-]+', '-', str(value), flags=re.UNICODE).replace('_', '-').strip('.-') or 'unknown'


def target_name(data, config):
    settings = data['settings']
    players = {int(p['index']): p for p in settings['players']}
    names = config.get('programs', {})
    def player(index):
        p = players[index]
        key = str(p['programId'])
        return label(names.get(key, f'program-{key}'))
    board_id = str(settings['boardId'])
    board = config.get('boards', {}).get(board_id)
    if board is None:
        initial = data['response']['log'][0]['board']
        for path in sorted((ROOT / 'mapjson').glob('*.json')):
            layout = json.loads(path.read_text(encoding='utf-8'))['layout']
            if [row.replace('C', '0').replace('H', '0') for row in layout] == initial:
                board = path.stem
                break
    stamp = datetime.fromisoformat(data['executedAt'].replace('Z', '+00:00'))
    if stamp.tzinfo is None:
        raise ValueError('executedAt has no timezone')
    date = stamp.astimezone(ZoneInfo('Asia/Tokyo')).strftime('%Y%m%d_%H%M%S')
    turns = int(settings['turn'])
    return f'{date}_{player(0)}_{player(1)}_{label(board or f"board-{board_id}")}_{turns}.json'


def rename_log(path, config):
    data = json.loads(path.read_text(encoding='utf-8'))
    # Other JSON files (e.g. maps) are left untouched.
    if not isinstance(data, dict) or not {'settings', 'response', 'executedAt'} <= data.keys():
        return None
    base = Path(target_name(data, config)).stem
    condition = base.split('_', 2)[2]
    pattern = re.compile(r'^\d{8}_\d{6}_' + re.escape(condition) + r'_(\d+)\.json$')
    # Continue across dates and process restarts; never reuse a remaining number.
    suffix = 1
    for existing in path.parent.glob('*.json'):
        match = pattern.fullmatch(existing.name)
        if match:
            suffix = max(suffix, int(match[1]) + 1)
    while True:
        target = path.with_name(f'{base}_{suffix}.json')
        try:
            # Exclusive creation prevents overwriting an existing log.
            os.link(path, target)
            break
        except FileExistsError:
            suffix += 1
    path.unlink()
    return target


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--once', action='store_true', help='Process existing files once, without watching')
    parser.add_argument('--directory', type=Path, default=ROOT / 'logs')
    args = parser.parse_args()
    directory = args.directory.resolve()
    directory.mkdir(parents=True, exist_ok=True)
    states = {}
    print(f'Watching {directory} (Ctrl+C to stop)', flush=True)
    while True:
        config = json.loads((ROOT / 'tools/log_names.json').read_text(encoding='utf-8'))
        now = time.monotonic()
        for path in sorted(directory.glob('*.json')):
            if NAMED.match(path.name) or path.is_symlink():
                continue
            try:
                stat = path.stat()
                signature = (stat.st_size, stat.st_mtime_ns)
                old = states.get(path)
                if old is None or old[0] != signature:
                    states[path] = (signature, now, False)
                    if not args.once:
                        continue
                elif old[2] or now - old[1] < 2:
                    continue
                target = rename_log(path, config)
                if target:
                    print(f'{path.name} -> {target.name}', flush=True)
                    states.pop(path, None)
                else:
                    states[path] = (signature, now, True)
            except (OSError, ValueError, KeyError, TypeError, IndexError) as error:
                # Incomplete downloads are retried; report each unchanged file once.
                old = states.get(path)
                if old and not old[2]:
                    print(f'Waiting/skipping {path.name}: {error}', flush=True)
                    states[path] = (old[0], old[1], True)
        if args.once:
            break
        time.sleep(1)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\nStopped.', flush=True)
