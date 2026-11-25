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

#include "IncomingCallDialog.h"

#include <QCloseEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QShortcut>
#include <QVBoxLayout>

namespace msip {

IncomingCallDialog::IncomingCallDialog(const ActiveCallInfo& call,
                                       const QString& accountLabel,
                                       QWidget* parent)
    : QDialog(parent)
    , m_callId(call.callId) {
    setObjectName("incomingCallDialog");
    setWindowTitle(tr("Chamada Recebida — MicroSIP"));
    setWindowIcon(QIcon(":/icons/microsip.svg"));
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowCloseButtonHint);
    setMinimumWidth(360);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 14);
    root->setSpacing(12);

    // Top Header Bar (Badge + Live Ringing Timer)
    auto* header = new QFrame(this);
    header->setObjectName("incomingHeader");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(14, 9, 14, 9);
    headerLayout->setSpacing(8);

    auto* badgeLbl = new QLabel(tr("📞 Chamada Recebida"), header);
    badgeLbl->setObjectName("incomingBadge");

    m_timerLbl = new QLabel("00:00", header);
    m_timerLbl->setObjectName("incomingTimer");
    m_timerLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    headerLayout->addWidget(badgeLbl, 1);
    headerLayout->addWidget(m_timerLbl);
    root->addWidget(header);

    // Caller Identity Block
    auto* bodyLayout = new QVBoxLayout();
    bodyLayout->setContentsMargins(16, 4, 16, 0);
    bodyLayout->setSpacing(10);

    QString displayName = call.remoteName.isEmpty() ? call.number : call.remoteName;
    if (displayName.isEmpty()) {
        displayName = tr("Desconhecido");
    }

    auto* callerLbl = new QLabel(displayName, this);
    callerLbl->setObjectName("incomingCaller");
    callerLbl->setAlignment(Qt::AlignCenter);
    callerLbl->setWordWrap(true);
    callerLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = callerLbl->font();
    f.setPointSize(17);
    f.setBold(true);
    callerLbl->setFont(f);
    bodyLayout->addWidget(callerLbl);

    if (!call.remoteName.isEmpty() && !call.number.isEmpty() && call.remoteName != call.number) {
        auto* uriLbl = new QLabel(call.number, this);
        uriLbl->setObjectName("incomingSub");
        uriLbl->setAlignment(Qt::AlignCenter);
        uriLbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bodyLayout->addWidget(uriLbl);
    }

    if (!accountLabel.isEmpty()) {
        auto* accLbl = new QLabel(tr("via %1").arg(accountLabel), this);
        accLbl->setObjectName("incomingSub");
        accLbl->setAlignment(Qt::AlignCenter);
        bodyLayout->addWidget(accLbl);
    }

    // Primary Answer / Decline Row
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);

    m_answerBtn = new QPushButton(tr("📞 Atender"), this);
    m_answerBtn->setObjectName("callBtn");
    m_answerBtn->setMinimumHeight(44);
    m_answerBtn->setAutoDefault(false);
    m_answerBtn->setDefault(true);
    connect(m_answerBtn, &QPushButton::clicked, this, [this]() {
        if (m_actionTaken) return;
        m_actionTaken = true;
        disableActionButtons();
        emit answerClicked(m_callId);
        accept();
    });

    m_rejectBtn = new QPushButton(tr("✕ Recusar"), this);
    m_rejectBtn->setObjectName("hangupBtn");
    m_rejectBtn->setMinimumHeight(44);
    m_rejectBtn->setAutoDefault(false);
    connect(m_rejectBtn, &QPushButton::clicked, this, [this]() {
        if (m_actionTaken) return;
        m_actionTaken = true;
        disableActionButtons();
        emit rejectClicked(m_callId);
        QDialog::reject();
    });

    btnRow->addWidget(m_answerBtn, 1);
    btnRow->addWidget(m_rejectBtn, 1);
    bodyLayout->addLayout(btnRow);

    // Quick Redirect / Forward Row
    auto* fwdRow = new QHBoxLayout();
    fwdRow->setSpacing(6);

    m_forwardEdit = new QLineEdit(this);
    m_forwardEdit->setPlaceholderText(tr("Encaminhar para ramal ou URI..."));

    m_forwardBtn = new QPushButton(tr("↪ Encaminhar"), this);
    m_forwardBtn->setAutoDefault(false);

    auto triggerForward = [this]() {
        if (m_actionTaken) return;
        QString dest = m_forwardEdit->text().trimmed();
        if (dest.isEmpty()) {
            m_forwardEdit->setFocus();
            return;
        }
        m_actionTaken = true;
        disableActionButtons();
        emit forwardClicked(m_callId, dest);
        QDialog::reject();
    };
    connect(m_forwardBtn, &QPushButton::clicked, this, triggerForward);
    connect(m_forwardEdit, &QLineEdit::returnPressed, this, triggerForward);

    fwdRow->addWidget(m_forwardEdit, 1);
    fwdRow->addWidget(m_forwardBtn);
    bodyLayout->addLayout(fwdRow);

    // Subtle Ignore (silence ringtone without rejecting) button
    m_ignoreBtn = new QPushButton(tr("🔕 Ignorar (silenciar toque)"), this);
    m_ignoreBtn->setAutoDefault(false);
    m_ignoreBtn->setFlat(true);
    m_ignoreBtn->setStyleSheet("QPushButton { font-size: 11px; color: #8b95a5; border: none; padding: 3px; } "
                               "QPushButton:hover { color: #cbd5e1; text-decoration: underline; }");
    connect(m_ignoreBtn, &QPushButton::clicked, this, [this]() {
        if (m_actionTaken) return;
        m_actionTaken = true;
        disableActionButtons();
        emit ignoreClicked(m_callId);
        QDialog::reject();
    });
    bodyLayout->addWidget(m_ignoreBtn, 0, Qt::AlignCenter);

    root->addLayout(bodyLayout);

    // Keyboard shortcut: Enter/Return answers when forward input is not focused
    auto* enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(enterShortcut, &QShortcut::activated, this, [this]() {
        if (!m_forwardEdit->hasFocus()) {
            m_answerBtn->click();
        }
    });

    // Live 1-second ringing timer
    m_ringTimer = new QTimer(this);
    m_ringTimer->setInterval(1000);
    connect(m_ringTimer, &QTimer::timeout, this, [this]() {
        ++m_elapsedSecs;
        m_timerLbl->setText(
            QString("%1:%2")
                .arg(m_elapsedSecs / 60, 2, 10, QChar('0'))
                .arg(m_elapsedSecs % 60, 2, 10, QChar('0'))
        );
    });
    m_ringTimer->start();

    m_answerBtn->setFocus();
}

void IncomingCallDialog::markDismissedByEngine() {
    m_actionTaken = true;
}

void IncomingCallDialog::disableActionButtons() {
    if (m_answerBtn) m_answerBtn->setEnabled(false);
    if (m_rejectBtn) m_rejectBtn->setEnabled(false);
    if (m_forwardBtn) m_forwardBtn->setEnabled(false);
    if (m_ignoreBtn) m_ignoreBtn->setEnabled(false);
    if (m_forwardEdit) m_forwardEdit->setEnabled(false);
}

void IncomingCallDialog::reject() {
    if (!m_actionTaken) {
        m_actionTaken = true;
        emit ignoreClicked(m_callId);
    }
    QDialog::reject();
}

void IncomingCallDialog::closeEvent(QCloseEvent* event) {
    if (!m_actionTaken) {
        m_actionTaken = true;
        emit ignoreClicked(m_callId);
    }
    QDialog::closeEvent(event);
}

} // namespace msip
