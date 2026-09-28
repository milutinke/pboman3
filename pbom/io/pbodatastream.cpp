#include "pbodatastream.h"

namespace pboman3::io {
    PboDataStream::PboDataStream(PboFile* file)
        : QDataStream(file),
          file_(file) {
    }

    PboDataStream& PboDataStream::operator>>(QString& out) {
        if (!file_->readCString(out)) {
            throw PboEofException();
        }
        return *this;
    }

    PboDataStream& PboDataStream::operator<<(const QString& src) {
        if (!file_->writeCString(src))
            throw PboEofException();
        return *this;
    }

}
