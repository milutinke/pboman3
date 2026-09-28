#include "unpackbackend.h"
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include "io/diskaccessexception.h"
#include "exception.h"
#include "util/log.h"

#define LOG(...) LOGGER("io/bb/UnpackBackend", __VA_ARGS__)

namespace pboman3::io {
    io::UnpackBackend::UnpackBackend(const QDir& folder) {
        if (!folder.exists())
            throw InvalidOperationException("The folder provided must exist");
        nodeFileSystem_ = QSharedPointer<NodeFileSystem>(new NodeFileSystem(folder));
    }

    void UnpackBackend::unpackSync(const PboNode* rootNode, const QList<PboNode*>& childNodes,
                                 const Cancel& cancel) {
        assert(rootNode);

        LOG(info, "Unpack", childNodes.count(), "nodes")

        for (const PboNode* childNode : childNodes) {
            if (cancel()) {
                LOG(info, "The extraction was cancelled - exiting")
                break;
            }

            unpackNode(rootNode, childNode, cancel);
        }
    }

    void UnpackBackend::unpackNode(const PboNode* rootNode, const PboNode* childNode,
                                   const Cancel& cancel) {
        if (childNode->nodeType() == PboNodeType::File)
            unpackFileNode(rootNode, childNode, cancel);
        else
            unpackFolderNode(rootNode, childNode, cancel);
    }

    void UnpackBackend::unpackFolderNode(const PboNode* rootNode, const PboNode* childNode,
                                         const Cancel& cancel) {
        for (const PboNode* child : *childNode) {
            if (cancel()) {
                LOG(info, "The extraction was cancelled - exiting")
                break;
            }
            unpackNode(rootNode, child, cancel);
        }
    }

    void UnpackBackend::unpackFileNode(const PboNode* rootNode, const PboNode* childNode,
                                       const Cancel& cancel) const {
        LOG(info, "Unpack the node", childNode->title())

        const QString filePath = nodeFileSystem_->allocatePath(rootNode, childNode);
        if (cancel())
            return;

        writeFileAtomically(filePath, childNode, cancel, true);
    }

    bool UnpackBackend::writeFileAtomically(const QString& filePath, const PboNode* childNode,
                                            const Cancel& cancel, const bool overwrite) const {
        const QFileInfo destination(filePath);
        QTemporaryFile staged(destination.dir().filePath(".pboman3-XXXXXX.tmp"));
        staged.setAutoRemove(true);
        if (!staged.open()) {
            throw DiskAccessException("Can not create a temporary output file.", filePath);
        }

        LOG(info, "Writing staged output", staged.fileName())
        const auto bsClose = qScopeGuard([&childNode] {
            if (childNode->binarySource->isOpen())
                childNode->binarySource->close();
        });
        childNode->binarySource->open();
        childNode->binarySource->writeToFs(&staged, cancel);

        if (cancel())
            return false;
        if (!staged.flush())
            throw DiskAccessException("Could not flush the temporary output file.", filePath);
        staged.close();

        if (!overwrite || !QFileInfo::exists(filePath)) {
            if (!staged.rename(filePath))
                throw DiskAccessException("Could not publish the extracted file.", filePath);
            staged.setAutoRemove(false);
            return true;
        }

        QTemporaryFile backup(destination.dir().filePath(".pboman3-XXXXXX.bak"));
        backup.setAutoRemove(false);
        if (!backup.open())
            throw DiskAccessException("Could not reserve a backup file.", filePath);
        const QString backupPath = backup.fileName();
        backup.close();
        if (!backup.remove())
            throw DiskAccessException("Could not prepare the backup file.", filePath);

        if (!QFile::rename(filePath, backupPath))
            throw DiskAccessException("Could not preserve the existing file.", filePath);

        if (!staged.rename(filePath)) {
            QFile::rename(backupPath, filePath);
            throw DiskAccessException("Could not publish the extracted file.", filePath);
        }
        staged.setAutoRemove(false);
        if (!QFile::remove(backupPath))
            LOG(warning, "Could not remove extraction backup", backupPath)
        return true;
    }
}
