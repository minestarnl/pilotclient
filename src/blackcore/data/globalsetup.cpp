/* Copyright (C) 2015
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "globalsetup.h"
#include "blackmisc/math/mathutils.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include <QStringList>

using namespace BlackMisc;
using namespace BlackMisc::Math;
using namespace BlackMisc::Network;

namespace BlackCore
{
    namespace Data
    {
        CGlobalSetup::CGlobalSetup() :
            ITimestampBased(0),
            m_dbRootDirectory("http://ubuntu12/swiftdatastore/public"),
            m_dbHttpPort(80),
            m_dbHttpsPort(443),
            m_vatsimBookings("http://vatbook.euroutepro.com/xml2.php"),
            m_vatsimMetars("http://metar.vatsim.net/metar.php"),
            m_vatsimDataFile(QStringList({ "http://info.vroute.net/vatsim-data.txt" })),
            m_bootstrap(QStringList({ "https://vatsim-germany.org:50443/mapping/public/bootstrap", "http://ubuntu12/public/bootstrap"})),
            m_swiftDbDataFiles(QStringList({})),
            m_fsdTestServers({ CServer("swift", "swift Testserver", "vatsim-germany.org", 6809, CUser("1234567", "swift Test User", "", "123456"), true) })
        { }

        CUrl CGlobalSetup::dbIcaoReader() const
        {
            return dbRootDirectory();
        }

        CUrl CGlobalSetup::dbModelReader() const
        {
            return dbRootDirectory();
        }

        CUrl CGlobalSetup::dbHomePage() const
        {
            return dbRootDirectory().withAppendedPath("/page/index.php");
        }

        CUrl CGlobalSetup::dbLoginService() const
        {
            return dbRootDirectory().
                   withAppendedPath("/service/jsonauthenticate.php").
                   withSwitchedScheme("https", m_dbHttpsPort);
        }

        bool CGlobalSetup::dbDebugFlag() const
        {
            if (!m_dbDebugFlag) { return false; }

            // further checks could go here
            return isDevelopment();
        }

        void CGlobalSetup::setServerDebugFlag(bool debug)
        {
            m_dbDebugFlag = debug;
        }

        bool CGlobalSetup::hasSameType(CGlobalSetup &otherSetup) const
        {
            return this->isDevelopment() == otherSetup.isDevelopment();
        }

        CUrl CGlobalSetup::vatsimMetars() const
        {
            return this->m_vatsimMetars.withAppendedQuery("id=all");
        }

        QString CGlobalSetup::convertToQString(bool i18n) const
        {
            return convertToQString(", ", i18n);
        }

        QString CGlobalSetup::convertToQString(const QString &separator, bool i18n) const
        {
            QString s("timestamp: ");
            s.append(this->getFormattedUtcTimestampYmdhms());
            s.append(separator);
            s.append("For development: ");
            s.append(boolToYesNo(isDevelopment()));
            s.append(separator);
            s.append("DB root directory: ");
            s.append(dbRootDirectory().convertToQString(i18n));
            s.append(separator);
            s.append("ICAO DB reader: ");
            s.append(dbIcaoReader().convertToQString(i18n));
            s.append(separator);
            s.append("Model DB reader: ");
            s.append(dbModelReader().convertToQString(i18n));
            s.append(separator);

            s.append("DB home page: ");
            s.append(dbHomePage().convertToQString(i18n));
            s.append(separator);
            s.append("DB login service: ");
            s.append(dbLoginService().convertToQString(i18n));
            s.append(separator);
            s.append("VATSIM bookings: ");
            s.append(vatsimBookings().convertToQString(i18n));
            s.append(separator);
            s.append("VATSIM METARs: ");
            s.append(vatsimMetars().convertToQString(i18n));
            s.append(separator);
            s.append("VATSIM data file: ");
            s.append(vatsimDataFile().convertToQString(i18n));
            s.append(separator);
            s.append("Bootstrap URLs: ");
            s.append(bootstrapUrls().convertToQString(i18n));
            s.append(separator);
            s.append("swift DB datafile locations: ");
            s.append(swiftDbDataFileLocations().convertToQString(i18n));
            s.append(separator);
            s.append("FSD test servers: ");
            s.append(fsdTestServers().convertToQString(i18n));
            return s;
        }

        CVariant CGlobalSetup::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (ITimestampBased::canHandleIndex(index)) { return ITimestampBased::propertyByIndex(index); }

            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexDbRootDirectory:
                return CVariant::fromValue(this->m_dbRootDirectory);
            case IndexDbHttpPort:
                return CVariant::fromValue(this->m_dbHttpPort);
            case IndexDbHttpsPort:
                return CVariant::fromValue(this->m_dbHttpsPort);
            case IndexDbLoginService:
                return CVariant::fromValue(this->dbLoginService());
            case IndexVatsimData:
                return CVariant::fromValue(this->m_vatsimDataFile);
            case IndexVatsimBookings:
                return CVariant::fromValue(this->m_vatsimDataFile);
            case IndexVatsimMetars:
                return CVariant::fromValue(this->m_vatsimMetars);
            case IndexBootstrap:
                return CVariant::fromValue(this->m_bootstrap);
            case IndexSwiftDbFiles:
                return CVariant::fromValue(this->m_swiftDbDataFiles);
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CGlobalSetup::setPropertyByIndex(const CVariant &variant, const BlackMisc::CPropertyIndex &index)
        {
            if (index.isMyself()) { (*this) = variant.to<CGlobalSetup>(); return; }
            if (ITimestampBased::canHandleIndex(index))
            {
                ITimestampBased::setPropertyByIndex(variant, index);
                return;
            }

            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexDbRootDirectory:
                this->m_dbRootDirectory.setPropertyByIndex(variant, index.copyFrontRemoved());
                break;
            case IndexDbHttpPort:
                this->m_dbHttpPort = variant.toInt();
                break;
            case IndexDbHttpsPort:
                this->m_dbHttpsPort = variant.toInt();
                break;
            case IndexDbLoginService:
                break;
            case IndexVatsimData:
                this->m_vatsimDataFile = variant.value<CUrlList>();
                break;
            case IndexVatsimBookings:
                this->m_vatsimBookings.setPropertyByIndex(variant, index.copyFrontRemoved());
                break;
            case IndexVatsimMetars:
                this->m_vatsimMetars.setPropertyByIndex(variant, index.copyFrontRemoved());
                break;
            case IndexBootstrap:
                this->m_bootstrap = variant.value<CUrlList>();
                break;
            case IndexSwiftDbFiles:
                this->m_swiftDbDataFiles = variant.value<CUrlList>();
                break;
            default:
                CValueObject::setPropertyByIndex(variant, index);
                break;
            }
        }

        const QString &CGlobalSetup::versionString()
        {
            static const QString v("0.6");
            return v;
        }
    } // ns
} // ns
