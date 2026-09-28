#include "nodefilesystem.h"
#include "sanitizedstring.h"
#include "io/diskaccessexception.h"
#include "util/log.h"
#include <QCryptographicHash>
#include <QFileInfo>

#define LOG(...) LOGGER("io/bb/NodeFileSystem", __VA_ARGS__)

namespace pboman3::io {
    using namespace domain;

    NodeFileSystem::NodeFileSystem(const QDir& folder)
        : QObject(),
          folder_(folder) {
        const QFileInfo root(folder.absolutePath());
        if (root.isSymLink() || !root.isDir())
            throw DiskAccessException("The output folder must be a real directory.", folder.absolutePath());
    }

    QString NodeFileSystem::allocatePath(const PboNode* node) const {
        assert(node);

        const QList<const PboNode*> parents = getParents(node);
        QString path = allocatePath(parents, node);

        return path;
    }

    QString NodeFileSystem::allocatePath(const PboNode* parent, const PboNode* node) const {
        assert(parent);
        assert(node);

        QList<const PboNode*> parents;
        parents.reserve(node->depth() - parent->depth());

        const PboNode* p = node->parentNode();
        while (p && p != parent) {
            parents.prepend(p);
            p = p->parentNode();
        }
        if (!p) {
            LOG(critical, "The provided rootNode is not a real parent of the provided childNode")
            throw InvalidOperationException("The provided rootNode is not a real parent of the provided childNode");
        }

        QString path = allocatePath(parents, node);

        return path;
    }

    QString NodeFileSystem::composeAbsolutePath(const PboNode* node) const {
        const QString fs = folder_.absolutePath() + QDir::separator();
        QString path = composePath(node, fs);
        return path;
    }

    QString NodeFileSystem::composeRelativePath(const PboNode* node) const {
        QString path = composePath(node, "");
        return path;
    }

    QList<const PboNode*> NodeFileSystem::getParents(const PboNode* node) const {
        QList<const PboNode*> parents;
        parents.reserve(node->depth());
        const PboNode* p = node->parentNode();
        while (p->parentNode()) {
            parents.prepend(p);
            p = p->parentNode();
        }
        return parents;
    }

    QString NodeFileSystem::allocatePath(const QList<const PboNode*>& parents, const PboNode* node) const {
        QDir local(folder_);
        for (const PboNode* par : parents) {
            const QString title = allocateSegment(par);
            const QString candidate = local.filePath(title);
            const QFileInfo info(candidate);
            if (info.isSymLink() || (info.exists() && !info.isDir()))
                throw DiskAccessException("The output path contains an unsafe directory component.", candidate);
            if (!info.exists() && !local.mkdir(title))
                throw DiskAccessException("Could not create the folder.", candidate);
            local.cd(title);
        }

        const QString path = local.filePath(allocateSegment(node));
        if (QFileInfo(path).isSymLink())
            throw DiskAccessException("The output path points to a symbolic link.", path);
        return path;
    }

    QString NodeFileSystem::composePath(const PboNode* node, const QString& rootPath) const {
        QString fs = rootPath;

        QList<const PboNode*> parents = getParents(node);

        QDir local(folder_);
        for (const PboNode* par : parents) {
            const QString title = allocateSegment(par);
            fs.append(title).append(QDir::separator());
            local.cd(title);
        }

        const QString title = allocateSegment(node);
        fs.append(title);

        return fs;
    }

    QString NodeFileSystem::allocateSegment(const PboNode* node) {
        const QString title = node->title();
        const QString base = SanitizedString(title);
        const PboNode* parent = node->parentNode();
        if (!parent)
            return base;

        int collisions = 0;
        for (const PboNode* sibling : *parent) {
            const QString siblingBase = SanitizedString(sibling->title());
            if (QString::compare(base, siblingBase, Qt::CaseInsensitive) == 0)
                ++collisions;
        }
        if (collisions < 2)
            return base;

        const QByteArray digest = QCryptographicHash::hash(title.toUtf8(), QCryptographicHash::Sha256).toHex().left(8);
        return base + "-" + QString::fromLatin1(digest);
    }
}
