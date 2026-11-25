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

#include "MainWindow.h"
#include "AccountDialog.h"
#include "SettingsDialog.h"
#include "ShortcutsDialog.h"
#include "TransferDialog.h"
#include "Theme.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QScreen>
#include <QShortcut>
#include <QStatusBar>
#include <QVBoxLayout>

namespace msip {

MainWindow::MainWindow(Settings* settings, Database* db, SipEngine* engine, QWidget* parent)
    : QMainWindow(parent)
    , m_settings(settings)
    , m_db(db)
    , m_engine(engine) {
    setWindowTitle("MicroSIP");
    setWindowIcon(QIcon(":/icons/microsip.svg"));
    resize(440, 620);

    m_shortcuts = m_settings->loadShortcuts();

    m_rootWidget = new QWidget(this);
    auto* outerLayout = new QVBoxLayout(m_rootWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // 1. Top Header Bar (Account switcher + Edit Account + Configure + Quick Menu)
    setupHeaderBar();
    outerLayout->addWidget(m_headerBar);

    // 2. Friendly onboarding banner when no SIP account is configured yet
    m_noAccountBanner = new QFrame(m_rootWidget);
    m_noAccountBanner->setStyleSheet("QFrame { background-color: #1e3a5f; border-bottom: 1px solid #2b5285; }");
    auto* bannerLayout = new QHBoxLayout(m_noAccountBanner);
    bannerLayout->setContentsMargins(10, 6, 10, 6);
    auto* bannerLbl = new QLabel(tr("Nenhuma conta SIP configurada."), m_noAccountBanner);
    bannerLbl->setStyleSheet("color: #e2ecf9; background: transparent; font-weight: 500;");
    auto* bannerBtn = new QPushButton(tr("+ Configurar Conta"), m_noAccountBanner);
    bannerBtn->setObjectName("primaryBtn");
    connect(bannerBtn, &QPushButton::clicked, this, &MainWindow::onOpenAccountAdd);
    bannerLayout->addWidget(bannerLbl, 1);
    bannerLayout->addWidget(bannerBtn);
    outerLayout->addWidget(m_noAccountBanner);

    // 3. Main Content (Tabs + Optional Shortcuts/BLF panel)
    m_contentContainer = new QWidget(m_rootWidget);
    m_mainLayout = new QBoxLayout(
        m_settings->shortcutsBottom ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight,
        m_contentContainer
    );
    m_mainLayout->setContentsMargins(0, 4, 0, 0);
    m_mainLayout->setSpacing(4);

    m_tabs = new QTabWidget(m_contentContainer);
    m_dialer = new DialerWidget(m_engine, m_settings, this);
    m_calls = new CallsWidget(m_db, this);
    m_contacts = new ContactsWidget(m_db, this);
    m_messages = new MessagesWidget(m_engine, m_db, this);

    m_tabs->addTab(m_dialer, tr("Discador"));
    m_tabs->addTab(m_calls, tr("Chamadas"));
    m_tabs->addTab(m_contacts, tr("Contatos"));
    m_tabs->addTab(m_messages, tr("Mensagens"));

    m_mainLayout->addWidget(m_tabs, 1);

    setupShortcutsPanel();
    outerLayout->addWidget(m_contentContainer, 1);

    setCentralWidget(m_rootWidget);

    setupQuickMenu();
    setupTray();

    // 4. Status Bar with LED + DND / AA / FWD pills
    m_statusDot = new QLabel("●", this);
    m_statusDot->setStyleSheet("color: #9e9e9e; font-size: 14px; padding-left: 4px; background: transparent;");
    m_statusText = new QLabel(tr("Offline"), this);
    m_statusText->setStyleSheet("background: transparent;");

    m_dndBtn = new QPushButton(tr("DND"), this);
    m_dndBtn->setObjectName("statusPill");
    m_dndBtn->setCheckable(true);
    m_dndBtn->setChecked(m_settings->dnd);
    m_dndBtn->setToolTip(tr("Não Perturbe (rejeita chamadas com 486 Ocupado)"));
    connect(m_dndBtn, &QPushButton::toggled, this, &MainWindow::onToggleDnd);

    m_aaBtn = new QPushButton(tr("AA"), this);
    m_aaBtn->setObjectName("statusPill");
    m_aaBtn->setCheckable(true);
    m_aaBtn->setChecked(m_settings->aa);
    m_aaBtn->setToolTip(tr("Atendimento Automático"));
    connect(m_aaBtn, &QPushButton::toggled, this, &MainWindow::onToggleAa);

    m_fwdBtn = new QPushButton(tr("FWD"), this);
    m_fwdBtn->setObjectName("statusPill");
    m_fwdBtn->setCheckable(true);
    m_fwdBtn->setChecked(m_settings->fwd);
    m_fwdBtn->setToolTip(tr("Encaminhamento de Chamadas"));
    connect(m_fwdBtn, &QPushButton::toggled, this, &MainWindow::onToggleFwd);

    statusBar()->addWidget(m_statusDot);
    statusBar()->addWidget(m_statusText, 1);
    statusBar()->addPermanentWidget(m_dndBtn);
    statusBar()->addPermanentWidget(m_aaBtn);
    statusBar()->addPermanentWidget(m_fwdBtn);

    // Wire cross-widget signals
    connect(m_dialer, &DialerWidget::openTransferRequested, this, &MainWindow::onOpenTransfer);
    connect(m_dialer, &DialerWidget::openMessagesRequested, this, [this](const QString& num) {
        m_messages->openConversation(num);
        m_tabs->setCurrentWidget(m_messages);
    });

    connect(m_calls, &CallsWidget::callNumberRequested, this, [this](const QString& num) {
        m_dialer->setNumber(num);
        m_tabs->setCurrentWidget(m_dialer);
        m_engine->makeCall(num);
    });
    connect(m_calls, &CallsWidget::messageNumberRequested, this, [this](const QString& num) {
        m_messages->openConversation(num);
        m_tabs->setCurrentWidget(m_messages);
    });
    connect(m_calls, &CallsWidget::addContactRequested, this, [this](const QString& num, const QString& name) {
        m_tabs->setCurrentWidget(m_contacts);
        m_contacts->openAddContactDialog(num, name);
    });

    connect(m_contacts, &ContactsWidget::callNumberRequested, this, [this](const QString& num) {
        m_dialer->setNumber(num);
        m_tabs->setCurrentWidget(m_dialer);
        m_engine->makeCall(num);
    });
    connect(m_contacts, &ContactsWidget::messageNumberRequested, this, [this](const QString& num) {
        m_messages->openConversation(num);
        m_tabs->setCurrentWidget(m_messages);
    });
    connect(m_contacts, &ContactsWidget::contactsModified, this, &MainWindow::refreshPresenceSubscriptions);

    // Wire SipEngine signals
    connect(m_engine, &SipEngine::registrationStateChanged, this, &MainWindow::onRegistrationChanged);
    connect(m_engine, &SipEngine::incomingCall, this, &MainWindow::onIncomingCall);
    connect(m_engine, &SipEngine::callStateChanged, this, &MainWindow::onCallStateChanged);
    connect(m_engine, &SipEngine::callEnded, this, &MainWindow::onCallEnded);
    connect(m_engine, &SipEngine::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_engine, &SipEngine::buddyStateChanged, this, &MainWindow::onBuddyStateChanged);
    connect(m_engine, &SipEngine::logMessage, this, &MainWindow::onLogMessage);

    applyTheme();
    applyAlwaysOnTop();
    rebuildAccountControls();
    rebuildShortcutsPanel();

    if (!m_settings->windowGeometry.isEmpty()) {
        restoreGeometry(m_settings->windowGeometry);
    }
    if (m_settings->activeTab >= 0 && m_settings->activeTab < m_tabs->count()) {
        m_tabs->setCurrentIndex(m_settings->activeTab);
    }

    // Keyboard shortcuts
    connect(new QShortcut(QKeySequence("Ctrl+,"), this), &QShortcut::activated, this, &MainWindow::onOpenSettings);
    connect(new QShortcut(QKeySequence("Ctrl+Q"), this), &QShortcut::activated, this, [this]() {
        m_reallyQuit = true;
        close();
    });
    for (int i = 0; i < 4; ++i) {
        connect(new QShortcut(QKeySequence(QString("Ctrl+%1").arg(i + 1)), this), &QShortcut::activated, this, [this, i]() {
            m_tabs->setCurrentIndex(i);
        });
    }

    onToggleDnd(m_settings->dnd);
    onToggleAa(m_settings->aa);
    if (m_settings->fwd && !m_settings->forwardingNumber.isEmpty()) {
        onToggleFwd(true);
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupHeaderBar() {
    m_headerBar = new QFrame(m_rootWidget);
    m_headerBar->setObjectName("headerBar");
    auto* row = new QHBoxLayout(m_headerBar);
    row->setContentsMargins(8, 6, 8, 6);
    row->setSpacing(6);

    m_accountCombo = new QComboBox(m_headerBar);
    m_accountCombo->setToolTip(tr("Selecionar conta SIP ativa"));
    m_accountCombo->setMinimumHeight(30);
    connect(m_accountCombo, QOverload<int>::of(&QComboBox::activated), this, &MainWindow::onAccountComboChanged);

    m_editAccBtn = new QPushButton(tr("✎ Conta"), m_headerBar);
    m_editAccBtn->setToolTip(tr("Editar conta SIP selecionada (ou adicionar nova)"));
    m_editAccBtn->setMinimumHeight(30);
    connect(m_editAccBtn, &QPushButton::clicked, this, &MainWindow::onEditCurrentAccount);

    m_configBtn = new QPushButton(tr("⚙ Config"), m_headerBar);
    m_configBtn->setToolTip(tr("Abrir Configurações (Áudio, Rede, Gravação, Interface)"));
    m_configBtn->setMinimumHeight(30);
    connect(m_configBtn, &QPushButton::clicked, this, &MainWindow::onOpenSettings);

    m_menuBtn = new QPushButton("☰", m_headerBar);
    m_menuBtn->setToolTip(tr("Menu MicroSIP"));
    m_menuBtn->setFixedSize(32, 30);

    row->addWidget(m_accountCombo, 1);
    row->addWidget(m_editAccBtn);
    row->addWidget(m_configBtn);
    row->addWidget(m_menuBtn);
}

void MainWindow::setupQuickMenu() {
    m_quickMenu = new QMenu(this);
    m_quickMenu->addAction(tr("+ Adicionar Conta SIP..."), this, &MainWindow::onOpenAccountAdd);
    m_quickMenu->addAction(tr("✎ Editar Conta Atual..."), this, &MainWindow::onEditCurrentAccount);

    m_accountsSubMenu = m_quickMenu->addMenu(tr("Contas Salvas"));
    m_quickMenu->addSeparator();

    m_quickMenu->addAction(tr("⚙ Configurações..."), this, &MainWindow::onOpenSettings);
    m_quickMenu->addAction(tr("★ Atalhos / BLF..."), this, &MainWindow::onOpenShortcuts);
    m_quickMenu->addSeparator();

    m_themeAct = m_quickMenu->addAction(tr("Alternar Tema Claro / Escuro"), this, &MainWindow::onToggleTheme);

    m_alwaysOnTopAct = m_quickMenu->addAction(tr("Sempre no Topo"));
    m_alwaysOnTopAct->setCheckable(true);
    m_alwaysOnTopAct->setChecked(m_settings->alwaysOnTop);
    connect(m_alwaysOnTopAct, &QAction::toggled, this, &MainWindow::onToggleAlwaysOnTop);

    m_quickMenu->addSeparator();
    m_quickMenu->addAction(tr("Ver Log SIP..."), this, &MainWindow::onOpenLogViewer);
    m_quickMenu->addAction(tr("↻ Reconectar SIP"), this, [this]() { m_engine->registerActiveAccount(); });
    m_quickMenu->addAction(tr("Sobre / Licença..."), this, &MainWindow::onOpenAbout);
    m_quickMenu->addSeparator();
    m_quickMenu->addAction(tr("Sair"), this, [this]() {
        m_reallyQuit = true;
        close();
    });

    m_menuBtn->setMenu(m_quickMenu);
}

void MainWindow::rebuildAccountControls() {
    auto accounts = m_settings->loadAllAccounts();

    // Update top QComboBox
    m_accountCombo->blockSignals(true);
    m_accountCombo->clear();

    int activeIdx = -1;
    for (int i = 0; i < accounts.size(); ++i) {
        const auto& acc = accounts[i];
        m_accountCombo->addItem(acc.effectiveLabel(), acc.id);
        if (acc.id == m_settings->accountId) {
            activeIdx = i;
        }
    }
    m_accountCombo->addItem(tr("+ Adicionar nova conta..."), -1);

    if (activeIdx >= 0) {
        m_accountCombo->setCurrentIndex(activeIdx);
    } else if (!accounts.isEmpty()) {
        m_accountCombo->setCurrentIndex(0);
    } else {
        m_accountCombo->setCurrentIndex(0);
    }
    m_accountCombo->blockSignals(false);

    m_noAccountBanner->setVisible(accounts.isEmpty());
    m_editAccBtn->setText(accounts.isEmpty() ? tr("+ Conta") : tr("✎ Conta"));

    // Update Accounts submenu
    if (m_accountsSubMenu) {
        m_accountsSubMenu->clear();
        m_accountsSubMenu->addAction(tr("+ Adicionar Conta..."), this, &MainWindow::onOpenAccountAdd);
        if (!accounts.isEmpty()) {
            m_accountsSubMenu->addSeparator();
            for (const auto& acc : accounts) {
                bool active = (acc.id == m_settings->accountId);
                QString label = active ? ("✓ " + acc.effectiveLabel()) : acc.effectiveLabel();
                auto* sub = m_accountsSubMenu->addMenu(label);
                sub->addAction(tr("Ativar"), this, [this, id = acc.id]() { onSelectAccount(id); });
                sub->addAction(tr("Editar..."), this, [this, id = acc.id]() { onOpenAccountEdit(id); });
            }
        }
    }
}

void MainWindow::onAccountComboChanged(int index) {
    if (index < 0) return;
    int id = m_accountCombo->itemData(index).toInt();
    if (id == -1) {
        onOpenAccountAdd();
        rebuildAccountControls();
    } else if (id > 0 && id != m_settings->accountId) {
        onSelectAccount(id);
    }
}

void MainWindow::onEditCurrentAccount() {
    if (m_settings->accountId > 0) {
        onOpenAccountEdit(m_settings->accountId);
    } else {
        auto accounts = m_settings->loadAllAccounts();
        if (!accounts.isEmpty()) {
            onOpenAccountEdit(accounts.first().id);
        } else {
            onOpenAccountAdd();
        }
    }
}

void MainWindow::setupTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) return;

    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/microsip.svg"), this);
    m_trayIcon->setToolTip("MicroSIP");

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(tr("Mostrar / Ocultar"), this, [this]() {
        if (isVisible()) hide();
        else { showNormal(); raise(); activateWindow(); }
    });
    m_trayMenu->addSeparator();

    m_trayDndAct = m_trayMenu->addAction(tr("Não Perturbe (DND)"));
    m_trayDndAct->setCheckable(true);
    m_trayDndAct->setChecked(m_settings->dnd);
    connect(m_trayDndAct, &QAction::toggled, m_dndBtn, &QPushButton::setChecked);

    m_trayAaAct = m_trayMenu->addAction(tr("Atendimento Automático (AA)"));
    m_trayAaAct->setCheckable(true);
    m_trayAaAct->setChecked(m_settings->aa);
    connect(m_trayAaAct, &QAction::toggled, m_aaBtn, &QPushButton::setChecked);

    m_trayMenu->addSeparator();
    m_trayMenu->addAction(tr("Configurações..."), this, &MainWindow::onOpenSettings);
    m_trayMenu->addAction(tr("Sair"), this, [this]() {
        m_reallyQuit = true;
        close();
    });

    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    m_trayIcon->show();
}

void MainWindow::setupShortcutsPanel() {
    m_shortcutsScroll = new QScrollArea(m_contentContainer);
    m_shortcutsScroll->setWidgetResizable(true);
    m_shortcutsContainer = new QWidget(m_shortcutsScroll);
    m_shortcutsScroll->setWidget(m_shortcutsContainer);
    m_mainLayout->addWidget(m_shortcutsScroll, 0);
}

void MainWindow::rebuildShortcutsPanel() {
    m_mainLayout->setDirection(m_settings->shortcutsBottom ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);

    bool show = m_settings->enableShortcuts && !m_shortcuts.isEmpty();
    m_shortcutsScroll->setVisible(show);
    if (!show) return;

    if (m_settings->shortcutsBottom) {
        m_shortcutsScroll->setMaximumWidth(QWIDGETSIZE_MAX);
        m_shortcutsScroll->setMaximumHeight(130);
    } else {
        m_shortcutsScroll->setMaximumHeight(QWIDGETSIZE_MAX);
        m_shortcutsScroll->setMaximumWidth(170);
    }

    delete m_shortcutsContainer;
    m_shortcutsContainer = new QWidget(m_shortcutsScroll);
    m_shortcutButtons.clear();

    auto* grid = new QGridLayout(m_shortcutsContainer);
    grid->setContentsMargins(6, 4, 6, 4);
    grid->setSpacing(5);

    int cols = m_settings->shortcutsBottom ? 3 : 1;
    for (int i = 0; i < m_shortcuts.size(); ++i) {
        const auto& sc = m_shortcuts[i];
        auto* btn = new QPushButton(sc.label, m_shortcutsContainer);
        btn->setMinimumHeight(32);
        btn->setToolTip(QString("%1 (%2)").arg(sc.number, sc.type));
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (i < m_shortcuts.size()) executeShortcut(m_shortcuts[i]);
        });
        grid->addWidget(btn, i / cols, i % cols);
        m_shortcutButtons.insert(i, btn);
    }
    grid->setRowStretch(m_shortcuts.size() / cols + 1, 1);

