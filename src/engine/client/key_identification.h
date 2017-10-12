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
#ifndef ENGINE_CLIENT_KEY_IDENTIFICATION_H_
#define ENGINE_CLIENT_KEY_IDENTIFICATION_H_

#include "common/Util.h"
#include "engine/client/keycodes.h"
#include "engine/qcommon/q_unicode.h"
#include "engine/framework/CommandSystem.h"


namespace Keyboard {
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
    static const Key CONSOLE;

    constexpr Key(): Key(Kind::INVALID, 0) { }
    explicit Key(keyNum_t key) {
        if (key <= 0 || key >= MAX_KEYS)
            *this = Key();
        else if (key == K_CONSOLE)
            *this = CONSOLE;
        else
            *this = Key(Kind::KEYNUM, Util::ordinal<keyNum_t>(key));
    }
    static Key FromScancode(int scancode);
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
        return kind_ != Kind::INVALID;
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

    // TODO(slipher): remove need for all of the following
    int AsLegacyInt() const {
        if (kind_ == Kind::CONSOLE)
            return Util::ordinal(K_CONSOLE);
        else if (kind_ == Kind::KEYNUM || (kind_ == Kind::UNICODE_CHAR && id_ < 127))
            return id_;
        else
            return K_NONE;
    }
    static Key FromLegacyInt(int k) {
        if (k == K_CONSOLE)
            return CONSOLE;
        if (MIN_PRINTABLE_ASCII <= k && k <= MAX_PRINTABLE_ASCII)
            return FromCharacter(k);
        if (k > 0 && k < keyNum_t::MAX_KEYS)
            return Key(Util::enum_cast<keyNum_t>(k));
        return NONE;
    }
};


Key StringToKey(Str::StringRef name);
std::string KeyToString(Key key);

void CompleteKeyName(Cmd::CompletionResult& completions, Str::StringRef prefix);

} // namespace Keyboard

#endif // ENGINE_CLIENT_KEY_IDENTIFICATION_H_
