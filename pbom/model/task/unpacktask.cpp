#include "unpacktask.h"
#include <QDir>
#include "pbojsonhelper.h"
#include "pbojson.h"
#include "io/bb/unpacktaskbackend.h"
#include "io/bs/pbobinarysource.h"
#include "io/pbonodeentity.h"
#include "domain/pbonode.h"
#include "domain/func.h"
#include "io/diskaccessexception.h"
#include "io/createdocumentreader.h"
#include "io/pbofileformatexception.h"
#include "io/bb/sanitizedpath.h"
#include "settings/getapplicationsettingsmanager.h"
#include "util/log.h"
#include "util/filenames.h"
#include <QRegularExpression>

#define LOG(...) LOGGER("model/task/UnpackTask", __VA_ARGS__)

namespace pboman3::model::task {
    using namespace io;

    UnpackTask::UnpackTask(QString pboPath, const QString& outputDir, const bool usePboPrefix,
                           FileConflictResolutionMode::Enum fileConflictResolutionMode)
        : pboPath_(std::move(pboPath)),
          outputDir_(outputDir),
          usePboPrefix_(usePboPrefix),
          fileConflictResolutionMode_(fileConflictResolutionMode) {
    }

    TaskResult UnpackTask::execute(const Cancel& cancel) {
        LOG(info, "PBO file: ", pboPath_)
        LOG(info, "Output dir: ", outputDir_.absolutePath())
        LOG(info, "Use pbo prefix: ", usePboPrefix_)

        emit taskThinking("Preparing to extract the file: " + pboPath_);

        QSharedPointer<PboDocument> document;
        QString diagnostic;
        if (!tryReadPboHeader(&document, &diagnostic))
            return TaskResult::failure(diagnostic);

        if (cancel())
            return TaskResult::cancelled();

        const QString* pboPrefix = nullptr;
        QStringList warnings;
        if (usePboPrefix_) {
            pboPrefix = GetPrefixValueUnsanitized(*document->headers());
            if (!pboPrefix) {
                const QString message = "The PBO file contains no $prefix$ header.";
                warnings.append(message);
                emit taskMessage(message);
            }
            else {
                LOG(info, "PBO prefix: ", *pboPrefix)
            }
        }

        QDir pboDir;
        if (!tryCreatePboDir(&pboDir, pboPrefix, &diagnostic))
            return TaskResult::failure(diagnostic);

        constexpr qsizetype startProgress = 0;
        qint32 endProgress = 0;
        CountFilesInTree(*document->root(), endProgress);
        emit taskInitialized(pboPath_, startProgress, endProgress);

        QStringList errors;
        std::function onError = [this, &errors](const QString& error) {
            errors.append(error);
            emit taskMessage(error);
        };

        int progress = startProgress;
        std::function onProgress = [this, &progress]() {
            progress++;
            emit taskProgress(progress);
        };

        UnpackTaskBackend be(pboDir, fileConflictResolutionMode_);
        be.setOnError(&onError);
        be.setOnProgress(&onProgress);

        QList<PboNode*> childNodes;
        childNodes.reserve(document->root()->count());
        for (PboNode* node : *document->root())
            childNodes.append(node);
        be.unpackSync(document->root(), childNodes, cancel);

        if (cancel())
            return TaskResult::cancelled();

        if (!errors.isEmpty()) {
            TaskResult result{TaskStatus::Failure, 0, 1};
            result.diagnostics = errors;
            return result;
        }

        if (!extractPboConfig(*document, pboDir, &diagnostic))
            return TaskResult::failure(diagnostic);

        LOG(info, "Unpack complete")
        TaskResult result = TaskResult::success();
        result.diagnostics = warnings;
        return result;
    }

    QDebug operator<<(QDebug debug, const UnpackTask& task) {
        return debug << "UnpackTask(PboPath=" << task.pboPath_ << ", OutputDir=" << task.outputDir_ << ")";
    }

