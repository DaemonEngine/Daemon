/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2017, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
* Neither the name of the Daemon developers nor the
names of its contributors may be used to endorse or promote products
derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#ifndef COMMON_KEY_IDENTIFICATION_H_
#define COMMON_KEY_IDENTIFICATION_H_

#include <unordered_map>

#include "common/Util.h"
#include "common/Serialize.h"
#include "common/String.h"
#include "engine/client/keycodes.h"
#include "engine/qcommon/q_unicode.h"


namespace Keyboard {

// These must be equal to SDL_SCANCODE_xxx. This is enforced by
// assertions in engine/client/key_identification.cpp.
namespace Scancode {
    constexpr int LALT = 226;
    constexpr int RALT = 230;
    constexpr int LSHIFT = 225;
    constexpr int RSHIFT = 229;
    constexpr int LCTRL = 224;
    constexpr int RCTRL = 228;
};

class Key
{
public:
    enum class Kind {
        INVALID,      // No key.
        UNICODE_CHAR, // The key that types this char (layout-dependent)
        KEYNUM,       // keyNum_t values, for virtual keys that aren't characters
                      //    Also used for mouse buttons
        SCANCODE,     // "Physical" keys, these are encouraged over UNICODE_CHAR because SDL is
                      //     inconsistent about whether it knows the Unicode character that a key
                      //     maps to, or just defaults to the QWERTY key at that location
        CONSOLE,      // Hack to integrate with existing code
    };
private:
    Kind kind_;
    int id_;
    constexpr Key(Kind kind, int id): kind_(kind), id_(id) { }

public:
    static const Key NONE;

    constexpr Key(): Key(Kind::INVALID, 0) { }
    explicit Key(keyNum_t key) {
        if (key <= 0 || key >= MAX_KEYS)
            *this = Key();
        else
            *this = Key(Kind::KEYNUM, Util::ordinal<keyNum_t>(key));
    }
    friend struct Util::SerializeTraits<Key>;

    static Key FromScancode(int scancode) {
        if (scancode > 0 /* SDL_SCANCODE_UNKNOWN */
            && scancode < 512 /* SDL_NUM_SCANCODES */) {
            return Key(Kind::SCANCODE, scancode);
        }
        return NONE;
    }
    static Key FromCharacter(int codePoint) {
        if (codePoint >= MIN_PRINTABLE_ASCII && codePoint <= UNICODE_MAX_CODE_POINT
            && !Q_Unicode_IsPrivateUse(codePoint)) {
            return Key(Kind::UNICODE_CHAR, Str::ctolower(codePoint));
        }
        return NONE;
    }
    int AsScancode() const {
        return kind_ == Kind::SCANCODE ? id_ : 0;
    }
    int AsCharacter() const {
        return kind_ == Kind::UNICODE_CHAR ? id_ : 0;
    }
    keyNum_t AsKeynum() const {
        return kind_ == Kind::KEYNUM ? Util::enum_cast<keyNum_t>(id_) : K_NONE;
    }

    Kind kind() const {
        return kind_;
    }
    bool operator==(Key other) const {
        return kind_ == other.kind_ && id_ == other.id_;
    }
    bool operator!=(Key other) const {
        return !(*this == other);
    }
    bool IsValid() const {
        return IsBindable() || kind_ == Kind::CONSOLE;
    }
    bool IsBindable() const {
        return kind_ == Kind::UNICODE_CHAR || kind_ == Kind::KEYNUM || kind_ == Kind::SCANCODE;
    }

    struct hash {
        size_t operator()(Key k) const {
            return std::hash<uint64_t>()(
                Util::ordinal<Kind, uint64_t>(k.kind_) << 32 | uint64_t(k.id_));
        }
    };

    int PackIntoInt() const {
        return Util::ordinal(kind_) << 24 | id_;
    }
    static Key UnpackFromInt(int value) {
        return Key(Util::enum_cast<Kind>(value >> 24), value & 0xFFFFFF);
    }

    // TODO(slipher): remove need for all of the following
    int AsLegacyInt() const {
        if (kind_ == Kind::KEYNUM || (kind_ == Kind::UNICODE_CHAR && id_ < 127))
            return id_;
        else
            return K_NONE;
    }
    static Key FromLegacyInt(int k) {
        if (MIN_PRINTABLE_ASCII <= k && k <= MAX_PRINTABLE_ASCII)
            return FromCharacter(k);
        if (k > 0 && k < keyNum_t::MAX_KEYS)
            return Key(Util::enum_cast<keyNum_t>(k));
        return NONE;
    }
};

// Returns either a special name (e.g. "SPACE") or just the character UTF8-encoded.
std::string CharToString(int ch);

// Gets the string associated with a keyNum_t, e.g. "F2" or "CAPSLOCK".
std::string KeynumToString(keyNum_t keynum);

// ';' -> "SEMICOLON" etc.
extern const std::unordered_map<char, Str::StringRef> SPECIAL_CHARACTER_NAMES;

// "SHIFT" -> K_SHIFT etc.
extern const std::unordered_map<Str::StringRef, keyNum_t, Str::IHash, Str::IEqual> keynames;

// "hw:LSHIFT" -> Scancode::LSHIFT etc.
struct ScancodeName {
	Str::StringRef name;
	int scancode;
};
extern const std::vector<ScancodeName> leftRightFunctionKeys;

} // namespace Keyboard


namespace Util {
template<> struct SerializeTraits<Keyboard::Key> {
    static void Write(Writer& stream, Keyboard::Key value)
    {
        stream.Write<Keyboard::Key::Kind>(value.kind_);
        stream.Write<int>(value.id_);
    }
    static Keyboard::Key Read(Reader& stream)
    {
        using Keyboard::Key;
        auto kind = stream.Read<Key::Kind>();
        auto id = stream.Read<int>();
        switch (kind) {
        case Key::Kind::UNICODE_CHAR:
            return Key::FromCharacter(id);
        case Key::Kind::SCANCODE:
            return Key::FromScancode(id);
        case Key::Kind::KEYNUM:
            return Key(Util::enum_cast<keyNum_t>(id));
        default:
            return Key::NONE;
        }
    }
};
}

#endif // COMMON_KEY_IDENTIFICATION_H_
