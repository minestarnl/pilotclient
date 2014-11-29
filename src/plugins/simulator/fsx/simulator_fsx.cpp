/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulator_fsx.h"
#include "simconnect_datadefinition.h"
#include "blacksim/fscommon/bcdconversions.h"
#include "blacksim/fsx/simconnectutilities.h"
#include "blacksim/fsx/fsxsimulatorsetup.h"
#include "blacksim/simulatorinfo.h"
#include "blackmisc/project.h"
#include "blackmisc/avairportlist.h"
#include "blackmisc/logmessage.h"

#include <QTimer>
#include <QtConcurrent>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;
using namespace BlackSim;
using namespace BlackSim::FsCommon;
using namespace BlackSim::Fsx;

namespace BlackSimPlugin
{
    namespace Fsx
    {
        BlackCore::ISimulator *CSimulatorFsxFactory::create(QObject *parent)
        {
            return new Fsx::CSimulatorFsx(parent);
        }

        BlackSim::CSimulatorInfo CSimulatorFsxFactory::getSimulatorInfo() const
        {
            return CSimulatorInfo::FSX();
        }

        CSimulatorFsx::CSimulatorFsx(QObject *parent) :
            ISimulator(parent),
            m_simulatorInfo(CSimulatorInfo::FSX()),
            m_fsuipc(new FsCommon::CFsuipc())
        {
            CFsxSimulatorSetup setup;
            setup.init(); // this fetches important settings on local side
            this->m_simulatorInfo.setSimulatorSetup(setup.getSettings());
        }

        CSimulatorFsx::~CSimulatorFsx()
        {
            disconnectFrom();
        }

        bool CSimulatorFsx::isConnected() const
        {
            return m_simConnected;
        }

        bool CSimulatorFsx::isRunning() const
        {
            return m_simRunning;
        }

        bool CSimulatorFsx::isFsuipcConnected() const
        {
            Q_ASSERT(m_fsuipc);
            return m_fsuipc->isConnected();
        }

        bool CSimulatorFsx::connectTo()
        {
            if (m_simConnected) { return true; }

            if (FAILED(SimConnect_Open(&m_hSimConnect, BlackMisc::CProject::systemNameAndVersionChar(), nullptr, 0, 0, 0)))
            {
                emit connectionStatusChanged(ConnectionFailed);
                emitSimulatorCombinedStatus(); // all status values at once
                return false;
            }
            else
            {
                Q_ASSERT(m_fsuipc);
                this->m_fsuipc->connect(); // connect FSUIPC too
            }

            initWhenConnected();
            m_simconnectTimerId = startTimer(10);
            m_simConnected = true;

            emit connectionStatusChanged(Connected);
            emitSimulatorCombinedStatus();
            return true;
        }

        void CSimulatorFsx::asyncConnectTo()
        {
            connect(&m_watcherConnect, SIGNAL(finished()), this, SLOT(ps_connectToFinished()));

            // simplified connect, timers and signals not in different thread
            auto asyncConnectFunc = [&]() -> bool
            {
                if (FAILED(SimConnect_Open(&m_hSimConnect, BlackMisc::CProject::systemNameAndVersionChar(), nullptr, 0, 0, 0))) { return false; }
                this->m_fsuipc->connect(); // FSUIPC too
                return true;
            };

            QFuture<bool> result = QtConcurrent::run(asyncConnectFunc);
            m_watcherConnect.setFuture(result);
        }

        bool CSimulatorFsx::disconnectFrom()
        {
            if (!m_simConnected) { return true; }

            if (m_hSimConnect)
            {
                SimConnect_Close(m_hSimConnect);
                this->m_fsuipc->disconnect();
            }

            if (m_simconnectTimerId)
            {
                killTimer(m_simconnectTimerId);
            }

            m_hSimConnect = nullptr;
            m_simconnectTimerId = -1;
            m_simConnected = false;

            emit connectionStatusChanged(Disconnected);
            emitSimulatorCombinedStatus();
            return true;
        }

