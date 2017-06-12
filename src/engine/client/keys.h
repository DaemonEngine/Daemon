/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef ENGINE_CLIENT_KEYS_H_
#define ENGINE_CLIENT_KEYS_H_

#include "common/Common.h"

#include "keycodes.h"

#include "framework/ConsoleField.h"
#include "qcommon/q_unicode.h"

#define MAX_TEAMS 4
#define DEFAULT_BINDING 0

struct qkey_t
{
    bool down;
    int      repeats; // if > 1, it is autorepeating
    char     *binding[ MAX_TEAMS ];
};

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
    Key(keyNum_t key) {
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
            return (int) K_CONSOLE;
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

} // namespace Keyboard

extern bool key_overstrikeMode;
extern std::unordered_map<Keyboard::Key, qkey_t, Keyboard::Key::hash> keys;
extern int      bindTeam;

void            Field_KeyDownEvent(Util::LineEditData& edit, Keyboard::Key key );
void            Field_CharEvent(Util::LineEditData& edit, int c );
void            Field_Draw(const Util::LineEditData& edit, int x, int y,
                           bool showCursor, bool noColorEscape, float alpha );

extern Console::Field  g_consoleField;
extern int      anykeydown;

void            Key_WriteBindings( fileHandle_t f );
void            Key_SetBinding( Keyboard::Key key, int team, const char *binding );
const char      *Key_GetBinding( Keyboard::Key key, int team );
bool        Key_IsDown( Keyboard::Key key );
void            Key_ClearStates();

void            Key_SetTeam( int newTeam );
int             Key_GetTeam( const char *arg, const char *cmd );

bool AnyKeyDown();

#endif // ENGINE_CLIENT_KEYS_H_
