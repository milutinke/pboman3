#include "fsrawbinarysource.h"
#include <QFileInfo>
#include "io/diskaccessexception.h"

namespace pboman3::io {
    FsRawBinarySource::FsRawBinarySource(QString path, qsizetype bufferSize)
        : AbstractBinarySource(std::move(path)),
          bufferSize_(bufferSize) {
    }

    void FsRawBinarySource::writeToPbo(QFileDevice* targetFile, const Cancel& cancel) {
        assert(file_->isOpen());
        writeRaw(targetFile, cancel);
    }

    void FsRawBinarySource::writeToFs(QFileDevice* targetFile, const Cancel& cancel) {
        assert(file_->isOpen());
        writeRaw(targetFile, cancel);
    }

    void FsRawBinarySource::writeRaw(QFileDevice* targetFile, const Cancel& cancel) const {
        const bool seek = file_->seek(0);
        assert(seek);

        QByteArray buf(bufferSize_, Qt::Initialization::Uninitialized);

        qsizetype remaining = file_->size();
        while (!cancel() && remaining > 0) {
            const qsizetype willRead = remaining > buf.size() ? buf.size() : remaining;
            const qint64 hasRead = file_->read(buf.data(), willRead);
            if (hasRead <= 0)
                throw DiskAccessException("Could not read from the source file.", file_->fileName());
            if (targetFile->write(buf.data(), hasRead) != hasRead)
                throw DiskAccessException("Could not write all data to the destination file.", targetFile->fileName());
            remaining -= hasRead;
        }
    }

    qint32 FsRawBinarySource::readOriginalSize() const {
        const QFileInfo fi(path());
        return static_cast<qint32>(fi.size());
    }

    qint32 FsRawBinarySource::readTimestamp() const {
        const QFileInfo fi(path());
        return static_cast<qint32>(fi.lastModified().toSecsSinceEpoch());
    }

    bool FsRawBinarySource::isCompressed() const {
        return false;
    }
}
