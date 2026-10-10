// Combat gates from OoTMM v32.3 data/world/mm/dungeons/moon.yml.
// Commit 413573d22d1d53e8a81931a70927f625b73488b9; MIT, OoTMM Team.
// Soul ownership is checked by the native caller.
/*
MIT License

Copyright (c) 2020-2022 OoTMM Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
namespace ComboRando::MmMoonTrialRules {
inline bool Dinolfos(bool sword, bool goron, bool zora, bool bubble, bool arrows) {
    return sword || goron || zora || bubble || arrows;
}
inline bool Garo(bool sword, bool goron, bool bubble, bool arrows) {
    return sword || goron || bubble || arrows;
}
inline bool IronKnuckle(bool sword, bool goron, bool bombs, bool bombchus, bool blastMask, unsigned masks) {
    return sword || goron || bombs || bombchus || (blastMask && masks >= 5);
}
}