    m_shortcutsScroll->setWidget(m_shortcutsContainer);
}

void MainWindow::executeShortcut(const Shortcut& sc) {
    if (sc.ringing && m_settings->enableFeatureCodeCP) {
        m_engine->callPickup(sc.number);
        return;
    }
    int cid = m_engine->primaryCallId();
    if (sc.type == "message") {
        m_messages->openConversation(sc.number);
        m_tabs->setCurrentWidget(m_messages);
    } else if (sc.type == "dtmf") {
        if (cid >= 0) m_engine->sendDtmf(cid, sc.number);
    } else if (sc.type == "transfer") {
        if (cid >= 0) m_engine->blindTransfer(cid, sc.number);
    } else if (sc.type == "callOrBT") {
        if (cid >= 0) m_engine->blindTransfer(cid, sc.number);
        else m_engine->makeCall(sc.number);
    } else if (sc.type == "attendedTransfer" || sc.type == "callOrAT") {
        if (cid >= 0) {
            m_engine->toggleHold(cid);
            m_engine->makeCall(sc.number);
        } else {
            m_engine->makeCall(sc.number);
        }
    } else if (sc.type == "conference") {
        m_engine->makeCall(sc.number);
        m_engine->joinConference();
    } else {
        m_dialer->setNumber(sc.number);
        m_engine->makeCall(sc.number);
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    m_settings->windowGeometry = saveGeometry();
    m_settings->activeTab = m_tabs->currentIndex();
    if (!m_reallyQuit && m_trayIcon && m_trayIcon->isVisible()) {
        m_settings->save();
        hide();
        event->ignore();
        return;
    }
    const auto dialogs = m_ringDialogs.values();
    m_ringDialogs.clear();
    for (auto* dlg : dialogs) {
        if (dlg) {
            dlg->markDismissedByEngine();
            dlg->close();
        }
    }
    m_settings->save();
    m_engine->shutdown();
    event->accept();
}

void MainWindow::handleIpcCommand(const QString& command) {
    QString cmd = command.trimmed();
    if (cmd.isEmpty() || cmd == "--show") {
        showNormal();
        raise();
        activateWindow();
    } else if (cmd == "--answer") {
        m_engine->answerFirstIncomingCall();
    } else if (cmd == "--hangupall") {
        m_engine->hangupAll();
    } else if (cmd == "--exit") {
        m_reallyQuit = true;
        close();
    } else {
        handleUriArgument(cmd);
    }
}

void MainWindow::handleUriArgument(const QString& uriArg) {
    QString target = uriArg.trimmed();
    if (target.startsWith("tel:", Qt::CaseInsensitive)) target = target.mid(4);
    else if (target.startsWith("callto:", Qt::CaseInsensitive)) target = target.mid(7);
    if (!target.isEmpty()) {
        m_dialer->setNumber(target);
        m_tabs->setCurrentWidget(m_dialer);
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::onRegistrationChanged(bool registered, int code, const QString& reason) {
    Q_UNUSED(code);
    Account acc = m_settings->activeAccount();
    QString accLabel = acc.isEmpty() ? tr("Sem conta") : acc.effectiveLabel();

    if (registered) {
        m_statusDot->setStyleSheet("color: #22c55e; font-size: 14px; padding-left: 4px; background: transparent;");
        m_statusText->setText(QString("%1 — %2").arg(accLabel, tr("Online")));
        refreshPresenceSubscriptions();
    } else {
        m_statusDot->setStyleSheet("color: #ef4444; font-size: 14px; padding-left: 4px; background: transparent;");
        m_statusText->setText(QString("%1 — %2").arg(accLabel, reason));
    }
    if (m_trayIcon) {
        m_trayIcon->setToolTip(QString("MicroSIP (%1)").arg(m_statusText->text()));
    }
    m_dialer->refreshActiveCalls();
}

void MainWindow::onIncomingCall(const ActiveCallInfo& call) {
    if (m_settings->bringToFrontOnIncoming) {
        showNormal();
        raise();
        activateWindow();
    }

    if (m_trayIcon) {
        QString caller = call.remoteName.isEmpty() ? call.number : QString("%1 (%2)").arg(call.remoteName, call.number);
        m_trayIcon->showMessage(tr("Chamada Recebida"), caller, QSystemTrayIcon::Information, 8000);
    }

    if (auto* existing = m_ringDialogs.value(call.callId, nullptr)) {
        existing->show();
        existing->raise();
        existing->activateWindow();
        QApplication::alert(existing);
        return;
    }

    QString accLabel = m_settings->activeAccount().effectiveLabel();
    auto* dlg = new IncomingCallDialog(call, accLabel, nullptr);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    // Position centered on screen, cascading if multiple calls are ringing simultaneously
    if (auto* screen = QGuiApplication::primaryScreen()) {
        QRect avail = screen->availableGeometry();
        QSize sz = dlg->sizeHint();
        int offset = m_ringDialogs.size() * 28;
        int x = avail.x() + (avail.width() - sz.width()) / 2 + offset;
        int y = avail.y() + (avail.height() - sz.height()) / 2 + offset;
        x = qBound(avail.left(), x, qMax(avail.left(), avail.right() - sz.width()));
        y = qBound(avail.top(), y, qMax(avail.top(), avail.bottom() - sz.height()));
        dlg->move(x, y);
    }

    m_ringDialogs.insert(call.callId, dlg);

    connect(dlg, &IncomingCallDialog::answerClicked, m_engine, &SipEngine::answerCall);
    connect(dlg, &IncomingCallDialog::rejectClicked, this, [this](int cid) { m_engine->rejectCall(cid, 486); });
    connect(dlg, &IncomingCallDialog::forwardClicked, m_engine, &SipEngine::forwardCall);
    connect(dlg, &IncomingCallDialog::ignoreClicked, this, [this](int) { m_engine->stopRingtone(); });
    connect(dlg, &QObject::destroyed, this, [this, cid = call.callId, dlg]() {
        if (m_ringDialogs.value(cid, nullptr) == dlg) {
            m_ringDialogs.remove(cid);
        }
    });

    dlg->show();
    dlg->raise();
    dlg->activateWindow();
    QApplication::alert(dlg);
}

void MainWindow::onCallStateChanged(const ActiveCallInfo& call) {
    if (!SipEngine::isIncomingRinging(call) && m_ringDialogs.contains(call.callId)) {
        if (auto* dlg = m_ringDialogs.take(call.callId)) {
            dlg->markDismissedByEngine();
            dlg->close();
        }
    }
}

void MainWindow::onCallEnded(int callId, const CallRecord& record) {
    if (m_ringDialogs.contains(callId)) {
        if (auto* dlg = m_ringDialogs.take(callId)) {
            dlg->markDismissedByEngine();
            dlg->close();
        }
    }
    m_calls->reload();
    if (record.type == CallDirection::Missed && m_trayIcon) {
        QString who = record.name.isEmpty() ? record.number : record.name;
        m_trayIcon->showMessage(tr("Chamada Perdida"), who, QSystemTrayIcon::Warning, 5000);
    }
}

void MainWindow::onMessageReceived(const SipMessage& msg) {
    m_messages->onIncomingMessage(msg);
    if (m_trayIcon) {
        m_trayIcon->showMessage(tr("Mensagem de %1").arg(msg.peerName), msg.body, QSystemTrayIcon::Information, 5000);
    }
}

void MainWindow::onBuddyStateChanged(const QString& number, PresenceStatus status, bool ringing, const QString& statusText) {
    m_contacts->updatePresence(number, status, ringing, statusText);

    for (int i = 0; i < m_shortcuts.size(); ++i) {
        if (m_shortcuts[i].number == number && m_shortcutButtons.contains(i)) {
            m_shortcuts[i].presenceStatus = status;
            m_shortcuts[i].ringing = ringing;
            auto* btn = m_shortcutButtons[i];

            QString prefix = "○ ";
            QString style;
            switch (status) {
                case PresenceStatus::Online:     prefix = "🟢 "; break;
                case PresenceStatus::Away:       prefix = "🟡 "; break;
                case PresenceStatus::Busy:
                case PresenceStatus::OnThePhone: prefix = "🔴 "; style = "font-weight: bold;"; break;
                case PresenceStatus::Ringing:    prefix = "🟠 "; style = "background-color: #ea580c; color: white; font-weight: bold;"; break;
                case PresenceStatus::Offline:    prefix = "⚫ "; break;
                default:                         prefix = "○ "; break;
            }
            btn->setText(prefix + m_shortcuts[i].label);
            btn->setStyleSheet(style);
        }
    }
}

void MainWindow::onLogMessage(const QString& msg) {
    QString line = QString("[%1] %2").arg(QDateTime::currentDateTime().toString("HH:mm:ss"), msg);
    m_logBuffer.append(line);
    if (m_logBuffer.size() > 500) {
        m_logBuffer.removeFirst();
    }
}

void MainWindow::onOpenLogViewer() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Log SIP"));
    dlg.resize(560, 380);
    auto* layout = new QVBoxLayout(&dlg);
    auto* text = new QPlainTextEdit(&dlg);
    text->setReadOnly(true);
    text->setPlainText(m_logBuffer.join('\n'));
    layout->addWidget(text, 1);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btns);
    dlg.exec();
}

void MainWindow::onOpenAbout() {
    QMessageBox::about(
        this,
        tr("Sobre o MicroSIP (Linux / NixOS)"),
        tr("<h3>MicroSIP 3.23.5 — Native Linux / NixOS Port</h3>"
           "<p>Softphone SIP nativo baseado em <b>Qt 6</b> e <b>PJSIP</b>.</p>"
           "<p><b>Projeto Original:</b> <a href='https://www.microsip.org/'>MicroSIP (www.microsip.org)</a><br/>"
           "Copyright &copy; 2011-2026 MicroSIP (<a href='https://www.microsip.org/'>www.microsip.org</a>)<br/>"
           "Port Nativo Linux / NixOS (Qt 6): Copyright &copy; 2026 Gabriel Aguiar Rocha (RAGton) &lt;aguiarrocha37@outlook.com&gt;</p>"
           "<p><b>Licença:</b> <a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>GNU General Public License v2.0 (GPL-2.0-or-later)</a></p>"
           "<p style='font-size:11px; color:#8b95a5;'>Este é um port comunitário não-oficial para Linux/NixOS derivado do código-fonte aberto do MicroSIP. "
           "Todos os direitos sobre o MicroSIP original pertencem à MicroSIP.</p>")
    );
}

void MainWindow::onOpenAccountAdd() {
    Account acc;
    AccountDialog dlg(acc, this);
    if (dlg.exec() == QDialog::Accepted) {
        int id = m_settings->saveAccount(dlg.resultAccount());
        m_settings->accountId = id;
        m_settings->save();
        rebuildAccountControls();
        m_engine->registerActiveAccount();
    }
}

void MainWindow::onOpenAccountEdit(int accountId) {
    Account acc;
    if (!m_settings->loadAccount(accountId, acc)) return;

    AccountDialog dlg(acc, this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.deleteRequested()) {
            m_settings->deleteAccount(accountId);
        } else {
            m_settings->saveAccount(dlg.resultAccount());
        }
        rebuildAccountControls();
        m_engine->registerActiveAccount();
    }
}

