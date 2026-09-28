#pragma once

#include <QFileDevice>

namespace pboman3::io {
    class DecompressionContext {
    public:
        int format;
        uint crc;
        QByteArray buffer;
        QIODevice* source;
        QFileDevice* target;

        DecompressionContext(QIODevice* pSource, QFileDevice* pTarget);

        void write(char data);

        void write(const QByteArray& data, int chunkSize);

        void setBuffer(qint64 offset, int length);

    private:
        void updateCrc(char data);

        void updateCrc(const QByteArray& data, int chunkSize);
    };
}
