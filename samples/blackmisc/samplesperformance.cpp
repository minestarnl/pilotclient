/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file
//! \ingroup sampleblackmisc

#include "samplesperformance.h"
#include "blackcore/db/databasereader.h"
#include "blackconfig/buildconfig.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/aviation/aircraftsituation.h"
#include "blackmisc/aviation/aircraftsituationlist.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/testing.h"

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QRegExp>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QVector>
#include <Qt>
#include <algorithm>
#include <iterator>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Simulation;
using namespace BlackConfig;
using namespace BlackCore::Db;

namespace BlackSample
{
    int CSamplesPerformance::samplesMisc(QTextStream &out)
    {
        QTime timer;
        int ms, number;
        BlackMisc::Aviation::CTesting::copy10kStations(1); // init

        // ATC stations, tradionally created
        timer.start();
        CAtcStationList atcs1 = BlackMisc::Aviation::CTesting::createAtcStations(10000);
        ms = timer.elapsed();
        out << "created (copy) " << atcs1.size() << " ATC stations in " << ms << "ms" << endl;

        timer.start();
        CAtcStationList atcs2 = BlackMisc::Aviation::CTesting::createAtcStations(100000);
        ms = timer.elapsed();
        out << "created (copy) " << atcs2.size() << " ATC stations in " << ms << "ms" << endl;

        // ATC stations, property index created
        timer.start();
        CAtcStationList atcs3 = BlackMisc::Aviation::CTesting::createAtcStations(10000, true);
        ms = timer.elapsed();
        out << "created (propertyIndex) " << atcs3.size() << " ATC stations in " << ms << "ms" << endl;

        timer.start();
        CAtcStationList atcs4 = BlackMisc::Aviation::CTesting::createAtcStations(100000, true);
        ms = timer.elapsed();
        out << "created (propertyIndex) " << atcs4.size() << " ATC stations in " << ms << "ms" << endl;

        // Sort by
        timer.start();
        atcs1.sortBy(&CAtcStation::getCallsign);
        ms = timer.elapsed();
        out << "Sorted by callsign " << atcs1.size() << " ATC stations in " << ms << "ms" << endl;

        timer.start();
        atcs2.sortBy(&CAtcStation::getCallsign);
        ms = timer.elapsed();
        out << "Sorted by callsign " << atcs2.size() << " ATC stations in " << ms << "ms" << endl;

        // Read data, this is what all our models do when displaying in a table view
        timer.start();
        BlackMisc::Aviation::CTesting::readStations(atcs1, false);
        ms = timer.elapsed();
        out << "Read (getters) " << atcs1.size() << " ATC stations in " << ms << "ms" << endl;

        timer.start();
        BlackMisc::Aviation::CTesting::readStations(atcs2, false);
        ms = timer.elapsed();
        out << "Read (getters) " << atcs2.size() << " ATC stations in " << ms << "ms"  << endl;

        timer.start();
        BlackMisc::Aviation::CTesting::readStations(atcs1, true);
        ms = timer.elapsed();
        out << "Read (propertyIndex) " << atcs1.size() << " ATC stations in " << ms << "ms" << endl;

        timer.start();
        BlackMisc::Aviation::CTesting::readStations(atcs2, true);
        ms = timer.elapsed();
        out << "Read (propertyIndex) " << atcs2.size() << " ATC stations in " << ms << "ms" << endl;

        // calculate
        number = 10000;
        timer.start();
        BlackMisc::Aviation::CTesting::calculateDistance(number);
        ms = timer.elapsed();
        out << "Calculated distances " << number << " in " << ms << "ms" << endl;

        number = 100000;
        timer.start();
        BlackMisc::Aviation::CTesting::calculateDistance(number);
        ms = timer.elapsed();
        out << "Calculated distances " << number << "in " << ms << "ms" << endl;

        // parse
        number = 100000;
        timer.start();
        BlackMisc::Aviation::CTesting::parseWgs(number);
        ms = timer.elapsed();
        out << "Parse WGS coordinates " << number << " in " << ms << "ms" << endl;

        // copy
        timer.start();
        number = 20;
        BlackMisc::Aviation::CTesting::copy10kStations(number);
        ms = timer.elapsed();
        out << "Copied 10k stations " << number << " times in " << ms << "ms" << endl;

        timer.start();
        number = 100;
        BlackMisc::Aviation::CTesting::copy10kStations(number);
        ms = timer.elapsed();
        out << "Copied 10k stations " << number << " times in " << ms << "ms" << endl;

        // Regex pattern matching with lists of 10000 strings containing random hex numbers
        auto generator = []() { return QString::number(qrand() | (qrand() << 16), 16); };
        QStringList strList1, strList2, strList3, strList4;
        std::generate_n(std::back_inserter(strList1), 100000, generator);
        std::generate_n(std::back_inserter(strList2), 100000, generator);
        std::generate_n(std::back_inserter(strList3), 100000, generator);
        std::generate_n(std::back_inserter(strList4), 100000, generator);
        QRegularExpression newRegex("^.*aaa.*$", QRegularExpression::CaseInsensitiveOption);
        QRegExp fullRegex(".*aaa.*", Qt::CaseInsensitive);
        QRegExp wildcardRegex("*aaa*", Qt::CaseInsensitive, QRegExp::Wildcard);
        QString containsStr("aaa");
        number = 0;
        timer.start();
        for (const auto &str : strList1) { if (newRegex.match(str).hasMatch()) number++; }
        ms = timer.elapsed();
        out << "new regex matched " << number << " of" << strList1.size() << " strings in " << ms << "ms"  << endl;
        number = 0;
        timer.start();
        for (const auto &str : strList2) { if (fullRegex.exactMatch(str)) number++; }
        ms = timer.elapsed();
        out << "full regex matched " << number << " of" << strList2.size() << " strings in " << ms << "ms"  << endl;
        number = 0;
        timer.start();
        for (const auto &str : strList3) { if (wildcardRegex.exactMatch(str)) number++; }
        ms = timer.elapsed();
        out << "wildcard matched " << number << " of " << strList3.size() << " strings in " << ms << "ms"  << endl;
        number = 0;
        timer.start();
        for (const auto &str : strList4) { if (str.contains(containsStr)) number++; }
        ms = timer.elapsed();
        out << "contains matched " << number << " of " << strList4.size() << " strings in " << ms << "ms" << endl;

        out << "-----------------------------------------------"  << endl;
        return 0;
    }

