#include "binarybackend.h"
#include "unpackbackend.h"
#include "io/diskaccessexception.h"
#include "util/log.h"
#include <QDateTime>
#include <QStandardPaths>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>

#define LOG(...) LOGGER("io/bb/BinaryBackend", __VA_ARGS__)

namespace pboman3::io {
    using namespace domain;
    BinaryBackend::BinaryBackend(const QString& name) {
        Q_UNUSED(name)
        const QDir root(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/published");
        if (!QDir().mkpath(root.absolutePath()))
            throw DiskAccessException("Could not create the published-file cache.", root.absolutePath());
        QThreadPool::globalInstance()->start([root] { cleanupExpiredSessions(root); });

        sessionPath_ = root.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
        const QDir session(sessionPath_);
        const QDir tree(session.filePath("tree"));
        const QDir exec(session.filePath("exec"));

        LOG(info, "Binary backend tree:", tree)
        LOG(info, "Binary backend exec:", exec)

        if (!QDir().mkpath(tree.absolutePath()))
            throw DiskAccessException("Could not create the folder.", tree.path());
        if (!QDir().mkpath(exec.absolutePath()))
            throw DiskAccessException("Could not create the folder.", exec.path());

        sessionLock_.reset(new QLockFile(session.filePath(".lock")));
        if (!sessionLock_->tryLock())
            throw DiskAccessException("Could not lock the published-file session.", sessionPath_);

        tempPath_ = tree.absolutePath();

        tempBackend_ = QSharedPointer<TempBackend>(new TempBackend(tree));
        execBackend_ = QSharedPointer<ExecBackend>(new ExecBackend(exec));
    }

    QList<QUrl> BinaryBackend::hddSync(const QList<PboNode*>& nodes, const Cancel& cancel) const {
        QList<QUrl> result = tempBackend_->hddSync(nodes, cancel);
        return result;
    }

    QString BinaryBackend::execSync(const PboNode* node, const Cancel& cancel) const {
        QString result = execBackend_->execSync(node, cancel);
        return result;
    }

    void BinaryBackend::unpackSync(const QDir& dest, const PboNode* rootNode, const QList<PboNode*>& childNodes,
                                   const Cancel& cancel) const {
        UnpackBackend unpack(dest);
        unpack.unpackSync(rootNode, childNodes, cancel);
    }

    void BinaryBackend::clear(const PboNode* node) const {
        tempBackend_->clear(node);
        if (node->nodeType() == PboNodeType::File)
            execBackend_->clear(node);
    }

    QDir BinaryBackend::getTempDir() const {
         return {tempPath_};
    }

    void BinaryBackend::cleanupExpiredSessions(const QDir& root) {
        const QDateTime expiry = QDateTime::currentDateTimeUtc().addDays(-7);
        const QFileInfoList sessions = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo& session : sessions) {
            if (session.lastModified().toUTC() >= expiry)
                continue;
            QLockFile lock(QDir(session.absoluteFilePath()).filePath(".lock"));
            if (!lock.tryLock(0))
                continue;
            lock.unlock();
            QDir(session.absoluteFilePath()).removeRecursively();
        }
    }
}
