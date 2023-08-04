// Tests for tinyformat::format. Most of these are taken from the upstream test:
// https://github.com/c42f/tinyformat/blob/master/tinyformat_test.cpp

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace TinyformatTest {
    namespace {
        // Record only the first error as more failures may be triggered on the way out
        static std::string tfError;
#define TINYFORMAT_ERROR(reason) (tfError.empty() ? (tfError = (reason), 0) : 0)

        // Include inside the namespace to avoid ODR issues
#include "tinyformat/tinyformat.h"

        template<typename... T>
        static std::string Format(const char* fmt, T&&... args)
        {
            tfError.clear();
            std::string result = tinyformat::format(fmt, std::forward<T>(args)...);
            if (!tfError.empty()) {
                ADD_FAILURE() << tfError;
            }
            return result;
        }

        template<typename... T>
        static void ExpectFormatError(std::string messageSubstr, const char* fmt, T&&... args)
        {
            tfError.clear();
            tinyformat::format(fmt, std::forward<T>(args)...);
            if (tfError.empty()) {
                ADD_FAILURE() << "error expected but none emitted with format string: " << fmt;
            } else {
                EXPECT_THAT(tfError, testing::HasSubstr(messageSubstr)) << "with format string: " << fmt;
            }
        }

        TEST(TinyformatTest, Basic)
        {
            EXPECT_EQ(Format("%s", "asdf"), "asdf");
            EXPECT_EQ(Format("%d", 1234), "1234");
            EXPECT_EQ(Format("%i", -5678), "-5678");
            EXPECT_EQ(Format("%o", 012), "12");
            EXPECT_EQ(Format("%u", 123456u), "123456");
            EXPECT_EQ(Format("%x", 0xdeadbeef), "deadbeef");
            EXPECT_EQ(Format("%X", 0xDEADBEEF), "DEADBEEF");
            EXPECT_EQ(Format("%e", 1.23456e10), "1.234560e+10");
            EXPECT_EQ(Format("%E", -1.23456E10), "-1.234560E+10");
            EXPECT_EQ(Format("%f", -9.8765), "-9.876500");
            EXPECT_EQ(Format("%F", 9.8765), "9.876500");
            EXPECT_EQ(Format("%g", 10), "10");
            EXPECT_EQ("7.2", Format("%g", 7.2));
            EXPECT_EQ(Format("%G", 100), "100");
            EXPECT_EQ(Format("%c", 65), "A");
            EXPECT_EQ(Format("%hc", (short)65), "A");
            EXPECT_EQ(Format("%lc", (long)65), "A");
            EXPECT_EQ(Format("%s", "asdf_123098"), "asdf_123098");
            EXPECT_EQ(Format("%%%s", "asdf"), "%asdf");
            EXPECT_EQ(Format("100%%"), "100%");
        }

        TEST(TinyformatTest, PosixNumberedArgumentSpecifiers)
        {
            EXPECT_EQ(Format("%2$d %1$d", 10, 20), "20 10");
            EXPECT_EQ(Format("%1$d", 10, 20), "10");
            EXPECT_EQ(Format("%2$d", 10, 20), "20");

            EXPECT_EQ(Format("%1$*2$.4f", 1234.1234567890, 10), " 1234.1235");
            EXPECT_EQ(Format("%1$10.*2$f", 1234.1234567890, 4), " 1234.1235");
            EXPECT_EQ(Format("%1$*3$.*2$f", 1234.1234567890, 4, 10), " 1234.1235");
            EXPECT_EQ(Format("%1$*2$.*3$f", 1234.1234567890, -10, 4), "1234.1235 ");

            EXPECT_EQ("9.07 foo 9.1", Format("%2$.2f %1$s %2$.1f", "foo", 9.07));
            EXPECT_EQ("    5", Format("%1$*1$s", 5));

            ExpectFormatError("out of range", "%2$d", 1);
            ExpectFormatError("out of range", "%0$d", 1);
            ExpectFormatError("out of range", "%1$.*3$d", 1, 2);
            ExpectFormatError("out of range", "%1$.*0$d", 1, 2);
            ExpectFormatError("out of range", "%3$*4$.*2$d", 1, 2, 3);
            ExpectFormatError("out of range", "%3$*0$.*2$d", 1, 2, 3);
        }

        TEST(TinyformatTest, Flags)
        {
            EXPECT_EQ(Format("%#x", 0x271828), "0x271828");
            EXPECT_EQ(Format("%#o", 0x271828), "011614050");
            EXPECT_EQ(Format("%#f", 3.0), "3.000000");
            EXPECT_EQ(Format("%+d", 3), "+3");
            EXPECT_EQ(Format("%+d", 0), "+0");
            EXPECT_EQ(Format("%+d", -3), "-3");
            EXPECT_EQ(Format("%010d", 100), "0000000100");
            EXPECT_EQ(Format("%010d", -10), "-000000010"); // sign should extend
            EXPECT_EQ(Format("%#010X", 0xBEEF), "0X0000BEEF");
            EXPECT_EQ(Format("% d", 10), " 10");
            EXPECT_EQ(Format("% d", -10), "-10");
            EXPECT_EQ(Format("%+.2d", 3), "+03");
            EXPECT_EQ(Format("%+.2d", -3), "-03");
            EXPECT_EQ(Format("%+ d", 10), "+10"); // '+' overrides ' '
            EXPECT_EQ(Format("% +d", 10), "+10");
            EXPECT_EQ(Format("%-010d", 10), "10        "); // '-' overrides '0'
            EXPECT_EQ(Format("%0-10d", 10), "10        ");
        }

        TEST(TinyformatTest, PrecisionAndWidth)
        {
            EXPECT_EQ(Format("%10d", -10), "       -10");
            EXPECT_EQ(Format("%.4d", 10), "0010");
            EXPECT_EQ(Format("%10.4f", 1234.1234567890), " 1234.1235");
            EXPECT_EQ(Format("%.f", 10.1), "10");
            EXPECT_EQ(Format("%.2s", "asdf"), "as"); // strings truncate to precision
            EXPECT_EQ(Format("%.2s", std::string("asdf")), "as");
            EXPECT_EQ(Format("%*.4f", 10, 1234.1234567890), " 1234.1235");
            EXPECT_EQ(Format("%10.*f", 4, 1234.1234567890), " 1234.1235");
            EXPECT_EQ(Format("%*.*f", 10, 4, 1234.1234567890), " 1234.1235");
            EXPECT_EQ(Format("%*.*f", -10, 4, 1234.1234567890), "1234.1235 ");
            EXPECT_EQ(Format("%.*f", -4, 1234.1234567890), "1234.123457"); // negative precision ignored
            EXPECT_EQ(Format("%.3d", std::numeric_limits<double>::infinity()), "inf");
            EXPECT_EQ(Format("%.3d", std::numeric_limits<double>::quiet_NaN()), "nan");
        }

        TEST(TinyformatTest, Errors)
        {
            // TODO check error message contents
            ExpectFormatError("", "%d", 5, 10);
            ExpectFormatError("", "%d %d", 1);
            ExpectFormatError("", "%123", 10);
            ExpectFormatError("", "%0*d", "thing that can't convert to int", 42);
            ExpectFormatError("", "%0.*d", "thing that can't convert to int", 42);
            ExpectFormatError("", "%*d", 1);
            ExpectFormatError("", "%.*d", 1);
            ExpectFormatError("", "%*.*d", 1, 2);
            ExpectFormatError("", "%n", 10);
        }

        TEST(TinyformatTest, ComplicatedFormat)
        {
            EXPECT_EQ(Format("%2$0.10f:%3$0*4$d:%1$+g:%6$s:%5$#X:%7$c:%%:%%asdf",
                3.13, 1.234, 42, 4, 0XDEAD, "str", (int)'X'),
                "1.2340000000:0042:+3.13:str:0XDEAD:X:%:%asdf");
            EXPECT_EQ(Format("%0.10f:%04d:%+g:%s:%#X:%c:%%:%%asdf",
                1.234, 42, 3.13, "str", 0XDEAD, (int)'X'),
                "1.2340000000:0042:+3.13:str:0XDEAD:X:%:%asdf");
        }

        TEST(TinyformatTest, MiscIntegers)
        {
            EXPECT_EQ(Format("%hhd", (char)65), "65");
            EXPECT_EQ(Format("%hhu", (unsigned char)65), "65");
            EXPECT_EQ(Format("%hhd", (signed char)65), "65");
            EXPECT_EQ(Format("%s", true), "true");
            EXPECT_EQ(Format("%d", true), "1");
            EXPECT_EQ(Format("%hd", (short)1000), "1000");
            EXPECT_EQ(Format("%ld", (long)100000), "100000");
            EXPECT_EQ(Format("%lld", (long long)100000), "100000");
            EXPECT_EQ(Format("%zd", (size_t)100000), "100000");
            EXPECT_EQ(Format("%td", (ptrdiff_t)100000), "100000");
            EXPECT_EQ(Format("%jd", 100000), "100000");
        }

    } // namespace
} // namespace TinyformatTest
