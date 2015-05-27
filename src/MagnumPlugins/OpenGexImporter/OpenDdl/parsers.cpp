/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "parsers.h"

#include <cstring>
#include <limits>
#include <tuple>
#include <Corrade/Utility/Debug.h>

#include "MagnumPlugins/OpenGexImporter/OpenDdl/Type.h"

namespace Magnum { namespace OpenDdl { namespace Implementation {

Debug operator<<(Debug debug, const ParseErrorType value) {
    switch(value) {
        #define _c(value) case ParseErrorType::value: return debug << "OpenDdl::ParseErrorType::" #value;
        _c(NoError)
        _c(InvalidEscapeSequence)
        _c(InvalidIdentifier)
        _c(InvalidName)
        _c(InvalidCharacterLiteral)
        _c(InvalidLiteral)
        _c(InvalidPropertyValue)
        _c(InvalidSubArraySize)
        _c(LiteralOutOfRange)
        _c(ExpectedIdentifier)
        _c(ExpectedName)
        _c(ExpectedLiteral)
        _c(ExpectedSeparator)
        _c(ExpectedListStart)
        _c(ExpectedListEnd)
        _c(ExpectedArraySizeEnd)
        _c(ExpectedPropertyValue)
        _c(ExpectedPropertyAssignment)
        _c(ExpectedPropertyListEnd)
        #undef _c
    }

    return debug << "OpenDdl::ParseErrorType::(invalid)";
}

namespace {

/* Cannot use std::isnum(), std::isalpha() etc., because they depend on locale */
template<Int> constexpr bool isBaseN(char);
template<> constexpr bool isBaseN<2>(char c) {
    return c == '0' || c == '1';
}
template<> constexpr bool isBaseN<8>(char c) {
    return c >= '0' && c <= '7';
}
template<> constexpr bool isBaseN<10>(char c) {
    return c >= '0' && c <= '9';
}
template<> constexpr bool isBaseN<16>(char c) {
    return isBaseN<10>(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
constexpr bool isBinaryPrefix(char c) {
    return c == 'b' || c == 'o' || c == 'x' ||
           c == 'B' || c == 'O' || c == 'X';
}
constexpr bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
constexpr bool isAlphaDecimal(char c) {
    return isAlpha(c) || isBaseN<10>(c);
}

template<class T> inline T parseHex(const char* data) {
    T out{};
    for(std::size_t i = 0; i != sizeof(T)*2; ++i)
        out |= ((data[i] >= 'a' ?
            0xa - 'a' : (data[i] >= 'A' ?
                0xA - 'A' :
                    0x0 - '0')) + data[i]) << (sizeof(T)*2 - i - 1)*8;
    return out;
}

inline bool equalsPrefix(const Containers::ArrayReference<const char> data, const char* const prefix) {
    return std::strncmp(data, prefix, data.size()) == 0;
}

template<std::size_t n> inline const char* prefix(const Containers::ArrayReference<const char> data, const char(&compare)[n]) {
    /* Propagate errors */
    if(!data) return nullptr;

    if(data.size() < n - 1) return nullptr;

    const char* const end = data + n - 1;
    return equalsPrefix(data.prefix(end), compare) ? end : nullptr;
}

void extractWithoutUnderscore(const Containers::ArrayReference<const char> data, std::string& buffer) {
    buffer.clear();
    for(char c: data) if(c != '_') buffer += c;
}

}

bool equals(const Containers::ArrayReference<const char> a, const Containers::ArrayReference<const char> b) {
    return a.size() == b.size() && equalsPrefix(a, b);
}

const char* whitespace(const Containers::ArrayReference<const char> data) {
    /* Propagate error */
    if(!data) return nullptr;

    const char* i = data;
    while(i != data.end()) {
        /* Whitespace */
        if(*i <= 32) ++i;

        /* Comment */
        else if(*i == '/' && i + 1 < data.end() && (i[1] == '*' || i[1] == '/'))
        {
            /* Single-line comment */
            if(i[1] == '/') for(const char* j = i + 2; j != data.end(); ++j) {
                if(*j == '\n') {
                    i = j + 1;
                    break;
                }

            /* Multi-line comment */
            } else for(const char* j = i + 2; j != data.end(); ++j) {
                if(*j == '*' && j + 1 != data.end() && *(j + 1) == '/') {
                    i = j + 2;
                    break;
                }
            }
        }

        /* Something else, done */
        else break;
    }

    return i;
}

std::pair<const char*, char> escapedChar(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Escape sequence is not standalone, thus we can't expect it anywhere */
    CORRADE_INTERNAL_ASSERT(!data.empty() && *data == '\\');

    if(data.size() < 2) {
        error = {ParseErrorType::InvalidEscapeSequence, data};
        return {};
    }

    /* Two-character escape */
    switch(data[1]) {
        case '\\': return {data + 2, '\\'};
        case '\'': return {data + 2, '\\'};
        case 'a':  return {data + 2, '\a'};
        case 'b':  return {data + 2, '\b'};
        case 'f':  return {data + 2, '\f'};
        case 'n':  return {data + 2, '\n'};
        case 'r':  return {data + 2, '\r'};
        case 't':  return {data + 2, '\t'};
        case 'v':  return {data + 2, '\v'};
        case '?':
        case '"':
            return {data + 2, data[1]};
    }

    /* Four-character escape */
    if(data.size() >= 4 && data[1] == 'x' && isBaseN<16>(data[2]) && isBaseN<16>(data[3]))
        return {data + 4, parseHex<char>(data + 2)};

    error = {ParseErrorType::InvalidEscapeSequence, data};
    return {};
}

const char* escapedUnicode(const Containers::ArrayReference<const char> data, std::string& out, ParseError& error) {
    /* Escape sequence is not standalone, thus we can't expect it anywhere */
    CORRADE_INTERNAL_ASSERT(!data.empty() && *data == '\\');

    if(data.size() < 2) {
        error = {ParseErrorType::InvalidEscapeSequence, data};
        return {};
    }

    if(data.size() >= 6 && data[1] == 'u') {
        Warning() << "Trade::OpenGexImporter::openData(): Unicode parsing not implemented";
        out += '?';
        return data + 6;
    }

    if(data.size() >= 8 && data[1] == 'U') {
        Warning() << "Trade::OpenGexImporter::openData(): Unicode parsing not implemented";
        out += '?';
        return data + 8;
    }

    char result;
    const char* end;
    std::tie(end, result) = escapedChar(data, error);
    out += result;
    return end;
}

const char* identifier(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return nullptr;

    if(data.empty()) {
        error = {ParseErrorType::ExpectedIdentifier};
        return nullptr;
    }

    if(!isAlpha(*data) && *data != '_') {
        error = {ParseErrorType::InvalidIdentifier, data};
        return nullptr;
    }

    const char* i = data + 1;
    for(; i != data.end(); ++i) {
        const char c = *i;

        if(!isAlphaDecimal(c) && c != '_') break;
    }

    return i;
}

std::pair<const char*, bool> boolLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(const char* const end = prefix(data, "true")) return {end, true};
    if(const char* const end = prefix(data, "false")) return {end, false};

    error = {ParseErrorType::InvalidLiteral, Type::Bool, data};
    return {};
}

std::pair<const char*, char> characterLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    if(data.size() >= 3 && data[0] == '\'') {
        /* Allowed ASCII character */
        if(((data[1] >= 0x20 && data[1] < '\'') ||
            (data[1] > '\'' && data[1] < '\\') ||
            (data[1] > '\\' && data[1] <= 0x7e)) && data[2] == '\'')
            return {data + 3, data[1]};

        /* Escaped char */
        if(data[1] == '\\') {
            const char* i;
            char result;
            std::tie(i, result) = escapedChar(data.suffix(1), error);
            if(i && i != data.end() && *i == '\'') return {i + 1, result};
        }
    }

