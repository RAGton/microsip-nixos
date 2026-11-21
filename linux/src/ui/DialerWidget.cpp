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

#include "DialerWidget.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStyle>

namespace msip {

DialerWidget::DialerWidget(SipEngine* engine, Settings* settings, QWidget* parent)
    : QWidget(parent)
    , m_engine(engine)
    , m_settings(settings) {
    auto* root = new QVBoxLayout(this);
    root->setSpacing(10);
    root->setContentsMargins(10, 10, 10, 10);

    // Number input + Backspace button
    auto* numRow = new QHBoxLayout();
    numRow->setSpacing(6);
    m_numberEdit = new QLineEdit(this);
    m_numberEdit->setPlaceholderText(tr("Digite o número ou URI SIP..."));
    m_numberEdit->setClearButtonEnabled(true);
    m_numberEdit->setMinimumHeight(38);
    QFont numFont = m_numberEdit->font();
    numFont.setPointSize(13);
    numFont.setBold(true);
    m_numberEdit->setFont(numFont);
    connect(m_numberEdit, &QLineEdit::returnPressed, this, &DialerWidget::onCallOrHangupClicked);

    m_backspaceBtn = new QPushButton("⌫", this);
    m_backspaceBtn->setToolTip(tr("Apagar último dígito"));
    m_backspaceBtn->setFixedSize(40, 38);
    connect(m_backspaceBtn, &QPushButton::clicked, this, [this]() {
        m_numberEdit->backspace();
        m_numberEdit->setFocus();
    });

    numRow->addWidget(m_numberEdit, 1);
    numRow->addWidget(m_backspaceBtn);
    root->addLayout(numRow);

    // Active Call Card
    m_callCard = new QFrame(this);
    m_callCard->setObjectName("activeCallCard");
    m_callCard->setProperty("inCall", false);
    auto* cardLayout = new QVBoxLayout(m_callCard);
    cardLayout->setContentsMargins(10, 8, 10, 8);
    cardLayout->setSpacing(4);

    m_activeCallCombo = new QComboBox(m_callCard);
    m_activeCallCombo->setVisible(false);
    connect(m_activeCallCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DialerWidget::refreshActiveCalls);
    cardLayout->addWidget(m_activeCallCombo);

    m_callTitleLabel = new QLabel(tr("Pronto para discar"), m_callCard);
    m_callTitleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = m_callTitleLabel->font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    m_callTitleLabel->setFont(titleFont);

    m_callMetaLabel = new QLabel(tr("Nenhuma chamada ativa"), m_callCard);
    m_callMetaLabel->setAlignment(Qt::AlignCenter);
    m_callMetaLabel->setStyleSheet("color: #8b95a5; font-size: 11px;");

    cardLayout->addWidget(m_callTitleLabel);
    cardLayout->addWidget(m_callMetaLabel);
    root->addWidget(m_callCard);

    // DTMF Keypad 3x4
    auto* keypadGrid = new QGridLayout();
    keypadGrid->setSpacing(6);
    struct KeyDef { const char* digit; const char* sub; };
    const KeyDef keys[12] = {
        {"1", ""},    {"2", "ABC"}, {"3", "DEF"},
        {"4", "GHI"}, {"5", "JKL"}, {"6", "MNO"},
        {"7", "PQRS"},{"8", "TUV"}, {"9", "WXYZ"},
        {"*", ""},    {"0", "+"},   {"#", ""}
    };

    for (int i = 0; i < 12; ++i) {
        QString label = keys[i].sub[0] ? QString("%1\n%2").arg(keys[i].digit, keys[i].sub) : QString(keys[i].digit);
        auto* btn = new QPushButton(label, this);
        btn->setObjectName("keypadBtn");
        btn->setMinimumHeight(46);
        QString d = keys[i].digit;
        connect(btn, &QPushButton::clicked, this, [this, d]() { onKeypadPressed(d); });
        keypadGrid->addWidget(btn, i / 3, i % 3);
    }
    root->addLayout(keypadGrid);

    // Primary Action Buttons
    auto* actionRow = new QHBoxLayout();
    actionRow->setSpacing(6);

    m_callBtn = new QPushButton(tr("📞 Ligar"), this);
    m_callBtn->setObjectName("callBtn");
    m_callBtn->setMinimumHeight(40);
    connect(m_callBtn, &QPushButton::clicked, this, &DialerWidget::onCallOrHangupClicked);

    m_hangupBtn = new QPushButton(tr("✕ Desligar"), this);
    m_hangupBtn->setObjectName("hangupBtn");
    m_hangupBtn->setMinimumHeight(40);
    m_hangupBtn->setEnabled(false);
    connect(m_hangupBtn, &QPushButton::clicked, this, [this]() {
        int id = selectedCallId();
        if (id >= 0) m_engine->hangupCall(id);
        else m_engine->hangupAll();
    });

    m_redialBtn = new QPushButton(tr("↻"), this);
    m_redialBtn->setToolTip(tr("Rediscar último número"));
    m_redialBtn->setFixedSize(42, 40);
    connect(m_redialBtn, &QPushButton::clicked, this, &DialerWidget::onRedialClicked);

    m_msgBtn = new QPushButton(tr("✉"), this);
    m_msgBtn->setToolTip(tr("Enviar mensagem SIP"));
    m_msgBtn->setFixedSize(42, 40);
    connect(m_msgBtn, &QPushButton::clicked, this, &DialerWidget::onMessageClicked);

    actionRow->addWidget(m_callBtn, 3);
    actionRow->addWidget(m_hangupBtn, 3);
    actionRow->addWidget(m_redialBtn);
    actionRow->addWidget(m_msgBtn);
    root->addLayout(actionRow);

    // In-call controls bar
    auto* callCtrlRow = new QHBoxLayout();
    callCtrlRow->setSpacing(4);

    m_holdBtn = new QPushButton(tr("Hold"), this);
    m_holdBtn->setToolTip(tr("Colocar chamada em espera"));
    m_holdBtn->setCheckable(true);
    m_holdBtn->setEnabled(false);
    connect(m_holdBtn, &QPushButton::clicked, this, &DialerWidget::onHoldClicked);

    m_muteBtn = new QPushButton(tr("Mute"), this);
    m_muteBtn->setToolTip(tr("Silenciar microfone nesta chamada"));
    m_muteBtn->setCheckable(true);
    m_muteBtn->setEnabled(false);
    connect(m_muteBtn, &QPushButton::clicked, this, &DialerWidget::onMuteClicked);

    m_transferBtn = new QPushButton(tr("Transf."), this);
    m_transferBtn->setToolTip(tr("Transferência cega ou assistida"));
    m_transferBtn->setEnabled(false);
    connect(m_transferBtn, &QPushButton::clicked, this, &DialerWidget::onTransferClicked);

    m_confBtn = new QPushButton(tr("Conf."), this);
    m_confBtn->setToolTip(tr("Unir chamadas em conferência"));
    m_confBtn->setEnabled(false);
    connect(m_confBtn, &QPushButton::clicked, this, &DialerWidget::onConferenceClicked);

    m_recordBtn = new QPushButton(tr("● Rec"), this);
    m_recordBtn->setToolTip(tr("Gravar chamada em WAV"));
    m_recordBtn->setCheckable(true);
    m_recordBtn->setEnabled(false);
    connect(m_recordBtn, &QPushButton::clicked, this, &DialerWidget::onRecordClicked);

    callCtrlRow->addWidget(m_holdBtn);
    callCtrlRow->addWidget(m_muteBtn);
    callCtrlRow->addWidget(m_transferBtn);
    callCtrlRow->addWidget(m_confBtn);
    callCtrlRow->addWidget(m_recordBtn);
    root->addLayout(callCtrlRow);

    // Audio volume & VU meters
    auto* audioBox = new QGridLayout();
    audioBox->setSpacing(4);

    m_spkMuteBtn = new QPushButton("🔊", this);
    m_spkMuteBtn->setCheckable(true);
    m_spkMuteBtn->setFixedSize(30, 26);
    m_spkMuteBtn->setToolTip(tr("Silenciar alto-falante"));
    connect(m_spkMuteBtn, &QPushButton::toggled, this, [this](bool muted) {
        m_engine->setGlobalMuteOutput(muted);
        m_spkMuteBtn->setText(muted ? "🔇" : "🔊");
    });

    m_spkSlider = new QSlider(Qt::Horizontal, this);
    m_spkSlider->setRange(0, 200);
    m_spkSlider->setValue(m_settings->volumeOutput);
    m_spkSlider->setToolTip(tr("Volume do alto-falante"));
    connect(m_spkSlider, &QSlider::valueChanged, this, [this](int val) {
        m_engine->setOutputVolume(val);
    });

    m_rxLevelBar = new QProgressBar(this);
    m_rxLevelBar->setRange(0, 100);
    m_rxLevelBar->setTextVisible(false);
    m_rxLevelBar->setFixedHeight(5);

    m_micMuteBtn = new QPushButton("🎤", this);
    m_micMuteBtn->setCheckable(true);
    m_micMuteBtn->setFixedSize(30, 26);
    m_micMuteBtn->setToolTip(tr("Silenciar microfone"));
    connect(m_micMuteBtn, &QPushButton::toggled, this, [this](bool muted) {
        m_engine->setGlobalMuteInput(muted);
        m_micMuteBtn->setText(muted ? "🚫" : "🎤");
    });

    m_micSlider = new QSlider(Qt::Horizontal, this);
    m_micSlider->setRange(0, 200);
    m_micSlider->setValue(m_settings->volumeInput);
    m_micSlider->setToolTip(tr("Ganho do microfone"));
    connect(m_micSlider, &QSlider::valueChanged, this, [this](int val) {
        m_engine->setInputVolume(val);
    });

    m_txLevelBar = new QProgressBar(this);
    m_txLevelBar->setRange(0, 100);
    m_txLevelBar->setTextVisible(false);
    m_txLevelBar->setFixedHeight(5);

    audioBox->addWidget(m_spkMuteBtn, 0, 0);
    audioBox->addWidget(m_spkSlider, 0, 1);
    audioBox->addWidget(m_rxLevelBar, 1, 1);
    audioBox->addWidget(m_micMuteBtn, 2, 0);
    audioBox->addWidget(m_micSlider, 2, 1);
    audioBox->addWidget(m_txLevelBar, 3, 1);
    root->addLayout(audioBox);

    root->addStretch();

    connect(m_engine, &SipEngine::activeCallsChanged, this, &DialerWidget::refreshActiveCalls);
    connect(m_engine, &SipEngine::audioLevelsUpdated, this, &DialerWidget::onAudioLevels);
}

void DialerWidget::setNumber(const QString& number) {
    m_numberEdit->setText(number);
}

QString DialerWidget::currentNumber() const {
    return m_numberEdit->text().trimmed();
}

int DialerWidget::selectedCallId() const {
    if (m_activeCallCombo->isVisible() && m_activeCallCombo->currentIndex() >= 0) {
        return m_activeCallCombo->currentData().toInt();
    }
    return m_engine->primaryCallId();
}

void DialerWidget::refreshActiveCalls() {
    auto calls = m_engine->activeCalls();
    int prevSelected = m_activeCallCombo->currentData().isValid() ? m_activeCallCombo->currentData().toInt() : -1;

    m_activeCallCombo->blockSignals(true);
    m_activeCallCombo->clear();
    int restoreIdx = 0;
    for (int i = 0; i < calls.size(); ++i) {
        const auto& c = calls[i];
        QString label = c.remoteName.isEmpty() ? c.number : QString("%1 (%2)").arg(c.remoteName, c.number);
        QString dur = QString("%1:%2").arg(c.duration / 60, 2, 10, QChar('0')).arg(c.duration % 60, 2, 10, QChar('0'));
        m_activeCallCombo->addItem(QString("[%1] %2 — %3").arg(dur, label, c.stateText), c.callId);
        if (c.callId == prevSelected) restoreIdx = i;
    }
    if (!calls.isEmpty()) {
        m_activeCallCombo->setCurrentIndex(restoreIdx);
    }
    m_activeCallCombo->setVisible(calls.size() > 1);
    m_activeCallCombo->blockSignals(false);

    bool hasCalls = !calls.isEmpty();
    m_callCard->setProperty("inCall", hasCalls);
    m_callCard->style()->unpolish(m_callCard);
    m_callCard->style()->polish(m_callCard);

    m_hangupBtn->setEnabled(hasCalls);
    m_holdBtn->setEnabled(hasCalls);
    m_muteBtn->setEnabled(hasCalls);
    m_transferBtn->setEnabled(hasCalls);
    m_confBtn->setEnabled(calls.size() > 1);
    m_recordBtn->setEnabled(hasCalls);

    if (!hasCalls) {
        m_callTitleLabel->setText(m_engine->isRegistered() ? tr("Pronto para discar") : m_engine->registrationStatusText());
        m_callMetaLabel->setText(m_engine->isRegistered() ? tr("Conta SIP registrada") : tr("Configure ou selecione uma conta no topo"));
        m_holdBtn->setChecked(false);
        m_muteBtn->setChecked(false);
        m_recordBtn->setChecked(false);
        m_recordBtn->setStyleSheet("");
        return;
    }

    int selId = selectedCallId();
    for (const auto& c : calls) {
        if (c.callId == selId) {
            QString who = c.remoteName.isEmpty() ? c.number : QString("%1 (%2)").arg(c.remoteName, c.number);
            QString dur = QString("%1:%2").arg(c.duration / 60, 2, 10, QChar('0')).arg(c.duration % 60, 2, 10, QChar('0'));
            QStringList badges;
            if (c.onHold) badges << "HOLD";
            if (c.muted) badges << "MUTE";
            if (c.recording) badges << "REC";
            if (c.inConference) badges << "CONF";
            if (c.srtpActive) badges << "🔒 SRTP";
            QString flagStr = badges.isEmpty() ? "" : (" • " + badges.join(" | "));

            m_callTitleLabel->setText(who);
            m_callMetaLabel->setText(QString("%1  •  %2%3").arg(c.stateText, dur, flagStr));

            m_holdBtn->setChecked(c.onHold);
            m_muteBtn->setChecked(c.muted);
            m_recordBtn->setChecked(c.recording);
            m_recordBtn->setStyleSheet(c.recording ? "QPushButton { background-color: #dc2626; color: white; border: none; font-weight: bold; }" : "");
            break;
        }
    }
}

void DialerWidget::onCallOrHangupClicked() {
    QString num = currentNumber();
    if (!num.isEmpty()) {
        m_engine->makeCall(num);
    } else if (!m_settings->lastCallNumber.isEmpty()) {
        m_numberEdit->setText(m_settings->lastCallNumber);
    }
}

void DialerWidget::onKeypadPressed(const QString& digit) {
    if (!digit.isEmpty()) {
        m_engine->playLocalDtmfDigit(digit.at(0).toLatin1());
    }
    int cid = selectedCallId();
    if (cid >= 0) {
        m_engine->sendDtmf(cid, digit);
    }
    m_numberEdit->insert(digit);
}

void DialerWidget::onHoldClicked() {
    int cid = selectedCallId();
    if (cid >= 0) m_engine->toggleHold(cid);
}

void DialerWidget::onMuteClicked() {
    int cid = selectedCallId();
    if (cid >= 0) m_engine->toggleMute(cid);
}

void DialerWidget::onRecordClicked() {
    int cid = selectedCallId();
    if (cid < 0) return;
    if (m_engine->isRecording(cid)) {
        m_engine->stopRecording(cid);
    } else {
        m_engine->startRecording(cid);
    }
}

void DialerWidget::onTransferClicked() {
    int cid = selectedCallId();
    if (cid >= 0) emit openTransferRequested(cid);
}

void DialerWidget::onConferenceClicked() {
    m_engine->joinConference();
}

void DialerWidget::onRedialClicked() {
    if (!m_settings->lastCallNumber.isEmpty()) {
        m_numberEdit->setText(m_settings->lastCallNumber);
        m_engine->makeCall(m_settings->lastCallNumber);
    }
}

void DialerWidget::onMessageClicked() {
    QString num = currentNumber();
    if (!num.isEmpty()) {
        emit openMessagesRequested(num);
    }
}

void DialerWidget::onAudioLevels(float tx, float rx) {
    m_txLevelBar->setValue(qBound(0, static_cast<int>(tx * 100), 100));
    m_rxLevelBar->setValue(qBound(0, static_cast<int>(rx * 100), 100));
}

} // namespace msip
