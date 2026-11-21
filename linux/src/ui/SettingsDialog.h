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

#include "core/Settings.h"
#include "core/SipEngine.h"

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>

namespace msip {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Settings* settings, SipEngine* engine, QWidget* parent = nullptr);

    void applyToSettings();

private:
    void populateAudioCombos(bool showAdvanced);

    Settings* m_settings = nullptr;
    SipEngine* m_engine = nullptr;

    // Audio
    QComboBox* m_ringDevCombo = nullptr;
    QComboBox* m_outDevCombo = nullptr;
    QComboBox* m_inDevCombo = nullptr;
    QCheckBox* m_showAdvAudioCheck = nullptr;
    QLineEdit* m_ringtoneEdit = nullptr;
    QListWidget* m_codecsList = nullptr;
    QCheckBox* m_vadCheck = nullptr;
    QCheckBox* m_ecCheck = nullptr;

    // Recording
    QLineEdit* m_recPathEdit = nullptr;
    QCheckBox* m_autoRecCheck = nullptr;

    // Network & SIP
    QSpinBox* m_sourcePortSpin = nullptr;
    QLineEdit* m_stunEdit = nullptr;
    QCheckBox* m_enableStunCheck = nullptr;
    QComboBox* m_dtmfCombo = nullptr;
    QCheckBox* m_rportCheck = nullptr;
    QLineEdit* m_userAgentEdit = nullptr;

    // Call Handling (AA / FWD / Feature Codes)
    QComboBox* m_aaModeCombo = nullptr;
    QSpinBox* m_aaDelaySpin = nullptr;
    QLineEdit* m_aaNumberEdit = nullptr;
    QLineEdit* m_fwdNumberEdit = nullptr;
    QSpinBox* m_fwdDelaySpin = nullptr;
    QSpinBox* m_autoHangupSpin = nullptr;
    QCheckBox* m_callWaitingCheck = nullptr;
    QCheckBox* m_singleModeCheck = nullptr;

    // UI
    QCheckBox* m_darkThemeCheck = nullptr;
    QCheckBox* m_enableShortcutsCheck = nullptr;
    QCheckBox* m_shortcutsBottomCheck = nullptr;
    QCheckBox* m_bringToFrontCheck = nullptr;
    QCheckBox* m_enableLogCheck = nullptr;
};

} // namespace msip
