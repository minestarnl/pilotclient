/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_TEXTMESSAGEVIEW_H
#define BLACKGUI_TEXTMESSAGEVIEW_H

#include "blackgui/blackguiexport.h"
#include "blackgui/models/textmessagelistmodel.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/network/textmessagelist.h"

class QWidget;

namespace BlackMisc { namespace Network { class CTextMessage; } }
namespace BlackGui
{
    namespace Views
    {
        //! Airports view
        class BLACKGUI_EXPORT CTextMessageView : public CViewBase<Models::CTextMessageListModel, BlackMisc::Network::CTextMessageList, BlackMisc::Network::CTextMessage>
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CTextMessageView(QWidget *parent = nullptr);

            //! Set display mode
            void setTextMessageMode(BlackGui::Models::CTextMessageListModel::TextMessageMode mode);
        };
    }
}
#endif // guard
