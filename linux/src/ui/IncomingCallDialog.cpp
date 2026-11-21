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

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace msip {

IncomingCallDialog::IncomingCallDialog(const ActiveCallInfo& call, QWidget* parent)
    : QDialog(parent)
    , m_callId(call.callId) {
    setWindowTitle(tr("Incoming Call"));
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setMinimumWidth(300);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);

    auto* titleLbl = new QLabel(tr("Incoming Call"), this);
    titleLbl->setAlignment(Qt::AlignCenter);
    titleLbl->setStyleSheet("color: #666;");

    QString displayName = call.remoteName.isEmpty() ? call.number : call.remoteName;
    auto* callerLbl = new QLabel(displayName, this);
    callerLbl->setAlignment(Qt::AlignCenter);
    QFont f = callerLbl->font();
    f.setPointSize(f.pointSize() + 4);
    f.setBold(true);
    callerLbl->setFont(f);

    auto* uriLbl = new QLabel(call.number, this);
    uriLbl->setAlignment(Qt::AlignCenter);
    uriLbl->setVisible(!call.remoteName.isEmpty() && call.remoteName != call.number);

    root->addWidget(titleLbl);
    root->addWidget(callerLbl);
    root->addWidget(uriLbl);

    auto* btnRow = new QHBoxLayout();
    auto* answerBtn = new QPushButton(tr("Answer"), this);
    answerBtn->setMinimumHeight(36);
    answerBtn->setStyleSheet("QPushButton { background-color: #2e7d32; color: white; font-weight: bold; border-radius: 4px; }");
    connect(answerBtn, &QPushButton::clicked, this, [this]() {
        emit answerClicked(m_callId);
        accept();
    });

    auto* rejectBtn = new QPushButton(tr("Decline"), this);
    rejectBtn->setMinimumHeight(36);
    rejectBtn->setStyleSheet("QPushButton { background-color: #c62828; color: white; font-weight: bold; border-radius: 4px; }");
    connect(rejectBtn, &QPushButton::clicked, this, [this]() {
        emit rejectClicked(m_callId);
        reject();
    });

    btnRow->addWidget(answerBtn);
    btnRow->addWidget(rejectBtn);
    root->addLayout(btnRow);

    // Quick redirect / forward row
    auto* fwdRow = new QHBoxLayout();
    m_forwardEdit = new QLineEdit(this);
    m_forwardEdit->setPlaceholderText(tr("Redirect to number..."));
    auto* fwdBtn = new QPushButton(tr("Forward"), this);
    connect(fwdBtn, &QPushButton::clicked, this, [this]() {
        QString dest = m_forwardEdit->text().trimmed();
        if (!dest.isEmpty()) {
            emit forwardClicked(m_callId, dest);
            reject();
        }
    });
    fwdRow->addWidget(m_forwardEdit, 1);
    fwdRow->addWidget(fwdBtn);
    root->addLayout(fwdRow);
}

} // namespace msip
