/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/context/contextapplicationproxy.h"
#include "blackmisc/dbus.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/genericdbusinterface.h"
#include "blackmisc/identifierlist.h"
#include "blackmisc/loghandler.h"

#include <QDBusConnection>
#include <QLatin1Literal>
#include <QObject>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackCore
{
    namespace Context
    {
        CContextApplicationProxy::CContextApplicationProxy(const QString &serviceName, QDBusConnection &connection, CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) : IContextApplication(mode, runtime)
        {
            this->m_dBusInterface = new CGenericDBusInterface(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(), connection, this);
            this->relaySignals(serviceName, connection);

            connect(this, &IContextApplication::messageLogged, this, [](const CStatusMessage & message, const CIdentifier & origin)
            {
                if (!origin.isFromSameProcess())
                {
                    CLogHandler::instance()->logRemoteMessage(message);
                }
            });
        }

        void CContextApplicationProxy::relaySignals(const QString &serviceName, QDBusConnection &connection)
        {
            // signals originating from impl side
            bool s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                        "messageLogged", this, SIGNAL(messageLogged(BlackMisc::CStatusMessage, BlackMisc::CIdentifier)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "logSubscriptionAdded", this, SIGNAL(logSubscriptionAdded(BlackMisc::CIdentifier, BlackMisc::CLogPattern)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "logSubscriptionRemoved", this, SIGNAL(logSubscriptionRemoved(BlackMisc::CIdentifier, BlackMisc::CLogPattern)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "settingsChanged", this, SIGNAL(settingsChanged(BlackMisc::CValueCachePacket, BlackMisc::CIdentifier)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "registrationChanged", this, SIGNAL(registrationChanged()));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "fakedSetComVoiceRoom", this, SIGNAL(fakedSetComVoiceRoom(BlackMisc::Audio::CVoiceRoomList)));
            Q_ASSERT(s);
            s = connection.connect(serviceName, IContextApplication::ObjectPath(), IContextApplication::InterfaceName(),
                                   "hotkeyActionsRegistered", this, SIGNAL(hotkeyActionsRegistered(QStringList, BlackMisc::CIdentifier)));
            Q_UNUSED(s);
            this->relayBaseClassSignals(serviceName, connection, IContextApplication::ObjectPath(), IContextApplication::InterfaceName());
        }

        void CContextApplicationProxy::logMessage(const CStatusMessage &message, const CIdentifier &origin)
        {
            if (subscribersOf(message).containsAnyNotIn(CIdentifierList({ origin, {} })))
            {
                this->m_dBusInterface->callDBus(QLatin1Literal("logMessage"), message, origin);
            }
        }

        void CContextApplicationProxy::addLogSubscription(const CIdentifier &subscriber, const CLogPattern &pattern)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("addLogSubscription"), subscriber, pattern);
        }

        void CContextApplicationProxy::removeLogSubscription(const CIdentifier &subscriber, const CLogPattern &pattern)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("removeLogSubscription"), subscriber, pattern);
        }

        CLogSubscriptionHash CContextApplicationProxy::getAllLogSubscriptions() const
        {
            return this->m_dBusInterface->callDBusRet<CLogSubscriptionHash>(QLatin1Literal("getAllLogSubscriptions"));
        }

        void CContextApplicationProxy::synchronizeLogSubscriptions()
        {
            // note this proxy method does not call synchronizeLogSubscriptions in core
            m_logSubscriptions = getAllLogSubscriptions();
            for (const auto &pattern : CLogHandler::instance()->getAllSubscriptions()) { this->addLogSubscription({}, pattern); }
        }

        void CContextApplicationProxy::changeSettings(const CValueCachePacket &settings, const CIdentifier &origin)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("changeSettings"), settings, origin);
        }

        BlackMisc::CValueCachePacket CContextApplicationProxy::getAllSettings() const
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CValueCachePacket>(QLatin1Literal("getAllSettings"));
        }

        QStringList CContextApplicationProxy::getUnsavedSettingsKeys() const
        {
            return this->m_dBusInterface->callDBusRet<QStringList>(QLatin1Literal("getUnsavedSettingsKeys"));
        }

        void CContextApplicationProxy::synchronizeLocalSettings()
        {
            // note this proxy method does not call synchronizeLocalSettings in core
            CSettingsCache::instance()->changeValuesFromRemote(this->getAllSettings(), CIdentifier::anonymous());
        }

        BlackMisc::CStatusMessage CContextApplicationProxy::saveSettings(const QString &keyPrefix)
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CStatusMessage>(QLatin1Literal("saveSettings"), keyPrefix);
        }

        BlackMisc::CStatusMessage CContextApplicationProxy::saveSettingsByKey(const QStringList &keys)
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CStatusMessage>(QLatin1Literal("saveSettingsByKey"), keys);
        }

        BlackMisc::CStatusMessage CContextApplicationProxy::loadSettings()
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CStatusMessage>(QLatin1Literal("loadSettings"));
        }

        void CContextApplicationProxy::registerHotkeyActions(const QStringList &actions, const CIdentifier &origin)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("registerHotkeyActions"), actions, origin);
        }

        void CContextApplicationProxy::callHotkeyAction(const QString &action, bool argument, const CIdentifier &origin)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("callHotkeyAction"), action, argument, origin);
        }

        BlackMisc::CIdentifier CContextApplicationProxy::registerApplication(const CIdentifier &application)
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CIdentifier>(QLatin1Literal("registerApplication"), application);
        }

        void CContextApplicationProxy::unregisterApplication(const CIdentifier &application)
        {
            this->m_dBusInterface->callDBus(QLatin1Literal("unregisterApplication"), application);
        }

        BlackMisc::CIdentifierList CContextApplicationProxy::getRegisteredApplications() const
        {
            return this->m_dBusInterface->callDBusRet<BlackMisc::CIdentifierList>(QLatin1Literal("getRegisteredApplications"));
        }

        bool CContextApplicationProxy::writeToFile(const QString &fileName, const QString &content)
        {
            if (fileName.isEmpty()) { return false; }
            return this->m_dBusInterface->callDBusRet<bool>(QLatin1Literal("writeToFile"), fileName, content);
        }

        QString CContextApplicationProxy::readFromFile(const QString &fileName) const
        {
            if (fileName.isEmpty()) { return ""; }
            return this->m_dBusInterface->callDBusRet<QString>(QLatin1Literal("readFromFile"), fileName);
        }

        bool CContextApplicationProxy::removeFile(const QString &fileName)
        {
            if (fileName.isEmpty()) { return false; }
            return this->m_dBusInterface->callDBusRet<bool>(QLatin1Literal("removeFile"), fileName);
        }

        bool CContextApplicationProxy::existsFile(const QString &fileName) const
        {
            if (fileName.isEmpty()) { return false; }
            return this->m_dBusInterface->callDBusRet<bool>(QLatin1Literal("existsFile"), fileName);
        }

        bool CContextApplicationProxy::isContextResponsive(const QString &dBusAddress, QString &msg, int timeoutMs)
        {
            const bool connected = CDBusServer::isDBusAvailable(dBusAddress, msg, timeoutMs);
            if (!connected) { return false; }

            static const QString dBusName("contexttest");
            QDBusConnection connection = CDBusServer::connectToDBus(dBusAddress, dBusName);
            CContextApplicationProxy proxy(BlackMisc::CDBusServer::coreServiceName(), connection, CCoreFacadeConfig::Remote, nullptr);
            const CIdentifier id("swift proxy test");
            const CIdentifier pingId = proxy.registerApplication(id);
            const bool ok = (id == pingId);
            if (ok)
            {
                proxy.unregisterApplication(id);
            }
            else
            {
                msg = "Mismatch in proxy ping, context not ready.";
            }
            CDBusServer::disconnectFromDBus(connection, dBusAddress);
            return ok;
        }
    } // namespace
} // namespace