    int CSamplesPerformance::samplesImplementationType(QTextStream &out, int numberOfCallsigns, int numberOfTimes)
    {
        const qint64 baseTimeEpoch = QDateTime::currentMSecsSinceEpoch();
        CAircraftSituationList situations = createSituations(baseTimeEpoch, numberOfCallsigns, numberOfTimes);

        QTime timer;
        out << "Created " << situations.size() << " situations" << endl;

        timer.start();
        for (int i = 0; i < 10; i++)
        {
            for (int cs = 0; cs < numberOfCallsigns; cs++)
            {
                CCallsign callsign("CS" + QString::number(cs));
                CAircraftSituationList r = situations.findByCallsign(callsign);
                Q_ASSERT(r.size() == numberOfTimes);
            }
        }
        out << "Reads by callsigns: " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (int i = 0; i < 10; i++)
        {
            for (int t = 0; t < numberOfTimes; t++)
            {
                CAircraftSituationList r = situations.findBefore(baseTimeEpoch + 1 + (DeltaTime * t));
                Q_ASSERT(r.size() == numberOfCallsigns * (t + 1));
            }
        }
        out << "Reads by times: " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (int t = 0; t < numberOfTimes; t++)
        {
            for (int cs = 0; cs < numberOfCallsigns; cs++)
            {
                CCallsign callsign("CS" + QString::number(cs));
                CAircraftSituationList r = situations.findByCallsign(callsign).findBefore(baseTimeEpoch + 1 + (DeltaTime * t));
                Q_UNUSED(r);
            }
        }
        out << "Reads by callsigns / times: " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (int t = 0; t < numberOfTimes; t++)
        {
            for (int cs = 0; cs < numberOfCallsigns; cs++)
            {
                CCallsign callsign("CS" + QString::number(cs));
                CAircraftSituationList r = situations.findBefore(baseTimeEpoch + 1 + (DeltaTime * t)).findByCallsign(callsign);
                Q_UNUSED(r);
            }
        }
        out << "Reads by times / callsigns: " << timer.elapsed() << "ms" << endl;

        timer.start();
        QHash<CCallsign, CAircraftSituationList> splitList = situations.splitPerCallsign();
        Q_ASSERT(splitList.size() == numberOfCallsigns);
        for (int t = 0; t < numberOfTimes; t++)
        {
            for (const CAircraftSituationList &slcs : splitList.values())
            {
                CAircraftSituationList r = slcs.findBefore(baseTimeEpoch + 1 + (DeltaTime * t));
                Q_UNUSED(r);
            }
        }
        out << "Split reads by callsigns / times: " << timer.elapsed() << "ms" << endl;

        situations.changeImpl<QVector<CAircraftSituation> >();
        out << "Changed to QVector" << endl;
        timer.start();
        for (int i = 0; i < 10; i++)
        {
            for (int cs = 0; cs < numberOfCallsigns; cs++)
            {
                CCallsign callsign("CS" + QString::number(cs));
                CAircraftSituationList r = situations.findByCallsign(callsign);
                Q_ASSERT(r.size() == numberOfTimes);
            }
        }
        out << "Reads by callsigns: " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (int i = 0; i < 10; i++)
        {
            for (int t = 0; t < numberOfTimes; t++)
            {
                CAircraftSituationList r = situations.findBefore(baseTimeEpoch + 1 + (DeltaTime * t));
                Q_ASSERT(r.size() == numberOfCallsigns * (t + 1));
            }
        }
        out << "Reads by times: " << timer.elapsed() << "ms" << endl << endl;
        return 0;
    }

