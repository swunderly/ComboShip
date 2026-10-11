"""Preserve the exact v32.3 MM room-clear hook and its soul-gated caller."""
import hashlib
from export_registry import PIN, arguments, json_bytes, publish, verified_blob

SOURCE = 'packages/generator/src/mm/actors.c'
OUTPUT = 'tools/ootmm/mm-wallmaster-spawn-source.json'


def function(text, signature):
    if text.count(signature) != 1:
        raise ValueError(f'Expected one {signature}')
    start = text.index(signature)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise ValueError('Unterminated pinned MM actor function')


def parse_source(blob):
    text = blob.decode('utf-8')
    return {
        'revision': PIN,
        'source': SOURCE,
        'sourceSha256': hashlib.sha256(blob).hexdigest(),
        'roomClearFunction': function(text, 'static int GetRoomClearFlagForActor('),
        'spawnWrapperFunction': function(text, 'Actor *Actor_SpawnAsChildAndCutsceneWrapper('),
    }


def main():
    args = arguments(__doc__)
    publish(args.output_root, {OUTPUT: json_bytes(parse_source(verified_blob(args.reference, SOURCE)))}, args.check)


if __name__ == '__main__':
    main()
