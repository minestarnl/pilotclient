/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSGUICOMPONENT_H
#define BLACKGUI_COMPONENTS_SETTINGSGUICOMPONENT_H

#include "blackgui/singleapplicationui.h"
#include "blackgui/settings/guisettings.h"
#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CSettingsGuiComponent; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * General GUI settings
         */
        class CSettingsGuiComponent :
            public QFrame, BlackGui::CSingleApplicationUi
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CSettingsGuiComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CSettingsGuiComponent();

            //! Hide opacity elements
            void hideOpacity(bool hide);

        public slots:
            //! GUI Opacity 0-100%
            void setGuiOpacity(double value);

        signals:
            //! Change the windows opacity 0..100
            void changedWindowsOpacity(int opacity);

        private:
            //! Selection radio buttons changed
            void selectionChanged();

            //! GUI settings changed
            void guiSettingsChanged();

            //! Widget style has changed
            void widgetStyleChanged(const QString &widgetStyle);

            QScopedPointer<Ui::CSettingsGuiComponent> ui;
            BlackMisc::CSetting<Settings::TGeneralGui> m_guiSettings { this, &CSettingsGuiComponent::guiSettingsChanged };
        };
    } // ns
} // ns
#endif // guard
