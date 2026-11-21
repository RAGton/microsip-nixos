/*
 * MicroSIP - Native Linux / NixOS Port (Qt 6 + PJSIP)
 * Based on MicroSIP (https://www.microsip.org)
 * Copyright (C) 2011-2026 MicroSIP (https://www.microsip.org)
 * Native Linux/NixOS Qt6 Port Copyright (C) 2026 Gabriel Aguiar Rocha (RAGton) <aguiarrocha37@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once

#include "core/Database.h"
#include "core/Types.h"

#include <QWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QPushButton>

namespace msip {

class ContactsWidget : public QWidget {
    Q_OBJECT
public:
    explicit ContactsWidget(Database* db, QWidget* parent = nullptr);

    void reload();
    QVector<Contact> contacts() const { return m_contacts; }
    void openAddContactDialog(const QString& number = {}, const QString& name = {});
    void updatePresence(const QString& number, PresenceStatus status, bool ringing, const QString& statusText);
    void importContacts();
    void exportContactsCsv();

signals:
    void callNumberRequested(const QString& number);
    void messageNumberRequested(const QString& number);
    void contactsModified();

private slots:
    void onContextMenu(const QPoint& pos);

private:
    void populateTable(const QString& filter);
    bool editContactDialog(Contact& c, bool isNew);

    Database* m_db = nullptr;
    QLineEdit* m_searchEdit = nullptr;
    QPushButton* m_addBtn = nullptr;
    QTableWidget* m_table = nullptr;
    QVector<Contact> m_contacts;
};

} // namespace msip