        bool CSimulatorFsx::canConnect() const
        {
            if (m_simConnected) { return true; }
            HANDLE hSimConnect; // temporary handle
            bool connect = false;
            if (FAILED(SimConnect_Open(&hSimConnect, BlackMisc::CProject::systemNameAndVersionChar(), nullptr, 0, 0, 0)))
            {
                connect = false;
            }
            else
            {
                connect = true;
            }
            SimConnect_Close(hSimConnect);
            return connect;
        }

        void CSimulatorFsx::addRemoteAircraft(const CCallsign &callsign, const BlackMisc::Aviation::CAircraftSituation &initialSituation)
        {
            SIMCONNECT_DATA_INITPOSITION initialPosition;
            initialPosition.Latitude = initialSituation.latitude().value(CAngleUnit::deg());
            initialPosition.Longitude = initialSituation.longitude().value(CAngleUnit::deg());
            initialPosition.Altitude = initialSituation.getAltitude().value(CLengthUnit::ft());
            initialPosition.Pitch = initialSituation.getPitch().value(CAngleUnit::deg());
            initialPosition.Bank = initialSituation.getBank().value(CAngleUnit::deg());
            initialPosition.Heading = initialSituation.getHeading().value(CAngleUnit::deg());
            initialPosition.Airspeed = 0;
            initialPosition.OnGround = 0;

            CSimConnectObject simObj;
            simObj.setCallsign(callsign);
            simObj.setRequestId(m_nextObjID);
            simObj.setObjectId(0);
            m_simConnectObjects.insert(callsign, simObj);
            ++m_nextObjID;

            HRESULT hr = SimConnect_AICreateNonATCAircraft(m_hSimConnect, "Boeing 737-800 Paint1", qPrintable(callsign.toQString().left(12)), initialPosition, simObj.getRequestId());
            Q_UNUSED(hr);

            addAircraftSituation(callsign, initialSituation);
        }

        void CSimulatorFsx::addAircraftSituation(const CCallsign &callsign, const CAircraftSituation &initialSituation)
        {
            Q_ASSERT(m_simConnectObjects.contains(callsign));

            CSimConnectObject simObj = m_simConnectObjects.value(callsign);
            simObj.getInterpolator()->addAircraftSituation(initialSituation);
            m_simConnectObjects.insert(callsign, simObj);
        }

        void CSimulatorFsx::removeRemoteAircraft(const CCallsign &callsign)
        {
            removeRemoteAircraft(m_simConnectObjects.value(callsign));
        }

        CSimulatorInfo CSimulatorFsx::getSimulatorInfo() const
        {
            return this->m_simulatorInfo;
        }

        void CSimulatorFsx::setAircraftModel(const BlackMisc::Network::CAircraftModel &model)
        {
            if (m_aircraftModel != model)
            {
                m_aircraftModel = model;
                emit aircraftModelChanged(model);
            }
        }