void MainWindow::onSelectAccount(int accountId) {
    m_settings->accountId = accountId;
    m_settings->save();
    rebuildAccountControls();
    m_engine->registerActiveAccount();
}

void MainWindow::onOpenSettings() {
    SettingsDialog dlg(m_settings, m_engine, this);
    if (dlg.exec() == QDialog::Accepted) {
        applyTheme();
        m_engine->applyAudioDevices();
        m_engine->applyAudioCodecs();
        rebuildShortcutsPanel();
    }
}

void MainWindow::onOpenShortcuts() {
    ShortcutsDialog dlg(m_shortcuts, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_shortcuts = dlg.resultShortcuts();
        m_settings->saveShortcuts(m_shortcuts);
        rebuildShortcutsPanel();
        refreshPresenceSubscriptions();
    }
}

void MainWindow::onOpenTransfer(int callId) {
    TransferDialog dlg(callId, m_engine->activeCalls(), this);
    if (dlg.exec() == QDialog::Accepted) {
        if (dlg.isAttended()) {
            m_engine->attendedTransfer(callId, dlg.targetCallId());
        } else {
            m_engine->blindTransfer(callId, dlg.targetNumber());
        }
    }
}

void MainWindow::onToggleAlwaysOnTop(bool checked) {
    m_settings->alwaysOnTop = checked;
    m_settings->save();
    applyAlwaysOnTop();
}

