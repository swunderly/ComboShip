"""Where the launcher and the room window use the co-op seed identity (combo/network/ComboAnchorProtocol.h).

Static scan of combo/ComboShip.cpp and combo/gui/ComboAnchorRoomWindow.cpp. multiplayer_protocol_tests.cpp
runs SeedIdentity, StampClientState and TeammateWarning themselves. A Wallmaster corpus seed comes from the
headless generator, which writes no displaySeed; corpus rows share master seeds across start contexts. So the
slot's sync seed, the journal commit and the icons of an imported layout seed all take SeedIdentity, and two
different layout seeds never sync (M1 plan, critic correction C5). Both games' client states carry that
identity, and the room window tells a teammate on another seed or version why nothing is shared, in plain
sentences and across games.
"""
from pathlib import Path
import argparse
import re
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--source', type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
sys.path.insert(0, str(source/'ComboAndroid/tests'))
import hostcxx


def read(relative):
    return (source/relative).read_text(encoding='utf-8')


def body(text, signature):
    """The braced body of the production function that starts at signature."""
    start = hostcxx.find(text, signature)
    opening = text.index('{', start)
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: ' ' * len(m.group()), text)
    depth = 0
    for end in range(opening, len(text)):
        depth += (masked[end] == '{') - (masked[end] == '}')
        if depth == 0:
            return text[start:end + 1]
    raise AssertionError('Unterminated production block ' + signature)


def require(condition, message):
    if not condition:
        sys.exit('FAIL: ' + message)


def ordered(text, *needles):
    start = 0
    for needle in needles:
        start = hostcxx.find(text, needle, start) + 1


ship = read('combo/ComboShip.cpp')
# One identity rule: no launcher path derives it from displaySeed/masterSeed by itself.
require(not hostcxx.contains(ship, 'rando.value("displaySeed"'), 'a launcher path reads displaySeed directly')
require(hostcxx.contains(body(ship, 'static void LoadComboCompletion(int slot) {'),
                         'ComboAnchor::sSyncSeed = ComboAnchorProtocol::SeedIdentity(rando);'),
        'the slot sync seed is not the seed identity')
require(hostcxx.contains(body(ship, 'static void Combo_WriteGameSave(int game, int fileNum, const char* json) {'),
                         'journal.value("seed", 0u) == ComboAnchorProtocol::SeedIdentity(rando)'),
        'the Anchor journal is committed against another seed than the sync seed')
reload_ = body(ship, 'static int Combo_OnReloadRequestImpl(const char* path, const nlohmann::json* profileSeed = nullptr) {')
combined = reload_[hostcxx.find(reload_, 'const auto catalog ='):]
try:
    ordered(combined, 'uint32_t displaySeed = j.contains("entranceLayout") ? ComboAnchorProtocol::SeedIdentity(j) '
                      ': j.value("displaySeed", 0u);',
            'SOH_SetComboSeedHash(displaySeed);', 'g_ConsolidatedJson = j.dump(2);', 'g_FinalizeDisplaySeed = displaySeed;')
except ValueError as error:
    sys.exit(f'FAIL: an imported layout seed does not show its identity, or is not kept as read: {error}')
require(not re.search(r'\bj\s*\[\s*"displaySeed"\s*\]\s*=', reload_), 'the import writes a displaySeed into the seed')

# Both games' client states go through Send; each reports the slot's identity.
send = body(ship, 'static void Send(const char* json) {')
for packet, field in (('HANDSHAKE', 'clientState'), ('UPDATE_CLIENT_STATE', 'state')):
    require(hostcxx.contains(send, f'if (type == "{packet}") ComboAnchorProtocol::StampClientState(p["{field}"], '
                                   'sSyncSeed.load());'), f'{packet} does not report the sync seed')
require(len(re.findall(r'\["seed"\]\s*=', send)) == 0, 'Send writes a seed outside StampClientState')
pump = body(ship, 'static void PumpSync() {')
require(hostcxx.contains(pump, 'p.value("comboSeed", 0u) != sSyncSeed.load()') and
        hostcxx.contains(pump, 'ComboAnchorProtocol::Expand(p, sSyncSeed.load())'),
        'progress or team state is no longer refused for another seed')
roster = body(ship, 'static const char* Combo_Anchor_GetRoster() {')
for field in ('seed', 'isSaveLoaded', 'teamId', 'clientVersion', 'online', 'self'):
    require(hostcxx.contains(roster, f'e["{field}"] ='), f'the roster no longer gives the room window {field}')
require(hostcxx.contains(ship, 'info.seed = c.value("seed", (uint32_t)0);'), 'the roster no longer reads seeds')

room = read('combo/gui/ComboAnchorRoomWindow.cpp')
draw = body(room, 'void ComboAnchorRoomWindow::DrawElement() {')
require(hostcxx.contains(room, '#include "network/ComboAnchorProtocol.h"') and
        hostcxx.contains(draw, 'const std::string warning = ComboAnchorProtocol::TeammateWarning(selfRow, c);') and
        hostcxx.contains(draw, 'ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.7f, 1.0f), "%s", warning.c_str());'),
        'the room window does not show the plain refusal as text')
for old in ('Continuing will break things', 'Will not work together', 'peerGame == selfGame', 'SetItemTooltip("Seed'):
    require(old not in draw, f'the room window still has {old!r}')
print('PASS: one seed identity for sync, journal and icons; both games report it; the room window explains a '
      'refusal in plain sentences across games')
