#include "decompressioncontext.h"
#include "lzhdecompressionexception.h"

namespace pboman3::io {
    DecompressionContext::DecompressionContext(QIODevice* pSource, QFileDevice* pTarget)
        : format(0),
        crc(0),
        source(pSource),
        target(pTarget) {
        buffer.resize(18);
    }

    void DecompressionContext::write(char data) {
        if (target->write(&data, sizeof data) != sizeof data)
            throw LzhDecompressionException("Could not write decompressed data");
        updateCrc(data);
    }

    void DecompressionContext::write(const QByteArray& data, int chunkSize) {
        if (target->write(data.data(), chunkSize) != chunkSize)
            throw LzhDecompressionException("Could not write decompressed data");
        updateCrc(data, chunkSize);
    }

    void DecompressionContext::setBuffer(qint64 offset, int length) {
        const qint64 pos = target->pos();
        const bool seek = target->seek(offset);
        assert(seek);
        if (!seek || target->read(buffer.data(), length) != length || !target->seek(pos))
            throw LzhDecompressionException("Could not read decompression history");
    }

    void DecompressionContext::updateCrc(char data) {
        crc = crc + static_cast<quint8>(data);
    }

    void DecompressionContext::updateCrc(const QByteArray& data, int chunkSize) {
        for (int i = 0; i < chunkSize; i++) {
            crc = crc + static_cast<quint8>(data[i]);
        }
    }
}
