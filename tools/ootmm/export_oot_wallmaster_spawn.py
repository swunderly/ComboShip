"""Preserve the exact v32.3 OoT room-clear and pull-up transition hooks."""
import hashlib
from export_mm_wallmaster_spawn import function
from export_registry import PIN, arguments, json_bytes, publish, verified_blob

SOURCE = 'packages/generator/src/oot/actors.c'
ACTOR = 'packages/generator/src/oot/actors/En/En_Wallmas.c'
HOOK = 'packages/generator/src/oot/actors/En/En_Wallmas.S'
OUTPUT = 'tools/ootmm/oot-wallmaster-spawn-source.json'


def build(reference):
    actor_spawn, actor, hook = [verified_blob(reference, path) for path in (SOURCE, ACTOR, HOOK)]
    return {'revision': PIN, 'sources': [
        {'path': path, 'sha256': hashlib.sha256(blob).hexdigest()}
        for path, blob in ((SOURCE, actor_spawn), (ACTOR, actor), (HOOK, hook))],
        'roomClearFunction': function(actor_spawn.decode(), 'static int GetRoomClearFlagForActor('),
        'transportFunction': function(actor.decode(), 'void EnWallmas_TakePlayerEr('),
        'transportHook': hook.decode()}


if __name__ == '__main__':
    args = arguments(__doc__)
    publish(args.output_root, {OUTPUT: json_bytes(build(args.reference))}, args.check)
