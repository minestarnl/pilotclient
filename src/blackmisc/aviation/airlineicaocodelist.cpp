/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/airlineicaocodelist.h"
#include "blackmisc/country.h"
#include "blackmisc/range.h"

#include <QJsonObject>
#include <QJsonValue>

namespace BlackMisc
{
    namespace Aviation
    {
        CAirlineIcaoCodeList::CAirlineIcaoCodeList()
        { }

        CAirlineIcaoCodeList::CAirlineIcaoCodeList(const CSequence<CAirlineIcaoCode> &other) :
            CSequence<CAirlineIcaoCode>(other)
        { }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByDesignator(const QString &designator) const
        {
            if (CAirlineIcaoCode::isValidAirlineDesignator(designator)) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.matchesDesignator(designator);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByIataCode(const QString &iata) const
        {
            if (iata.isEmpty()) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.matchesIataCode(iata);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByDesignatorOrIataCode(const QString &designatorOrIata) const
        {
            if (designatorOrIata.isEmpty()) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.matchesDesignatorOrIataCode(designatorOrIata);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByVDesignator(const QString &designator) const
        {
            if (!CAirlineIcaoCode::isValidAirlineDesignator(designator)) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.matchesVDesignator(designator);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByVDesignatorOrIataCode(const QString &designatorOrIata) const
        {
            if (designatorOrIata.isEmpty()) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.matchesVDesignatorOrIataCode(designatorOrIata);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByCountryIsoCode(const QString &isoCode) const
        {
            if (isoCode.length() != 2) { return CAirlineIcaoCodeList(); }
            const QString iso(isoCode.toUpper());
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.getCountry().getIsoCode() == iso;
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findBySimplifiedNameContaining(const QString &containedString) const
        {
            if (containedString.isEmpty()) { return CAirlineIcaoCodeList(); }
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.isContainedInSimplifiedName(containedString);
            });
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::findByMilitary(bool military) const
        {
            return this->findBy([&](const CAirlineIcaoCode & code)
            {
                return code.isMilitary() == military;
            });
        }

        CAirlineIcaoCode CAirlineIcaoCodeList::smartAirlineIcaoSelector(const CAirlineIcaoCode &icaoPattern, const CCallsign &callsign) const
        {
            const CAirlineIcaoCode patternUsed = icaoPattern.thisOrCallsignCode(callsign);
            if (patternUsed.hasValidDbKey())
            {
                return this->findByKey(icaoPattern.getDbKey(), icaoPattern);
            }

            // search by parts
            CAirlineIcaoCodeList codes;
            if (patternUsed.hasValidDesignator())
            {
                codes = this->findByVDesignator(patternUsed.getVDesignator());
            }
            else
            {
                codes = this->findByIataCode(patternUsed.getIataCode());
            }

            if (codes.size() == 1) { return codes.front(); }

            // further reduce
            if (patternUsed.hasValidCountry())
            {
                CAirlineIcaoCodeList countryCodes = codes.findByCountryIsoCode(patternUsed.getCountry().getIsoCode());
                if (!countryCodes.isEmpty()) { return countryCodes.front(); }
            }

            if (!codes.isEmpty()) { return codes.front(); }
            return patternUsed;
        }

        CAirlineIcaoCode CAirlineIcaoCodeList::findBestMatchByCallsign(const CCallsign &callsign) const
        {
            if (this->isEmpty() || callsign.isEmpty()) { return CAirlineIcaoCode(); }
            const QString airline = callsign.getAirlineSuffix().toUpper();
            if (airline.isEmpty()) { return CAirlineIcaoCode(); }
            const CAirlineIcaoCode airlineCode = (airline.length() == 3) ?
                                                 this->findFirstByOrDefault(&CAirlineIcaoCode::getDesignator, airline) :
                                                 this->findFirstByOrDefault(&CAirlineIcaoCode::getVDesignator, airline);
            return airlineCode;
        }

        CAirlineIcaoCodeList CAirlineIcaoCodeList::fromDatabaseJson(const QJsonArray &array,  bool ignoreIncomplete)
        {
            CAirlineIcaoCodeList codes;
            for (const QJsonValue &value : array)
            {
                const CAirlineIcaoCode icao(CAirlineIcaoCode::fromDatabaseJson(value.toObject()));
                if (ignoreIncomplete && !icao.hasCompleteData())
                {
                    continue;
                }
                codes.push_back(icao);
            }
            return codes;
        }

        QStringList CAirlineIcaoCodeList::toIcaoDesignatorCompleterStrings(bool combinedString, bool sort) const
        {
            QStringList c;
            for (const CAirlineIcaoCode &icao : *this)
            {
                if (combinedString)
                {
                    if (!icao.hasValidDbKey()) { continue; }
                    const QString cs(icao.getCombinedStringWithKey());
                    if (cs.isEmpty()) { continue; }
                    c.append(cs);
                }
                else
                {
                    const QString d(icao.getDesignator());
                    if (c.contains(d) || d.isEmpty()) { continue; }
                    c.append(d);
                }
            }
            if (sort) { c.sort(); }
            return c;
        }

        QStringList CAirlineIcaoCodeList::toIcaoDesignatorNameCountryCompleterStrings(bool sort) const
        {
            QStringList c;
            for (const CAirlineIcaoCode &icao : *this)
            {
                if (!icao.hasValidDesignator()) { continue; }
                const QString cs(icao.getDesignatorNameCountry());
                if (cs.isEmpty()) { continue; }
                c.append(cs);
            }
            if (sort) { c.sort(); }
            return c;
        }

        QStringList CAirlineIcaoCodeList::toNameCompleterStrings(bool sort) const
        {
            QStringList c;
            for (const CAirlineIcaoCode &icao : *this)
            {
                if (!icao.hasValidDbKey()) { continue; }
                const QString cs(icao.getNameWithKey());
                if (cs.isEmpty()) { continue; }
                c.append(cs);
            }
            if (sort) { c.sort(); }
            return c;
        }
    } // namespace
} // namespace
