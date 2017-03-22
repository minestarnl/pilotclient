/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSSIMULATORBASICSCOMPONENT_H
#define BLACKGUI_COMPONENTS_SETTINGSSIMULATORBASICSCOMPONENT_H

#include "blackmisc/simulation/simulatorsettings.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include <QFrame>
#include <QFileDialog>

namespace Ui { class CSettingsSimulatorBasicsComponent; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * Driver independent parts of simulator settings, ie those one are also used independent of the driver.
         * Example: model paths used in loaders
         */
        class CSettingsSimulatorBasicsComponent : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CSettingsSimulatorBasicsComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CSettingsSimulatorBasicsComponent();

            //! Show / hide selector
            void hideSelector(bool show);

            //! Simulator
            void setSimulator(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Save data
            void save();

        private slots:
            void ps_modelFileDialog();
            void ps_excludeFileDialog();
            void ps_simulatorFileDialog();
            void ps_simulatorDirectoryEntered();
            void ps_copyDefaults();
            void ps_adjustModelDirectory();
            void ps_reset();
            void ps_simulatorChanged();

        private:
            QScopedPointer<Ui::CSettingsSimulatorBasicsComponent> ui;
            BlackMisc::Simulation::CMultiSimulatorSettings m_settings { this };
            Qt::CaseSensitivity m_fileCaseSensitivity = BlackMisc::CFileUtils::osFileNameCaseSensitivity();

            //! Optimize for small layout
            void setSmallLayout(bool small);

            //! Exclude directories from line edit
            QStringList parseDirectories(const QString &rawString) const;

            //! Add a directory
            QStringList addDirectory(const QString &directory, const QStringList &existingDirs);

            //! Display the directories
            void displayExcludeDirectoryPatterns(const QStringList &dirs);

            //! Display the directories
            void displayModelDirectories(const QStringList &dirs);

            //! Current settings
            BlackMisc::Simulation::CSimulatorSettings getSettings(const BlackMisc::Simulation::CSimulatorInfo &simulator) const;

            //! Display simulator`s settings
            void displaySettings(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Default values for simulator displayed as placeholder text
            void displayDefaultValuesAsPlaceholder(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Model directory either from input or settings or default
            QString getFileBrowserModelDirectory() const;

            //! Simulator directory either from input or settings or default
            QString getFileBrowserSimulatorDirectory() const;
        };
    } // ns
} // ns

#endif // guard
