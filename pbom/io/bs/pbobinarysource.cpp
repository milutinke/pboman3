#include "pbobinarysource.h"
#include "io/diskaccessexception.h"
#include "io/lzh/lzh.h"
#include "io/lzh/lzhdecompressionexception.h"
#include <QBuffer>

namespace pboman3::io {
    PboBinarySource::PboBinarySource(const QString& path, const PboDataInfo& dataInfo, qsizetype bufferSize)
        : AbstractBinarySource(path),
          dataInfo_(dataInfo),
          bufferSize_(bufferSize) {
    }

    void PboBinarySource::writeToPbo(QFileDevice* targetFile, const Cancel& cancel) {
        assert(file_->isOpen());
        writeRaw(targetFile, cancel);
    }

    void PboBinarySource::writeToFs(QFileDevice* targetFile, const Cancel& cancel) {
        assert(file_->isOpen());
        if (isCompressed()) {
            if (!tryWriteDecompressed(targetFile, cancel))
                writeRaw(targetFile, cancel);
        } else {
            writeRaw(targetFile, cancel);
        }
    }

    void PboBinarySource::writeRaw(QFileDevice* targetFile, const Cancel& cancel) const {
        const bool seek = file_->seek(dataInfo_.dataOffset);
        assert(seek);

        QByteArray buf(bufferSize_, Qt::Initialization::Uninitialized);

        qint64 remaining = dataInfo_.dataSize;
        while (!cancel() && remaining > 0) {
            const qsizetype willRead = remaining > buf.size() ? buf.size() : remaining;
            const qint64 hasRead = file_->read(buf.data(), willRead);
            if (hasRead <= 0)
                throw DiskAccessException("For some reason could not read from the file.", file_->fileName());
            if (targetFile->write(buf.data(), hasRead) != hasRead)
                throw DiskAccessException("Could not write all data to the destination file.", targetFile->fileName());
            remaining -= hasRead;
        }
    }

    bool PboBinarySource::tryWriteDecompressed(QFileDevice* targetFile, const Cancel& cancel) const {
        try {
            if (!file_->seek(dataInfo_.dataOffset))
                throw DiskAccessException("Could not seek to the compressed file entry.", file_->fileName());

            QByteArray compressed(dataInfo_.dataSize, Qt::Initialization::Uninitialized);
            if (file_->read(compressed.data(), compressed.size()) != compressed.size())
                throw DiskAccessException("The compressed file entry is truncated.", file_->fileName());

            QBuffer boundedSource(&compressed);
            boundedSource.open(QIODeviceBase::ReadOnly);
            Lzh::decompress(&boundedSource, targetFile, dataInfo_.originalSize, cancel);
            return true;
        } catch (LzhDecompressionException&) {
            targetFile->resize(0);
            return false;
        }
    }

    const PboDataInfo& PboBinarySource::getInfo() const {
        return dataInfo_;
    }

    qint32 PboBinarySource::readOriginalSize() const {
        return dataInfo_.originalSize;
    }

    qint32 PboBinarySource::readTimestamp() const {
        return dataInfo_.timestamp;
    }

    bool PboBinarySource::isCompressed() const {
        return dataInfo_.compressed;
    }
}
