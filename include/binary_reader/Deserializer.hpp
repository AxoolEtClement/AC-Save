#pragma once
#include "binaryReader.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace AC_Save
{
    template <class T>
    struct Deserializer;
    // Mirrors `writable` in coreSerializer.hpp, except `read` takes `BinaryReader&`
    // and returns `T` by value instead of taking `const T&`.
    template <class T>
    concept readable = requires(BinaryReader& r) {
        { Deserializer<T>::read(r) } -> std::convertible_to<T>;
    };

    template <readable T>
    [[nodiscard]] T read(BinaryReader& r) {return Deserializer<T>::read(r);}

    // Unsigned deserializing
    template <>
    struct Deserializer<std::uint8_t> {[[nodiscard]] static std::uint8_t read(BinaryReader& r) { return r.readU8(); }};
    template <>
    struct Deserializer<std::uint16_t> {[[nodiscard]] static std::uint16_t read(BinaryReader& r) { return r.readU16(); }};
    template <>
    struct Deserializer<std::uint32_t> {[[nodiscard]] static std::uint32_t read(BinaryReader& r) { return r.readU32(); }};
    template <>
    struct Deserializer<std::uint64_t> {[[nodiscard]] static std::uint64_t read(BinaryReader& r) { return r.readU64(); }};

    // Signed deserializing
    template <>
    struct Deserializer<std::int8_t> {[[nodiscard]] static std::int8_t read(BinaryReader& r) { return r.readI8(); }};
    template <>
    struct Deserializer<std::int16_t> {[[nodiscard]] static std::int16_t read(BinaryReader& r) { return r.readI16(); }};
    template <>
    struct Deserializer<std::int32_t> {[[nodiscard]] static std::int32_t read(BinaryReader& r) { return r.readI32(); }};
    template <>
    struct Deserializer<std::int64_t> {[[nodiscard]] static std::int64_t read(BinaryReader& r) { return r.readI64(); }};

    // Other useful types

    template <>
    struct Deserializer<bool> {[[nodiscard]] static bool read(BinaryReader& r) { return r.readBool(); }};
    template <>
    struct Deserializer<float> {[[nodiscard]] static float read(BinaryReader& r) { return r.readF32(); }};
    template <>
    struct Deserializer<double> {[[nodiscard]] static double read(BinaryReader& r) { return r.readF64(); }};

    template <>
    struct Deserializer<Bytes> {[[nodiscard]] static Bytes read(BinaryReader& r) { return r.readBytes(); }};
    template <>
    struct Deserializer<std::string> {[[nodiscard]] static std::string read(BinaryReader& r) { return r.readString(); }};

    template <readable T>
    struct Deserializer<std::vector<T>>
    {
        [[nodiscard]] static std::vector<T> read(BinaryReader& r)
        {
            const std::uint32_t count = r.readU32();
            
            if (count > r.remaining())
                throw std::length_error("Sequence count exceeds remaining bytes");

            std::vector<T> items;
            items.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i)
                items.push_back(Deserializer<T>::read(r));

            return items;
        }
    };

    template <class E>
        requires std::is_enum_v<E>
    struct Deserializer<E>
    {
        [[nodiscard]] static E read(BinaryReader& r)
        {
            using U = std::underlying_type_t<E>;
            return static_cast<E>(Deserializer<U>::read(r));
        }
    };
}