    bool UnpackTask::tryReadPboHeader(QSharedPointer<PboDocument>* document, QString* diagnostic) {
        try {
            const auto settings = settings::GetApplicationSettingsManager()->readSettings();
            const DocumentReader reader = CreateDocumentReader(pboPath_, settings.junkFilterEnable);
            *document = reader.read();
            LOG(debug, "The document:", *document)
            return true;
        } catch (const DiskAccessException& ex) {
            LOG(warning, "Got error while opening the file:", ex)
            *diagnostic = "Can not read the file | " + pboPath_;
            emit taskMessage(*diagnostic);
            return false;
        } catch (const PboFileFormatException& ex) {
            LOG(warning, "Got error while reading the file document:", ex)
            *diagnostic = "The file is not a PBO | " + pboPath_;
            emit taskMessage(*diagnostic);
            return false;
        }
    }

    bool UnpackTask::tryCreatePboDir(QDir* dir, const QString* pboPrefix, QString* diagnostic) {
        QString extractPath;
        try {
            if (!tryUsePboPrefixAsPath(pboPrefix, extractPath)) {
                extractPath = FileNames::getFileNameWithoutExtension(QFileInfo(pboPath_).fileName());
            }
        } catch (const PboPrefixException& ex) {
            LOG(warning, ex.message())
            *diagnostic = ex.windowMessage();
            emit taskMessage(*diagnostic);
            return false;
        }

        QString absPath = outputDir_.absoluteFilePath(extractPath);
        if (!outputDir_.exists(extractPath) && !outputDir_.mkpath(extractPath)) {
            LOG(warning, "Could not create the directory:", absPath)
            *diagnostic = "Could not create the directory | " + absPath;
            emit taskMessage(*diagnostic);
            return false;
        }

        LOG(info, "PBO output dir:", absPath)
        *dir = QDir(absPath);
        return true;
    }

    bool UnpackTask::tryCreateEntryDir(const QDir& pboDir, const QSharedPointer<PboNode>& entry) {
        QDir local(pboDir);
        PboPath path = entry->makePath();
        auto it = path.begin();
        const auto last = path.end() - 1;

        while (it != last) {
            if (!local.exists(*it) && !local.mkdir(*it)) {
                LOG(warning, "Could not create the directory:", local.absolutePath())
                emit taskMessage("Could not create the directory | " + local.absolutePath());
                return false;
            }
            local.cd(*it);
            ++it;
        }

        return true;
    }

    bool UnpackTask::extractPboConfig(const PboDocument& document, const QDir& dir, QString* diagnostic) {
        const PboJson options = PboJsonHelper::extractFrom(document);
        LOG(info, "Extracted the PBO pack config, Options=", options)
        try {
            const QString configFilePath = PboJsonHelper::getConfigFilePath(dir, fileConflictResolutionMode_);
            PboJsonHelper::saveTo(options, configFilePath);
            return true;
        } catch (const DiskAccessException& ex) {
            LOG(info, ex.message())
            //remove the "." symbol from the end
            *diagnostic = ex.message().left(ex.message().length() - 1) + " | " + ex.file();
            emit taskMessage(*diagnostic);
            return false;
        }
    }

    bool UnpackTask::tryUsePboPrefixAsPath(const QString* pboPrefix, QString& result) {
        if (!pboPrefix || pboPrefix->isEmpty()) {
            return false;
        }

        const QString raw = *pboPrefix;
        static const QRegularExpression drivePath("^[A-Za-z]:");
        if (raw.startsWith('/') || raw.startsWith('\\') || drivePath.match(raw).hasMatch()) {
            throw PboPrefixException("PBO prefix must be a relative path:" + *pboPrefix,
                                     "PBO prefix must be a relative path | " + *pboPrefix);
        }

        const QStringList rawSegments = raw.split(QRegularExpression("[\\\\/]"), Qt::SkipEmptyParts);
        QStringList safeSegments;
        safeSegments.reserve(rawSegments.size());
        for (const QString& segment : rawSegments) {
            if (segment == ".")
                continue;
            if (segment == "..") {
                throw PboPrefixException("The directory must not leave the unpack folder:" + *pboPrefix,
                                         "The directory must not leave the unpack folder | " + *pboPrefix);
            }
            safeSegments.append(segment);
        }

        if (safeSegments.isEmpty())
            return false;

        SanitizedPath sp(safeSegments.join('/'));
        result = sp;
        return true;
    }

    PBOMAN_EX_IMPL_DEFAULT(UnpackTask::PboPrefixException)
}
