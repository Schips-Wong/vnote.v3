#include "bundlenotebook.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <core/configmgr.h>
#include <core/exception.h>
#include <core/historymgr.h>
#include <notebookbackend/inotebookbackend.h>
#include <notebookconfigmgr/bundlenotebookconfigmgr.h>
#include <notebookconfigmgr/notebookconfig.h>
#include <utils/fileutils.h>
#include <utils/pathutils.h>

#include "notebookdatabaseaccess.h"
#include "notebooktagmgr.h"

using namespace vnotex;

BundleNotebook::BundleNotebook(const NotebookParameters &p_paras,
                               const QSharedPointer<NotebookConfig> &p_notebookConfig,
                               QObject *p_parent)
    : Notebook(p_paras, p_parent), m_configVersion(p_notebookConfig->m_version),
      m_history(p_notebookConfig->m_history), m_tagGraph(p_notebookConfig->m_tagGraph),
      m_extraConfigs(p_notebookConfig->m_extraConfigs) {
  setupDatabase();
}

BundleNotebook::~BundleNotebook() { m_dbAccess->close(); }

BundleNotebookConfigMgr *BundleNotebook::getBundleNotebookConfigMgr() const {
  return static_cast<BundleNotebookConfigMgr *>(getConfigMgr().data());
}

void BundleNotebook::setupDatabase() {
  const auto dbPath = getLocalDatabasePath();

  // Make sure the cache folder exists.
  QDir().mkpath(QFileInfo(dbPath).absolutePath());

  m_dbAccess = new NotebookDatabaseAccess(this, dbPath, this);
}

QString BundleNotebook::getLocalDatabasePath() const {
  // Key the cache by the notebook root path so that different notebooks never share a database
  // file, and the same notebook always maps to the same local file on this machine.
  const QString rootPath = PathUtils::cleanPath(getRootFolderAbsolutePath());
  const QString key = QString::fromLatin1(
      QCryptographicHash::hash(rootPath.toUtf8(), QCryptographicHash::Sha1).toHex());

  const QString cacheFolder = PathUtils::concatenateFilePath(
      ConfigMgr::getInst().getUserFolder(), QStringLiteral("notebook_db_cache"));
  return PathUtils::concatenateFilePath(cacheFolder, key + QStringLiteral(".db"));
}

void BundleNotebook::initializeInternal() {
  initDatabase();

  if (m_configVersion != getConfigMgr()->getCodeVersion()) {
    updateNotebookConfig();
  }
}

void BundleNotebook::initDatabase() {
  m_dbAccess->initialize(m_configVersion);

  // The database is now a per-machine local cache. Remove the legacy in-notebook database file
  // (if any) so that it is no longer synchronized/shared and cannot cause conflicts.
  {
    const auto legacyPath = BundleNotebookConfigMgr::getDatabasePath();
    if (getBackend()->exists(legacyPath)) {
      try {
        getBackend()->removeFile(legacyPath);
      } catch (Exception &p_e) {
        qWarning() << "failed to remove legacy database file" << legacyPath << p_e.what();
      }
    }
  }

  if (m_dbAccess->isFresh()) {
    // For previous version notebook without DB, just ignore the node Id from config.
    int cnt = 0;
    fillNodeTableFromConfig(getRootNode().data(), m_configVersion < 2, cnt);
    qDebug() << "fillNodeTableFromConfig nodes count" << cnt;

    fillTagTableFromTagGraph();

    cnt = 0;
    fillTagTableFromConfig(getRootNode().data(), cnt);
  }

  if (m_tagMgr) {
    m_tagMgr->update();
  }
}

void BundleNotebook::updateNotebookConfig() { getBundleNotebookConfigMgr()->writeNotebookConfig(); }

void BundleNotebook::removeNotebookConfig() {
  getBundleNotebookConfigMgr()->removeNotebookConfig();
}

void BundleNotebook::remove() {
  // Remove all nodes.
  removeNode(getRootNode());

  // Remove notebook config.
  removeNotebookConfig();

  // Remove the local database cache.
  m_dbAccess->close();
  QFile::remove(getLocalDatabasePath());

  // Remove notebook root folder if it is empty.
  if (!FileUtils::removeDirIfEmpty(getRootFolderAbsolutePath())) {
    qInfo() << QStringLiteral("root folder of notebook (%1) is not empty and needs manual clean up")
                   .arg(getRootFolderAbsolutePath());
  }
}

