#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "blackgui/atcstationlistmodel.h"
#include "blackcore/dbus_server.h"
#include "blackcore/context_network.h"
#include <QMouseEvent>

using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackGui;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Settings;
using namespace BlackMisc::Voice;

/*
 * Constructor
 */
MainWindow::MainWindow(GuiModes::WindowMode windowMode, QWidget *parent) :
    QMainWindow(parent, windowMode == GuiModes::WindowFrameless ? (Qt::Window | Qt::FramelessWindowHint) : Qt::Tool),
    ui(new Ui::MainWindow),
    m_infoWindow(nullptr),
    m_init(false), m_windowMode(windowMode), m_coreMode(GuiModes::CoreExternal),
    m_coreAvailable(false), m_contextNetworkAvailable(false), m_contextVoiceAvailable(false),
    m_dBusConnection("dummy"), m_coreRuntime(nullptr),
    m_atcListOnline(nullptr), m_atcListBooked(nullptr),
    m_trafficServerList(nullptr), m_aircraftsInRange(nullptr),
    m_contextApplication(nullptr), m_contextNetwork(nullptr), m_contextVoice(nullptr), m_contextSettings(nullptr),
    m_timerUpdateAtcStationsOnline(nullptr), m_timerUpdateAircraftsInRange(nullptr),
    m_timerCollectedCockpitUpdates(nullptr), m_timerContextWatchdog(nullptr),
    m_contextMenuAudio(nullptr)
{
    if (windowMode == GuiModes::WindowFrameless)
    {
        // http://stackoverflow.com/questions/18316710/frameless-and-transparent-window-qt5
        this->setAttribute(Qt::WA_NoSystemBackground, true);
        this->setAttribute(Qt::WA_TranslucentBackground, true);
        // this->setAttribute(Qt::WA_PaintOnScreen);
    }
    ui->setupUi(this);
}

/*
 * Destructor
 */
MainWindow::~MainWindow() {}

/*
 * Graceful shutdown
 */
void MainWindow::gracefulShutdown()
{
    if (!this->m_init) return;
    this->m_init = false;
    if (this->m_infoWindow)
    {
        this->m_infoWindow->close();
        this->m_infoWindow = nullptr;
    }

    // if we have a context, we shut some things down
    if (this->m_contextNetworkAvailable)
    {
        if (this->m_contextNetwork->isConnected())
        {
            if (this->m_contextVoiceAvailable)
            {
                this->m_contextVoice->leaveAllVoiceRooms();
            }
            this->m_contextNetwork->disconnectFromNetwork();
        }
    }

    if (this->m_timerUpdateAircraftsInRange)
    {
        this->m_timerUpdateAircraftsInRange->stop();
        this->m_timerUpdateAircraftsInRange->disconnect(this);
    }
    if (this->m_timerUpdateAtcStationsOnline)
    {
        this->m_timerUpdateAtcStationsOnline->stop();
        this->m_timerUpdateAtcStationsOnline->disconnect(this);
    }
    if (this->m_timerContextWatchdog)
    {
        this->m_timerContextWatchdog->stop();
        this->m_timerContextWatchdog->disconnect(this);
    }
    if (this->m_timerCollectedCockpitUpdates)
    {
        this->m_timerCollectedCockpitUpdates->stop();
        this->m_timerCollectedCockpitUpdates->disconnect(this);
    }
}

/*
 * Close event, window closes
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    this->gracefulShutdown();
    QMainWindow::closeEvent(event);
    QApplication::exit();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // this->ui->fr_PseudoWindowBar->geometry().contains(event->pos())
    if (this->m_windowMode == GuiModes::WindowFrameless && event->buttons() & Qt::LeftButton)
    {
        move(event->globalPos() - this->m_dragPosition);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (this->m_windowMode == GuiModes::WindowFrameless && event->button() == Qt::LeftButton)
    {
        this->m_dragPosition = event->globalPos() - this->frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

/*
 * Select correct main page
 */
void MainWindow::setMainPage(bool start)
{
    if (start)
    {
        this->ui->sw_MainMiddle->setCurrentIndex(0);
        return;
    }

    QObject *sender = QObject::sender();
    if (sender == this->ui->pb_MainConnect || sender == this->ui->pb_MainStatus)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageStatus);
    else if (sender == this->ui->pb_MainAtc)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageAtc);
    else if (sender == this->ui->pb_MainAircrafts)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageAircrafts);
    else if (sender == this->ui->pb_MainCockpit)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageCockpit);
    else if (sender == this->ui->pb_MainTextMessages)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageTextMessages);
    else if (sender == this->ui->pb_MainFlightplan)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageFlightplan);
    else if (sender == this->ui->pb_MainSettings)
        this->ui->sw_MainMiddle->setCurrentIndex(MainPageSettings);
}

