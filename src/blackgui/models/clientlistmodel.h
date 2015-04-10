/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_CLIENTLISTMODEL_H
#define BLACKGUI_CLIENTLISTMODEL_H

#include "blackgui/blackguiexport.h"
#include <QAbstractItemModel>
#include "blackmisc/network/clientlist.h"
#include "blackgui/models/listmodelbase.h"

namespace BlackGui
{
    namespace Models
    {
        //! Client list model
        class BLACKGUI_EXPORT CClientListModel : public CListModelBase<BlackMisc::Network::CClient, BlackMisc::Network::CClientList>
        {

        public:
            //! Constructor
            explicit CClientListModel(QObject *parent = nullptr);

            //! Destructor
            virtual ~CClientListModel() {}

            //! \copydoc QAbstractListModel::data()
            virtual QVariant data(const QModelIndex &index, int role) const override;

        };
    } // namespace
} // namespace
#endif // guard
