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

#include "SingleInstance.h"

#include <QLocalSocket>

namespace msip {

SingleInstance::SingleInstance(const QString& serverName, QObject* parent)
    : QObject(parent)
    , m_serverName(serverName) {}

SingleInstance::~SingleInstance() {
    if (m_server) {
        m_server->close();
        QLocalServer::removeServer(m_serverName);
    }
}

bool SingleInstance::sendToRunningInstance(const QString& message) {
    QLocalSocket socket;
    socket.connectToServer(m_serverName, QIODevice::WriteOnly);
    if (!socket.waitForConnected(400)) {
        return false;
    }
    QByteArray payload = message.toUtf8();
    socket.write(payload);
    socket.flush();
    socket.waitForBytesWritten(400);
    socket.disconnectFromServer();
    return true;
}

bool SingleInstance::startServer() {
    if (m_server) return true;
    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);

    if (!m_server->listen(m_serverName)) {
        // Stale socket from a previous crash: remove and retry
        QLocalServer::removeServer(m_serverName);
        if (!m_server->listen(m_serverName)) {
            return false;
        }
    }
    connect(m_server, &QLocalServer::newConnection, this, &SingleInstance::onNewConnection);
    return true;
}

void SingleInstance::onNewConnection() {
    while (auto* client = m_server->nextPendingConnection()) {
        connect(client, &QLocalSocket::readyRead, this, [this, client]() {
            QByteArray data = client->readAll();
            if (!data.isEmpty()) {
                emit messageReceived(QString::fromUtf8(data));
            }
            client->disconnectFromServer();
        });
        connect(client, &QLocalSocket::disconnected, client, &QLocalSocket::deleteLater);
    }
}

} // namespace msip