/*
 * Set main page
 */
void MainWindow::setMainPage(MainWindow::MainPageIndex mainPage)
{
    this->ui->sw_MainMiddle->setCurrentIndex(mainPage);
}

/*
 * Given main page selected?
 */
bool MainWindow::isMainPageSelected(MainWindow::MainPageIndex mainPage) const
{
    return this->ui->sw_MainMiddle->currentIndex() == static_cast<int>(mainPage);
}

/*
 * Connect to Network
 */
void MainWindow::toggleNetworkConnection()
{
    CStatusMessageList msgs;
    if (!this->isContextNetworkAvailableCheck()) return;

    this->ui->lbl_StatusNetworkConnectedIcon->setPixmap(this->m_resPixmapConnectionConnecting);
    if (!this->m_contextNetwork->isConnected())
    {
        QString cs = this->ui->le_SettingsAircraftCallsign->text();
        if (cs.isEmpty())
        {
            this->displayStatusMessage(CStatusMessage::getValidationError("missing callsign"));
            return;
        }

        // Login is based on setting current server
        msgs = this->m_contextNetwork->connectToNetwork();
    }
    else
    {
        if (this->m_contextVoiceAvailable) this->m_contextVoice->leaveAllVoiceRooms();
        msgs = this->m_contextNetwork->disconnectFromNetwork();
    }
    if (!msgs.isEmpty()) this->displayStatusMessages(msgs);
}

/*
 * Is the network context available?
 */
bool MainWindow::isContextNetworkAvailableCheck()
{
    if (this->m_contextNetworkAvailable) return true;
    this->displayStatusMessage(CStatusMessage(CStatusMessage::TypeCore, CStatusMessage::SeverityError, "Network context not available, no updates this time"));
    return false;
}

/*
 * Is the voice context available?
 */
bool MainWindow::isContextVoiceAvailableCheck()
{
    if (this->m_contextVoiceAvailable) return true;
    this->displayStatusMessage(CStatusMessage(CStatusMessage::TypeCore, CStatusMessage::SeverityError, "Voice context not available"));
    return false;
}

/*
 * Display a status message
 */
void MainWindow::displayStatusMessage(const CStatusMessage &message)
{
    this->ui->sb_MainStatusBar->showMessage(message.getMessage(), 3000);
    this->ui->te_StatusMessages->insertPlainText(message.toQString(true).append("\n"));
    if (message.getSeverity() == CStatusMessage::SeverityError) this->displayOverlayInfo(message);

}

/*
 * Display a status message
 */
void MainWindow::displayStatusMessages(const CStatusMessageList &messages)
{
    if (messages.isEmpty()) return;
    foreach(CStatusMessage msg, messages)
    {
        this->displayStatusMessage(msg);
    }
}

/*
* Connection terminated
*/
void MainWindow::connectionTerminated()
{
    this->updateGuiStatusInformation();
}

/*
* Connection status changed
*/
void MainWindow::connectionStatusChanged(uint /** from **/, uint to)
{
    // CContextNetwork::ConnectionStatus statusFrom = static_cast<CContextNetwork::ConnectionStatus>(from);
    INetwork::ConnectionStatus statusTo = static_cast<INetwork::ConnectionStatus>(to);

    // always
    this->updateGuiStatusInformation();

    if (statusTo == INetwork::Connected)
    {
        QTimer::singleShot(5 * 1000, this, SLOT(reloadAircraftsInRange()));
        QTimer::singleShot(5 * 1000, this, SLOT(reloadAtcStationsOnline()));
    }
}

/*
* Timer event
*/
void MainWindow::timerBasedUpdates()
{
    QObject *sender = QObject::sender();
    if (sender == this->m_timerUpdateAtcStationsOnline)
    {
        int t = this->ui->hs_SettingsGuiAtcRefreshTime->value() * 1000;
        this->m_timerUpdateAtcStationsOnline->start(t);
        if (ui->tv_AtcStationsOnline->isVisible())
            this->reloadAtcStationsOnline();
    }
    if (sender == this->m_timerUpdateAircraftsInRange)
    {
        int t = this->ui->hs_SettingsGuiAircraftRefreshTime->value() * 1000;
        this->m_timerUpdateAircraftsInRange->start(t);
        if (ui->tv_AircraftsInRange->isVisible())
            this->reloadAircraftsInRange();
    }
    else if (sender == this->m_timerContextWatchdog)
    {
        this->setContextAvailability();
        this->updateGuiStatusInformation();
    }

    // own aircraft
    if (sender == this->m_timerUpdateAircraftsInRange || sender == this->m_timerUpdateAtcStationsOnline)
    {
        this->reloadOwnAircraft(); // regular updates
    }
}

