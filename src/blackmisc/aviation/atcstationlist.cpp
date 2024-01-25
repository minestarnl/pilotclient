// SPDX-FileCopyrightText: Copyright (C) 2013 swift Project Community / Contributors
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-swift-pilot-client-1

#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/mixin/mixincompare.h"
#include "blackmisc/predicates.h"
#include "blackmisc/range.h"

#include <QString>
#include <QtGlobal>

using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Network;

BLACK_DEFINE_SEQUENCE_MIXINS(BlackMisc::Aviation, CAtcStation, CAtcStationList)

namespace BlackMisc::Aviation
{
    CAtcStationList::CAtcStationList() {}

    CAtcStationList::CAtcStationList(const CSequence<CAtcStation> &other) : CSequence<CAtcStation>(other)
    {}

    CAtcStationList CAtcStationList::findIfComUnitTunedInChannelSpacing(const CComSystem &comUnit) const
    {
        return this->findBy([&](const CAtcStation &atcStation) {
            return atcStation.isComUnitTunedToFrequency(comUnit);
        });
    }

    bool CAtcStationList::hasComUnitTunedInChannelSpacing(const CComSystem &comUnit) const
    {
        return this->containsBy([&](const CAtcStation &atcStation) {
            return atcStation.isComUnitTunedToFrequency(comUnit);
        });
    }

    CAtcStationList CAtcStationList::findIfFrequencyIsWithinSpacing(const CFrequency &frequency)
    {
        if (frequency.isNull()) { return CAtcStationList(); }
        return this->findBy([&](const CAtcStation &atcStation) {
            return atcStation.isAtcStationFrequency(frequency);
        });
    }

    bool CAtcStationList::updateIfMessageChanged(const CInformationMessage &im, const CCallsign &callsign, bool overrideWithNewer)
    {
        const CInformationMessage::InformationType type = im.getType();

        // for loop just to get reference
        bool unequal = false;
        for (CAtcStation &station : *this)
        {
            if (station.getCallsign() != callsign) { continue; }

            const CInformationMessage m = station.getInformationMessage(type);
            if (m.getType() == CInformationMessage::Unspecified) { break; }

            if (m.getMessage() == im.getMessage())
            {
                if (!overrideWithNewer) { break; }
                if (!im.isNewerThan(m)) { break; }
            }
            else
            {
                unequal = true;
            }
            station.setMessage(im);
            break; // only count unequals
        }
        return unequal;
    }

    int CAtcStationList::setOnline(const CCallsign &callsign, bool online)
    {
        int c = 0;
        for (CAtcStation &station : *this)
        {
            if (station.getCallsign() != callsign) { continue; }
            if (station.setOnline(online)) { c++; }
        }
        return c;
    }

    CAtcStationList CAtcStationList::stationsWithValidFrequency() const
    {
        return this->findBy(&CAtcStation::hasValidFrequency, true);
    }

    CUserList CAtcStationList::getControllers() const
    {
        return this->findBy(Predicates::MemberValid(&CAtcStation::getController)).transform(Predicates::MemberTransform(&CAtcStation::getController));
    }

    int CAtcStationList::removeIfOutsideRange()
    {
        return this->removeIf(&CAtcStation::isInRange, false);
    }

    CAtcStationList CAtcStationList::findInRange() const
    {
        if (this->isEmpty()) { return {}; }
        CAtcStationList copy(*this);
        copy.removeIfOutsideRange();
        return copy;
    }

    void CAtcStationList::sortByAtcSuffixSortOrderAndDistance()
    {
        this->sortBy(&CAtcStation::getSuffixSortOrder, &CAtcStation::getRelativeDistance);
    }

    CAtcStationList CAtcStationList::sortedByAtcSuffixSortOrderAndDistance() const
    {
        CAtcStationList stations = *this;
        stations.sortByAtcSuffixSortOrderAndDistance();
        return stations;
    }

    QHash<QString, CAtcStationList> CAtcStationList::splitPerSuffix(bool sort) const
    {
        if (this->isEmpty()) { return QHash<QString, CAtcStationList>(); }
        const CAtcStationList stations = sort ? this->sortedByAtcSuffixSortOrderAndDistance() : *this;

        QString suffix;
        QHash<QString, CAtcStationList> split;
        for (const CAtcStation &s : stations)
        {
            const QString currentSuffix = s.getCallsignSuffix();
            if (suffix != currentSuffix)
            {
                suffix = currentSuffix;
                split[currentSuffix] = CAtcStationList();
            }
            split[currentSuffix].push_back(s);
        }
        return split;
    }
} // namespace
