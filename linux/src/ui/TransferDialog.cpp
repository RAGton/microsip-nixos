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

#include "TransferDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace msip {

TransferDialog::TransferDialog(int sourceCallId, const QVector<ActiveCallInfo>& activeCalls, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Transfer Call"));
    setMinimumWidth(340);

    auto* root = new QVBoxLayout(this);

    m_blindRadio = new QRadioButton(tr("Blind Transfer to number:"), this);
    m_blindRadio->setChecked(true);
    m_numberEdit = new QLineEdit(this);
    m_numberEdit->setPlaceholderText(tr("Enter extension or SIP URI..."));

    m_attendedRadio = new QRadioButton(tr("Attended Transfer to active call:"), this);
    m_callsCombo = new QComboBox(this);
    for (const auto& c : activeCalls) {
        if (c.callId != sourceCallId) {
            QString label = c.remoteName.isEmpty() ? c.number : QString("%1 (%2)").arg(c.remoteName, c.number);
            m_callsCombo->addItem(label, c.callId);
        }
    }
    bool hasOtherCalls = m_callsCombo->count() > 0;
    m_attendedRadio->setEnabled(hasOtherCalls);
    m_callsCombo->setEnabled(false);

    connect(m_blindRadio, &QRadioButton::toggled, this, [this](bool checked) {
        m_numberEdit->setEnabled(checked);
        m_callsCombo->setEnabled(!checked && m_callsCombo->count() > 0);
    });

    root->addWidget(m_blindRadio);
    root->addWidget(m_numberEdit);
    root->addWidget(m_attendedRadio);
    root->addWidget(m_callsCombo);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

bool TransferDialog::isAttended() const {
    return m_attendedRadio->isChecked();
}

QString TransferDialog::targetNumber() const {
    return m_numberEdit->text().trimmed();
}

int TransferDialog::targetCallId() const {
    return m_callsCombo->currentData().isValid() ? m_callsCombo->currentData().toInt() : -1;
}

} // namespace msip
