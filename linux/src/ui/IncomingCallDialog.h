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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

namespace msip {

class IncomingCallDialog : public QDialog {
    Q_OBJECT
public:
    explicit IncomingCallDialog(const ActiveCallInfo& call,
                                const QString& accountLabel = QString(),
                                QWidget* parent = nullptr);

    int callId() const { return m_callId; }
    void markDismissedByEngine();

signals:
    void answerClicked(int callId);
    void rejectClicked(int callId);
    void forwardClicked(int callId, const QString& dest);
    void ignoreClicked(int callId);

protected:
    void closeEvent(QCloseEvent* event) override;
    void reject() override;

private:
    void disableActionButtons();

    int m_callId = -1;
    bool m_actionTaken = false;
    int m_elapsedSecs = 0;

    QTimer* m_ringTimer = nullptr;
    QLabel* m_timerLbl = nullptr;
    QPushButton* m_answerBtn = nullptr;
    QPushButton* m_rejectBtn = nullptr;
    QPushButton* m_forwardBtn = nullptr;
    QPushButton* m_ignoreBtn = nullptr;
    QLineEdit* m_forwardEdit = nullptr;
};

} // namespace msip
