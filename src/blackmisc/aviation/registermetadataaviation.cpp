/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "registermetadataaviation.h"
#include "aviation.h"

namespace BlackMisc
{
    namespace Aviation
    {
        void registerMetadata()
        {
            CAdfSystem::registerMetadata();
            CAircraftEngine::registerMetadata();
            CAircraftEngineList::registerMetadata();
            CAircraftIcaoCode::registerMetadata();
            CAircraftIcaoCodeList::registerMetadata();
            CAircraftLights::registerMetadata();
            CAircraftParts::registerMetadata();
            CAircraftPartsList::registerMetadata();
            CAircraftSituation::registerMetadata();
            qRegisterMetaType<CAircraftSituation::IsOnGround>();
            qRegisterMetaType<CAircraftSituation::OnGroundDetails>();
            qRegisterMetaType<CAircraftSituation::AltitudeCorrection>();
            CAircraftSituationChange::registerMetadata();
            qRegisterMetaType<CAircraftSituationChange::GuessedSceneryDeviation>();
            CAircraftSituationList::registerMetadata();
            CAirlineIcaoCode::registerMetadata();
            CAirlineIcaoCodeList::registerMetadata();
            CAirportIcaoCode::registerMetadata();
            CAirport::registerMetadata();
            CAirportList::registerMetadata();
            CAltitude::registerMetadata();
            qRegisterMetaType<CAltitude::ReferenceDatum>();
            qRegisterMetaType<CAltitude::AltitudeType>();
            CAtcStation::registerMetadata();
            CAtcStationList::registerMetadata();
            CCallsign::registerMetadata();
            qRegisterMetaType<CCallsign::TypeHint>();
            CCallsignSet::registerMetadata();
            CComSystem::registerMetadata();
            CFlightPlan::registerMetadata();
            CFlightPlanList::registerMetadata();
            CFlightPlanRemarks::registerMetadata();
            CHeading::registerMetadata();
            qRegisterMetaType<CHeading::ReferenceNorth>();
            CInformationMessage::registerMetadata();
            CLivery::registerMetadata();
            CLiveryList::registerMetadata();
            CNavSystem::registerMetadata();
            CSelcal::registerMetadata();
            CTrack::registerMetadata();
            CTransponder::registerMetadata();
        }
    } // ns
} // ns
