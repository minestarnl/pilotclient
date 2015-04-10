/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_INFOAREA_H
#define BLACKGUI_INFOAREA_H

#include "blackgui/blackguiexport.h"
#include "dockwidgetinfoarea.h"
#include "enableforframelesswindow.h"
#include <QMainWindow>
#include <QTabBar>
#include <QPixmap>
#include <QList>

namespace BlackGui
{
    //! Info area, hosting dockable widgets
    //! \sa CDockWidgetInfoArea
    class BLACKGUI_EXPORT CInfoArea : public QMainWindow, CEnableForFramelessWindow
    {
        Q_OBJECT

    public:
        //! Destructor
        virtual ~CInfoArea();

        //! Add items to context menu
        void addToContextMenu(QMenu *menu) const;

        //! Is the area floating?
        bool isFloating() const { return this->m_infoAreaFloating; }

        //! Is given widget selected. Means it is not floating, and the one selected
        bool isSelectedDockWidgetInfoArea(const CDockWidgetInfoArea *infoArea) const;

        //! Get the selected info area (non floating, selected in tabbar)
        const CDockWidgetInfoArea *getSelectedDockInfoArea() const;

        //! Get the selected info area (non floating, selected in tabbar)
        int getSelectedDockInfoAreaIndex() const;

        //! Own dockable widgets
        QList<const CDockWidgetInfoArea *> getDockWidgetInfoAreas() const;

        //! Create a list of actions to select the info areas.
        //! This could be used in a menu or somewhere else.
        //! \param parent which will own the action (deletion)
        QList<QAction *> getInfoAreaSelectActions(QWidget *parent) const;

        //! Create a list of actions to select the info areas and toogle its floating state.
        //! This could be used in a menu or somewhere else.
        //! \param parent which will own the action (deletion)
        QList<QAction *> getInfoAreaToggleFloatingActions(QWidget *parent) const;

        //! Docked area indexes
        QList<int> getAreaIndexesDockedOrFloating(bool floating) const;

    signals:
        //! Tab bar changed
        void changedInfoAreaTabBarIndex(int index);

        //! Status of info area changed
        void changedInfoAreaStatus(int currentTabIndex, QList<int> dockedAreas, QList<int> floatingAreas);

        //! Whole info area floating
        void changedWholeInfoAreaFloating(bool floating);

    public slots:
        //! Dock all widgets
        void dockAllWidgets();

        //! Adjust size for all dock widgets
        void adjustSizeForAllDockWidgets();

        //! All widgets floating
        void floatAllWidgets();

        //! Toggle dock / floating of the whole info area
        virtual void toggleFloatingWholeInfoArea();

        //! Toggle floating of index
        void toggleFloatingByIndex(int areaIndex);

        //! Toggle visibilty
        void toggleVisibility(int areaIndex);

        //! Select area
        void selectArea(int areaIndex);

        //! Set current tab bar index by given widget
        void selectArea(const CDockWidgetInfoArea *dockWidgetInfoArea);

        //! Select area (sender is QAction)
        void selectAreaByAction();

        //! Toggle area floating (sender is QAction)
        void toggleAreaFloatingByAction();

        //! Select next left tab
        void selectLeftTab();

        //! Select next right tab
        void selectRightTab();

        //! Display status message in all info areas (according their state)
        void displayStatusMessage(const BlackMisc::CStatusMessage &statusMessage);

        //! Display status messages in all info areas (according their state)
        void displayStatusMessages(const BlackMisc::CStatusMessageList &statusMessages);

    protected:
        //! Constructor
        explicit CInfoArea(QWidget *parent = nullptr);

        //! \copydoc QWidget::closeEvent
        virtual void closeEvent(QCloseEvent *event) override;

        //! \copydoc QWidget::paintEvent
        virtual void paintEvent(QPaintEvent *event) override;

        //! \copydoc QWidget::keyPressEvent
        //! \remarks nor fully sufficient, as the info area is hardly having focus
        virtual void keyPressEvent(QKeyEvent *event) override;

