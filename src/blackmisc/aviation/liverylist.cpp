/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/liverylist.h"
#include "blackmisc/metaclassprivate.h"
#include "blackmisc/predicates.h"
#include "blackmisc/range.h"

#include <QtGlobal>
#include <tuple>

using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;


namespace BlackMisc
{
    namespace Aviation
    {

        CLiveryList::CLiveryList() { }

        CLiveryList::CLiveryList(const CSequence<CLivery> &other) :
            CSequence<CLivery>(other)
        { }

        CLiveryList CLiveryList::findByAirlineIcaoDesignator(const QString &icao) const
        {
            QString i(icao.trimmed().toUpper());
            if (i.isEmpty()) { return CLiveryList(); }
            return this->findBy(&CLivery::getAirlineIcaoCodeDesignator, i);
        }

        CLivery CLiveryList::findStdLiveryByAirlineIcaoVDesignator(const QString &icao) const
        {
            QString i(icao.trimmed().toUpper());
            if (i.isEmpty()) { return CLivery(); }
            return this->findFirstByOrDefault([&](const CLivery & livery)
            {
                if (!livery.isAirlineStandardLivery()) { return false; }
                return livery.getAirlineIcaoCode().matchesVDesignator(i);
            });
        }

        CLiveryList CLiveryList::findStdLiveriesBySimplifiedAirlineName(const QString &containedString) const
        {
            if (containedString.isEmpty()) { return CLiveryList(); }
            return this->findBy([&](const CLivery & livery)
            {
                // keep isAirlineStandardLivery first (faster)
                return livery.isAirlineStandardLivery() &&
                       livery.isContainedInSimplifiedAirlineName(containedString);
            });
        }

        CLivery CLiveryList::findStdLiveryByAirlineIcaoVDesignator(const CAirlineIcaoCode &icao) const
        {
            return this->findStdLiveryByAirlineIcaoVDesignator(icao.getVDesignator());
        }

        CLivery CLiveryList::findByCombinedCode(const QString &combinedCode) const
        {
            if (!CLivery::isValidCombinedCode(combinedCode)) { return CLivery(); }
            return this->findFirstByOrDefault([&](const CLivery & livery)
            {
                return livery.matchesCombinedCode(combinedCode);
            });
        }

        QStringList CLiveryList::getCombinedCodes(bool sort) const
        {
            if (this->isEmpty()) { return QStringList(); }
            QStringList codes = this->transform(Predicates::MemberTransform(&CLivery::getCombinedCode));
            if (sort) { codes.sort(); }
            return codes;
        }

        QStringList CLiveryList::getCombinedCodesPlusInfo(bool sort) const
        {
            if (this->isEmpty()) { return QStringList(); }
            QStringList codes = this->transform(Predicates::MemberTransform(&CLivery::getCombinedCodePlusInfo));
            if (sort) { codes.sort(); }
            return codes;
        }

        CLivery CLiveryList::smartLiverySelector(const CLivery &liveryPattern) const
        {
            // multiple searches are slow, maybe we can performance optimize this
            // in the futuew

            // first try on id, that would be perfect
            if (liveryPattern.hasValidDbKey())
            {
                int k = liveryPattern.getDbKey();
                const CLivery l(this->findByKey(k));
                if (l.hasCompleteData()) { return l; }
            }

            // by combined code
            if (liveryPattern.hasCombinedCode())
            {
                QString cc(liveryPattern.getCombinedCode());
                const CLivery l(this->findByCombinedCode(cc));
                if (l.hasCompleteData()) { return l; }
            }

            // by airline
            if (liveryPattern.hasValidAirlineDesignator())
            {
                const QString icao(liveryPattern.getAirlineIcaoCodeDesignator());
                const CLivery l(this->findStdLiveryByAirlineIcaoVDesignator(icao));
                if (l.hasCompleteData()) { return l; }
            }

            // lenient search by name contained (slow)
            if (liveryPattern.getAirlineIcaoCode().hasName())
            {
                const QString search(liveryPattern.getAirlineIcaoCode().getSimplifiedName());
                const CLiveryList liveries(this->findStdLiveriesBySimplifiedAirlineName(search));
                if (!liveries.isEmpty())
                {
                    return liveries.front();
                }
            }
            return CLivery();
        }
    } // namespace
} // namespace
