/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_FSCOMMON_SIMULATORFSCOMMON_H
#define BLACKSIMPLUGIN_FSCOMMON_SIMULATORFSCOMMON_H

#include "../plugincommon/simulatorplugincommon.h"
#include "blackmisc/simulation/interpolator.h"
#include "blackmisc/simulation/fscommon/aircraftcfgparser.h"
#include "fsuipc.h"

#include <QObject>
#include <memory>

namespace BlackSimPlugin
{
    namespace FsCommon
    {
        //! Common base class for MS flight simulators
        class CSimulatorFsCommon : public Common::CSimulatorPluginCommon
        {
            Q_OBJECT
            Q_INTERFACES(BlackCore::ISimulator)
            Q_INTERFACES(BlackMisc::Simulation::ISimulationEnvironmentProvider)
            Q_INTERFACES(BlackMisc::Simulation::IInterpolationSetupProvider)

        public:
            //! Destructor
            virtual ~CSimulatorFsCommon() override;

            //! FSUIPC connected?
            bool isFsuipcConnected() const;

            //! FSUIPC on/off, correctly disconnecting/connecting
            bool useFsuipc(bool on);

            // ---------------------- ISimulator ------------------
            virtual bool disconnectFrom() override;
            virtual bool isPaused() const override { return m_simPaused; }
            virtual bool isTimeSynchronized() const override { return m_simTimeSynced; }
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override;
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override;
            virtual BlackMisc::Aviation::CAirportList getAirportsInRange(bool recalculateDistance) const override;
            // ---------------------- ISimulator ------------------

        protected:
            //! Constructor
            CSimulatorFsCommon(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                               BlackMisc::Simulation::IOwnAircraftProvider    *ownAircraftProvider,
                               BlackMisc::Simulation::IRemoteAircraftProvider *renderedAircraftProvider,
                               BlackMisc::Weather::IWeatherGridProvider       *weatherGridProvider,
                               BlackMisc::Network::IClientProvider            *clientProvider,
                               QObject *parent = nullptr);

            //! Init the internal objects
            virtual void initSimulatorInternals() override;

            //! \name When swift DB data are read
            //! @{
            virtual void onSwiftDbAirportsRead() override;
            //! @}

            //! \addtogroup swiftdotcommands
            //! @{
            //! <pre>
            //! .drv fsuipc  on|off      FSUIPC on/off
            //! </pre>
            //! @}
            virtual bool parseDetails(const BlackMisc::CSimpleCommandParser &parser) override;

            //! Register help
            static void registerHelp();

            CFsuipc *m_fsuipc = nullptr;                            //!< FSUIPC
            bool m_useFsuipc  = false;                              //!< use FSUIPC
            bool m_simPaused  = false;                              //!< Simulator paused?
            bool m_simTimeSynced = false;                           //!< Time synchronized?
            BlackMisc::PhysicalQuantities::CTime m_syncTimeOffset;  //!< time offset
            BlackMisc::Aviation::CAirportList    m_airportsInRangeFromSimulator; //!< airports in range of own aircraft

            // cockpit as set in SIM
            BlackMisc::Aviation::CComSystem   m_simCom1;            //!< cockpit COM1 state in simulator
            BlackMisc::Aviation::CComSystem   m_simCom2;            //!< cockpit COM2 state in simulator
            BlackMisc::Aviation::CTransponder m_simTransponder;     //!< cockpit xpdr state in simulator
            BlackMisc::Aviation::CSelcal      m_selcal;             //!< SELCAL as in cockpit
        };
    } // namespace
} // namespace

#endif // guard
