#pragma once
#include "binaryWriter.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace AC_Save
{
    template<class T>
    concept writable = requires(BinaryWriter& w, const T& v) { Serializer<T>::write(w, v); };

    static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
    static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);

    template <writable T>
    void write(BinaryWriter& w, const T& v) { Serializer<T>::write(w, v);}

    // unsinged Serializing :
    template <> struct Serializer<std::uint8_t>{static void write(BinaryWriter& w, std::uint8_t v) {w.writeU8(v);}};
    template <> struct Serializer<std::uint16_t>{static void write(BinaryWriter& w, std::uint16_t v) {w.writeU16(v);}};
    template <> struct Serializer<std::uint32_t>{static void write(BinaryWriter& w, std::uint32_t v) {w.writeU32(v);}};
    template <> struct Serializer<std::uint64_t>{static void write(BinaryWriter& w, std::uint64_t v) {w.writeU64(v);}};

    // singed Serializing :

    template <> struct Serializer<std::int8_t>{static void write(BinaryWriter& w, std::int8_t v) {w.writeI8(v);}};
    template <> struct Serializer<std::int16_t>{static void write(BinaryWriter& w, std::int16_t v) {w.writeI16(v);}};
    template <> struct Serializer<std::int32_t>{static void write(BinaryWriter& w, std::int32_t v) {w.writeI32(v);}};
    template <> struct Serializer<std::int64_t>{static void write(BinaryWriter& w, std::int64_t v) {w.writeI64(v);}};

    // other useful types :

    template <> struct Serializer<bool>{static void write(BinaryWriter& w, bool v) {w.writeBool(v);}};
    template <> struct Serializer<float>{static void write(BinaryWriter& w, float v) {w.writeF32(v);}};
    template <> struct Serializer<double>{static void write(BinaryWriter& w, double v) {w.writeF64(v);}};

    template <> struct Serializer<std::span<const std::byte>>{static void write(BinaryWriter& w, std::span<const std::byte> v)
    {
        if (v.size() > std::numeric_limits<uint32_t>::max())
            throw std::length_error("Byte blob too large");
        w.writeU32(static_cast<std::uint32_t>(v.size()));
        w.writeBytes(v);
    }};
    template <> struct Serializer<std::string_view>{static void write(BinaryWriter& w, const std::string_view& v) {w.writeString(v);}};
    template <> struct Serializer<std::string>{static void write(BinaryWriter& w, const std::string& v) {w.writeString(v);}};

    template <writable T> struct Serializer<std::vector<T>>
    {
        static void write(BinaryWriter& w, const std::vector<T>& items)
        {
            if (items.size() > std::numeric_limits<std::uint32_t>::max())
                throw std::length_error("Sequence too large");
            w.writeU32(static_cast<std::uint32_t>(items.size()));
            for (const T& it : items)
                Serializer<T>::write(w, it);
        }
    };
}