HistoryI *BundleNotebook::history() { return this; }

const QVector<HistoryItem> &BundleNotebook::getHistory() const { return m_history; }

void BundleNotebook::removeHistory(const QString &p_itemPath) {
  HistoryMgr::removeHistoryItem(m_history, p_itemPath);

  updateNotebookConfig();
}

void BundleNotebook::renameHistory(const QString &p_oldPath, const QString &p_newPath) {
  HistoryMgr::renameHistoryItem(m_history, p_oldPath, p_newPath);

  updateNotebookConfig();
}

void BundleNotebook::addHistory(const HistoryItem &p_item) {
  HistoryItem item(p_item);
  item.m_path = getBackend()->getRelativePath(item.m_path);
  HistoryMgr::insertHistoryItem(m_history, item);

  updateNotebookConfig();
}

void BundleNotebook::clearHistory() {
  m_history.clear();

  updateNotebookConfig();
}

const QJsonObject &BundleNotebook::getExtraConfigs() const { return m_extraConfigs; }

void BundleNotebook::setExtraConfig(const QString &p_key, const QJsonObject &p_obj) {
  m_extraConfigs[p_key] = p_obj;

  updateNotebookConfig();
}

void BundleNotebook::fillNodeTableFromConfig(Node *p_node, bool p_ignoreId, int &p_totalCnt) {
  bool ret = m_dbAccess->addNode(p_node, p_ignoreId);
  if (!ret) {
    qWarning() << "failed to add node to DB" << p_node->getName() << p_ignoreId;
    return;
  }

  if (++p_totalCnt % 10) {
    QCoreApplication::processEvents();
  }

  const auto &children = p_node->getChildrenRef();
  for (const auto &child : children) {
    fillNodeTableFromConfig(child.data(), p_ignoreId, p_totalCnt);
  }
}

NotebookDatabaseAccess *BundleNotebook::getDatabaseAccess() const { return m_dbAccess; }

bool BundleNotebook::rebuildDatabase() {
  Q_ASSERT(m_dbAccess);
  m_dbAccess->close();

  const auto dbPath = getLocalDatabasePath();
  QFile dbFile(dbPath);
  if (dbFile.exists() && !dbFile.remove()) {
    qWarning() << "failed to delete database file" << dbPath;
    if (!m_dbAccess->open()) {
      qWarning() << "failed to open notebook database (restart is needed)";
    }
    return false;
  }

  m_dbAccess->deleteLater();

  setupDatabase();
  initDatabase();

  emit tagsUpdated();

  return true;
}

const QString &BundleNotebook::getTagGraph() const { return m_tagGraph; }

void BundleNotebook::updateTagGraph(const QString &p_tagGraph) {
  if (m_tagGraph == p_tagGraph) {
    return;
  }

  m_tagGraph = p_tagGraph;
  updateNotebookConfig();
}

void BundleNotebook::fillTagTableFromTagGraph() {
  auto tagGraph = NotebookTagMgr::stringToTagGraph(m_tagGraph);
  for (const auto &tagPair : tagGraph) {
    if (!m_dbAccess->addTag(tagPair.m_parent)) {
      qWarning() << "failed to add tag to DB" << tagPair.m_parent;
      continue;
    }

    if (!m_dbAccess->addTag(tagPair.m_child, tagPair.m_parent)) {
      qWarning() << "failed to add tag to DB" << tagPair.m_child;
      continue;
    }
  }

  QCoreApplication::processEvents();
}

void BundleNotebook::fillTagTableFromConfig(Node *p_node, int &p_totalCnt) {
  // @p_node must already exists in node table.
  bool ret = m_dbAccess->updateNodeTags(p_node);
  if (!ret) {
    qWarning() << "failed to add tags of node to DB" << p_node->getName() << p_node->getTags();
    return;
  }

  if (++p_totalCnt % 10) {
    QCoreApplication::processEvents();
  }

  const auto &children = p_node->getChildrenRef();
  for (const auto &child : children) {
    fillTagTableFromConfig(child.data(), p_totalCnt);
  }
}

NotebookTagMgr *BundleNotebook::getTagMgr() const {
  if (!m_tagMgr) {
    auto th = const_cast<BundleNotebook *>(this);
    th->m_tagMgr = new NotebookTagMgr(th);
  }

  return m_tagMgr;
}

TagI *BundleNotebook::tag() { return getTagMgr(); }

int BundleNotebook::getConfigVersion() const { return m_configVersion; }
