#include "pbofile.h"

namespace pboman3::io {
    PboFile::PboFile(const QString& name)
        : QFile(name) {
    }

    int PboFile::readCString(QString& value) {
        bool found = false;
        int len = 0;
        startTransaction();
        while (!atEnd()) {
            char data;
            if (read(&data, sizeof data) && data == 0) {
                found = true;
                break;
            }
            len++;
        }
        rollbackTransaction();

        if (found) {
            const auto initialPos = pos();
            if (len) {
                QByteArray bytes(len, Qt::Initialization::Uninitialized);
                if (read(bytes.data(), len) != len)
                    return 0;
                value = QString::fromUtf8(bytes);
            }
            seek(pos() + 1);
            return static_cast<int>(pos() - initialPos);
        }
        return 0;
    }

    int PboFile::writeCString(const QString& value) {
        const QByteArray bytes = value.toUtf8();
        if (write(bytes) != bytes.size())
            return 0;
        constexpr char zero = 0;
        if (write(&zero, sizeof zero) != sizeof zero)
            return 0;
        return static_cast<int>(bytes.size() + sizeof zero);
    }
}
