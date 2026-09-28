#include "unpacktaskbackend.h"
#include "io/diskaccessexception.h"
#include "io/fileconflictresolutionpolicy.h"
#include "util/log.h"

#define LOG(...) LOGGER("io/bb/UnpackTaskBackend", __VA_ARGS__)

namespace pboman3::io {
    UnpackTaskBackend::UnpackTaskBackend(const QDir& folder, FileConflictResolutionMode::Enum conflictResolutionMode)
        : UnpackBackend(folder),
          onError_(nullptr),
          onProgress_(nullptr) {
        conflictResolutionPolicy_ = QSharedPointer<FileConflictResolutionPolicy>(
            new FileConflictResolutionPolicy(conflictResolutionMode));
    }

    void UnpackTaskBackend::setOnError(std::function<void(const QString&)>* callback) {
        onError_ = callback;
    }

    void UnpackTaskBackend::setOnProgress(std::function<void()>* callback) {
        onProgress_ = callback;
    }

    void UnpackTaskBackend::unpackFileNode(const PboNode* rootNode, const PboNode* childNode,
                                           const Cancel& cancel) const {
        LOG(debug, "Unpack the node", childNode->title())

        QString basePath;
        try {
            basePath = nodeFileSystem_->allocatePath(rootNode, childNode);
        } catch (const DiskAccessException& ex) {
            LOG(warning, ex)
            //remove the "." symbol from the end
            error(ex.message().left(ex.message().length() - 1) + " | " + ex.file());
            progress();
            return;
        }

        if (cancel())
            return;

        QString filePath;
        while (!cancel()) {
            try {
                filePath = conflictResolutionPolicy_->resolvePotentialConflicts(basePath);
                const bool overwrite = conflictResolutionPolicy_->mode() == FileConflictResolutionMode::Enum::Overwrite;
                writeFileAtomically(filePath, childNode, cancel, overwrite);
                break;
            } catch (const DiskAccessException& ex) {
                const bool retryCopy = conflictResolutionPolicy_->mode() == FileConflictResolutionMode::Enum::Copy
                    && QFileInfo::exists(filePath);
                if (retryCopy)
                    continue;
                LOG(warning, ex.message())
                error(ex.message() + " | " + filePath);
                break;
            } catch (const AppException& ex) {
                LOG(warning, ex.message())
                error(ex.message() + " | " + filePath);
                break;
            }
        }

        progress();
    }

    void UnpackTaskBackend::error(const QString& error) const {
        if (onError_) {
            (*onError_)(error);
        }
    }

    void UnpackTaskBackend::progress() const {
        if (onProgress_) {
            (*onProgress_)();
        }
    }
}
