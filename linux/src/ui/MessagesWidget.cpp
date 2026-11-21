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

#include "MessagesWidget.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QSplitter>
#include <QVBoxLayout>

namespace msip {

MessagesWidget::MessagesWidget(SipEngine* engine, Database* db, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
    , m_db(db) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    // Quick start conversation bar
    auto* topRow = new QHBoxLayout();
    m_newPeerEdit = new QLineEdit(this);
    m_newPeerEdit->setPlaceholderText(tr("Número ou URI SIP para conversar..."));
    auto* newBtn = new QPushButton(tr("Nova Conversa"), this);
    newBtn->setObjectName("primaryBtn");
    connect(newBtn, &QPushButton::clicked, this, &MessagesWidget::onNewChatClicked);
    connect(m_newPeerEdit, &QLineEdit::returnPressed, this, &MessagesWidget::onNewChatClicked);
    topRow->addWidget(m_newPeerEdit, 1);
    topRow->addWidget(newBtn);
    root->addLayout(topRow);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_peersList = new QListWidget(this);
    m_peersList->setMaximumWidth(145);
    connect(m_peersList, &QListWidget::itemSelectionChanged, this, &MessagesWidget::onPeerSelected);

    auto* rightPane = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    m_chatView = new QTextBrowser(this);
    m_chatView->setOpenExternalLinks(true);

    auto* inputRow = new QHBoxLayout();
    m_msgEdit = new QLineEdit(this);
    m_msgEdit->setPlaceholderText(tr("Digite uma mensagem SIP..."));
    connect(m_msgEdit, &QLineEdit::returnPressed, this, &MessagesWidget::onSendClicked);

    m_sendBtn = new QPushButton(tr("Enviar"), this);
    m_sendBtn->setObjectName("primaryBtn");
    connect(m_sendBtn, &QPushButton::clicked, this, &MessagesWidget::onSendClicked);

    inputRow->addWidget(m_msgEdit, 1);
    inputRow->addWidget(m_sendBtn);

    rightLayout->addWidget(m_chatView, 1);
    rightLayout->addLayout(inputRow);

    splitter->addWidget(m_peersList);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(1, 1);

    root->addWidget(splitter, 1);

    reloadPeers();
}

void MessagesWidget::reloadPeers() {
    QString selected = m_currentPeer;
    m_peersList->clear();
    for (const QString& peer : m_db->getMessagePeers()) {
        m_peersList->addItem(peer);
    }
    if (!selected.isEmpty()) {
        auto items = m_peersList->findItems(selected, Qt::MatchExactly);
        if (!items.isEmpty()) {
            m_peersList->setCurrentItem(items.first());
        }
    }
}

void MessagesWidget::openConversation(const QString& peerNumber) {
    QString clean = SipEngine::extractNumberFromUri(peerNumber.trimmed());
    if (clean.isEmpty()) return;

    auto items = m_peersList->findItems(clean, Qt::MatchExactly);
    if (items.isEmpty()) {
        m_peersList->insertItem(0, clean);
        m_peersList->setCurrentRow(0);
    } else {
        m_peersList->setCurrentItem(items.first());
    }
    m_currentPeer = clean;
    loadConversation(clean);
    m_msgEdit->setFocus();
}

void MessagesWidget::onIncomingMessage(const SipMessage& msg) {
    reloadPeers();
    if (m_currentPeer == msg.peerUri) {
        loadConversation(m_currentPeer);
    }
}

void MessagesWidget::onPeerSelected() {
    auto* item = m_peersList->currentItem();
    if (!item) return;
    m_currentPeer = item->text();
    loadConversation(m_currentPeer);
}

void MessagesWidget::onNewChatClicked() {
    QString peer = m_newPeerEdit->text().trimmed();
    if (!peer.isEmpty()) {
        m_newPeerEdit->clear();
        openConversation(peer);
    }
}

void MessagesWidget::onSendClicked() {
    QString text = m_msgEdit->text().trimmed();
    if (text.isEmpty() || m_currentPeer.isEmpty()) return;

    if (m_engine->sendMessage(m_currentPeer, text)) {
        m_msgEdit->clear();
        loadConversation(m_currentPeer);
        reloadPeers();
    }
}

void MessagesWidget::loadConversation(const QString& peer) {
    m_chatView->clear();
    auto msgs = m_db->getMessagesForPeer(peer);
    QString html;
    for (const auto& m : msgs) {
        QString ts = QDateTime::fromSecsSinceEpoch(m.timestamp).toString("HH:mm");
        QString sender = m.incoming ? m.peerName.toHtmlEscaped() : tr("Você");
        QString align = m.incoming ? "left" : "right";
        QString bg = m.incoming ? "#2c3442" : "#1d4ed8";
        html += QString(
            "<div style='text-align:%1; margin-bottom:8px;'>"
            "<span style='display:inline-block; background-color:%2; color:#f8fafc; "
            "padding:6px 10px; border-radius:8px;'>"
            "<b style='font-size:11px; color:#cbd5e1;'>%3</b> "
            "<span style='font-size:10px; color:#94a3b8;'>%4</span><br/>%5"
            "</span></div>"
        ).arg(align, bg, sender, ts, m.body.toHtmlEscaped());
    }
    m_chatView->setHtml(html);
}

} // namespace msip
