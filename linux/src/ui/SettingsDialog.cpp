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

#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace msip {

SettingsDialog::SettingsDialog(Settings* settings, SipEngine* engine, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_engine(engine) {
    setWindowTitle(tr("Settings"));
    setMinimumSize(460, 440);

    auto* root = new QVBoxLayout(this);
    auto* tabs = new QTabWidget(this);

    // --- Tab 1: Audio ---
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);

        m_ringDevCombo = new QComboBox(page);
        m_outDevCombo = new QComboBox(page);
        m_inDevCombo = new QComboBox(page);
        m_ringDevCombo->addItem(tr("Default"), "");
        m_outDevCombo->addItem(tr("Default"), "");
        m_inDevCombo->addItem(tr("Default"), "");

        for (const auto& d : m_engine->audioDevices()) {
            if (d.outputCount > 0) {
                m_ringDevCombo->addItem(d.name, d.name);
                m_outDevCombo->addItem(d.name, d.name);
            }
            if (d.inputCount > 0) {
                m_inDevCombo->addItem(d.name, d.name);
            }
        }
        auto selectCombo = [](QComboBox* cb, const QString& val) {
            int idx = cb->findData(val);
            if (idx >= 0) cb->setCurrentIndex(idx);
        };
        selectCombo(m_ringDevCombo, m_settings->audioRingDevice);
        selectCombo(m_outDevCombo, m_settings->audioOutputDevice);
        selectCombo(m_inDevCombo, m_settings->audioInputDevice);

        auto* ringRow = new QHBoxLayout();
        m_ringtoneEdit = new QLineEdit(m_settings->ringtone, page);
        m_ringtoneEdit->setPlaceholderText(tr("Built-in tone (or choose .wav)"));
        auto* ringBrowse = new QPushButton(tr("..."), page);
        ringBrowse->setMaximumWidth(36);
        connect(ringBrowse, &QPushButton::clicked, this, [this]() {
            QString f = QFileDialog::getOpenFileName(this, tr("Select Ringtone"), QString(), tr("WAV Audio (*.wav)"));
            if (!f.isEmpty()) m_ringtoneEdit->setText(f);
        });
        ringRow->addWidget(m_ringtoneEdit, 1);
        ringRow->addWidget(ringBrowse);

        m_codecsList = new QListWidget(page);
        m_codecsList->setMaximumHeight(130);
        m_codecsList->setDragDropMode(QAbstractItemView::InternalMove);
        QStringList enabled = m_settings->audioCodecs.split(' ', Qt::SkipEmptyParts);
        QStringList avail = m_engine->availableAudioCodecs();
        for (const QString& c : enabled) {
            if (avail.contains(c)) {
                auto* item = new QListWidgetItem(c, m_codecsList);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Checked);
            }
        }
        for (const QString& c : avail) {
            if (!enabled.contains(c)) {
                auto* item = new QListWidgetItem(c, m_codecsList);
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(Qt::Unchecked);
            }
        }

        m_vadCheck = new QCheckBox(tr("Voice Activity Detection (VAD)"), page);
        m_vadCheck->setChecked(m_settings->vad);
        m_ecCheck = new QCheckBox(tr("Echo Cancellation (EC)"), page);
        m_ecCheck->setChecked(m_settings->ec);

        auto* recRow = new QHBoxLayout();
        m_recPathEdit = new QLineEdit(m_settings->recordingPath, page);
        auto* recBrowse = new QPushButton(tr("..."), page);
        recBrowse->setMaximumWidth(36);
        connect(recBrowse, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, tr("Recording Folder"), m_recPathEdit->text());
            if (!dir.isEmpty()) m_recPathEdit->setText(dir);
        });
        recRow->addWidget(m_recPathEdit, 1);
        recRow->addWidget(recBrowse);

        m_autoRecCheck = new QCheckBox(tr("Auto-Record all calls (WAV)"), page);
        m_autoRecCheck->setChecked(m_settings->autoRecording);

        form->addRow(tr("Ring Device:"), m_ringDevCombo);
        form->addRow(tr("Speaker:"), m_outDevCombo);
        form->addRow(tr("Microphone:"), m_inDevCombo);
        form->addRow(tr("Ringtone:"), ringRow);
        form->addRow(tr("Audio Codecs:"), m_codecsList);
        form->addRow("", m_vadCheck);
        form->addRow("", m_ecCheck);
        form->addRow(tr("Recordings Path:"), recRow);
        form->addRow("", m_autoRecCheck);

        tabs->addTab(page, tr("Audio & Recording"));
    }

    // --- Tab 2: Network & SIP ---
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);

        m_sourcePortSpin = new QSpinBox(page);
        m_sourcePortSpin->setRange(0, 65535);
        m_sourcePortSpin->setValue(m_settings->sourcePort);
        m_sourcePortSpin->setSpecialValueText(tr("Random (0)"));

        m_enableStunCheck = new QCheckBox(tr("Enable STUN Server"), page);
        m_enableStunCheck->setChecked(m_settings->enableStun);
        m_stunEdit = new QLineEdit(m_settings->stun, page);
        m_stunEdit->setPlaceholderText("stun.l.google.com:19302");

        m_dtmfCombo = new QComboBox(page);
        m_dtmfCombo->addItem(tr("Auto"), static_cast<int>(DtmfMethod::Auto));
        m_dtmfCombo->addItem(tr("RFC 2833"), static_cast<int>(DtmfMethod::Rfc2833));
        m_dtmfCombo->addItem(tr("In-Band"), static_cast<int>(DtmfMethod::InBand));
        m_dtmfCombo->addItem(tr("SIP INFO"), static_cast<int>(DtmfMethod::SipInfo));
        int dIdx = m_dtmfCombo->findData(static_cast<int>(m_settings->dtmfMethod));
        if (dIdx >= 0) m_dtmfCombo->setCurrentIndex(dIdx);

        m_rportCheck = new QCheckBox(tr("Enable rport"), page);
        m_rportCheck->setChecked(m_settings->rport);

        m_userAgentEdit = new QLineEdit(m_settings->userAgent, page);

        form->addRow(tr("Source Port:"), m_sourcePortSpin);
        form->addRow("", m_enableStunCheck);
        form->addRow(tr("STUN Server:"), m_stunEdit);
        form->addRow(tr("DTMF Method:"), m_dtmfCombo);
        form->addRow("", m_rportCheck);
        form->addRow(tr("User-Agent:"), m_userAgentEdit);

        tabs->addTab(page, tr("Network & SIP"));
    }

    // --- Tab 3: Calls & UI ---
    {
        auto* page = new QWidget(this);
        auto* form = new QFormLayout(page);

        m_aaModeCombo = new QComboBox(page);
        m_aaModeCombo->addItem(tr("Button / Manual"), "button");
        m_aaModeCombo->addItem(tr("Always Auto-Answer"), "all");
        m_aaModeCombo->addItem(tr("Disabled"), "");
        int aaIdx = m_aaModeCombo->findData(m_settings->autoAnswer);
        if (aaIdx >= 0) m_aaModeCombo->setCurrentIndex(aaIdx);

        m_aaDelaySpin = new QSpinBox(page);
        m_aaDelaySpin->setRange(0, 120);
        m_aaDelaySpin->setValue(m_settings->autoAnswerDelay);
        m_aaDelaySpin->setSuffix(" s");

        m_aaNumberEdit = new QLineEdit(m_settings->autoAnswerNumber, page);
        m_aaNumberEdit->setPlaceholderText(tr("Optional caller filter"));

        m_fwdNumberEdit = new QLineEdit(m_settings->forwardingNumber, page);
        m_fwdNumberEdit->setPlaceholderText(tr("Target extension or SIP URI"));

        m_fwdDelaySpin = new QSpinBox(page);
        m_fwdDelaySpin->setRange(0, 120);
        m_fwdDelaySpin->setValue(m_settings->forwardingDelay);
        m_fwdDelaySpin->setSuffix(" s");

        m_autoHangupSpin = new QSpinBox(page);
        m_autoHangupSpin->setRange(0, 86400);
        m_autoHangupSpin->setValue(m_settings->autoHangUpTime);
        m_autoHangupSpin->setSuffix(" s (0 = off)");

        m_callWaitingCheck = new QCheckBox(tr("Call Waiting"), page);
        m_callWaitingCheck->setChecked(m_settings->callWaiting);

        m_singleModeCheck = new QCheckBox(tr("Single Call Mode"), page);
        m_singleModeCheck->setChecked(m_settings->singleMode);

        m_darkThemeCheck = new QCheckBox(tr("Usar Tema Escuro"), page);
        m_darkThemeCheck->setChecked(m_settings->darkTheme);

        m_enableShortcutsCheck = new QCheckBox(tr("Mostrar Painel de Atalhos / BLF"), page);
        m_enableShortcutsCheck->setChecked(m_settings->enableShortcuts);

        m_shortcutsBottomCheck = new QCheckBox(tr("Posicionar Painel de Atalhos na Base"), page);
        m_shortcutsBottomCheck->setChecked(m_settings->shortcutsBottom);

        m_bringToFrontCheck = new QCheckBox(tr("Trazer Janela para Frente ao Receber Chamada"), page);
        m_bringToFrontCheck->setChecked(m_settings->bringToFrontOnIncoming);

        m_enableLogCheck = new QCheckBox(tr("Salvar Log do PJSIP em Arquivo"), page);
        m_enableLogCheck->setChecked(m_settings->enableLog);

        form->addRow(tr("Atendimento Automático:"), m_aaModeCombo);
        form->addRow(tr("Atraso do AA:"), m_aaDelaySpin);
        form->addRow(tr("Filtro de Número (AA):"), m_aaNumberEdit);
        form->addRow(tr("Encaminhar para:"), m_fwdNumberEdit);
        form->addRow(tr("Atraso Encaminhamento:"), m_fwdDelaySpin);
        form->addRow(tr("Desligar Automaticamente:"), m_autoHangupSpin);
        form->addRow("", m_callWaitingCheck);
        form->addRow("", m_singleModeCheck);
        form->addRow("", m_darkThemeCheck);
        form->addRow("", m_enableShortcutsCheck);
        form->addRow("", m_shortcutsBottomCheck);
        form->addRow("", m_bringToFrontCheck);
        form->addRow("", m_enableLogCheck);

        tabs->addTab(page, tr("Chamadas & Interface"));
    }

    root->addWidget(tabs);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        applyToSettings();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void SettingsDialog::applyToSettings() {
    m_settings->audioRingDevice = m_ringDevCombo->currentData().toString();
    m_settings->audioOutputDevice = m_outDevCombo->currentData().toString();
    m_settings->audioInputDevice = m_inDevCombo->currentData().toString();
    m_settings->ringtone = m_ringtoneEdit->text().trimmed();

    QStringList selectedCodecs;
    for (int i = 0; i < m_codecsList->count(); ++i) {
        auto* item = m_codecsList->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedCodecs.append(item->text());
        }
    }
    if (!selectedCodecs.isEmpty()) {
        m_settings->audioCodecs = selectedCodecs.join(' ');
    }

    m_settings->vad = m_vadCheck->isChecked();
    m_settings->ec = m_ecCheck->isChecked();
    m_settings->recordingPath = m_recPathEdit->text().trimmed();
    m_settings->autoRecording = m_autoRecCheck->isChecked();

    m_settings->sourcePort = m_sourcePortSpin->value();
    m_settings->enableStun = m_enableStunCheck->isChecked();
    m_settings->stun = m_stunEdit->text().trimmed();
    m_settings->dtmfMethod = static_cast<DtmfMethod>(m_dtmfCombo->currentData().toInt());
    m_settings->rport = m_rportCheck->isChecked();
    m_settings->userAgent = m_userAgentEdit->text().trimmed();

    m_settings->autoAnswer = m_aaModeCombo->currentData().toString();
    m_settings->autoAnswerDelay = m_aaDelaySpin->value();
    m_settings->autoAnswerNumber = m_aaNumberEdit->text().trimmed();
    m_settings->forwardingNumber = m_fwdNumberEdit->text().trimmed();
    m_settings->forwardingDelay = m_fwdDelaySpin->value();
    m_settings->autoHangUpTime = m_autoHangupSpin->value();
    m_settings->callWaiting = m_callWaitingCheck->isChecked();
    m_settings->singleMode = m_singleModeCheck->isChecked();

    m_settings->darkTheme = m_darkThemeCheck->isChecked();
    m_settings->enableShortcuts = m_enableShortcutsCheck->isChecked();
    m_settings->shortcutsBottom = m_shortcutsBottomCheck->isChecked();
    m_settings->bringToFrontOnIncoming = m_bringToFrontCheck->isChecked();
    m_settings->enableLog = m_enableLogCheck->isChecked();

    m_settings->save();
}

} // namespace msip
