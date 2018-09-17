/* Copyright (C) 2018
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "installfsxterrainprobecomponent.h"
#include "ui_installfsxterrainprobecomponent.h"
#include "blackgui/overlaymessagesframe.h"
#include "blackgui/guiutility.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include "blackmisc/simulation/simulatorinfo.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/fileutils.h"

#include <QPushButton>
#include <QFileDialog>
#include <QPointer>
#include <QTimer>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::FsCommon;

namespace BlackGui
{
    namespace Components
    {
        CInstallFsxTerrainProbeComponent::CInstallFsxTerrainProbeComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CInstallFsxTerrainProbeComponent)
        {
            ui->setupUi(this);
            ui->le_Source->setText(CDirectoryUtils::shareTerrainProbeDirectory());

            ui->comp_SimulatorSelector->setMode(CSimulatorSelector::RadioButtons);
            ui->comp_SimulatorSelector->setFsxP3DOnly();
            ui->comp_SimulatorSelector->setValue(CSimulatorInfo::fsx());

            connect(ui->pb_Copy, &QPushButton::released, this, &CInstallFsxTerrainProbeComponent::copyProbe);
            connect(ui->pb_DirectoryBrowser, &QPushButton::released, this, &CInstallFsxTerrainProbeComponent::selectSimObjectsDir);
            connect(ui->comp_SimulatorSelector, &CSimulatorSelector::changed, this, &CInstallFsxTerrainProbeComponent::onSimulatorChanged);

            QPointer<CInstallFsxTerrainProbeComponent> myself;
            QTimer::singleShot(250, this, [ = ]
            {
                if (!myself) { return; }
                this->onSimulatorChanged(ui->comp_SimulatorSelector->getValue());
            });
        }

        CInstallFsxTerrainProbeComponent::~CInstallFsxTerrainProbeComponent()
        { }

        void CInstallFsxTerrainProbeComponent::copyProbe()
        {
            if (ui->le_Target->text().isEmpty()) { return; }
            CStatusMessageList msgs;
            const int copied = CFsCommonUtil::copyFsxTerrainProbeFiles(ui->le_Target->text(), msgs);
            COverlayMessagesWizardPage *mf = CGuiUtility::nextOverlayMessageWizardPage(this);
            if (mf) { mf->showOverlayMessages(msgs, false, 5000); }
            Q_UNUSED(copied);
        }

        void CInstallFsxTerrainProbeComponent::onSimulatorChanged(const CSimulatorInfo &simulator)
        {
            const QStringList modelDirs = m_simulatorSettings.getModelDirectoriesOrDefault(simulator);
            if (!modelDirs.isEmpty() && !modelDirs.front().isEmpty())
            {
                ui->le_Target->setText(modelDirs.front());
            }
            else
            {
                ui->le_Target->clear();
            }
        }

        void CInstallFsxTerrainProbeComponent::selectSimObjectsDir()
        {
            const QString startDirectory = CFileUtils::fixWindowsUncPath(ui->le_Target->text());
            const QString dir = QFileDialog::getExistingDirectory(this, "SimObjects directory", startDirectory,
                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        }
    } // ns
} // ns