    error = {ParseErrorType::InvalidCharacterLiteral, data};
    return {};
}

namespace {

template<class> struct IntegralType;
template<> struct IntegralType<Float> { typedef UnsignedInt Type; };
#ifndef MAGNUM_TARGET_GLES
template<> struct IntegralType<Double> { typedef UnsignedLong Type; };
#endif
template<class T> using IntegralTypeFor = typename IntegralType<T>::Type;

template<class> struct ExtractToType;
/** @todo isn't there something better for extracting to unsigned int on webgl? */
#ifndef MAGNUM_TARGET_WEBGL
template<> struct ExtractToType<UnsignedLong> {
    typedef UnsignedLong Type;
    static UnsignedLong extract(const std::string& buffer, Int base) {
        return std::stoul(buffer, nullptr, base);
    }
};
template<> struct ExtractToType<UnsignedByte>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<Byte>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<UnsignedShort>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<Short>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<UnsignedInt>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<Int>: ExtractToType<UnsignedLong> {};
template<> struct ExtractToType<Long>: ExtractToType<UnsignedLong> {};
#else
template<> struct ExtractToType<UnsignedInt> {
    typedef UnsignedInt Type;
    static UnsignedInt extract(const std::string& buffer, Int base) {
        return std::stoi(buffer, nullptr, base);
    }
};
template<> struct ExtractToType<UnsignedByte>: ExtractToType<UnsignedInt> {};
template<> struct ExtractToType<Byte>: ExtractToType<UnsignedInt> {};
template<> struct ExtractToType<UnsignedShort>: ExtractToType<UnsignedInt> {};
template<> struct ExtractToType<Short>: ExtractToType<UnsignedInt> {};
template<> struct ExtractToType<Int>: ExtractToType<UnsignedInt> {};
#endif
template<class T> using ExtractedType = typename ExtractToType<T>::Type;

template<> struct ExtractToType<Float> {
    static Float extract(const std::string& buffer) { return std::stof(buffer); }
};
#ifndef MAGNUM_TARGET_GLES
template<> struct ExtractToType<Double> {
    static Double extract(const std::string& buffer) { return std::stod(buffer); }
};
#endif

template<class> constexpr Type typeFor();
#define _c(T) template<> constexpr Type typeFor<T>() { return Type::T; }
_c(UnsignedByte)
_c(Byte)
_c(UnsignedShort)
_c(Short)
_c(UnsignedInt)
_c(Int)
#ifndef MAGNUM_TARGET_WEBGL
_c(UnsignedLong)
_c(Long)
#endif
_c(Float)
#ifndef MAGNUM_TARGET_GLES
_c(Double)
#endif
#undef _c

template<Int base> const char* possiblyNumericCharacters(const Containers::ArrayReference<const char> data) {
    /* Propagate errors */
    if(!data) return {};

    const char* i = data;
    while(i != data.end() && (isBaseN<base>(*i) || (i != data && *i == '_'))) ++i;

    return i;
}

template<Int base, class T> const char* numericCharacters(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    const char* const i = possiblyNumericCharacters<base>(data);

    if(i == data) {
        error = {ParseErrorType::InvalidLiteral, typeFor<T>(), data};
        return {};
    }

    return i;
}

template<Int base, class T> std::pair<const char*, T> baseNLiteral(const Containers::ArrayReference<const char> data, std::string& buffer, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    const char* i = numericCharacters<base, T>(data, error);

    /* Propagate errors */
    if(!i) return {};

    extractWithoutUnderscore(data.prefix(i), buffer);
    const ExtractedType<T> out = ExtractToType<T>::extract(buffer, base);
    if(out > ExtractedType<T>(std::numeric_limits<T>::max())) {
        error = {ParseErrorType::LiteralOutOfRange, typeFor<T>(), data};
        return {};
    }

    return {i, T(out)};
}

}

template<class T> std::tuple<const char*, T, Int> integralLiteral(const Containers::ArrayReference<const char> data, std::string& buffer, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedLiteral, typeFor<T>(), data};
        return {};
    }