        bool CSimulatorFsx::updateOwnSimulatorCockpit(const CAircraft &ownAircraft)
        {
            CComSystem newCom1 = ownAircraft.getCom1System();
            CComSystem newCom2 = ownAircraft.getCom2System();
            CTransponder newTransponder = ownAircraft.getTransponder();

            bool changed = false;
            if (newCom1 != this->m_ownAircraft.getCom1System())
            {
                if (newCom1.getFrequencyActive() != this->m_ownAircraft.getCom1System().getFrequencyActive())
                {
                    CFrequency newFreq = newCom1.getFrequencyActive();
                    SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom1Active,
                                                   CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (newCom1.getFrequencyStandby() != this->m_ownAircraft.getCom1System().getFrequencyStandby())
                {
                    CFrequency newFreq = newCom1.getFrequencyStandby();
                    SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom1Standby,
                                                   CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                this->m_ownAircraft.setCom1System(newCom1);
                changed = true;
            }

            if (newCom2 != this->m_ownAircraft.getCom2System())
            {
                if (newCom2.getFrequencyActive() != this->m_ownAircraft.getCom2System().getFrequencyActive())
                {
                    CFrequency newFreq = newCom2.getFrequencyActive();
                    SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom2Active,
                                                   CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (newCom2.getFrequencyStandby() != this->m_ownAircraft.getCom2System().getFrequencyStandby())
                {
                    CFrequency newFreq = newCom2.getFrequencyStandby();
                    SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom2Standby,
                                                   CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                this->m_ownAircraft.setCom2System(newCom2);
                changed = true;
            }

            CTransponder xpdr = this->m_ownAircraft.getTransponder();
            if (newTransponder != xpdr)
            {
                if (newTransponder.getTransponderCode() != xpdr.getTransponderCode())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTransponderCode,
                                                   CBcdConversions::transponderCodeToBcd(newTransponder), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                    changed = true;
                }

                if (newTransponder.getTransponderMode() != xpdr.getTransponderMode())
                {
                    if (m_useSbOffsets)
                    {
                        byte ident = newTransponder.isIdentifying() ? 1 : 0; // 1 is ident
                        byte standby = newTransponder.isInStandby() ? 1 : 0; // 1 is standby
                        HRESULT hr = S_OK;

                        hr += SimConnect_SetClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::DataClientAreaSbIdent, NULL, 0, 1, &ident);
                        hr += SimConnect_SetClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::DataClientAreaSbStandby, NULL, 0, 1, &standby);
                        if (hr != S_OK)
                        {
                            CLogMessage(this).warning("Setting transponder mode failed (SB offsets)");
                        }
                    }
                    changed = true;
                }

                if (changed) { this->m_ownAircraft.setTransponder(newTransponder); }
            }

            // avoid changes of cockpit back to old values due to an outdated read back value
            if (changed) { m_skipCockpitUpdateCycles = SkipUpdateCyclesForCockpit; }

            // bye
            return changed;
        }

        void CSimulatorFsx::displayStatusMessage(const BlackMisc::CStatusMessage &message) const
        {
            QByteArray m = message.getMessage().toLocal8Bit().constData();
            m.append('\0');

            SIMCONNECT_TEXT_TYPE type = SIMCONNECT_TEXT_TYPE_PRINT_BLACK;
            switch (message.getSeverity())
            {
            case CStatusMessage::SeverityDebug:
                return;
            case CStatusMessage::SeverityInfo:
                type = SIMCONNECT_TEXT_TYPE_PRINT_GREEN;
                break;
            case CStatusMessage::SeverityWarning:
                type = SIMCONNECT_TEXT_TYPE_PRINT_YELLOW;
                break;
            case CStatusMessage::SeverityError:
                type = SIMCONNECT_TEXT_TYPE_PRINT_RED;
                break;
            }
            HRESULT hr = SimConnect_Text(
                             m_hSimConnect, type, 7.5, EventTextMessage, m.size(),
                             m.data()
                         );
            Q_UNUSED(hr);
        }

        void CSimulatorFsx::displayTextMessage(const BlackMisc::Network::CTextMessage &message) const
        {
            this->displayStatusMessage(message.asStatusMessage(true, true));
        }

        CAirportList CSimulatorFsx::getAirportsInRange() const
        {
            return this->m_airportsInRange;
        }

        void CSimulatorFsx::setTimeSynchronization(bool enable, BlackMisc::PhysicalQuantities::CTime offset)
        {
            this->m_simTimeSynced = enable;
            this->m_syncTimeOffset = offset;
        }

        void CSimulatorFsx::onSimRunning()
        {
            if (m_simRunning) { return; }
            m_simRunning = true;
            HRESULT hr;
            hr = SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestOwnAircraft,
                                                   CSimConnectDefinitions::DataOwnAircraft,
                                                   SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);

            hr += SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestOwnAircraftTitle,
                                                    CSimConnectDefinitions::DataOwnAircraftTitle,
                                                    SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND,
                                                    SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);

