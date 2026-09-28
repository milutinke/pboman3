#include "pboheaderreader.h"
#include <QFileInfo>
#include <limits>
#include "pbofileformatexception.h"
#include "pboheaderio.h"

namespace pboman3::io {
    namespace {
        void addDataSize(qint64& total, const PboNodeEntity& entry) {
            const qint64 size = entry.dataSize();
            if (size < 0 || total > std::numeric_limits<qint64>::max() - size)
                throw PboFileFormatException("The file entry sizes are corrupted.");
            total += size;
        }
    }

    PboFileHeader PboHeaderReader::readFileHeader(PboFile* file) {
        QList<QSharedPointer<PboHeaderEntity>> headers;
        QList<QSharedPointer<PboNodeEntity>> entries;

        const PboHeaderIO reader(file);
        QSharedPointer<PboNodeEntity> entry = reader.readNextEntry();

        if (!entry) {
            throw PboFileFormatException("The file is not a valid PBO.");
        }

        qint64 dataBlockEnd = 0;
        if (entry->isSignature()) {
            QSharedPointer<PboHeaderEntity> header = reader.readNextHeader();
            while (header && !header->isBoundary()) {
                headers.append(header);
                header = reader.readNextHeader();
            }
            if (!header) {
                throw PboFileFormatException("The file headers are corrupted.");
            }
        } else if (entry->isContent()) {
            entries.append(entry);
            addDataSize(dataBlockEnd, *entry);
        } else {
            throw PboFileFormatException("The file first entry is corrupted.");
        }

        entry = reader.readNextEntry();
        while (entry && !entry->isBoundary()) {
            entries.append(entry);
            addDataSize(dataBlockEnd, *entry);
            entry = reader.readNextEntry();
        }
        if (!entry || !entry->isBoundary()) {
            throw PboFileFormatException("The file entries list is corrupted.");
        }

        const qint64 dataBlockStart = file->pos();
        if (dataBlockEnd > std::numeric_limits<qint64>::max() - dataBlockStart)
            throw PboFileFormatException("The file data range is corrupted.");
        dataBlockEnd += dataBlockStart;

        if (dataBlockEnd > file->size())
            throw PboFileFormatException("The file data block is truncated.");

        QByteArray signature;
        if (dataBlockEnd == file->size())
            return PboFileHeader{headers, entries, dataBlockStart, signature};

        // A signed PBO stores a zero separator before the optional 20-byte SHA-1 value.
        if (!file->seek(dataBlockEnd + 1))
            throw PboFileFormatException("The file signature offset is corrupted.");

        if (!file->atEnd()) {
            constexpr int sha1Size = 20;
            signature.resize(sha1Size);
            const qint64 read = file->read(signature.data(), signature.size());
            if (read != signature.size())
                signature.truncate(0);
        }

        return PboFileHeader{headers, entries, dataBlockStart, signature};
    }
}