    const char* i = data;

    /* Sign */
    T sign = T(1);
    if(*i == '+') ++i;
    else if(*i == '-') {
        if(!std::is_signed<T>{}) {
            error = {ParseErrorType::LiteralOutOfRange, typeFor<T>(), data};
            return {};
        }

        sign = T(-1);
        ++i;
    }

    T value;
    Int base;

    /* Char literal */
    if(i != data.end() && *i == '\'') {
        base = 256;
        std::tie(i, value) = characterLiteral(data.suffix(i), error);

    /* Binary/octal/hex literal */
    } else if(i + 1 < data.end() && *i == '0' && isBinaryPrefix(i[1])) switch(i[1]) {
        case 'x':
        case 'X': {
            base = 16;
            std::tie(i, value) = baseNLiteral<16, T>(data.suffix(i + 2), buffer, error);
            break;
        }
        case 'o':
        case 'O': {
            base = 8;
            std::tie(i, value) = baseNLiteral<8, T>(data.suffix(i + 2), buffer, error);
            break;
        }
        case 'b':
        case 'B': {
            base = 2;
            std::tie(i, value) = baseNLiteral<2, T>(data.suffix(i + 2), buffer, error);
            break;
        }

        default: CORRADE_ASSERT_UNREACHABLE();

    /* Decimal literal  */
    } else {
        base = 10;
        std::tie(i, value) = baseNLiteral<10, T>(data.suffix(i), buffer, error);
    }

