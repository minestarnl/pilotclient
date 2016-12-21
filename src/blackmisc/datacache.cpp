/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE

#include "blackmisc/atomicfile.h"
#include "blackmisc/datacache.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/identifier.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/processinfo.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFlags>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonValuePtr>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTimer>
#include <Qt>
#include <memory>
#include <utility>
#include <chrono>

namespace BlackMisc
{

    using Private::CValuePage;
    using Private::CDataPageQueue;

    class CDataCacheRevision::LockGuard
    {
    public:
        LockGuard(const LockGuard &) = delete;
        LockGuard &operator =(const LockGuard &) = delete;
        LockGuard(LockGuard &&other) noexcept : m_movedFrom(true) { *this = std::move(other); }
        LockGuard &operator =(LockGuard &&other) noexcept
        {
            auto tuple = std::tie(other.m_movedFrom, other.m_keepPromises, other.m_rev);
            std::tie(m_movedFrom, m_keepPromises, m_rev).swap(tuple);
            return *this;
        }

        ~LockGuard()
        {
            if (! m_movedFrom) { m_rev->finishUpdate(m_keepPromises); }
        }

        operator bool() const { return ! m_movedFrom; }

    private:
        LockGuard() : m_movedFrom(true) {}
        LockGuard(CDataCacheRevision *rev) : m_movedFrom(! rev), m_rev(rev) {}
        LockGuard &keepPromises() { m_keepPromises = true; return *this; }
        friend class CDataCacheRevision;

        bool m_movedFrom = false;
        bool m_keepPromises = false;
        CDataCacheRevision *m_rev = nullptr;
    };

    CDataCache::CDataCache()
    {
        if (! QDir::root().mkpath(persistentStore()))
        {
            CLogMessage(this).error("Failed to create directory '%1'") << persistentStore();
        }

        connect(this, &CValueCache::valuesChangedByLocal, this, &CDataCache::saveToStoreAsync);
        connect(this, &CValueCache::valuesChangedByLocal, this, [ = ](CValueCachePacket values)
        {
            values.setSaved();
            changeValuesFromRemote(values, CIdentifier());
        });
        connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &CDataCache::loadFromStoreAsync);
        connect(&m_serializer, &CDataCacheSerializer::valuesLoadedFromStore, this, &CDataCache::changeValuesFromRemote, Qt::DirectConnection);

