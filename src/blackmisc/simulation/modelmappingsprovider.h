/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_MODELMAPPINGS_H
#define BLACKMISC_SIMULATION_MODELMAPPINGS_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/network/aircraftmappinglist.h"
#include <QObject>

namespace BlackMisc
{
    namespace Simulation
    {
        //! Model mappings interface, different mapping readers (e.g. from database, from vPilot data files)
        //! can implement this, but provide the same mapping list.
        class BLACKMISC_EXPORT IModelMappingsProvider : public QObject
        {
            Q_OBJECT

        public:
            //! Constructor
            IModelMappingsProvider(QObject *parent = nullptr);

            //! Destructor
            virtual ~IModelMappingsProvider() {}

            //! Load data
            virtual bool read() = 0;

            //! Get list
            const BlackMisc::Network::CAircraftMappingList &getMappingList() const;

        protected:
            BlackMisc::Network::CAircraftMappingList m_mappings; //!< Mappings
        };

        //! Model mappings dummy
        class BLACKMISC_EXPORT CModelMappingsProviderDummy : public IModelMappingsProvider
        {
        public:
            //! Constructor
            CModelMappingsProviderDummy(QObject *parent = nullptr) : IModelMappingsProvider(parent) {}

            //! Destructor
            virtual ~CModelMappingsProviderDummy() {}

            //! Load data
            virtual bool read() override { return true; }
        };
    } // ns
} // ns

#endif // guard
