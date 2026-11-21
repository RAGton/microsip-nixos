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

#include "core/Database.h"
#include "core/Settings.h"
#include "core/SipEngine.h"

#include "DialerWidget.h"
#include "CallsWidget.h"
#include "ContactsWidget.h"
#include "MessagesWidget.h"
#include "IncomingCallDialog.h"

#include <QMainWindow>
#include <QBoxLayout>
#include <QComboBox>
#include <QFrame>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QHash>
#include <QScrollArea>
#include <QStringList>

namespace msip {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Settings* settings, Database* db, SipEngine* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

    void handleUriArgument(const QString& uriArg);
    void handleIpcCommand(const QString& command);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onRegistrationChanged(bool registered, int code, const QString& reason);
    void onIncomingCall(const msip::ActiveCallInfo& call);
    void onCallStateChanged(const msip::ActiveCallInfo& call);
    void onCallEnded(int callId, const msip::CallRecord& record);
    void onMessageReceived(const msip::SipMessage& msg);
    void onBuddyStateChanged(const QString& number, msip::PresenceStatus status, bool ringing, const QString& statusText);
    void onLogMessage(const QString& msg);

    void onOpenAccountAdd();
    void onOpenAccountEdit(int accountId);
    void onEditCurrentAccount();
    void onSelectAccount(int accountId);
    void onAccountComboChanged(int index);
    void onOpenSettings();
    void onOpenShortcuts();
    void onOpenLogViewer();
    void onOpenAbout();
    void onOpenTransfer(int callId);
    void onToggleAlwaysOnTop(bool checked);
    void onToggleTheme();
    void onToggleDnd(bool checked);
    void onToggleAa(bool checked);
    void onToggleFwd(bool checked);

    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void rebuildAccountControls();
    void rebuildShortcutsPanel();
    void executeShortcut(const Shortcut& sc);

private:
    void setupHeaderBar();
    void setupQuickMenu();
    void setupTray();
    void setupShortcutsPanel();
    void refreshPresenceSubscriptions();
    void applyAlwaysOnTop();
    void applyTheme();

    Settings* m_settings = nullptr;
    Database* m_db = nullptr;
    SipEngine* m_engine = nullptr;

    QWidget* m_rootWidget = nullptr;
    QFrame* m_headerBar = nullptr;
    QComboBox* m_accountCombo = nullptr;
    QPushButton* m_editAccBtn = nullptr;
    QPushButton* m_configBtn = nullptr;
    QPushButton* m_menuBtn = nullptr;
    QMenu* m_quickMenu = nullptr;
    QMenu* m_accountsSubMenu = nullptr;
    QAction* m_alwaysOnTopAct = nullptr;
    QAction* m_themeAct = nullptr;

    QFrame* m_noAccountBanner = nullptr;

    QWidget* m_contentContainer = nullptr;
    QBoxLayout* m_mainLayout = nullptr;
    QTabWidget* m_tabs = nullptr;

    DialerWidget* m_dialer = nullptr;
    CallsWidget* m_calls = nullptr;
    ContactsWidget* m_contacts = nullptr;
    MessagesWidget* m_messages = nullptr;

    QScrollArea* m_shortcutsScroll = nullptr;
    QWidget* m_shortcutsContainer = nullptr;
    QVector<Shortcut> m_shortcuts;
    QHash<int, QPushButton*> m_shortcutButtons;

    QLabel* m_statusDot = nullptr;
    QLabel* m_statusText = nullptr;
    QPushButton* m_dndBtn = nullptr;
    QPushButton* m_aaBtn = nullptr;
    QPushButton* m_fwdBtn = nullptr;

    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
    QAction* m_trayDndAct = nullptr;
    QAction* m_trayAaAct = nullptr;

    QHash<int, IncomingCallDialog*> m_ringDialogs;
    QStringList m_logBuffer;
    bool m_reallyQuit = false;
};

} // namespace msip
