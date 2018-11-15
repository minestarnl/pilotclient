/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "settingsxswiftbuscomponent.h"
#include "ui_settingsxswiftbuscomponent.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/simulation/xplane/xswiftbusconfigwriter.h"

#include <QComboBox>

using namespace BlackMisc;
using namespace BlackMisc::Simulation::Settings;
using namespace BlackMisc::Simulation::XPlane;

namespace BlackGui
{
    namespace Components
    {
        const CLogCategoryList &CSettingsXSwiftBusComponent::getLogCategories()
        {
            static const CLogCategoryList cats { CLogCategory::guiComponent() };
            return cats;
        }

        CSettingsXSwiftBusComponent::CSettingsXSwiftBusComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CSettingsXSwiftBusComponent)
        {
            ui->setupUi(this);

            connect(ui->pb_Save, &QPushButton::released, this, &CSettingsXSwiftBusComponent::saveServer);
            connect(ui->pb_Reset, &QPushButton::released, this, &CSettingsXSwiftBusComponent::resetServer);

            const QString dBusAddress = m_xSwiftBusServerSetting.get();
            ui->comp_DBusServer->setForXSwiftBus();
            ui->comp_DBusServer->set(dBusAddress);
        }

        CSettingsXSwiftBusComponent::~CSettingsXSwiftBusComponent()
        { }

        void CSettingsXSwiftBusComponent::resetServer()
        {
            const QString s = TXSwiftBusServer::defaultValue();
            ui->comp_DBusServer->set(s);
        }

        void CSettingsXSwiftBusComponent::saveServer()
        {
            const QString dBusAddress = ui->comp_DBusServer->getDBusAddress();
            if (dBusAddress.isEmpty()) { return; }
            if (dBusAddress != m_xSwiftBusServerSetting.getThreadLocal())
            {
                const CStatusMessage msg = m_xSwiftBusServerSetting.setAndSave(dBusAddress);
                CXSwiftBusConfigWriter xswiftbusConfigWriter;
                xswiftbusConfigWriter.setDBusAddress(dBusAddress);
                xswiftbusConfigWriter.updateInAllXPlaneVersions();
                CLogMessage::preformatted(msg);
            }
        }
    } // ns
} // ns
