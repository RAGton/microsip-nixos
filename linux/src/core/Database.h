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

#include "Types.h"
#include <QString>
#include <QVector>
#include <optional>
#include <sqlite3.h>

namespace msip {

class Database {
public:
    explicit Database(const QString& dbPath);
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool isOpen() const { return m_db != nullptr; }

    // Calls
    int addCall(const CallRecord& call);
    void updateCall(const CallRecord& call);
    void deleteCall(int id);
    void deleteAllCalls();
    QVector<CallRecord> getAllCalls(int limit = 1000) const;

    // Contacts
    int addContact(const Contact& contact);
    void updateContact(const Contact& contact);
    void deleteContact(int id);
    void deleteAllContacts();
    QVector<Contact> getAllContacts() const;
    std::optional<Contact> findContactByNumber(const QString& number) const;

    // SIP Messages
    int addMessage(const SipMessage& msg);
    QVector<SipMessage> getMessagesForPeer(const QString& peerUri) const;
    QStringList getMessagePeers() const;
    void deleteMessagesForPeer(const QString& peerUri);

private:
    void initSchema();
    void execSql(const char* sql) const;

    sqlite3* m_db = nullptr;
};

} // namespace msip
