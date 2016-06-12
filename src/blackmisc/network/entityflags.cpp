/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/entityflags.h"
#include "blackmisc/verify.h"

#include <QStringList>
#include <QtGlobal>
#include <bitset>

namespace BlackMisc
{
    namespace Network
    {
        QString CEntityFlags::flagToString(CEntityFlags::EntityFlag flag)
        {
            switch (flag)
            {
            case AircraftIcaoEntity: return "Aircraft ICAO";
            case AirlineIcaoEntity: return "Airline ICAO";
            case AllEntities: return "All";
            case AllIcaoAndCountries: return "All ICAO + country";
            case AllIcaoEntities: return "All ICAO";
            case BookingEntity: return "VATSIM bookings";
            case CountryEntity: return "Country";
            case DistributorEntity: return "Distributor";
            case InfoObjectEntity: return "Info objects";
            case LiveryEntity: return "Livery";
            case ModelEntity: return "Model";
            case NoEntity: return "no data";
            case VatsimDataFile: return "VATSIM data file";
            case VatsimStatusFile: return "VATSIM status file";
            default:
                BLACK_VERIFY_X(false, Q_FUNC_INFO, "wrong flags");
                return "wrong flags";
            }
        }

        QString CEntityFlags::flagToString(BlackMisc::Network::CEntityFlags::Entity flag)
        {
            QStringList list;
            if (flag.testFlag(AircraftIcaoEntity)) list << "Aircraft ICAO";
            if (flag.testFlag(AirlineIcaoEntity)) list << "Airline ICAO";
            if (flag.testFlag(BookingEntity)) list << "VATSIM bookings";
            if (flag.testFlag(CountryEntity)) list << "Country";
            if (flag.testFlag(DistributorEntity)) list << "Distributor";
            if (flag.testFlag(InfoObjectEntity)) list << "Info objects";
            if (flag.testFlag(LiveryEntity)) list << "Livery";
            if (flag.testFlag(ModelEntity)) list << "Model";
            if (flag.testFlag(NoEntity)) list << "no data";
            if (flag.testFlag(VatsimDataFile)) list << "VATSIM data file";
            if (flag.testFlag(VatsimStatusFile)) list << "VATSIM status file";
            return list.join(',');
        }

        bool CEntityFlags::isSingleEntity(BlackMisc::Network::CEntityFlags::Entity flag)
        {
            return numberOfEntities(flag) == 1;
        }

        int CEntityFlags::numberOfEntities(BlackMisc::Network::CEntityFlags::Entity flag)
        {
            const int c = std::bitset<sizeof(flag)>(flag).count();
            return c;
        }

        QString CEntityFlags::flagToString(CEntityFlags::ReadState flag)
        {
            switch (flag)
            {
            case ReadFinished: return "finished";
            case ReadFinishedRestricted: return "finished (restricted)";
            case ReadFailed: return "failed";
            case StartRead: return "read started";
            default:
                BLACK_VERIFY_X(false, Q_FUNC_INFO, "wrong flags");
                return "wrong flags";
            }
        }

        CStatusMessage::StatusSeverity CEntityFlags::flagToSeverity(CEntityFlags::ReadState state)
        {
            switch (state)
            {
            case ReadFinished:
            case ReadFinishedRestricted:
            case StartRead:
                return CStatusMessage::SeverityInfo;
            case ReadFailed:
                return CStatusMessage::SeverityError;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "Missing state");
                return CStatusMessage::SeverityInfo;
            }
        }

        CEntityFlags::EntityFlag CEntityFlags::entityToEntityFlag(Entity entity)
        {
            return static_cast<EntityFlag>(static_cast<int>(entity));
        }

        CEntityFlags::Entity CEntityFlags::iterateDbEntities(Entity &entities)
        {
            if (entities == NoEntity || entities == InfoObjectEntity) { return NoEntity; }
            if (entities.testFlag(AircraftIcaoEntity)) { entities &= ~AircraftIcaoEntity; return AircraftIcaoEntity; }
            if (entities.testFlag(AirlineIcaoEntity))  { entities &= ~AirlineIcaoEntity; return AirlineIcaoEntity; }
            if (entities.testFlag(LiveryEntity))       { entities &= ~LiveryEntity; return LiveryEntity; }
            if (entities.testFlag(CountryEntity))      { entities &= ~CountryEntity; return CountryEntity; }
            if (entities.testFlag(ModelEntity))        { entities &= ~ModelEntity; return ModelEntity; }
            if (entities.testFlag(DistributorEntity))  { entities &= ~DistributorEntity; return DistributorEntity; }
            return NoEntity;
        }

        bool CEntityFlags::isWarningOrAbove(CEntityFlags::ReadState state)
        {
            CStatusMessage::StatusSeverity s = flagToSeverity(state);
            switch (s)
            {
            case CStatusMessage::SeverityError:
            case CStatusMessage::SeverityWarning:
                return true;
            default:
                return false;
            }
        }

        bool CEntityFlags::anySwiftDbEntity(Entity entities)
        {
            return
                entities.testFlag(AircraftIcaoEntity) || entities.testFlag(AirlineIcaoEntity) ||
                entities.testFlag(CountryEntity) ||
                entities.testFlag(ModelEntity) || entities.testFlag(LiveryEntity);
        }

        void CEntityFlags::registerMetadata()
        {
            // this is no value class and I register enums here,
            // that's why I do not use the Mixins
            qRegisterMetaType<CEntityFlags::Entity>();
            qRegisterMetaType<CEntityFlags::EntityFlag>();
            qRegisterMetaType<CEntityFlags::ReadState>();
        }
    } // namespace
} // namespace
