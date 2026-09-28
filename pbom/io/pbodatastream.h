#pragma once

#include "pbofile.h"
#include <QException>
#include <QtEndian>
#include <array>
#include <type_traits>

namespace pboman3::io {
    template <typename T, bool = std::is_enum_v<T>>
    struct PboStoredType {
        using type = T;
    };

    template <typename T>
    struct PboStoredType<T, true> {
        using type = std::underlying_type_t<T>;
    };

    class PboEofException: public QException {    
    };

    template <typename T>
    concept PboHeaderType = std::is_integral_v<T> || std::is_enum_v<T>;

    class PboDataStream : public QDataStream {
    public:
        explicit PboDataStream(PboFile* file);

        PboDataStream& operator>>(QString& out);

        PboDataStream& operator<<(const QString& src);

        template <PboHeaderType T>
        PboDataStream& operator>>(T& out) {
            using Value = typename PboStoredType<T>::type;
            using UnsignedValue = std::make_unsigned_t<Value>;
            std::array<unsigned char, sizeof(Value)> bytes{};
            if (file_->read(reinterpret_cast<char*>(bytes.data()), bytes.size()) != bytes.size()) {
                throw PboEofException();
            }
            const Value value = static_cast<Value>(qFromLittleEndian<UnsignedValue>(bytes.data()));
            if constexpr (std::is_enum_v<T>)
                out = static_cast<T>(value);
            else
                out = value;
            return *this;
        }

        template <PboHeaderType T>
        PboDataStream& operator<<(const T& src) {
            using Value = typename PboStoredType<T>::type;
            using UnsignedValue = std::make_unsigned_t<Value>;
            std::array<unsigned char, sizeof(Value)> bytes{};
            const Value value = static_cast<Value>(src);
            qToLittleEndian<UnsignedValue>(static_cast<UnsignedValue>(value), bytes.data());
            if (file_->write(reinterpret_cast<const char*>(bytes.data()), bytes.size()) != bytes.size())
                throw PboEofException();
            return *this;
        }

    private:
        PboFile* file_;
    };
}
