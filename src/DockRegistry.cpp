/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DockRegistry_p.h"
#include "DockWidget.h"
#include "Logging_p.h"
#include "DebugWindow_p.h"
#include "LastPosition_p.h"
#include "multisplitter/MultiSplitterLayout_p.h"

#include <QPointer>
#include <QDebug>

using namespace KDDockWidgets;

DockRegistry::DockRegistry(QObject *parent)
    : QObject(parent)
{
    KDDockWidgets::setLoggingFilterRules();

#ifdef DOCKS_DEVELOPER_MODE
    if (qEnvironmentVariableIntValue("KDDOCKWIDGETS_SHOW_DEBUG_WINDOW") == 1) {
        auto dv = new Debug::DebugWindow();
        dv->show();
    }
#endif
}

void DockRegistry::maybeDelete()
{
    if (isEmpty())
        delete this;
}

bool DockRegistry::isEmpty() const
{
    return m_dockWidgets.isEmpty() && m_mainWindows.isEmpty() && m_nestedWindows.isEmpty();
}

DockRegistry *DockRegistry::self()
{
    static QPointer<DockRegistry> s_dockRegistry;

    if (!s_dockRegistry) {
        s_dockRegistry = new DockRegistry();
    }

    return s_dockRegistry;
}

void DockRegistry::registerDockWidget(DockWidget *dock)
{
    if (dock->name().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "DockWidget" << dock << " doesn't have an ID";
    } else if (auto other = dockByName(dock->name())) {
        qWarning() << Q_FUNC_INFO << "Another DockWidget" << other << "with name" << dock->name() << " already exists." << dock;
    }

    m_dockWidgets << dock;
}

void DockRegistry::unregisterDockWidget(DockWidget *dock)
{
    m_dockWidgets.removeOne(dock);
    maybeDelete();
}

void DockRegistry::registerMainWindow(MainWindow *mainWindow)
{
    if (mainWindow->name().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "MainWindow" << mainWindow << " doesn't have an ID";
    } else if (auto other = mainWindowByName(mainWindow->name())) {
        qWarning() << Q_FUNC_INFO << "Another MainWindow" << other << "with name" << mainWindow->name() << " already exists." << mainWindow;
    }

    m_mainWindows << mainWindow;
}

void DockRegistry::unregisterMainWindow(MainWindow *mainWindow)
{
    m_mainWindows.removeOne(mainWindow);
    maybeDelete();
}

void DockRegistry::registerNestedWindow(FloatingWindow *window)
{
    m_nestedWindows << window;
}

void DockRegistry::unregisterNestedWindow(FloatingWindow *window)
{
    m_nestedWindows.removeOne(window);
    maybeDelete();
}

void DockRegistry::registerLayout(MultiSplitterLayout *layout)
{
    m_layouts << layout;
}

void DockRegistry::unregisterLayout(MultiSplitterLayout *layout)
{
    m_layouts.removeOne(layout);
}

DockWidget *DockRegistry::dockByName(const QString &name) const
{
    for (auto dock : qAsConst(m_dockWidgets)) {
        if (dock->name() == name)
            return dock;
    }

    return nullptr;
}

MainWindow *DockRegistry::mainWindowByName(const QString &name) const
{
    for (auto mainWindow : qAsConst(m_mainWindows)) {
        if (mainWindow->name() == name)
            return mainWindow;
    }

    return nullptr;
}

bool DockRegistry::isSane() const
{
    QSet<QString> names;
    for (auto dock : qAsConst(m_dockWidgets)) {
        const QString name = dock->name();
        if (name.isEmpty()) {
            qWarning() << "DockRegistry::isSane: DockWidget" << dock << "is missing a name";
            return false;
        } else if (names.contains(name)) {
            qWarning() << "DockRegistry::isSane: dockWidgets with duplicate names:" << name;
            return false;
        } else {
            names.insert(name);
        }
    }

    names.clear();
    for (auto mainwindow : qAsConst(m_mainWindows)) {
        const QString name = mainwindow->name();
        if (name.isEmpty()) {
            qWarning() << "DockRegistry::isSane: MainWindow" << mainwindow << "is missing a name";
            return false;
        } else if (names.contains(name)) {
            qWarning() << "DockRegistry::isSane: mainWindow with duplicate names:" << name;
            return false;
        } else {
            names.insert(name);
        }
    }

    return true;
}

DockWidget::List DockRegistry::dockwidgets() const
{
    return m_dockWidgets;
}

DockWidget::List DockRegistry::closedDockwidgets() const
{
    DockWidget::List result;
    result.reserve(m_dockWidgets.size());

    for (DockWidget *dw : m_dockWidgets) {
        if (dw->parent() == nullptr && !dw->isVisible())
            result.push_back(dw);
    }

    return result;
}

MainWindow::List DockRegistry::mainwindows() const
{
    return m_mainWindows;
}

QVector<MultiSplitterLayout *> DockRegistry::layouts() const
{
    return m_layouts;
}

const QVector<FloatingWindow *> DockRegistry::nestedwindows() const
{
    // Returns all the FloatingWindow which aren't being deleted
    QVector<FloatingWindow *> result;
    result.reserve(m_nestedWindows.size());
    for (FloatingWindow *fw : m_nestedWindows) {
        if (!fw->beingDeleted())
            result.push_back(fw);
    }

    return result;
}

void DockRegistry::clear(bool deleteStaticAnchors)
{
    for (auto dw : qAsConst(m_dockWidgets)) {
        dw->close();
        dw->lastPosition()->removePlaceholders();
    }

    for (auto mw : qAsConst(m_mainWindows))
        mw->multiSplitterLayout()->clear(deleteStaticAnchors);

    qCDebug(restoring) << Q_FUNC_INFO << "; dockwidgets=" << m_dockWidgets.size()
                       << "; nestedwindows=" << m_nestedWindows.size();
}

void DockRegistry::ensureAllFloatingWidgetsAreMorphed()
{
    for (DockWidget *dw : qAsConst(m_dockWidgets)) {
        if (dw->window() == dw && dw->isVisible())
            dw->morphIntoFloatingWindow();
    }
}

DockRegistry::~DockRegistry()
{
}