        if (! QFile::exists(m_revisionFileName)) { QFile(m_revisionFileName).open(QFile::WriteOnly); }
        m_watcher.addPath(m_revisionFileName);
        m_serializer.loadFromStore({}, false, true); // load pinned values
        QTimer::singleShot(0, this, [this] { m_serializer.start(); loadFromStoreAsync(); }); // only start the serializer if the main thread event loop runs
    }

    CDataCache *CDataCache::instance()
    {
        static CDataCache cache;
        return &cache;
    }

    const QString &CDataCache::persistentStore()
    {
        static const QString dir = getCacheRootDirectory() + "/data/cache/core";
        return dir;
    }

    QString CDataCache::filenameForKey(const QString &key)
    {
        return persistentStore() + "/" + CValueCache::filenameForKey(key);
    }

    QStringList CDataCache::enumerateStore() const
    {
        return enumerateFiles(persistentStore());
    }

    bool CDataCache::synchronize(const QString &key)
    {
        constexpr auto timeout = std::chrono::seconds(1);
        constexpr auto ready = std::future_status::ready;
        constexpr auto zero = std::chrono::seconds::zero();

        auto future = m_revision.promiseLoadedValue(key, getTimestampSync(key));
        if (future.valid())
        {
            std::future_status s {};
            do { s = future.wait_for(timeout); }
            while (s != ready && m_revision.isNewerValueAvailable(key, getTimestampSync(key)));
            if (s != ready) { s = future.wait_for(zero); }
            if (s != ready) { return false; }
            try { future.get(); }
            catch (const std::future_error &) { return false; }   // broken promise
            return true;
        }
        return false;
    }

    void CDataCache::setTimeToLive(const QString &key, int ttl)
    {
        QTimer::singleShot(0, &m_serializer, [this, key, ttl] { m_revision.setTimeToLive(key, ttl); });
    }

    void CDataCache::renewTimestamp(const QString &key, qint64 timestamp)
    {
        QTimer::singleShot(0, &m_serializer, [this, key, timestamp] { m_revision.overrideTimestamp(key, timestamp); });
    }

    qint64 CDataCache::getTimestampOnDisk(const QString &key)
    {
        return m_revision.getTimestampOnDisk(key);
    }

    void CDataCache::pinValue(const QString &key)
    {
        QTimer::singleShot(0, &m_serializer, [this, key] { m_revision.pinValue(key); });
    }

    void CDataCache::deferValue(const QString &key)
    {
        QTimer::singleShot(0, &m_serializer, [this, key] { m_revision.deferValue(key); });
    }

    void CDataCache::admitValue(const QString &key, bool triggerLoad)
    {
        m_revision.admitValue(key);
        if (triggerLoad) { loadFromStoreAsync(); }
    }

    void CDataCache::sessionValue(const QString &key)
    {
        QTimer::singleShot(0, &m_serializer, [this, key] { m_revision.sessionValue(key); });
    }

    QString lockFileError(const QLockFile &lock)
    {
        switch (lock.error())
        {
        case QLockFile::NoError: return "No error";
        case QLockFile::PermissionError: return "Insufficient permission";
        case QLockFile::UnknownError: return "Unknown filesystem error";
        case QLockFile::LockFailedError:
            {
                QString hostname, appname;
                qint64 pid = 0;
                lock.getLockInfo(&pid, &hostname, &appname);
                return QString("Lock open in another process (%1 %2 on %3)").arg(hostname, QString::number(pid), appname);
            }
        default: return "Bad error number";
        }
    }

    void CDataCache::saveToStoreAsync(const BlackMisc::CValueCachePacket &values)
    {
        QTimer::singleShot(0, &m_serializer, [this, values]
        {
            m_serializer.saveToStore(values.toVariantMap(), getAllValuesWithTimestamps());
        });
    }

    void CDataCache::loadFromStoreAsync()
    {
        QTimer::singleShot(0, &m_serializer, [this]
        {
            m_serializer.loadFromStore(getAllValuesWithTimestamps());
        });
    }

    void CDataCache::connectPage(CValuePage *page)
    {
        auto *queue = new CDataPageQueue(page);
        connect(page, &CValuePage::valuesWantToCache, this, &CDataCache::changeValues);
        connect(this, &CDataCache::valuesChanged, queue, &CDataPageQueue::queueValuesFromCache, Qt::DirectConnection);
    }

    void CDataPageQueue::queueValuesFromCache(const CValueCachePacket &values, QObject *changedBy)
    {
        QMutexLocker lock(&m_mutex);
        if (m_queue.isEmpty())
        {
            singleShot(0, this, [this] { setQueuedValuesFromCache(); });
        }
        m_queue.push_back(std::make_pair(values, changedBy));
    }

    void CDataPageQueue::setQueuedValuesFromCache()
    {
        QMutexLocker lock(&m_mutex);
        decltype(m_queue) queue;
        qSwap(m_queue, queue);
        lock.unlock();

        for (const auto &pair : queue)
        {
            m_page->setValuesFromCache(pair.first, pair.second);
        }
    }

    void CDataPageQueue::setQueuedValueFromCache(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        decltype(m_queue) filtered;
        for (auto &pair : m_queue)
        {
            if (pair.first.contains(key))
            {
                filtered.push_back({ pair.first.takeByKey(key), pair.second });
            }
        }
        lock.unlock();
        for (const auto &pair : filtered)
        {
            m_page->setValuesFromCache(pair.first, pair.second);
        }
    }

    const CLogCategoryList &CDataCacheSerializer::getLogCategories()
    {
        static const BlackMisc::CLogCategoryList cats { BlackMisc::CLogCategory::cache() };
        return cats;
    }

    CDataCacheSerializer::CDataCacheSerializer(CDataCache *owner, const QString &revisionFileName) :
        CContinuousWorker(owner),
        m_cache(owner),
        m_revisionFileName(revisionFileName)
    {}

    const QString &CDataCacheSerializer::persistentStore() const
    {
        return m_cache->persistentStore();
    }

    void CDataCacheSerializer::saveToStore(const BlackMisc::CVariantMap &values, const BlackMisc::CValueCachePacket &baseline)
    {
        m_cache->m_revision.notifyPendingWrite();
        auto lock = loadFromStore(baseline, true); // last-minute check for remote changes before clobbering the revision file
        for (const auto &key : values.keys()) { m_deferredChanges.remove(key); } // ignore changes that we are about to overwrite

        if (! lock) { return; }
        m_cache->m_revision.writeNewRevision(baseline.toTimestampMap());

        auto msg = m_cache->saveToFiles(persistentStore(), values, baseline.toTimestampMapString(values.keys()));
        msg.setCategories(this);
        CLogMessage::preformatted(msg);

        applyDeferredChanges(); // apply changes which we grabbed at the last minute above
    }

    CDataCacheRevision::LockGuard CDataCacheSerializer::loadFromStore(const BlackMisc::CValueCachePacket &baseline, bool defer, bool pinsOnly)
    {
        auto lock = m_cache->m_revision.beginUpdate(baseline.toTimestampMap(), ! pinsOnly, pinsOnly);
        if (lock && m_cache->m_revision.isPendingRead())
        {
            CValueCachePacket newValues;
            auto msg = m_cache->loadFromFiles(persistentStore(), m_cache->m_revision.keysWithNewerTimestamps(), baseline.toVariantMap(), newValues, m_cache->m_revision.timestampsAsString());
            newValues.setTimestamps(m_cache->m_revision.newerTimestamps());

            auto missingKeys = m_cache->m_revision.keysWithNewerTimestamps().subtract(newValues.keys());
            if (! missingKeys.isEmpty()) { m_cache->m_revision.writeNewRevision({}, missingKeys); }

            msg.setCategories(this);
            CLogMessage::preformatted(msg);
            m_deferredChanges.insert(newValues);
        }

        if (! defer) { applyDeferredChanges(); }
        return lock;
    }

    void CDataCacheSerializer::applyDeferredChanges()
    {
        if (! m_deferredChanges.isEmpty())
        {
            m_deferredChanges.setSaved();
            emit valuesLoadedFromStore(m_deferredChanges, CIdentifier::anonymous());
            deliverPromises(m_cache->m_revision.loadedValuePromises());
            m_deferredChanges.clear();
        }
    }

    void CDataCacheSerializer::deliverPromises(std::vector<std::promise<void>> i_promises)
    {
        QTimer::singleShot(0, Qt::PreciseTimer, this, [this, promises = std::make_shared<decltype(i_promises)>(std::move(i_promises))]()
        {
            for (auto &promise : *promises)
            {
                promise.set_value();
            }
        });
    }

    class BLACKMISC_EXPORT CDataCacheRevision::Session
    {
    public:
        // cppcheck-suppress unusedFunction
        Session(const QString &filename) : m_filename(filename) {}
        void updateSession();
        const QUuid &uuid() const { return m_uuid; }
    private:
        const QString m_filename;
        QUuid m_uuid;
    };

    CDataCacheRevision::CDataCacheRevision(const QString &basename) : m_basename(basename), m_session(std::make_unique<Session>(m_basename + "/.session")) {}

    CDataCacheRevision::~CDataCacheRevision() = default;

    CDataCacheRevision::LockGuard CDataCacheRevision::beginUpdate(const QMap<QString, qint64> &timestamps, bool updateUuid, bool pinsOnly)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        Q_ASSERT(! m_lockFile.isLocked());

        if (! m_lockFile.lock())
        {
            CLogMessage(this).error("Failed to lock %1: %2") << m_basename << lockFileError(m_lockFile);
            return {};
        }
        m_updateInProgress = true;
        LockGuard guard(this);

        m_timestamps.clear();
        m_originalTimestamps.clear();

        QFile revisionFile(m_basename + "/.rev");
        if (revisionFile.exists())
        {
            if (! revisionFile.open(QFile::ReadOnly | QFile::Text))
            {
                CLogMessage(this).error("Failed to open %1: %2") << revisionFile.fileName() << revisionFile.errorString();
                return {};
            }

            auto json = QJsonDocument::fromJson(revisionFile.readAll()).object();
            if (json.contains("uuid") && json.contains("timestamps"))
            {
                m_originalTimestamps = fromJson(json.value("timestamps").toObject());

                QUuid uuid(json.value("uuid").toString());
                if (uuid == m_uuid && m_admittedQueue.isEmpty())
                {
                    if (m_pendingWrite) { return guard; }
                    return {};
                }
                if (updateUuid) { m_uuid = uuid; }

                auto timesToLive = fromJson(json.value("ttl").toObject());
                for (auto it = m_originalTimestamps.cbegin(); it != m_originalTimestamps.cend(); ++it)
                {
                    auto current = timestamps.value(it.key(), -1);
                    auto ttl = timesToLive.value(it.key(), -1);
                    if (current < it.value() && (ttl < 0 || QDateTime::currentMSecsSinceEpoch() < it.value() + ttl))
                    {
                        m_timestamps.insert(it.key(), it.value());
                    }
                }
                if (m_timestamps.isEmpty())
                {
                    if (m_pendingWrite) { return guard; }
                    return {};
                }

                if (pinsOnly)
                {
                    auto pins = fromJson(json.value("pins").toArray());
                    for (const auto &key : m_timestamps.keys())
                    {
                        if (! pins.contains(key)) { m_timestamps.remove(key); }
                    }
                }

                auto deferrals = fromJson(json.value("deferrals").toArray());
                m_admittedValues.unite(m_admittedQueue);
                if (updateUuid) { m_admittedQueue.clear(); }
                else if (! m_admittedQueue.isEmpty()) { m_admittedQueue.intersect(QSet<QString>::fromList(m_timestamps.keys())); }

                for (const auto &key : m_timestamps.keys())
                {
                    if (deferrals.contains(key) && ! m_admittedValues.contains(key)) { m_timestamps.remove(key); }
                }

                m_session->updateSession();
                auto sessionIds = sessionFromJson(json.value("session").toObject());
                for (auto it = sessionIds.cbegin(); it != sessionIds.cend(); ++it)
                {
                    m_sessionValues[it.key()] = it.value();
                    if (it.value() != m_session->uuid()) { m_timestamps.remove(it.key()); }
                }
            }
            else if (revisionFile.size() > 0)
            {
                CLogMessage(this).error("Invalid format of %1") << revisionFile.fileName();

                if (m_pendingWrite) { return guard; }
                return {};
            }
        }

        m_pendingRead = true;
        return guard;
    }

    void CDataCacheRevision::writeNewRevision(const QMap<QString, qint64> &i_timestamps, const QSet<QString> &excludeKeys)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        Q_ASSERT(m_lockFile.isLocked());

        CAtomicFile revisionFile(m_basename + "/.rev");
        if (! revisionFile.open(QFile::WriteOnly | QFile::Text))
        {
            CLogMessage(this).error("Failed to open %1: %2") << revisionFile.fileName() << revisionFile.errorString();
            return;
        }

        m_uuid = CIdentifier().toUuid();
        auto timestamps = m_originalTimestamps;
        for (auto it = i_timestamps.cbegin(); it != i_timestamps.cend(); ++it)
        {
            if (it.value()) { timestamps.insert(it.key(), it.value()); }
        }
        for (const auto &key : excludeKeys) { timestamps.remove(key); }

        for (auto it = timestamps.cbegin(); it != timestamps.cend(); ++it)
        {
            if (m_sessionValues.contains(it.key())) { m_sessionValues[it.key()] = m_session->uuid(); }
        }

        QJsonObject json;
        json.insert("uuid", m_uuid.toString());
        json.insert("timestamps", toJson(timestamps));
        json.insert("ttl", toJson(m_timesToLive));
        json.insert("pins", toJson(m_pinnedValues));
        json.insert("deferrals", toJson(m_deferredValues));
        json.insert("session", toJson(m_sessionValues));
        revisionFile.write(QJsonDocument(json).toJson());

        if (! revisionFile.checkedClose())
        {
            CLogMessage(this).error("Failed to write to %1: %2 (%3 %4)") << revisionFile.fileName() << revisionFile.errorString() << QThread::currentThread()->objectName() << Q_FUNC_INFO;
        }
    }

    void CDataCacheRevision::finishUpdate(bool keepPromises)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        Q_ASSERT(m_lockFile.isLocked());

        m_updateInProgress = false;
        m_pendingRead = false;
        m_pendingWrite = false;
        if (! keepPromises) { breakPromises(); }
        m_lockFile.unlock();
    }

    bool CDataCacheRevision::isPendingRead() const
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        return ! m_timestamps.isEmpty();
    }

    void CDataCacheRevision::notifyPendingWrite()
    {
        QMutexLocker lock(&m_mutex);

        m_pendingWrite = true;
    }

    QSet<QString> CDataCacheRevision::keysWithNewerTimestamps() const
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        return QSet<QString>::fromList(m_timestamps.keys());
    }

    const QMap<QString, qint64> &CDataCacheRevision::newerTimestamps() const
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        return m_timestamps;
    }

    bool CDataCacheRevision::isNewerValueAvailable(const QString &key, qint64 timestamp)
    {
        QMutexLocker lock(&m_mutex);

        // Temporary guard object returned by beginUpdate is deleted at the end of the full expression,
        // don't try to split the conditional into multiple statements.
        // If a future is still waiting for the next update to begin, we don't want to break its associated promise.
        return (m_updateInProgress || m_pendingWrite || beginUpdate({{ key, timestamp }}, false).keepPromises())
        &&(m_timestamps.contains(key) || m_admittedQueue.contains(key));
    }

    std::future<void> CDataCacheRevision::promiseLoadedValue(const QString &key, qint64 currentTimestamp)
    {
        QMutexLocker lock(&m_mutex);

        if (isNewerValueAvailable(key, currentTimestamp))
        {
            std::promise<void> promise;
            auto future = promise.get_future();
            m_promises.push_back(std::move(promise));
            return future;
        }
        return {};
    }

    std::vector<std::promise<void>> CDataCacheRevision::loadedValuePromises()
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(m_updateInProgress);
        return std::move(m_promises); // move into the return value, so m_promises becomes empty
    }

    void CDataCacheRevision::breakPromises()
    {
        QMutexLocker lock(&m_mutex);

        if (! m_promises.empty())
        {
            CLogMessage(this).debug() << "Breaking" << m_promises.size() << "promises";
            m_promises.clear();
        }
    }

    QString CDataCacheRevision::timestampsAsString() const
    {
        QMutexLocker lock(&m_mutex);

        QStringList result;
        for (auto it = m_timestamps.cbegin(); it != m_timestamps.cend(); ++it)
        {
            result.push_back(it.key() + "(" + QDateTime::fromMSecsSinceEpoch(it.value(), Qt::UTC).toString(Qt::ISODate) + ")");
        }
        return result.join(",");
    }

    void CDataCacheRevision::setTimeToLive(const QString &key, int ttl)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        m_timesToLive.insert(key, ttl);
    }

    void CDataCacheRevision::overrideTimestamp(const QString &key, qint64 timestamp)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        Q_ASSERT(! m_lockFile.isLocked());

        if (! m_lockFile.lock())
        {
            CLogMessage(this).error("Failed to lock %1: %2") << m_basename << lockFileError(m_lockFile);
            m_lockFile.unlock();
            return;
        }

        CAtomicFile revisionFile(m_basename + "/.rev");
        if (revisionFile.exists())
        {
            if (! revisionFile.open(QFile::ReadWrite | QFile::Text))
            {
                CLogMessage(this).error("Failed to open %1: %2") << revisionFile.fileName() << revisionFile.errorString();
                m_lockFile.unlock();
                return;
            }

            auto json = QJsonDocument::fromJson(revisionFile.readAll()).object();
            auto timestamps = json.value("timestamps").toObject();
            timestamps.insert(key, timestamp);
            json.insert("timestamps", timestamps);

            if (!(revisionFile.seek(0) && revisionFile.resize(0) && revisionFile.write(QJsonDocument(json).toJson()) && revisionFile.checkedClose()))
            {
                CLogMessage(this).error("Failed to write to %1: %2 (%3 %4)") << revisionFile.fileName() << revisionFile.errorString() << QThread::currentThread()->objectName() << Q_FUNC_INFO;
            }
        }
        m_lockFile.unlock();
    }

    qint64 CDataCacheRevision::getTimestampOnDisk(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        if (m_lockFile.isLocked()) { return m_originalTimestamps.value(key); }

        if (! m_lockFile.lock())
        {
            CLogMessage(this).error("Failed to lock %1: %2") << m_basename << lockFileError(m_lockFile);
            m_lockFile.unlock();
            return 0;
        }

        qint64 result = 0;
        QFile revisionFile(m_basename + "/.rev");
        if (revisionFile.exists())
        {
            if (revisionFile.open(QFile::ReadOnly | QFile::Text))
            {
                auto json = QJsonDocument::fromJson(revisionFile.readAll()).object();
                result = static_cast<qint64>(json.value("timestamps").toObject().value(key).toDouble());
            }
            else
            {
                CLogMessage(this).error("Failed to open %1: %2") << revisionFile.fileName() << revisionFile.errorString();
            }
        }
        m_lockFile.unlock();
        return result;
    }

    void CDataCacheRevision::pinValue(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        m_pinnedValues.insert(key);
    }

    void CDataCacheRevision::deferValue(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        m_deferredValues.insert(key);
    }

    void CDataCacheRevision::admitValue(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        m_admittedQueue.insert(key);
    }

    void CDataCacheRevision::sessionValue(const QString &key)
    {
        QMutexLocker lock(&m_mutex);

        Q_ASSERT(! m_updateInProgress);
        m_sessionValues[key]; // insert default-constructed value, unless key already present
    }

    QJsonObject CDataCacheRevision::toJson(const QMap<QString, qint64> &timestamps)
    {
        QJsonObject result;
        for (auto it = timestamps.begin(); it != timestamps.end(); ++it)
        {
            result.insert(it.key(), it.value());
        }
        return result;
    }

    QMap<QString, qint64> CDataCacheRevision::fromJson(const QJsonObject &timestamps)
    {
        QMap<QString, qint64> result;
        for (auto it = timestamps.begin(); it != timestamps.end(); ++it)
        {
            result.insert(it.key(), static_cast<qint64>(it.value().toDouble()));
        }
        return result;
    }

    QJsonArray CDataCacheRevision::toJson(const QSet<QString> &pins)
    {
        QJsonArray result;
        for (auto it = pins.begin(); it != pins.end(); ++it)
        {
            result.push_back(*it);
        }
        return result;
    }

    QSet<QString> CDataCacheRevision::fromJson(const QJsonArray &pins)
    {
        QSet<QString> result;
        for (auto it = pins.begin(); it != pins.end(); ++it)
        {
            result.insert(it->toString());
        }
        return result;
    }

    QJsonObject CDataCacheRevision::toJson(const QMap<QString, QUuid> &timestamps)
    {
        QJsonObject result;
        for (auto it = timestamps.begin(); it != timestamps.end(); ++it)
        {
            result.insert(it.key(), it.value().toString());
        }
        return result;
    }

    QMap<QString, QUuid> CDataCacheRevision::sessionFromJson(const QJsonObject &session)
    {
        QMap<QString, QUuid> result;
        for (auto it = session.begin(); it != session.end(); ++it)
        {
            result.insert(it.key(), QUuid(it.value().toString()));
        }
        return result;
    }

    void CDataCacheRevision::Session::updateSession()
    {
        CAtomicFile file(m_filename);
        bool ok = file.open(QIODevice::ReadWrite | QFile::Text);
        if (! ok)
        {
            CLogMessage(this).error("Failed to open session file %1: %2") << m_filename << file.errorString();
            return;
        }
        auto json = QJsonDocument::fromJson(file.readAll()).object();
        QUuid uuid(json.value("uuid").toString());
        CSequence<CProcessInfo> apps;
        auto status = apps.convertFromJsonNoThrow(json.value("apps").toObject(), this, QStringLiteral("Error in %1 apps object").arg(m_filename));
        apps.removeIf([](const CProcessInfo &pi) { return ! pi.exists(); });

        if (apps.isEmpty()) { uuid = CIdentifier().toUuid(); }
        m_uuid = uuid;

        CProcessInfo currentProcess = CProcessInfo::currentProcess();
        Q_ASSERT(currentProcess.exists());
        apps.replaceOrAdd(currentProcess, currentProcess);
        json.insert("apps", apps.toJson());
        json.insert("uuid", uuid.toString());
        if (!(file.seek(0) && file.resize(0) && file.write(QJsonDocument(json).toJson()) && file.checkedClose()))
        {
            CLogMessage(this).error("Failed to write to session file %1: %2 (%3 %4)") << m_filename << file.errorString() << QThread::currentThread()->objectName() << Q_FUNC_INFO;
        }
    }

}

//! \endcond
