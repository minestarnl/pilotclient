/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "swiftcore.h"
#include "blackgui/components/commandinput.h"
#include "blackgui/components/coreinfoareacomponent.h"
#include "blackgui/components/coresettingsdialog.h"
#include "blackgui/components/logcomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/stylesheetutility.h"
#include "blackcore/context/contextaudio.h"
#include "blackcore/corefacade.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/icons.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/logpattern.h"
#include "ui_swiftcore.h"

#include <QFlags>
#include <QGroupBox>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QtGlobal>

class QWidget;

using namespace BlackMisc;
using namespace BlackCore;
using namespace BlackGui;
using namespace BlackGui::Components;

CSwiftCore::CSwiftCore(QWidget *parent) :
    CSystemTrayWindow(CIcons::swiftCore24(), parent),
    CIdentifiable(this),
    ui(new Ui::CSwiftCore)
{
    Q_ASSERT(sGui);
    ui->setupUi(this);
    sGui->initMainApplicationWindow(this);
    const QString name(sGui->getApplicationNameAndVersion());
    setSystemTrayMode(MinimizeToTray | QuitOnClose);
    setSystemTrayToolTip(name);

    m_mwaLogComponent = ui->comp_InfoArea->getLogComponent();
    m_mwaOverlayFrame = nullptr;
    m_mwaStatusBar = nullptr;

    connect(ui->pb_Restart, &QPushButton::clicked, this, &CSwiftCore::restart);
    connect(sGui, &CGuiApplication::styleSheetsChanged, this, &CSwiftCore::onStyleSheetsChanged);

    this->initLogDisplay();
    this->initStyleSheet();
    this->initMenus();
    this->initAudio();

    // log
    CStatusMessage m = CStatusMessage(this).info("Cmd: " + CGuiApplication::arguments().join(" "));
    this->appendLogMessage(m);

    // command line
    ui->lep_CommandLineInput->setIdentifier(this->identifier());
    connect(ui->lep_CommandLineInput, &CCommandInput::commandEntered, sGui->getCoreFacade(), &CCoreFacade::parseCommandLine);
}

CSwiftCore::~CSwiftCore()
{ }

void CSwiftCore::initStyleSheet()
{
    const QString s = sGui->getStyleSheetUtility().styles(
    {
        CStyleSheetUtility::fileNameFonts(),
        CStyleSheetUtility::fileNameStandardWidget(),
        CStyleSheetUtility::fileNameSwiftCore()
    }
    );
    this->setStyleSheet(s);
}

void CSwiftCore::appendLogMessage(const CStatusMessage &message)
{
    ui->comp_InfoArea->getLogComponent()->appendStatusMessageToList(message);
}

void CSwiftCore::showSettingsDialog()
{
    if (!m_settingsDialog)
    {
        m_settingsDialog.reset(new CCoreSettingsDialog(this));
    }
    m_settingsDialog->show();
}

void CSwiftCore::onStyleSheetsChanged()
{
    this->initStyleSheet();
}

void CSwiftCore::initLogDisplay()
{
    m_mwaLogComponent->showDetails(false);
    CLogHandler::instance()->install(true);
    CLogHandler::instance()->enableConsoleOutput(false); // default disable
    auto logHandler = CLogHandler::instance()->handlerForPattern(
                          CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityInfo)
                      );
    logHandler->subscribe(this, &CSwiftCore::appendLogMessage);
    ui->comp_InfoArea->getLogComponent()->showFilterDialog(); // add a filter dialog
}

void CSwiftCore::initMenus()
{
    sGui->addMenuFile(*ui->menu_File);
    sGui->addMenuWindow(*ui->menu_Window);
    sGui->addMenuHelp(*ui->menu_Help);
    connect(ui->menu_SettingsDialog, &QAction::triggered, this, &CSwiftCore::showSettingsDialog);
}

void CSwiftCore::initAudio()
{
    if (!sGui->getIContextAudio()) { return; }
    ui->lbl_AudioRunsWhere->setText(sGui->getIContextAudio()->audioRunsWhereInfo());
    if (sGui->getIContextAudio()->isUsingImplementingObject())
    {
        ui->rb_AudioOnCore->setChecked(true);
    }
    else
    {
        ui->rb_AudioOnGui->setChecked(true);
    }
}

void CSwiftCore::restart()
{
    ui->pb_Restart->setEnabled(false);
    const QStringList args = this->getRestartCmdArgs();
    sGui->restartApplication(args, { "--coreaudio" });
}

QString CSwiftCore::getAudioCmdFromRadioButtons() const
{
    if (ui->rb_AudioOnCore->isChecked()) { return "--coreaudio"; }
    return "";
}

QStringList CSwiftCore::getRestartCmdArgs() const
{
    const QString coreAudio = this->getAudioCmdFromRadioButtons();
    QStringList cmds = ui->comp_DBusSelector->getDBusCmdLineArgs();
    if (!coreAudio.isEmpty()) { cmds.append(coreAudio); }
    return cmds;
}
