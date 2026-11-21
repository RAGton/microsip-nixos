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

#include "core/Database.h"
#include "core/Settings.h"
#include "core/SingleInstance.h"
#include "core/SipEngine.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("microsip");
    app.setApplicationDisplayName("MicroSIP");
    app.setApplicationVersion("3.23.5-linux");
    app.setOrganizationName("MicroSIP");
    app.setWindowIcon(QIcon(":/icons/microsip.svg"));
    app.setQuitOnLastWindowClosed(false);

    qRegisterMetaType<msip::Account>();
    qRegisterMetaType<msip::CallRecord>();
    qRegisterMetaType<msip::Contact>();
    qRegisterMetaType<msip::Shortcut>();
    qRegisterMetaType<msip::SipMessage>();
    qRegisterMetaType<msip::ActiveCallInfo>();
    qRegisterMetaType<msip::PresenceStatus>();

    QCommandLineParser parser;
    parser.setApplicationDescription("MicroSIP Native Linux/NixOS SIP Softphone (Qt 6 + PJSIP)");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption minimizedOpt({"m", "minimized"}, "Iniciar minimizado na bandeja do sistema");
    QCommandLineOption answerOpt("answer", "Atender chamada recebida na instância em execução");
    QCommandLineOption hangupAllOpt("hangupall", "Encerrar todas as chamadas na instância em execução");
    QCommandLineOption exitOpt("exit", "Fechar a instância em execução");

    parser.addOption(minimizedOpt);
    parser.addOption(answerOpt);
    parser.addOption(hangupAllOpt);
    parser.addOption(exitOpt);
    parser.addPositionalArgument("uri", "URI opcional sip:/tel:/callto: para discar");
    parser.process(app);

    auto posArgs = parser.positionalArguments();
    QString ipcCmd = "--show";
    if (parser.isSet(exitOpt)) ipcCmd = "--exit";
    else if (parser.isSet(answerOpt)) ipcCmd = "--answer";
    else if (parser.isSet(hangupAllOpt)) ipcCmd = "--hangupall";
    else if (!posArgs.isEmpty()) ipcCmd = posArgs.first();

    // Enforce single running instance via local UNIX socket
    msip::SingleInstance instance("microsip-qt-ipc");
    if (instance.sendToRunningInstance(ipcCmd)) {
        return 0;
    }
    if (parser.isSet(exitOpt) || parser.isSet(answerOpt) || parser.isSet(hangupAllOpt)) {
        // Control command without a running instance
        return 0;
    }
    instance.startServer();

    msip::Settings settings;
    settings.load();

    msip::Database db(settings.dbFile);
    msip::SipEngine engine(&settings, &db);

    msip::MainWindow window(&settings, &db, &engine);
    QObject::connect(&instance, &msip::SingleInstance::messageReceived, &window, &msip::MainWindow::handleIpcCommand);

    engine.initialize();

    if (!parser.isSet(minimizedOpt) && !settings.startMinimized) {
        window.show();
    }

    if (!posArgs.isEmpty()) {
        window.handleUriArgument(posArgs.first());
    }

    return app.exec();
}