    int CSamplesPerformance::interpolatorScenario(QTextStream &out, int numberOfCallsigns, int numberOfTimes)
    {
        const qint64 baseTimeEpoch = QDateTime::currentMSecsSinceEpoch();
        CAircraftSituationList situations = createSituations(baseTimeEpoch, numberOfCallsigns, numberOfTimes);
        CAircraftSituationList situationsBefore;
        CAircraftSituationList situationsAfter;

        qint64 halfTime = baseTimeEpoch + DeltaTime * numberOfTimes / 2;

        QTime timer;
        timer.start();
        for (int cs = 0; cs < numberOfCallsigns; cs++)
        {
            CCallsign callsign("CS" + QString::number(cs));
            situationsBefore = situations.findBefore(halfTime).findByCallsign(callsign);
            situationsAfter = situations.findAfter(halfTime - 1).findByCallsign(callsign);
        }
        out << "Reads by time, callsigns: " << timer.elapsed() << "ms" << endl;

        timer.start();
        situationsBefore = situations.findBefore(halfTime);
        situationsAfter = situations.findAfter(halfTime - 1);
        for (int cs = 0; cs < numberOfCallsigns; cs++)
        {
            CCallsign callsign("CS" + QString::number(cs));
            CAircraftSituationList csSituationsBefore = situationsBefore.findByCallsign(callsign);
            CAircraftSituationList csSituationsAfter = situationsAfter.findByCallsign(callsign);
            Q_UNUSED(csSituationsBefore);
            Q_UNUSED(csSituationsAfter);
        }
        out << "Split by time upfront, then callsigns: " << timer.elapsed() << "ms" << endl;
        int b = situationsBefore.size();
        int a = situationsAfter.size();
        Q_ASSERT(a + b == numberOfTimes * numberOfCallsigns);
        Q_UNUSED(a);
        Q_UNUSED(b);

        timer.start();
        QHash<CCallsign, CAircraftSituationList> csSituations = situations.splitPerCallsign();
        out << "Split by " << csSituations.size() << " callsigns, " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (const CAircraftSituationList &csl : csSituations.values())
        {
            CAircraftSituationList csSituationsBefore = csl.findBefore(halfTime);
            CAircraftSituationList csSituationsAfter = csl.findAfter(halfTime - 1);
            a = csSituationsBefore.size();
            b = csSituationsAfter.size();
            Q_ASSERT(a + b == numberOfTimes);
            Q_UNUSED(csSituationsBefore);
            Q_UNUSED(csSituationsAfter);
        }
        out << "Split by callsign, by time: " << timer.elapsed() << "ms" << endl;

        out << endl;
        return 0;
    }

    int CSamplesPerformance::samplesJson(QTextStream &out)
    {
        QTime timer;
        auto situations = createSituations(0, 10000, 10);

        timer.start();
        QString s = situations.toJsonString();
        out << "Convert 100,000 aircraft situations to JSON:   " << timer.elapsed() << "ms" << endl;

        timer.start();
        situations.convertFromJson(s);
        out << "Convert 100,000 aircraft situations from JSON: " << timer.elapsed() << "ms" << endl << endl;

        return 0;
    }