    /** @todo C++14: use {} */
    return std::make_tuple(i, sign*value, base);
}

template std::tuple<const char*, UnsignedByte, Int> integralLiteral<UnsignedByte>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, Byte, Int> integralLiteral<Byte>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, UnsignedShort, Int> integralLiteral<UnsignedShort>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, Short, Int> integralLiteral<Short>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, UnsignedInt, Int> integralLiteral<UnsignedInt>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, Int, Int> integralLiteral<Int>(Containers::ArrayReference<const char>, std::string&, ParseError&);
#ifndef MAGNUM_TARGET_WEBGL
template std::tuple<const char*, UnsignedLong, Int> integralLiteral<UnsignedLong>(Containers::ArrayReference<const char>, std::string&, ParseError&);
template std::tuple<const char*, Long, Int> integralLiteral<Long>(Containers::ArrayReference<const char>, std::string&, ParseError&);
#endif

template<class T> std::pair<const char*, T> floatingPointLiteral(const Containers::ArrayReference<const char> data, std::string& buffer, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedLiteral, Type::Float, data};
        return {};
    }

    const char* i = data;

    /* Sign */
    T sign = T(1);
    if(*i == '+') ++i;
    else if(*i == '-') {
        sign = T(-1);
        ++i;
    }

    /* Binary literal */
    if(i + 1 < data.end() && *i == '0' && isBinaryPrefix(i[1])) {
        IntegralTypeFor<T> integralValue;
        switch(i[1]) {
            case 'x':
            case 'X': {
                std::tie(i, integralValue) = baseNLiteral<16, IntegralTypeFor<T>>(data.suffix(i + 2), buffer, error);
                break;
            }
            case 'o':
            case 'O': {
                std::tie(i, integralValue) = baseNLiteral<8, IntegralTypeFor<T>>(data.suffix(i + 2), buffer, error);
                break;
            }
            case 'b':
            case 'B': {
                std::tie(i, integralValue) = baseNLiteral<2, IntegralTypeFor<T>>(data.suffix(i + 2), buffer, error);
                break;
            }

            default: CORRADE_ASSERT_UNREACHABLE();
        }

        const T floatValue = reinterpret_cast<T&>(integralValue);
        return {i, sign*floatValue};
    }

    /* Decimal before dot */
    const char* const before = i;
    i = possiblyNumericCharacters<10>(data.suffix(i));

    /* Dot and decimal after dot */
    if(i && i + 1 < data.end() && *i == '.') {
        i = possiblyNumericCharacters<10>(data.suffix(i + 1));

        /* Expecting at least .0 or 0. */
        if(before + 1 == i) {
            error = {ParseErrorType::InvalidLiteral, typeFor<T>(), data};
            return {};
        }

    /* Expecting at least one numeric character */
    } else if(before == i) {
        error = {ParseErrorType::InvalidLiteral, typeFor<T>(), data};
        return {};
    }

    /* Exponent etc */
    if(i && i != data.end() && (*i == 'e' || *i == 'E')) {
        ++i;

        /* Exponent sign */
        if(*i == '+' || *i == '-') ++i;

        i = numericCharacters<10, T>(data.suffix(i), error);
    }

    /* Propagate errors */
    if(!i) return {};

    /** @todo verifying out-of-range */

    extractWithoutUnderscore(data.prefix(i), buffer);
    return {i, ExtractToType<T>::extract(buffer)};
}

template std::pair<const char*, Float> floatingPointLiteral<Float>(Containers::ArrayReference<const char>, std::string&, ParseError&);
#ifndef MAGNUM_TARGET_GLES
template std::pair<const char*, Double> floatingPointLiteral<Double>(Containers::ArrayReference<const char>, std::string&, ParseError&);
#endif

std::pair<const char*, std::string> stringLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty() || *data != '"') {
        error = {ParseErrorType::ExpectedLiteral, Type::String, data};
        return {};
    }

    std::string out;

    for(const char* i = data + 1; i != data.end(); ) {
        const char c = *i;

        if(UnsignedByte(c) < 0x20) {
            error = {ParseErrorType::InvalidLiteral, Type::String, i};
            return {};
        }

        /* Escaped character */
        if(c == '\\')
            i = escapedUnicode(data.suffix(i), out, error);

        /* End of string, try searching for continuation */
        else if(c == '"') {
            const char* j = whitespace(data.suffix(i + 1));

            /* Continuation not found, done */
            if(j + 1 >= data.end() || *j != '"')
                return {j, out};

            i = j + 1;

        /* Any other character, append it to result */
        } else {
            out += c;
            ++i;
        }
    }

    error = {ParseErrorType::LiteralOutOfRange, Type::String};
    return {};
}

