/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/fsdsetup.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/variant.h"
#include "blackmisc/verify.h"
#include "blackmisc/comparefunctions.h"

#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackMisc
{
    namespace Network
    {
        CFsdSetup::CFsdSetup(SendReceiveDetails sendReceive) : m_sendReceive(sendReceive)
        { }

        CFsdSetup::CFsdSetup(const QString &codec, SendReceiveDetails sendReceive)
            : m_textCodec(codec), m_sendReceive(sendReceive) {}

        CFsdSetup::SendReceiveDetails CFsdSetup::getSendReceiveDetails() const
        {
            return static_cast<SendReceiveDetails>(m_sendReceive);
        }

        QString CFsdSetup::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            static const QString s("Codec: '%1' details: '%2'");
            return s.arg(this->getTextCodec(), CFsdSetup::sendReceiveDetailsToString(this->getSendReceiveDetails()));
        }

        QString CFsdSetup::sendReceiveDetailsToString(SendReceiveDetails details)
        {
            static const QString ds("Send parts; %1 gnd: %2 interim: %3 Receive parts: %4 gnd: %5 interim: %6");
            return ds.arg(boolToYesNo(details.testFlag(SendAircraftParts)),
                          boolToYesNo(details.testFlag(SendGndFlag)),
                          boolToYesNo(details.testFlag(SendInterimPositions)),
                          boolToYesNo(details.testFlag(ReceiveAircraftParts)),
                          boolToYesNo(details.testFlag(ReceiveGndFlag)),
                          boolToYesNo(details.testFlag(ReceiveInterimPositions)));
        }

        void CFsdSetup::setSendReceiveDetails(bool partsSend, bool partsReceive, bool gndSend, bool gndReceive, bool interimSend, bool interimReceive)
        {
            SendReceiveDetails s = Nothing;
            if (partsSend)    { s |= SendAircraftParts; }
            if (partsReceive) { s |= ReceiveAircraftParts; }
            if (gndSend)      { s |= SendGndFlag; }
            if (gndReceive)   { s |= ReceiveGndFlag; }
            if (interimSend)  { s |= SendInterimPositions; }
            if (interimReceive) { s |= ReceiveInterimPositions; }
            this->setSendReceiveDetails(s);
        }

        const CFsdSetup &CFsdSetup::vatsimStandard()
        {
            static const CFsdSetup s(AllWithoutGnd);
            return s;
        }

        CStatusMessageList CFsdSetup::validate() const
        {
            static const CLogCategoryList cats(CLogCategoryList(this).join({ CLogCategory::validation()}));
            CStatusMessageList msgs;
            if (this->getTextCodec().isEmpty()) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityError, "No codec")); }
            if (!textCodecNames(true, true).contains(this->getTextCodec())) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityError, "Unrecognized codec name")); }
            msgs.addCategories(cats);
            return msgs;
        }

        CVariant CFsdSetup::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTextCodec: return CVariant::fromValue(m_textCodec);
            case IndexSendReceiveDetails: return CVariant::fromValue(m_sendReceive);
            default: return CValueObject::propertyByIndex(index);
            }
        }

        void CFsdSetup::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CFsdSetup>(); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTextCodec: this->setTextCodec(variant.value<QString>()); break;
            case IndexSendReceiveDetails: this->setSendReceiveDetails(variant.value<SendReceiveDetails>()); break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        int CFsdSetup::comparePropertyByIndex(const CPropertyIndex &index, const CFsdSetup &compareValue) const
        {
            if (index.isMyself()) { return this->convertToQString(true).compare(compareValue.convertToQString()); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTextCodec: return this->getTextCodec().compare(compareValue.getTextCodec());
            case IndexSendReceiveDetails: return Compare::compare(m_sendReceive, compareValue.m_sendReceive);
            default: break;
            }
            BLACK_VERIFY_X(false, Q_FUNC_INFO, qUtf8Printable("No comparison for index " + index.toQString()));
            return 0;
        }
    } // namespace
} // namespace
