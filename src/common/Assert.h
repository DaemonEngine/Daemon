/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef COMMON_ASSERT_H_
#define COMMON_ASSERT_H_

/*
 * Daemon asserts to be used instead of the regular C stdlib assert function (if you don't
 * use assert yet, you should start now!). In debug ASSERT(condition) will trigger an error,
 * otherwise in release it does nothing at runtime.
 *
 * If possible use the ASSERT_(EQ|NQ|LT|LE|GT|GE)(a, b) variants that are respectively
 * equivalent to ASSERT(a OP b) with being ==, !=, <, <=, >, >=. These variants are better
 * because they print the values of the operands in the error message, so you have more
 * information to start debugging when the assert fires.
 *
 * In case of name clashes (with for example a testing library), you can define the
 * DAEMON_SKIP_ASSERT_SHORTHANDS to only define the DAEMON_ prefixed macros. Avoid using
 * the shorthands in header files, so that the headers can be included in tests.
 *
 * These asserts feature:
 *     - Logging of the error with file, line and function information.
 *     - The printing of operands when using the specialized versions.
 *     - Breaking in the debugger when an assert is triggered and a debugger is attached.
 *     - Use the assert information to help the compiler optimizer in release builds.
 */

#ifdef DEBUG_BUILD
#define DAEMON_ASSERTS_ENABLED
#endif

// MSVC triggers a warning in /W4 for do {} while(0). SDL worked around this by using
// (0,0) and points out that it looks like an owl face.
#if defined(_MSC_VER)
    #define DAEMON_ASSERT_LOOP_CONDITION (0,0)
#else
    #define DAEMON_ASSERT_LOOP_CONDITION (0)
#endif

// DAEMON_ASSERT_CALLSITE_HELPER generates the actual assert code. In Debug it does what you would
// expect of an assert and in release it tries to give hints to make the compiler generate better code.
#ifdef DAEMON_ASSERTS_ENABLED
    #define DAEMON_ASSERT_CALLSITE_HELPER(file, func, line, _0, code, condition, message) \
        do { \
            code; \
            if (!(condition)) { \
                std::string msg__ = Str::Format("Assertion failure at %s:%s (%s): %s", file, line, func, message); \
                if (Sys::IsDebuggerAttached()) { \
                    Log::Warn(msg__); \
                    BREAKPOINT(); \
                } \
                Sys::Error(msg__); \
            } \
        } while(DAEMON_ASSERT_LOOP_CONDITION)
#else
    #if defined(_MSC_VER)
        #define DAEMON_ASSERT_CALLSITE_HELPER(file, func, line, directExpression, _1, _2, _3) \
            __assume(directExpression)
    #elif defined(__clang__) && defined(__builtin_assume)
        #define DAEMON_ASSERT_CALLSITE_HELPER(file, func, line, directExpression, _1, _2, _3) \
            __builtin_assume(directExpression)
    #else
        #define DAEMON_ASSERT_CALLSITE_HELPER(file, func, line, directExpression, _1, _2, _3)
    #endif
#endif

/*
 * You can define new ASSERT variants by using DAEMON_ASSERT_CALLSITE:
 *     - directExpression should be the condition expression, evaluating to a bool. It is used
 *       when hinting the compiler optimizer
 *     - code should be used to defined variables that will appear in condition and message. Operands
 *       should be evaluated once here using "auto&& var__ = (op)" so that functions are not called
 *       multiple times
 *     - condition is the same expression as directExpression but using variables defined in code instead
 *     - message should evaluate to a string explaining the error and dumping the variables defined in code.
 * The trailing underscores on variable names are to avoid local variable shadowing warnings.
 */
#define DAEMON_ASSERT_CALLSITE(directExpression, code, condition, message) \
    DAEMON_ASSERT_CALLSITE_HELPER(__FILE__, __func__, __LINE__, directExpression, code, condition, message)

#define DAEMON_ASSERT(condition) DAEMON_ASSERT_CALLSITE( \
        (condition), \
        , \
        (condition), \
        Str::Format("\"%s\" is false", #condition) \
    )

#define DAEMON_ASSERT_EQ(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) == (b), \
        auto&& expected__ = (a); auto&& actual__ = (b), \
        expected__ == actual__, \
        Str::Format("\"%s == %s\" expected: %s, actual: %s", #a, #b, AssertDetail::Printable(expected__), AssertDetail::Printable(actual__)) \
    )
