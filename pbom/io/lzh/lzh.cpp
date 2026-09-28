#include "lzh.h"
#include "compressionchunk.h"
#include "lzhdecompressionexception.h"
#include "io/diskaccessexception.h"
#include "util/log.h"
#include <QtEndian>

#define LOG(...) LOGGER("io/lzh/Lzh", __VA_ARGS__)

namespace pboman3::io {
    void Lzh::decompress(QIODevice* source, QFileDevice* target, int outputLength, const Cancel& cancel) {
        DecompressionContext ctx(source, target);
        const qint64 maxTargetOffset = target->pos() + outputLength;
        const qint64 maxSourceOffset = source->size() - 2;
        while (target->pos() < maxTargetOffset && !source->atEnd() && !cancel()) {
            char format;
            if (source->read(&format, sizeof format) != sizeof format)
                throw LzhDecompressionException("Compressed input is truncated");
            for (char i = 0; i < 8 && target->pos() < maxTargetOffset && source->pos() < maxSourceOffset; i++) {
                ctx.format = format >> i & 0x01;
                processBlock(ctx);
            }
        }
        if (!cancel()) {
            //does not make sense to check validity if cancel
            //as it won't be valid
            const bool valid = isValid(ctx);
            if (!valid) {
                LOG(warning, "The file checksums did not match - throwing")
                throw LzhDecompressionException("Could not decompress the file");
            }
        }
    }

    void Lzh::compress(QFileDevice* source, QFileDevice* target, const Cancel& cancel) {
        assert(source->pos() == 0 && "File offset must be 0 for LZH compression");

        CompressionBuffer dict(CompressionBuffer::defaultSize);

        while (!source->atEnd() && !cancel()) {
            CompressionChunk chunk;
            chunk.compose(source, dict);
            chunk.flush(target);
        }

        if (!cancel())
            writeCrc(source, target);
    }

    void Lzh::processBlock(DecompressionContext& ctx) {
        constexpr int packetFormatUncompressed = 1;
        constexpr char whitespaceChar = 0x20;

        if (ctx.format == packetFormatUncompressed) {
            char data;
            if (ctx.source->read(&data, sizeof data) != sizeof data)
                throw LzhDecompressionException("Compressed input is truncated");
            ctx.write(data);
        } else {
            unsigned char bytes[sizeof(quint16)];
            if (ctx.source->read(reinterpret_cast<char*>(bytes), sizeof bytes) != sizeof bytes)
                throw LzhDecompressionException("Compressed input is truncated");
            const quint16 pointer = qFromLittleEndian<quint16>(bytes);
            qint64 rpos = ctx.target->pos() - static_cast<qint64>((pointer & 0x00ff))
                - static_cast<qint64>(((pointer & 0xf000) >> 4));
            int rlen = ((pointer & 0x0f00) >> 8) + 3;

            if (rpos + rlen < 0) {
                for (int i = 0; i < rlen; i++)
                    ctx.write(whitespaceChar);
            } else {
                while (rpos < 0) {
                    ctx.write(whitespaceChar);
                    rpos++;
                    rlen--;
                }
                if (rlen > 0) {
                    const int chunkSize = rpos + rlen > ctx.target->pos()
                                              ? static_cast<int>(ctx.target->pos() - rpos)
                                              : rlen;
                    ctx.setBuffer(rpos, chunkSize);

                    while (rlen >= chunkSize && chunkSize > 0) {
                        ctx.write(ctx.buffer, chunkSize);
                        rlen -= chunkSize;
                    }
                    for (int j = 0; j < rlen; j++) {
                        ctx.write(ctx.buffer[j]);
                    }
                }
            }
        }
    }

    bool Lzh::isValid(const DecompressionContext& ctx) {
        bool valid = false;

        if (ctx.source->size() - ctx.source->pos() >= static_cast<qint32>(sizeof(uint))) {
            unsigned char bytes[sizeof(quint32)];
            if (ctx.source->read(reinterpret_cast<char*>(bytes), sizeof bytes) != sizeof bytes)
                return false;
            const quint32 crc = qFromLittleEndian<quint32>(bytes);
            valid = crc == ctx.crc;
        }

        return valid;
    }

    void Lzh::writeCrc(QFileDevice* source, QFileDevice* target) {
        QByteArray buffer(1024, Qt::Initialization::Uninitialized);
        quint32 crc = 0;

        const bool seek = source->seek(0);
        assert(seek);

        while (!source->atEnd()) {
            const qint64 read = source->read(buffer.data(), buffer.length());
            for (qint64 i = 0; i < read; i++) {
                crc += static_cast<quint8>(buffer[i]);
            }
        }

        unsigned char bytes[sizeof(crc)];
        qToLittleEndian<quint32>(crc, bytes);
        if (target->write(reinterpret_cast<const char*>(bytes), sizeof bytes) != sizeof bytes)
            throw DiskAccessException("Could not write compression checksum.", target->fileName());
    }
}