        //! Preferred size when floating (size hint)
        virtual QSize getPreferredSizeWhenFloating(int areaIndex) const = 0;

        //! Info area (index) to icon
        virtual const QPixmap &indexToPixmap(int areaIndex) const = 0;

        //! Init area after(!) GUI is initialized
        void initInfoArea();

    protected:
        //! Tab position for docked widgets tab
        //! \remarks North or South working, East / West not
        void ps_setTabBarPosition(QTabWidget::TabPosition position);

    private:
        QList<CDockWidgetInfoArea *> m_dockWidgetInfoAreas ;
        QTabBar *m_tabBar = nullptr;
        bool m_showTabTexts     = true;   //!< texts for tabs
        bool m_infoAreaFloating = false;  //!< whole info area floating?
        bool m_showTabBar       = true;   //!< show the tab bar?
        bool m_lockTabBar       = false;  //!< locked: means double clicks ignored

        //! Tabify the widgets
        void tabifyAllWidgets();

        //! Untabify
        void unTabifyAllWidgets();

        //! Valid area index?
        bool isValidAreaIndex(int areaIndex) const;

        //! Corresponding dockable widget for given tab index
        CDockWidgetInfoArea *getDockWidgetInfoAreaByTabBarIndex(int tabBarIndex);

        //! Corresponding dockable widget for given window title
        CDockWidgetInfoArea *getDockWidgetInfoAreaByWindowTitle(const QString &title);

        //! Corresponding dockable widget for given window title
        int getAreaIndexByWindowTitle(const QString &title) const;

        //! Tab bar index by title
        int getTabBarIndexByTitle(const QString &title) const;

        //! Widget to tab bar index
        int dockWidgetInfoAreaToTabBarIndex(const CDockWidgetInfoArea *dockWidgetInfoArea) const;

        //! Features of the dockable widgets
        void setFeaturesForDockableWidgets(QDockWidget::DockWidgetFeatures features);

        //! Number of tabbed widgets
        int countDockedWidgetInfoAreas() const;

        //! Set the tab's icons
        void setTabPixmaps();

        //! Connect all widgets
        void connectAllWidgets();

        //! Margins for the floating widgets
        void setMarginsWhenFloating(int left, int top, int right, int bottom);

        //! Margins for the floating widgets (when frameless)
        void setMarginsWhenFramelessFloating(int left, int top, int right, int bottom);

        //! Margins for the dockable widgets
        void setMarginsWhenDocked(int left, int top, int right, int bottom);

        //! Nested info areas
        QList<CInfoArea *> getChildInfoAreas() const { return this->findChildren<CInfoArea *>(); }

        //! Direct dock widget areas, not the nested dock widget areas
        //! \remarks result stored in m_dockableWidgets
        QList<CDockWidgetInfoArea *> findOwnDockWidgetInfoAreas() const;

        //! Settings based on ini file
        void iniFileBasedSettings();

    private slots:
        //! Tab bar has been double clicked
        void ps_tabBarDoubleClicked(int tabBarIndex);

        //! A widget has changed its top level
        void ps_onWidgetTopLevelChanged(CDockWidget *dockWidget, bool topLevel);

        //! Style sheet has been changed
        void ps_onStyleSheetChanged();

        //! Context menu
        void ps_showContextMenu(const QPoint &pos);

        //! Show the tab texts, or just the icons
        void ps_showTabTexts(bool show);

        //! Show tab bar
        void ps_showTabBar(bool show);

        //! Toogle lock tabbar
        void ps_toggleTabBarLocked(bool locked);

        //! Toggle tab position North - South
        void ps_toggleTabBarPosition();

        //! Set dock area used
        void ps_setDockArea(Qt::DockWidgetArea area);

        //! Dock / floating of the whole info area
        void ps_setWholeInfoAreaFloating(bool floating);

        //! Emit current status, \sa changedInfoAreaStatus
        void ps_emitInfoAreaStatus();

        //! Tab bar index changed
        void ps_onTabBarIndexChanged(int tabBarIndex);

    };
} // namespace

#endif // guard
