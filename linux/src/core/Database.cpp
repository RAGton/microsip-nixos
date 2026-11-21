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

#include "Database.h"

#include <QDebug>

namespace msip {

Database::Database(const QString& dbPath) {
    QByteArray pathUtf8 = dbPath.toUtf8();
    if (sqlite3_open(pathUtf8.constData(), &m_db) != SQLITE_OK) {
        qWarning() << "Failed to open SQLite DB:" << (m_db ? sqlite3_errmsg(m_db) : "unknown");
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
        return;
    }
    execSql("PRAGMA busy_timeout=5000;");
    execSql("PRAGMA journal_mode=WAL;");
    initSchema();
}

Database::~Database() {
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void Database::execSql(const char* sql) const {
    if (!m_db) return;
    char* errMsg = nullptr;
    if (sqlite3_exec(m_db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        qWarning() << "SQLite error:" << (errMsg ? errMsg : "unknown") << "in:" << sql;
        sqlite3_free(errMsg);
    }
}

void Database::initSchema() {
    // Matches MicroSIP 3.23.5 call_log schema
    execSql(
        "CREATE TABLE IF NOT EXISTS call_log ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  callId TEXT NOT NULL,"
        "  number TEXT NOT NULL,"
        "  name TEXT NOT NULL,"
        "  type INTEGER,"
        "  time INTEGER,"
        "  duration INTEGER,"
        "  info TEXT NOT NULL"
        ");"
    );
    execSql("CREATE INDEX IF NOT EXISTS idx_call_log_time ON call_log(time DESC);");

    // Trim to latest 1000 records like original MicroSIP
    execSql(
        "DELETE FROM call_log "
        "WHERE (SELECT COUNT(*) FROM call_log) > 1000 "
        "AND id <= (SELECT id FROM call_log ORDER BY id DESC LIMIT 1 OFFSET 1000);"
    );

    execSql(
        "CREATE TABLE IF NOT EXISTS contacts ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  number TEXT NOT NULL,"
        "  name TEXT NOT NULL,"
        "  firstname TEXT DEFAULT '',"
        "  lastname TEXT DEFAULT '',"
        "  phone TEXT DEFAULT '',"
        "  mobile TEXT DEFAULT '',"
        "  email TEXT DEFAULT '',"
        "  address TEXT DEFAULT '',"
        "  city TEXT DEFAULT '',"
        "  state TEXT DEFAULT '',"
        "  zip TEXT DEFAULT '',"
        "  comment TEXT DEFAULT '',"
        "  presence INTEGER DEFAULT 0,"
        "  starred INTEGER DEFAULT 0,"
        "  info TEXT DEFAULT ''"
        ");"
    );

    execSql(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  peerUri TEXT NOT NULL,"
        "  peerName TEXT NOT NULL,"
        "  body TEXT NOT NULL,"
        "  timestamp INTEGER NOT NULL,"
        "  incoming INTEGER NOT NULL,"
        "  status TEXT DEFAULT ''"
        ");"
    );
    execSql("CREATE INDEX IF NOT EXISTS idx_messages_peer ON messages(peerUri, timestamp);");
}

static QString colText(sqlite3_stmt* stmt, int col) {
    const unsigned char* txt = sqlite3_column_text(stmt, col);
    return txt ? QString::fromUtf8(reinterpret_cast<const char*>(txt)) : QString();
}

int Database::addCall(const CallRecord& call) {
    if (!m_db) return -1;
    const char* sql =
        "INSERT INTO call_log (callId, number, name, type, time, duration, info) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    QByteArray callId = call.callId.toUtf8();
    QByteArray number = call.number.toUtf8();
    QByteArray name = call.name.toUtf8();
    QByteArray info = call.info.toUtf8();

    sqlite3_bind_text(stmt, 1, callId.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, number.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(call.type));
    sqlite3_bind_int64(stmt, 5, call.time);
    sqlite3_bind_int(stmt, 6, call.duration);
    sqlite3_bind_text(stmt, 7, info.constData(), -1, SQLITE_TRANSIENT);

    int rowId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        rowId = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return rowId;
}

void Database::updateCall(const CallRecord& call) {
    if (!m_db || call.id <= 0) return;
    const char* sql =
        "UPDATE call_log SET callId=?, number=?, name=?, type=?, time=?, duration=?, info=? "
        "WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    QByteArray callId = call.callId.toUtf8();
    QByteArray number = call.number.toUtf8();
    QByteArray name = call.name.toUtf8();
    QByteArray info = call.info.toUtf8();

    sqlite3_bind_text(stmt, 1, callId.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, number.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, static_cast<int>(call.type));
    sqlite3_bind_int64(stmt, 5, call.time);
    sqlite3_bind_int(stmt, 6, call.duration);
    sqlite3_bind_text(stmt, 7, info.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 8, call.id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteCall(int id) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM call_log WHERE id=?;", -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteAllCalls() {
    execSql("DELETE FROM call_log;");
}

QVector<CallRecord> Database::getAllCalls(int limit) const {
    QVector<CallRecord> result;
    if (!m_db) return result;

    QString sql = "SELECT id, callId, number, name, type, time, duration, info "
                  "FROM call_log ORDER BY time DESC, id DESC";
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    sql += ";";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.toUtf8().constData(), -1, &stmt, nullptr) != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CallRecord rec;
        rec.id = sqlite3_column_int(stmt, 0);
        rec.callId = colText(stmt, 1);
        rec.number = colText(stmt, 2);
        rec.name = colText(stmt, 3);
        rec.type = static_cast<CallDirection>(sqlite3_column_int(stmt, 4));
        rec.time = sqlite3_column_int64(stmt, 5);
        rec.duration = sqlite3_column_int(stmt, 6);
        rec.info = colText(stmt, 7);
        result.push_back(rec);
    }
    sqlite3_finalize(stmt);
    return result;
}

int Database::addContact(const Contact& c) {
    if (!m_db) return -1;
    const char* sql =
        "INSERT INTO contacts (number, name, firstname, lastname, phone, mobile, email, "
        "address, city, state, zip, comment, presence, starred, info) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, c.number.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c.firstname.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c.lastname.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, c.phone.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, c.mobile.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, c.email.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, c.address.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, c.city.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, c.state.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, c.zip.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, c.comment.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, c.presence ? 1 : 0);
    sqlite3_bind_int(stmt, 14, c.starred ? 1 : 0);
    sqlite3_bind_text(stmt, 15, c.info.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    int rowId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        rowId = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return rowId;
}

void Database::updateContact(const Contact& c) {
    if (!m_db || c.id <= 0) return;
    const char* sql =
        "UPDATE contacts SET number=?, name=?, firstname=?, lastname=?, phone=?, mobile=?, "
        "email=?, address=?, city=?, state=?, zip=?, comment=?, presence=?, starred=?, info=? "
        "WHERE id=?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return;

    sqlite3_bind_text(stmt, 1, c.number.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, c.name.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, c.firstname.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, c.lastname.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, c.phone.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, c.mobile.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, c.email.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, c.address.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, c.city.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, c.state.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, c.zip.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, c.comment.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 13, c.presence ? 1 : 0);
    sqlite3_bind_int(stmt, 14, c.starred ? 1 : 0);
    sqlite3_bind_text(stmt, 15, c.info.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, c.id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteContact(int id) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM contacts WHERE id=?;", -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void Database::deleteAllContacts() {
    execSql("DELETE FROM contacts;");
}

QVector<Contact> Database::getAllContacts() const {
    QVector<Contact> result;
    if (!m_db) return result;

    const char* sql =
        "SELECT id, number, name, firstname, lastname, phone, mobile, email, "
        "address, city, state, zip, comment, presence, starred, info "
        "FROM contacts ORDER BY starred DESC, name COLLATE NOCASE ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Contact c;
        c.id = sqlite3_column_int(stmt, 0);
        c.number = colText(stmt, 1);
        c.name = colText(stmt, 2);
        c.firstname = colText(stmt, 3);
        c.lastname = colText(stmt, 4);
        c.phone = colText(stmt, 5);
        c.mobile = colText(stmt, 6);
        c.email = colText(stmt, 7);
        c.address = colText(stmt, 8);
        c.city = colText(stmt, 9);
        c.state = colText(stmt, 10);
        c.zip = colText(stmt, 11);
        c.comment = colText(stmt, 12);
        c.presence = sqlite3_column_int(stmt, 13) != 0;
        c.starred = sqlite3_column_int(stmt, 14) != 0;
        c.info = colText(stmt, 15);
        result.push_back(c);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<Contact> Database::findContactByNumber(const QString& number) const {
    if (!m_db || number.isEmpty()) return std::nullopt;
    const char* sql =
        "SELECT id, number, name, firstname, lastname, phone, mobile, email, "
        "address, city, state, zip, comment, presence, starred, info "
        "FROM contacts WHERE number=? LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(stmt, 1, number.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    std::optional<Contact> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Contact c;
        c.id = sqlite3_column_int(stmt, 0);
        c.number = colText(stmt, 1);
        c.name = colText(stmt, 2);
        c.firstname = colText(stmt, 3);
        c.lastname = colText(stmt, 4);
        c.phone = colText(stmt, 5);
        c.mobile = colText(stmt, 6);
        c.email = colText(stmt, 7);
        c.address = colText(stmt, 8);
        c.city = colText(stmt, 9);
        c.state = colText(stmt, 10);
        c.zip = colText(stmt, 11);
        c.comment = colText(stmt, 12);
        c.presence = sqlite3_column_int(stmt, 13) != 0;
        c.starred = sqlite3_column_int(stmt, 14) != 0;
        c.info = colText(stmt, 15);
        out = c;
    }
    sqlite3_finalize(stmt);
    return out;
}

int Database::addMessage(const SipMessage& msg) {
    if (!m_db) return -1;
    const char* sql =
        "INSERT INTO messages (peerUri, peerName, body, timestamp, incoming, status) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, msg.peerUri.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, msg.peerName.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, msg.body.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, msg.timestamp);
    sqlite3_bind_int(stmt, 5, msg.incoming ? 1 : 0);
    sqlite3_bind_text(stmt, 6, msg.status.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    int rowId = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        rowId = static_cast<int>(sqlite3_last_insert_rowid(m_db));
    }
    sqlite3_finalize(stmt);
    return rowId;
}

QVector<SipMessage> Database::getMessagesForPeer(const QString& peerUri) const {
    QVector<SipMessage> result;
    if (!m_db) return result;

    const char* sql =
        "SELECT id, peerUri, peerName, body, timestamp, incoming, status "
        "FROM messages WHERE peerUri=? ORDER BY timestamp ASC, id ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;
    sqlite3_bind_text(stmt, 1, peerUri.toUtf8().constData(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SipMessage m;
        m.id = sqlite3_column_int(stmt, 0);
        m.peerUri = colText(stmt, 1);
        m.peerName = colText(stmt, 2);
        m.body = colText(stmt, 3);
        m.timestamp = sqlite3_column_int64(stmt, 4);
        m.incoming = sqlite3_column_int(stmt, 5) != 0;
        m.status = colText(stmt, 6);
        result.push_back(m);
    }
    sqlite3_finalize(stmt);
    return result;
}

QStringList Database::getMessagePeers() const {
    QStringList peers;
    if (!m_db) return peers;
    const char* sql =
        "SELECT peerUri FROM messages GROUP BY peerUri ORDER BY MAX(timestamp) DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return peers;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        peers.append(colText(stmt, 0));
    }
    sqlite3_finalize(stmt);
    return peers;
}

void Database::deleteMessagesForPeer(const QString& peerUri) {
    if (!m_db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM messages WHERE peerUri=?;", -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, peerUri.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace msip