void MainWindow::onToggleTheme() {
    m_settings->darkTheme = !m_settings->darkTheme;
    m_settings->save();
    applyTheme();
}

void MainWindow::applyTheme() {
    qApp->setStyleSheet(buildAppStyleSheet(m_settings->darkTheme));
}

void MainWindow::applyAlwaysOnTop() {
    bool wasVisible = isVisible();
    setWindowFlag(Qt::WindowStaysOnTopHint, m_settings->alwaysOnTop);
    if (wasVisible) show();
}

void MainWindow::onToggleDnd(bool checked) {
    m_settings->dnd = checked;
    m_settings->save();
    if (m_trayDndAct && m_trayDndAct->isChecked() != checked) {
        m_trayDndAct->setChecked(checked);
    }
    m_dndBtn->setStyleSheet(checked ? "QPushButton#statusPill { background-color: #dc2626; color: white; border-color: #ef4444; }" : "");
}

void MainWindow::onToggleAa(bool checked) {
    m_settings->aa = checked;
    m_settings->save();
    if (m_trayAaAct && m_trayAaAct->isChecked() != checked) {
        m_trayAaAct->setChecked(checked);
    }
    m_aaBtn->setStyleSheet(checked ? "QPushButton#statusPill { background-color: #16a34a; color: white; border-color: #22c55e; }" : "");
}

void MainWindow::onToggleFwd(bool checked) {
    if (checked && m_settings->forwardingNumber.isEmpty()) {
        QMessageBox::information(this, tr("Encaminhamento"), tr("Configure um número de encaminhamento em Configurações primeiro."));
        m_fwdBtn->setChecked(false);
        return;
    }
    m_settings->fwd = checked;
    m_settings->save();
    m_fwdBtn->setStyleSheet(checked ? "QPushButton#statusPill { background-color: #ea580c; color: white; border-color: #f97316; }" : "");
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (isVisible()) hide();
        else { showNormal(); raise(); activateWindow(); }
    }
}

void MainWindow::refreshPresenceSubscriptions() {
    m_engine->refreshSubscriptions(m_contacts->contacts(), m_shortcuts);
}

} // namespace msip
