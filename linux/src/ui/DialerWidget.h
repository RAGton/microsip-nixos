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
#include "core/SipEngine.h"
#include "core/Settings.h"

#include <QWidget>
#include <QFrame>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QProgressBar>
#include <QComboBox>

namespace msip {

class DialerWidget : public QWidget {
    Q_OBJECT
public:
    explicit DialerWidget(SipEngine* engine, Settings* settings, QWidget* parent = nullptr);

    void setNumber(const QString& number);
    QString currentNumber() const;
    void refreshActiveCalls();

signals:
    void openTransferRequested(int callId);
    void openMessagesRequested(const QString& number);

private slots:
    void onCallOrHangupClicked();
    void onKeypadPressed(const QString& digit);
    void onHoldClicked();
    void onMuteClicked();
    void onRecordClicked();
    void onTransferClicked();
    void onConferenceClicked();
    void onRedialClicked();
    void onMessageClicked();
    void onAudioLevels(float tx, float rx);

private:
    int selectedCallId() const;

    SipEngine* m_engine = nullptr;
    Settings* m_settings = nullptr;

    QLineEdit* m_numberEdit = nullptr;
    QPushButton* m_backspaceBtn = nullptr;

    QFrame* m_callCard = nullptr;
    QComboBox* m_activeCallCombo = nullptr;
    QLabel* m_callTitleLabel = nullptr;
    QLabel* m_callMetaLabel = nullptr;

    QPushButton* m_callBtn = nullptr;
    QPushButton* m_hangupBtn = nullptr;
    QPushButton* m_redialBtn = nullptr;
    QPushButton* m_msgBtn = nullptr;

    QPushButton* m_holdBtn = nullptr;
    QPushButton* m_muteBtn = nullptr;
    QPushButton* m_transferBtn = nullptr;
    QPushButton* m_confBtn = nullptr;
    QPushButton* m_recordBtn = nullptr;

    QPushButton* m_spkMuteBtn = nullptr;
    QPushButton* m_micMuteBtn = nullptr;
    QSlider* m_spkSlider = nullptr;
    QSlider* m_micSlider = nullptr;
    QProgressBar* m_rxLevelBar = nullptr;
    QProgressBar* m_txLevelBar = nullptr;
};

} // namespace msip
