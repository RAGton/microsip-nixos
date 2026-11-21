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

#include <QObject>
#include <QLocalServer>
#include <QString>

namespace msip {

class SingleInstance : public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(const QString& serverName = "microsip-qt-ipc", QObject* parent = nullptr);
    ~SingleInstance() override;

    // Returns true if another instance is already running and the message was delivered.
    bool sendToRunningInstance(const QString& message);

    // Starts listening as the primary instance.
    bool startServer();

signals:
    void messageReceived(const QString& message);

private slots:
    void onNewConnection();

private:
    QString m_serverName;
    QLocalServer* m_server = nullptr;
};

} // namespace msip
