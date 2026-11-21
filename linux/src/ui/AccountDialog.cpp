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

#include "AccountDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace msip {

AccountDialog::AccountDialog(const Account& account, QWidget* parent)
    : QDialog(parent)
    , m_original(account) {
    setWindowTitle(account.id > 0 ? tr("Edit Account") : tr("Add Account"));
    setMinimumWidth(420);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout();

    m_labelEdit = new QLineEdit(account.label, this);
    m_serverEdit = new QLineEdit(account.server, this);
    m_proxyEdit = new QLineEdit(account.proxy, this);
    m_usernameEdit = new QLineEdit(account.username, this);
    m_domainEdit = new QLineEdit(account.domain, this);
    m_authIdEdit = new QLineEdit(account.authID, this);
    m_passwordEdit = new QLineEdit(account.password, this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_displayNameEdit = new QLineEdit(account.displayName, this);
    m_voicemailEdit = new QLineEdit(account.voicemailNumber, this);
    m_dialPrefixEdit = new QLineEdit(account.dialingPrefix, this);

    m_srtpCombo = new QComboBox(this);
    m_srtpCombo->addItem(tr("Disabled"), "");
    m_srtpCombo->addItem(tr("Optional"), "optional");
    m_srtpCombo->addItem(tr("Mandatory"), "mandatory");
    int srtpIdx = m_srtpCombo->findData(account.srtp.toLower());
    if (srtpIdx >= 0) m_srtpCombo->setCurrentIndex(srtpIdx);

    m_transportCombo = new QComboBox(this);
    m_transportCombo->addItem(tr("UDP + TCP (Default)"), "");
    m_transportCombo->addItem("UDP", "udp");
    m_transportCombo->addItem("TCP", "tcp");
    m_transportCombo->addItem("TLS", "tls");
    int trIdx = m_transportCombo->findData(account.transport.toLower());
    if (trIdx >= 0) m_transportCombo->setCurrentIndex(trIdx);

    m_publicAddrEdit = new QLineEdit(account.publicAddr, this);

    m_regRefreshSpin = new QSpinBox(this);
    m_regRefreshSpin->setRange(30, 86400);
    m_regRefreshSpin->setValue(account.registerRefresh > 0 ? account.registerRefresh : 300);
    m_regRefreshSpin->setSuffix(" s");

    m_keepAliveSpin = new QSpinBox(this);
    m_keepAliveSpin->setRange(0, 600);
    m_keepAliveSpin->setValue(account.keepAlive);
    m_keepAliveSpin->setSuffix(" s");

    m_publishCheck = new QCheckBox(tr("Publish Presence"), this);
    m_publishCheck->setChecked(account.publish);
    m_iceCheck = new QCheckBox(tr("Enable ICE"), this);
    m_iceCheck->setChecked(account.ice);
    m_allowRewriteCheck = new QCheckBox(tr("Allow IP Rewrite"), this);
    m_allowRewriteCheck->setChecked(account.allowRewrite);
    m_disableSessionTimerCheck = new QCheckBox(tr("Disable Session Timers"), this);
    m_disableSessionTimerCheck->setChecked(account.disableSessionTimer);

    form->addRow(tr("Account Name:"), m_labelEdit);
    form->addRow(tr("SIP Server:"), m_serverEdit);
    form->addRow(tr("SIP Proxy:"), m_proxyEdit);
    form->addRow(tr("Username:"), m_usernameEdit);
    form->addRow(tr("Domain:"), m_domainEdit);
    form->addRow(tr("Login (Auth ID):"), m_authIdEdit);
    form->addRow(tr("Password:"), m_passwordEdit);
    form->addRow(tr("Display Name:"), m_displayNameEdit);
    form->addRow(tr("Voicemail Number:"), m_voicemailEdit);
    form->addRow(tr("Dialing Prefix:"), m_dialPrefixEdit);
    form->addRow(tr("Media Encryption (SRTP):"), m_srtpCombo);
    form->addRow(tr("Transport:"), m_transportCombo);
    form->addRow(tr("Public Address:"), m_publicAddrEdit);
    form->addRow(tr("Register Refresh:"), m_regRefreshSpin);
    form->addRow(tr("Keep-Alive:"), m_keepAliveSpin);
    form->addRow("", m_publishCheck);
    form->addRow("", m_iceCheck);
    form->addRow("", m_allowRewriteCheck);
    form->addRow("", m_disableSessionTimerCheck);

    root->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    if (account.id > 0) {
        auto* delBtn = new QPushButton(tr("Delete Account"), this);
        delBtn->setStyleSheet("color: #c62828;");
        connect(delBtn, &QPushButton::clicked, this, [this]() {
            if (QMessageBox::question(this, tr("Delete Account"), tr("Delete this SIP account?")) == QMessageBox::Yes) {
                m_deleteRequested = true;
                accept();
            }
        });
        btnRow->addWidget(delBtn);
    }
    btnRow->addStretch();

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    btnRow->addWidget(buttons);

    root->addLayout(btnRow);
}

Account AccountDialog::resultAccount() const {
    Account a = m_original;
    a.label = m_labelEdit->text().trimmed();
    a.server = m_serverEdit->text().trimmed();
    a.proxy = m_proxyEdit->text().trimmed();
    a.username = m_usernameEdit->text().trimmed();
    a.domain = m_domainEdit->text().trimmed();
    a.authID = m_authIdEdit->text().trimmed();
    a.password = m_passwordEdit->text();
    a.displayName = m_displayNameEdit->text().trimmed();
    a.voicemailNumber = m_voicemailEdit->text().trimmed();
    a.dialingPrefix = m_dialPrefixEdit->text().trimmed();
    a.srtp = m_srtpCombo->currentData().toString();
    a.transport = m_transportCombo->currentData().toString();
    a.publicAddr = m_publicAddrEdit->text().trimmed();
    a.registerRefresh = m_regRefreshSpin->value();
    a.keepAlive = m_keepAliveSpin->value();
    a.publish = m_publishCheck->isChecked();
    a.ice = m_iceCheck->isChecked();
    a.allowRewrite = m_allowRewriteCheck->isChecked();
    a.disableSessionTimer = m_disableSessionTimerCheck->isChecked();
    return a;
}

} // namespace msip