/*
* Context availability
*/
void MainWindow::setContextAvailability()
{
    qint64 t = QDateTime::currentMSecsSinceEpoch();
    this->m_coreAvailable = this->m_contextApplication->ping(t) == t;
    this->m_contextNetworkAvailable = this->m_coreAvailable || this->m_contextNetwork->usingLocalObjects();
    this->m_contextVoiceAvailable = this->m_coreAvailable || this->m_contextVoice->usingLocalObjects();
}

/*
* Middle panel changed
*/
void MainWindow::middlePanelChanged(int /* index */)
{
    if (this->isContextNetworkAvailableCheck())
    {
        // remark, ATC stations is handled by tab changed

        if (this->ui->sw_MainMiddle->currentWidget() == this->ui->pg_AircraftsInRange)
        {
            if (this->m_aircraftsInRange->rowCount() < 1)
                this->reloadAircraftsInRange();
        }
    }
}

/*
* Update GUI
*/
void MainWindow::updateGuiStatusInformation()
{
    const QString now = QDateTime::currentDateTimeUtc().toString("yyyy - MM - dd HH: mm: ss");
    QString network("unavailable");
    if (this->m_contextNetworkAvailable)
    {
        network = this->m_contextNetwork->usingLocalObjects() ? "local" : now;
    }

    // handle voice, mute
    QString voice("unavailable");
    if (this->m_contextVoiceAvailable)
    {
        voice = this->m_contextVoice->usingLocalObjects() ? "local" : now;
        this->ui->pb_SoundMute->setEnabled(true);
    }
    else
    {
        // voice not available
        this->ui->pb_SoundMute->setEnabled(false);
    }

    // update status fields
    this->ui->le_StatusNetworkContext->setText(network);
    this->ui->le_StatusVoiceContext->setText(voice);
    this->ui->cb_StatusWithDBus->setCheckState(this->m_coreMode ? Qt::Checked : Qt::Unchecked);

    // Connected button
    if (this->m_contextNetworkAvailable && this->m_contextNetwork->isConnected())
    {
        if (this->ui->lbl_StatusNetworkConnectedIcon->toolTip().startsWith("dis", Qt::CaseInsensitive))
            this->ui->lbl_StatusNetworkConnectedIcon->setToolTip(now);
        this->ui->pb_MainConnect->setText("Disconnect");
        this->ui->pb_MainConnect->setStyleSheet("background-color: green");
        this->ui->lbl_StatusNetworkConnectedIcon->setPixmap(this->m_resPixmapConnectionConnected);
    }
    else
    {
        this->ui->lbl_StatusNetworkConnectedIcon->setToolTip("disconnected");
        this->ui->pb_MainConnect->setText("Connect");
        this->ui->pb_MainConnect->setStyleSheet("background-color: ");
        this->ui->lbl_StatusNetworkConnectedIcon->setPixmap(this->m_resPixmapConnectionDisconnected);
    }
}

/*
 * Opacity 0-100
 */
void MainWindow::changeWindowOpacity(int opacity)
{
    if (opacity < 0)
    {
        QObject *sender = QObject::sender();
        if (sender == this->ui->pb_MainKeypadOpacity050)
            opacity = 50;
        else if (sender == this->ui->pb_MainKeypadOpacity100)
            opacity = 100;
        else
            return;
    }
    qreal o = opacity / 100.0;
    o = o < 0.3 ? 0.3 : o;
    QWidget::setWindowOpacity(o);
    this->ui->hs_SettingsGuiOpacity->setValue(o * 100.0);
}

/*
 * Display the info window
 */
void MainWindow::displayOverlayInfo(const QString &message)
{
    if (!this->m_infoWindow)
    {
        this->m_infoWindow = new CInfoWindow(this);
    }

    // display window
    if (message.isEmpty())
    {
        this->m_infoWindow->hide();
    }
    else
    {
        this->m_infoWindow->setInfoMessage(message);
    }
}

/*
 * Info window by
 */
void MainWindow::displayOverlayInfo(const CStatusMessage &message)
{
    this->displayOverlayInfo(message.getMessage());
    // further code goes here, such as marking errors as red ...
}
