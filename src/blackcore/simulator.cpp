/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulator.h"

using namespace BlackMisc::Aviation;
using namespace BlackSim;

namespace BlackCore
{
    void ISimulator::emitSimulatorCombinedStatus()
    {
        emit simulatorStatusChanged(isConnected(), isSimulating(), isPaused());
    }

    CSimulatorCommon::CSimulatorCommon(const BlackSim::CSimulatorInfo &simInfo, BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider, BlackMisc::Simulation::IRenderedAircraftProvider *renderedAircraftProvider, QObject *parent)
        : ISimulator(parent), COwnAircraftProviderSupport(ownAircraftProvider), CRenderedAircraftProviderSupport(renderedAircraftProvider), m_simulatorInfo(simInfo)
    { }

    int CSimulatorCommon::getMaxRenderedAircraft() const
    {
        return m_maxRenderedAircraft;
    }

    void CSimulatorCommon::setMaxRenderedAircraft(int maxRenderedAircraft)
    {
        m_maxRenderedAircraft = maxRenderedAircraft;
    }

    CSimulatorInfo CSimulatorCommon::getSimulatorInfo() const
    {
        return m_simulatorInfo;
    }

} // namespace
