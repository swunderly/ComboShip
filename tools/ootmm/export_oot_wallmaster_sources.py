"""Extract the private OoT Wallmaster source classifier from the exact v32.3 Git blob."""
import hashlib
import re
from export_registry import PIN, arguments, json_bytes, publish, read_registry, verified_blob

SOURCE = 'packages/generator/src/oot/actors/En/En_Wallmas.c'
OUTPUT = 'tools/ootmm/oot-wallmaster-source.json'
SCENES = {
    'SCE_OOT_TEMPLE_FOREST': 'ForestTemple',
    'SCE_OOT_TEMPLE_SHADOW': 'ShadowTemple',
    'SCE_OOT_TEMPLE_SPIRIT': 'SpiritTemple',
    'SCE_OOT_BOTTOM_OF_THE_WELL': 'BottomOfTheWell',
    'SCE_OOT_GERUDO_TRAINING_GROUND': 'GerudoTrainingGround',
    'SCE_OOT_INSIDE_GANON_CASTLE': 'InsideGanonsCastle',
}


def function_source(original):
    text = original.decode('utf-8')
    marker = 'static int EnWallmas_GetDestEr('
    if text.count(marker) != 1:
        raise ValueError('Expected exactly one EnWallmas_GetDestEr definition')
    start = text.index(marker)
    opening = text.index('{', start)
    depth = 0
    for end in range(opening, len(text)):
        depth += (text[end] == '{') - (text[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise ValueError('Unterminated Wallmaster source function')


class SourceParser:
    """Deliberately small C grammar; every token of the source function is consumed."""
    def __init__(self, text):
        # The pinned function has no comments; whitespace changes are harmless.
        self.tokens = re.findall(r'0x[0-9a-fA-F]+|[A-Za-z_]\w*|\d+|==|->|[{}()*;:,=?]', text)
        if ''.join(self.tokens) != re.sub(r'\s+', '', text):
            raise ValueError('Unsupported Wallmaster source token')
        self.pos = 0

    def peek(self):
        return self.tokens[self.pos] if self.pos < len(self.tokens) else None

    def take(self):
        value = self.peek()
        if value is None:
            raise ValueError('Unexpected end of Wallmaster source')
        self.pos += 1
        return value

    def expect(self, *tokens):
        for token in tokens:
            if self.take() != token:
                raise ValueError(f'Unexpected Wallmaster source syntax; expected {token}')

    def room(self):
        token = self.take()
        if not re.fullmatch(r'0x[0-9a-fA-F]+|0|[1-9][0-9]*', token):
            raise ValueError('Expected literal room')
        value = int(token, 16 if token.startswith('0x') else 10)
        if not 0 <= value <= 255:
            raise ValueError('Room exceeds Actor.room range')
        return value

    def destination(self):
        token = self.take()
        if not re.fullmatch(r'ENTR_OOT_WALLMASTER_[A-Z0-9_]+', token):
            raise ValueError('Expected OoT Wallmaster source constant')
        return token[len('ENTR_'):]

    def terminal(self):
        if self.peek() == 'UNREACHABLE':
            self.expect('UNREACHABLE', '(', ')', ';')
            return None
        self.expect('return')
        result = self.destination()
        self.expect(';')
        return result

    def rule(self):
        rooms = {}
        if self.peek() == 'switch':
            self.expect('switch', '(', 'room', ')', '{')
            while self.peek() == 'case':
                self.expect('case')
                room = self.room()
                self.expect(':')
                if room in rooms:
                    raise ValueError('Duplicate actor room')
                rooms[room] = self.terminal()
            self.expect('default', ':')
            default = self.terminal()
            self.expect('}')
        else:
            self.expect('return')
            if self.peek() == 'room':
                self.expect('room', '==')
                room = self.room()
                self.expect('?')
                rooms[room] = self.destination()
                self.expect(':')
            default = self.destination()
            self.expect(';')
        return {'rooms': {str(k): v for k, v in rooms.items()}, 'default': default}

    def parse(self):
        self.expect('static', 'int', 'EnWallmas_GetDestEr', '(', 'Actor', '*', 'this', ',',
                    'PlayState', '*', 'play', ')', '{', 'u8', 'room', ';', 'room', '=',
                    'this', '->', 'room', ';', 'switch', '(', 'play', '->', 'sceneId', ')', '{')
        result = []
        seen = set()
        while self.peek() == 'case':
            self.expect('case')
            scene = self.take()
            if scene not in SCENES or scene in seen:
                raise ValueError('Unknown or duplicate scene')
            seen.add(scene)
            self.expect(':')
            result.append({'scene': scene, 'context': SCENES[scene], **self.rule()})
        self.expect('default', ':', 'UNREACHABLE', '(', ')', ';', '}', '}')
        if self.peek() is not None or seen != set(SCENES):
            raise ValueError('Trailing tokens or incomplete scene coverage')
        return result


def parse_source(original, records):
    function = function_source(original)
    rules = SourceParser(function).parse()
    names = {name for rule in rules for name in [rule['default'], *rule['rooms'].values()] if name}
    sources = [r for r in records if r['name'] in names]
    if len(names) != 13 or len(sources) != 13:
        raise ValueError('Expected 13 registry source records')
    if any(r['game'] != 'oot' or r['type'] != 'wallmaster' or r['reverse'] or r['to'] != 'OOT VOID'
           for r in sources):
        raise ValueError('Invalid Wallmaster source metadata')
    return {'upstreamCommit': PIN, 'source': SOURCE,
            'sourceSha256': hashlib.sha256(original).hexdigest(),
            'functionSource': function, 'rules': rules,
            'sources': [{k: r[k] for k in ('name', 'game', 'id', 'type', 'reverse', 'to')} for r in sources]}


def main():
    args = arguments(__doc__)
    _, records = read_registry(args.reference)
    original = verified_blob(args.reference, SOURCE)
    publish(args.output_root, {OUTPUT: json_bytes(parse_source(original, records))}, args.check)
    print('Verified 13 OoT Wallmaster sources from the exact v32.3 runtime blob')


if __name__ == '__main__':
    main()
