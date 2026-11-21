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
#include "core/SipEngine.h"
#include "core/Types.h"

#include <QWidget>
#include <QListWidget>
#include <QTextBrowser>
#include <QLineEdit>
#include <QPushButton>

namespace msip {

class MessagesWidget : public QWidget {
    Q_OBJECT
public:
    explicit MessagesWidget(SipEngine* engine, Database* db, QWidget* parent = nullptr);

    void openConversation(const QString& peerNumber);
    void onIncomingMessage(const SipMessage& msg);
    void reloadPeers();

private slots:
    void onPeerSelected();
    void onSendClicked();
    void onNewChatClicked();

private:
    void loadConversation(const QString& peer);

    SipEngine* m_engine = nullptr;
    Database* m_db = nullptr;

    QLineEdit* m_newPeerEdit = nullptr;
    QListWidget* m_peersList = nullptr;
    QTextBrowser* m_chatView = nullptr;
    QLineEdit* m_msgEdit = nullptr;
    QPushButton* m_sendBtn = nullptr;

    QString m_currentPeer;
};

} // namespace msip
