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

#include "core/Types.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>

namespace msip {

class AccountDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountDialog(const Account& account, QWidget* parent = nullptr);

    Account resultAccount() const;
    bool deleteRequested() const { return m_deleteRequested; }

private:
    Account m_original;
    bool m_deleteRequested = false;

    QLineEdit* m_labelEdit = nullptr;
    QLineEdit* m_serverEdit = nullptr;
    QLineEdit* m_proxyEdit = nullptr;
    QLineEdit* m_usernameEdit = nullptr;
    QLineEdit* m_domainEdit = nullptr;
    QLineEdit* m_authIdEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_displayNameEdit = nullptr;
    QLineEdit* m_voicemailEdit = nullptr;
    QLineEdit* m_dialPrefixEdit = nullptr;
    QComboBox* m_srtpCombo = nullptr;
    QComboBox* m_transportCombo = nullptr;
    QLineEdit* m_publicAddrEdit = nullptr;
    QSpinBox* m_regRefreshSpin = nullptr;
    QSpinBox* m_keepAliveSpin = nullptr;
    QCheckBox* m_publishCheck = nullptr;
    QCheckBox* m_iceCheck = nullptr;
    QCheckBox* m_allowRewriteCheck = nullptr;
    QCheckBox* m_disableSessionTimerCheck = nullptr;
};

} // namespace msip