    int CSamplesPerformance::samplesJsonModel(QTextStream &out)
    {
        const QString dir = CBuildConfig::getSwiftStaticDbFilesDir();
        const QString file = QDir(dir).filePath("models.json");
        QFile modelFile(file);
        Q_ASSERT_X(modelFile.exists(), Q_FUNC_INFO, "Model file does not exist");

        out << "Load DB JSON file " << modelFile.fileName() << endl;
        const QString data = CFileUtils::readFileToString(modelFile.fileName());
        Q_ASSERT_X(!data.isEmpty(), Q_FUNC_INFO, "Model file empty");

        // DB format, all models denormalized in DB JSON format
        CDatabaseReader::JsonDatastoreResponse response;
        CDatabaseReader::stringToDatastoreResponse(data, response);
        QTime timer;
        timer.start();
        const CAircraftModelList dbModels = CAircraftModelList::fromDatabaseJson(response);
        int ms = timer.elapsed();
        out << "Read via DB JSON format: " << dbModels.size() << " models in " << ms << "ms" << endl;

        // swift JSON format
        const QJsonObject swiftJsonObject = dbModels.toJson();
        out << "Converted to swift JSON" << endl;

        CAircraftModelList swiftModels;
        timer.start();
        swiftModels.convertFromJson(swiftJsonObject);
        ms = timer.elapsed();
        out << "Read via swift JSON format: " << swiftModels.size() << " models in " << ms << "ms" << endl;

        Q_ASSERT_X(swiftModels.size() == dbModels.size(), Q_FUNC_INFO, "Mismatching container size");
        return 0;
    }

    int CSamplesPerformance::samplesString(QTextStream &out)
    {
        QTime timer;
        static const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789~_-=+!\"@#$%^&*()[]{} \t;:\\/?,.<>";
        QStringList strings;
        std::generate_n(std::back_inserter(strings), 100000, []
        {
            QString s;
            std::generate_n(std::back_inserter(s), 10, [] { return chars[qrand() % chars.size()]; });
            return s;
        });
        QString bigString = strings.join("\n");

        QRegularExpression upperRegex("[A-Z]");
        upperRegex.optimize();

        timer.start();
        for (const QString &s : strings)
        {
            auto c = containsChar(s, [](QChar c) { return c.isUpper(); });
            Q_UNUSED(c);
        }
        out << "Check 100,000 strings for containing uppercase letter: (utility)     " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (const QString &s : strings)
        {
            auto c = s.contains(upperRegex);
            Q_UNUSED(c);
        }
        out << "Check 100,000 strings for containing uppercase letter: (regex)       " << timer.elapsed() << "ms" << endl << endl;

        timer.start();
        for (const QString &s : strings)
        {
            auto i = indexOfChar(s, [](QChar c) { return c.isUpper(); });
            Q_UNUSED(i);
        }
        out << "Check 100,000 strings for index of first uppercase letter: (utility) " << timer.elapsed() << "ms" << endl;

        timer.start();
        for (const QString &s : strings)
        {
            auto i = s.indexOf(upperRegex);
            Q_UNUSED(i);
        }
        out << "Check 100,000 strings for index of first uppercase letter: (regex)   " << timer.elapsed() << "ms" << endl << endl;

        auto temp = strings;
        timer.start();
        for (QString &s : strings)
        {
            removeChars(s, [](QChar c) { return c.isUpper(); });
        }
        out << "Remove from 100,000 strings all uppercase letters: (utility)         " << timer.elapsed() << "ms" << endl;
        strings = temp;

        timer.start();
        for (QString &s : strings)
        {
            s.remove(upperRegex);
        }
        out << "Remove from 100,000 strings all uppercase letters: (regex)           " << timer.elapsed() << "ms" << endl << endl;

        timer.start();
        {
            auto lines = splitLines(bigString);
            Q_UNUSED(lines);
        }
        out << "Split 100,000 line string into list of lines: (QStringList)          " << timer.elapsed() << "ms" << endl;

        timer.start();
        {
            auto lines = splitLinesRefs(bigString);
            Q_UNUSED(lines);
        }
        out << "Split 100,000 line string into list of lines: (QList<QStringRef>)    " << timer.elapsed() << "ms" << endl;

        return 0;
    }

    CAircraftSituationList CSamplesPerformance::createSituations(qint64 baseTimeEpoch, int numberOfCallsigns, int numberOfTimes)
    {
        CAircraftSituationList situations;
        for (int cs = 0; cs < numberOfCallsigns; cs++)
        {
            CCallsign callsign("CS" + QString::number(cs));
            CCoordinateGeodetic coordinate(cs, cs, cs);
            CAltitude alt(cs, CAltitude::MeanSeaLevel, CLengthUnit::m());
            for (int t = 0; t < numberOfTimes; t++)
            {
                CAircraftSituation s(callsign, coordinate, alt);
                s.setMSecsSinceEpoch(baseTimeEpoch + DeltaTime * t);
                situations.push_back(s);
            }
        }
        return situations;
    }

} // namespace
