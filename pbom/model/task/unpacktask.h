#pragma once

#include <QDir>
#include "task.h"
#include "domain/pbodocument.h"
#include "io/fileconflictresolutionmode.h"
#include "exception.h"

namespace pboman3::model::task {
    using namespace domain;
    using namespace io;

    class UnpackTask : public Task {
    public:
        UnpackTask(QString pboPath, const QString& outputDir, const bool usePboPrefix,
                   FileConflictResolutionMode::Enum fileConflictResolutionMode);

        [[nodiscard]] TaskResult execute(const Cancel& cancel) override;

        friend QDebug operator <<(QDebug debug, const UnpackTask& task);

    private:
        QString pboPath_;
        QDir outputDir_;
        bool usePboPrefix_;
        FileConflictResolutionMode::Enum fileConflictResolutionMode_;

        bool tryReadPboHeader(QSharedPointer<PboDocument>* document, QString* diagnostic);

        bool tryCreatePboDir(QDir* dir, const QString* pboPrefix, QString* diagnostic);

        bool tryCreateEntryDir(const QDir& pboDir, const QSharedPointer<PboNode>& entry);

        bool extractPboConfig(const PboDocument& document, const QDir& dir, QString* diagnostic);

        static bool tryUsePboPrefixAsPath(const QString* pboPrefix, QString& result);

    public:
        class PboPrefixException : public AppException {
        public:
            PboPrefixException(QString consoleMessage, QString windowMessage)
                : AppException(std::move(consoleMessage)),
                  windowMessage_(std::move(windowMessage)) {
            }

            PBOMAN_EX_HEADER(PboPrefixException)

            [[nodiscard]] const QString& windowMessage() const {
                return windowMessage_;
            }

        private:
            QString windowMessage_;
        };
    };
}