#define DAEMON_ASSERT_NQ(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) != (b), \
        auto&& notExpected__ = (a); auto&& actual__ = (b), \
        notExpected__ != actual__, \
        Str::Format("\"%s != %s\" not expected: %s, actual: %s", #a, #b, AssertDetail::Printable(notExpected__), AssertDetail::Printable(actual__)) \
    )

#define DAEMON_ASSERT_LT(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) < (b), \
        auto&& bound__ = (a); auto&& actual__ = (b), \
        bound__ < actual__, \
        Str::Format("\"%s < %s\" bound: %s, actual: %s", #a, #b, AssertDetail::Printable(bound__), AssertDetail::Printable(actual__)) \
    )
#define DAEMON_ASSERT_LE(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) <= (b), \
        auto&& bound__ = (a); auto&& actual__ = (b), \
        bound__ <= actual__, \
        Str::Format("\"%s <= %s\" bound: %s, actual: %s", #a, #b, AssertDetail::Printable(bound__), AssertDetail::Printable(actual__)) \
    )
#define DAEMON_ASSERT_GT(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) > (b), \
        auto&& bound__ = (a); auto&& actual__ = (b), \
        bound__ > actual__, \
        Str::Format("\"%s > %s\" bound: %s, actual: %s", #a, #b, AssertDetail::Printable(bound__), AssertDetail::Printable(actual__)) \
    )
#define DAEMON_ASSERT_GE(a, b) DAEMON_ASSERT_CALLSITE( \
        (a) >= (b), \
        auto&& bound__ = (a); auto&& actual__ = (b), \
        bound__ >= actual__, \
        Str::Format("\"%s >= %s\" bound: %s, actual: %s", #a, #b, AssertDetail::Printable(bound__), AssertDetail::Printable(actual__)) \
    )

#if !defined(DAEMON_SKIP_ASSERT_SHORTHANDS)
    #define ASSERT DAEMON_ASSERT
    #define ASSERT_EQ DAEMON_ASSERT_EQ
    #define ASSERT_NQ DAEMON_ASSERT_NQ
    #define ASSERT_LT DAEMON_ASSERT_LT
    #define ASSERT_LE DAEMON_ASSERT_LE
    #define ASSERT_GT DAEMON_ASSERT_GT
    #define ASSERT_GE DAEMON_ASSERT_GE
    #define ASSERT_UNREACHABLE DAEMON_ASSERT_UNREACHABLE
#endif

// You can put ASSERT_UNREACHABLE() in places that must never be reached.
#define DAEMON_ASSERT_UNREACHABLE() DAEMON_ASSERT_CALLSITE(false, , false, "Unreachable code hit."); UNREACHABLE();

#ifdef DAEMON_ASSERTS_ENABLED
// This stuff is so that the ASSERT_cc variants can be used on objects that don't have a
// function defined for printing to a stream.
namespace AssertDetail {
    template<typename ...>
    struct CanTinyformatPrintIt {
        static constexpr bool value = false;
    };
    template<typename T>
    struct CanTinyformatPrintIt<T, decltype(std::declval<std::ostream&>() << std::declval<const T&>(), void())> {
        static constexpr bool value = true;
    };

    // Forward the value as-is if the format string library can handle it.
    template<typename T>
    typename std::enable_if<CanTinyformatPrintIt<T, void>::value, const T&>::type
    Printable(const T& value) {
        return value;
    }

    // Specialization for enums so we can see the integer value.
    template<typename T>
    typename std::enable_if<!CanTinyformatPrintIt<T, void>::value && std::is_enum<T>::value, std::string>::type
    Printable(T value) {
        return Str::Format("(%s)%d", typeid(T).name(), Util::ordinal(value));
    }

    // Just send some nonsense string otherwise.
    template<typename T>
    typename std::enable_if<!CanTinyformatPrintIt<T, void>::value && !std::is_enum<T>::value, std::string>::type
    Printable(const T& value) {
        return Str::Format("[%s @ %p]", typeid(T).name(), &value);
    }

} // namespace AssertDetail
#endif // DAEMON_ASSERTS_ENABLED

#endif // COMMON_ASSERT_H_