            hr += SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestSimEnvironment,
                                                    CSimConnectDefinitions::DataSimEnvironment,
                                                    SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND,
                                                    SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);

            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_RequestDataOnSimObject failed");
                return;
            }

            // Request the data from SB only when its changed and only ONCE so we don't have to run a 1sec event to get/set this info ;)
            hr += SimConnect_RequestClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::RequestSbData,
                                               CSimConnectDefinitions::DataClientAreaSb, SIMCONNECT_CLIENT_DATA_PERIOD_SECOND, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED);

            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_RequestClientData failed");
                return;
            }

            emit simulatorStarted();
            emitSimulatorCombinedStatus();
        }

        void CSimulatorFsx::onSimStopped()
        {
            if (!m_simRunning) return;
            m_simRunning = false;
            emit simulatorStopped();
            emitSimulatorCombinedStatus();
        }

        void CSimulatorFsx::onSimFrame()
        {
            updateOtherAircrafts();
        }

        void CSimulatorFsx::onSimExit()
        {
            this->onSimStopped();
        }

        void CSimulatorFsx::updateOwnAircraftFromSim(DataDefinitionOwnAircraft simulatorOwnAircraft)
        {
            BlackMisc::Geo::CCoordinateGeodetic position;
            position.setLatitude(CLatitude(simulatorOwnAircraft.latitude, CAngleUnit::deg()));
            position.setLongitude(CLongitude(simulatorOwnAircraft.longitude, CAngleUnit::deg()));

            BlackMisc::Aviation::CAircraftSituation aircraftSituation;
            aircraftSituation.setPosition(position);
            aircraftSituation.setPitch(CAngle(simulatorOwnAircraft.pitch, CAngleUnit::deg()));
            aircraftSituation.setBank(CAngle(simulatorOwnAircraft.bank, CAngleUnit::deg()));
            aircraftSituation.setHeading(CHeading(simulatorOwnAircraft.trueHeading, CHeading::True, CAngleUnit::deg()));
            aircraftSituation.setGroundspeed(CSpeed(simulatorOwnAircraft.velocity, CSpeedUnit::kts()));
            aircraftSituation.setAltitude(CAltitude(simulatorOwnAircraft.altitude, CAltitude::MeanSeaLevel, CLengthUnit::ft()));
            m_ownAircraft.setSituation(aircraftSituation);

            CComSystem com1 = m_ownAircraft.getCom1System(); // set defaults
            CComSystem com2 = m_ownAircraft.getCom2System();

            // When I change cockpit values in the SIM (from GUI to simulator, not originating from simulator)
            // it takes a little while before these values are set in the simulator.
            // To avoid jitters, I wait some update cylces to stabilize the values
            if (m_skipCockpitUpdateCycles < 1)
            {
                com1.setFrequencyActive(CFrequency(simulatorOwnAircraft.com1ActiveMHz, CFrequencyUnit::MHz()));
                com1.setFrequencyStandby(CFrequency(simulatorOwnAircraft.com1StandbyMHz, CFrequencyUnit::MHz()));
                m_ownAircraft.setCom1System(com1);

                com2.setFrequencyActive(CFrequency(simulatorOwnAircraft.com2ActiveMHz, CFrequencyUnit::MHz()));
                com2.setFrequencyStandby(CFrequency(simulatorOwnAircraft.com2StandbyMHz, CFrequencyUnit::MHz()));
                m_ownAircraft.setCom2System(com2);

                m_ownAircraft.setTransponderCode(simulatorOwnAircraft.transponderCode);

            }
            else
            {
                --m_skipCockpitUpdateCycles;
            }
        }

        void CSimulatorFsx::updateOwnAircraftFromSim(DataDefinitionClientAreaSb sbDataArea)
        {
            if (sbDataArea.isIdent())
            {
                this->m_ownAircraft.setTransponderMode(CTransponder::StateIdent);
            }
            else
            {
                this->m_ownAircraft.setTransponderMode(sbDataArea.isStandby() ? CTransponder::StateStandby : CTransponder::ModeC);
            }
        }

        void CSimulatorFsx::setSimConnectObjectID(DWORD requestID, DWORD objectID)
        {
            // First check, if this request id belongs to us
            auto it = std::find_if(m_simConnectObjects.begin(), m_simConnectObjects.end(),
            [requestID](const CSimConnectObject & obj) { return obj.getRequestId() == static_cast<int>(requestID); });
            if (it == m_simConnectObjects.end()) { return; }

            // belongs to use
            it->setObjectId(objectID);
            SimConnect_AIReleaseControl(m_hSimConnect, objectID, requestID);
            SimConnect_TransmitClientEvent(m_hSimConnect, objectID, EventFreezeLat, 1,
                                           SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            SimConnect_TransmitClientEvent(m_hSimConnect, objectID, EventFreezeAlt, 1,
                                           SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            SimConnect_TransmitClientEvent(m_hSimConnect, objectID, EventFreezeAtt, 1,
                                           SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);

            DataDefinitionGearHandlePosition gearHandle;
            gearHandle.gearHandlePosition = 1;

            SimConnect_SetDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::DataGearHandlePosition, objectID, SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0, sizeof(gearHandle), &gearHandle);
        }

        void CSimulatorFsx::timerEvent(QTimerEvent *event)
        {
            Q_UNUSED(event);
            ps_dispatch();
        }

        void CSimulatorFsx::ps_dispatch()
        {
            SimConnect_CallDispatch(m_hSimConnect, SimConnectProc, this);
            this->m_fsuipc->process();
        }

        void CSimulatorFsx::ps_connectToFinished()
        {
            if (m_watcherConnect.result())
            {
                initWhenConnected();
                m_simconnectTimerId = startTimer(50);
                m_simConnected = true;

                emit connectionStatusChanged(Connected);
                emitSimulatorCombinedStatus();
            }
            else
            {
                m_simConnected = false;
                emit connectionStatusChanged(ConnectionFailed);
                emitSimulatorCombinedStatus();
            }
        }

        void CSimulatorFsx::removeRemoteAircraft(const CSimConnectObject &simObject)
        {
            SimConnect_AIRemoveObject(m_hSimConnect, simObject.getObjectId(), simObject.getRequestId());
            m_simConnectObjects.remove(simObject.getCallsign());
        }

        HRESULT CSimulatorFsx::initWhenConnected()
        {
            // called when connected

            HRESULT hr = initEvents();
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: initEvents failed");
                return hr;
            }

            // inti data definitions and SB data area
            hr += initDataDefinitionsWhenConnected();
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: initDataDefinitionsWhenConnected failed");
                return hr;
            }

            return hr;
        }

        HRESULT CSimulatorFsx::initEvents()
        {
            HRESULT hr = S_OK;
            // System events, see http://msdn.microsoft.com/en-us/library/cc526983.aspx#SimConnect_SubscribeToSystemEvent
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventSimStatus, "Sim");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventObjectAdded, "ObjectAdded");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventObjectRemoved, "ObjectRemoved");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventFrame, "Frame");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventPause, "Pause");
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_SubscribeToSystemEvent failed");
                return hr;
            }

            // Mapped events, see event ids here: http://msdn.microsoft.com/en-us/library/cc526980.aspx
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventPauseToggle, "PAUSE_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, SystemEventSlewToggle, "SLEW_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeLat, "FREEZE_LATITUDE_LONGITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeAlt, "FREEZE_ALTITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeAtt, "FREEZE_ATTITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom1Active, "COM_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom1Standby, "COM_STBY_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom2Active, "COM2_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom2Standby, "COM2_STBY_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTransponderCode, "XPNDR_SET");

            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluYear, "ZULU_YEAR_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluDay, "ZULU_DAY_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluHours, "ZULU_HOURS_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluMinutes, "ZULU_MINUTES_SET");
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_MapClientEventToSimEvent failed");
                return hr;
            }

            // facility
            hr += SimConnect_SubscribeToFacilities(m_hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, m_nextObjID++);
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_SubscribeToFacilities failed");
                return hr;
            }
            return hr;
        }

        HRESULT CSimulatorFsx::initDataDefinitionsWhenConnected()
        {
            return CSimConnectDefinitions::initDataDefinitionsWhenConnected(m_hSimConnect);
        }

        void CSimulatorFsx::updateOtherAircrafts()
        {
            foreach(CSimConnectObject simObj, m_simConnectObjects)
            {
                if (simObj.getInterpolator()->hasEnoughAircraftSituations())
                {
                    SIMCONNECT_DATA_INITPOSITION position;
                    CAircraftSituation situation = simObj.getInterpolator()->getCurrentSituation();
                    position.Latitude = situation.latitude().value();
                    position.Longitude = situation.longitude().value();
                    position.Altitude = situation.getAltitude().value(CLengthUnit::ft());
                    position.Pitch = situation.getPitch().value();
                    position.Bank = situation.getBank().value();
                    position.Heading = situation.getHeading().value(CAngleUnit::deg());
                    position.Airspeed = situation.getGroundSpeed().value(CSpeedUnit::kts());

                    //! \todo : epic fail for helicopters and VTOPs!
                    position.OnGround = position.Airspeed < 30 ? 1 : 0;

                    DataDefinitionRemoteAircraftSituation ddAircraftSituation;
                    ddAircraftSituation.position = position;

                    //! \todo Gear handling with new protocol extension
                    DataDefinitionGearHandlePosition gearHandle;
                    gearHandle.gearHandlePosition = position.Altitude < 1000 ? 1 : 0;

                    if (simObj.getObjectId() != 0)
                    {
                        SimConnect_SetDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSituation, simObj.getObjectId(), SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0, sizeof(ddAircraftSituation), &ddAircraftSituation);

                        // With the following SimConnect call all aircrafts loose their red tag. No idea why though.
                        SimConnect_SetDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::DataGearHandlePosition, simObj.getObjectId(), SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0, sizeof(DataDefinitionGearHandlePosition), &gearHandle);
                    }
                }
            }
        }

        void CSimulatorFsx::synchronizeTime(const CTime &zuluTimeSim, const CTime &localTimeSim)
        {
            if (!this->m_simTimeSynced) return;
            if (!this->isConnected()) return;
            if (m_syncDeferredCounter > 0)
            {
                --m_syncDeferredCounter;
            }
            Q_UNUSED(localTimeSim);

            QDateTime myDateTime = QDateTime::currentDateTimeUtc();
            if (!this->m_syncTimeOffset.isZeroEpsilonConsidered())
            {
                int offsetSeconds = this->m_syncTimeOffset.valueRounded(CTimeUnit::s(), 0);
                myDateTime = myDateTime.addSecs(offsetSeconds);
            }
            QTime myTime = myDateTime.time();
            DWORD h = myTime.hour();
            DWORD m = myTime.minute();
            int targetMins = myTime.hour() * 60 + myTime.minute();
            int simMins = zuluTimeSim.valueRounded(CTimeUnit::min());
            int diffMins = qAbs(targetMins - simMins);
            if (diffMins < 2) return;

            HRESULT hr = S_OK;
            hr += SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTimeZuluHours, h, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            hr += SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTimeZuluMinutes, m, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            if (hr != S_OK)
            {
                CLogMessage(this).warning("Sending time sync failed!");
            }

            m_syncDeferredCounter = 5; // allow some time to sync
            CLogMessage(this).info("Synchronized time to UTC: %1") << myTime.toString();
        }
    } // namespace
} // namespace