std::pair<const char*, std::string> nameLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedName, data};
        return {};
    }

    const char* i = data;
    if(*i != '$' && *i != '%') {
        error = {ParseErrorType::InvalidName, data};
        return {};
    }

    i = identifier(data.suffix(data + 1), error);
    return {i, i ? std::string{data, std::size_t(i - data)} : std::string{}};
}

std::pair<const char*, Containers::ArrayReference<const char>> referenceLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedLiteral, Type::Reference};
        return {};
    }

    if(const char* const end = prefix(data, "null"))
        return {end, nullptr};

    const char* i = data;
    if(*i != '$' && *i != '%') {
        error = {ParseErrorType::InvalidLiteral, Type::Reference, data};
        return {};
    }

    i = identifier(data.suffix(1), error);

    for(; i && i != data.end(); ) {
        if(*i != '%') break;
        i = identifier(data.suffix(i + 1), error);
    }

    return {i, i ? data.prefix(i) : nullptr};
}

std::pair<const char*, Type> possiblyTypeLiteral(const Containers::ArrayReference<const char> data) {
    #define _c(identifier, type) \
        if(const char* const c = prefix(data, #identifier)) return {c, Type::type};
    _c(bool, Bool)
    _c(unsigned_int8, UnsignedByte)
    _c(int8, Byte)
    _c(unsigned_int16, UnsignedShort)
    _c(int16, Short)
    _c(unsigned_int32, UnsignedInt)
    _c(int32, Int)
    #ifndef MAGNUM_TARGET_WEBGL
    _c(unsigned_int64, UnsignedLong)
    _c(int64, Long)
    #endif
    /** @todo Half */
    _c(float, Float)
    #ifndef MAGNUM_TARGET_GLES
    _c(double, Double)
    #endif
    _c(string, String)
    _c(ref, Reference)
    _c(type, Type)
    #undef _c

    return {};
}

std::pair<const char*, Type> typeLiteral(const Containers::ArrayReference<const char> data, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedLiteral, Type::Type, data};
        return {};
    }

    const char* i;
    Type type;
    std::tie(i, type) = possiblyTypeLiteral(data);
    if(i) return {i, type};

    error = {ParseErrorType::InvalidLiteral, Type::Type, data};
    return {};
}

std::pair<const char*, InternalPropertyType> propertyValue(const Containers::ArrayReference<const char> data, bool& boolValue, Int& integerValue, Float& floatingPointValue, std::string& stringValue, Containers::ArrayReference<const char>& referenceValue, Type& typeValue, std::string& buffer, ParseError& error) {
    /* Propagate errors */
    if(!data) return {};

    if(data.empty()) {
        error = {ParseErrorType::ExpectedPropertyValue};
        return {};
    }

    const char* i = data;

    /* String literal */
    if(*i == '"') {
        std::tie(i, stringValue) = stringLiteral(data, error);
        return {i, InternalPropertyType::String};
    }

    /* Reference literal */
    if(*i == '%' || *i == '$') {
        std::tie(i, referenceValue) = referenceLiteral(data, error);
        return {i, InternalPropertyType::Reference};
    }

    /* Numeric literal */
    if((*i >= '0' && *i <= '9') || *i == '.' || *i == '\'') {
        /* Float literal if there is dot */
        for(const char* j = i; j != data.end(); ++j) {
            if(*j == '.') {
                std::tie(i, floatingPointValue) = floatingPointLiteral<Float>(data, buffer, error);
                return {i, InternalPropertyType::Float};
            }

            if(*j != '+' && *j != '-' && *j != '_' && !isBaseN<10>(*j)) break;
        }

        /* Integer literal otherwise */
        Int base;
        std::tie(i, integerValue, base) = integralLiteral<Int>(data, buffer, error);
        switch(base) {
            case 2:
            case 8:
            case 16: return {i, InternalPropertyType::Binary};
            case 10: return {i, InternalPropertyType::Integral};
            case 256: return {i, InternalPropertyType::Character};
        }

        CORRADE_ASSERT_UNREACHABLE();
    }

    /* Null reference literal */
    if(const char* const end = prefix(data, "null")) {
        stringValue = "null";
        return {end, InternalPropertyType::Reference};
    }

    /* Boolean literals */
    if(const char* const end = prefix(data, "true")) {
        boolValue = true;
        return {end, InternalPropertyType::Bool};
    }
    if(const char* const end = prefix(data, "false")) {
        boolValue = false;
        return {end, InternalPropertyType::Bool};
    }

    /* Possibly type literal */
    std::tie(i, typeValue) = possiblyTypeLiteral(data);
    if(i) return {i, InternalPropertyType::Type};

    error = {ParseErrorType::InvalidPropertyValue, data};
    return {};
}

}}}
