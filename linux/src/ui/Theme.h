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

#include <QString>

namespace msip {

inline QString buildAppStyleSheet(bool dark) {
    if (dark) {
        return R"(
            QMainWindow, QDialog, QWidget {
                background-color: #1b1e23;
                color: #e6eaf0;
                font-family: "Inter", "Noto Sans", "Segoe UI", sans-serif;
                font-size: 13px;
            }
            QFrame#headerBar {
                background-color: #22262e;
                border-bottom: 1px solid #303642;
            }
            QFrame#activeCallCard {
                background-color: #222938;
                border: 1px solid #2e4a73;
                border-radius: 10px;
            }
            QFrame#activeCallCard[inCall="true"] {
                background-color: #1b3124;
                border: 1px solid #2e7d32;
            }
            QTabWidget::pane {
                border: none;
                background-color: #1b1e23;
            }
            QTabBar::tab {
                background-color: #22262e;
                color: #9aa5b4;
                padding: 8px 14px;
                margin-right: 2px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                font-weight: 600;
            }
            QTabBar::tab:selected {
                background-color: #1b1e23;
                color: #ffffff;
                border-bottom: 2px solid #3b82f6;
            }
            QTabBar::tab:hover:!selected {
                background-color: #2a2f3a;
                color: #d6dde8;
            }
            QLineEdit, QComboBox, QSpinBox, QTextBrowser, QPlainTextEdit {
                background-color: #252a33;
                color: #f0f4f8;
                border: 1px solid #363d4a;
                border-radius: 6px;
                padding: 6px 8px;
                selection-background-color: #3b82f6;
            }
            QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
                border: 1px solid #3b82f6;
            }
            QComboBox::drop-down {
                border: none;
                width: 22px;
            }
            QComboBox QAbstractItemView {
                background-color: #252a33;
                color: #f0f4f8;
                border: 1px solid #3b82f6;
                border-radius: 6px;
                padding: 4px;
                outline: 0;
                selection-background-color: #2563eb;
                selection-color: #ffffff;
            }
            QComboBox QAbstractItemView::item {
                min-height: 26px;
                padding: 4px 8px;
                border-radius: 4px;
            }
            QScrollBar:vertical {
                background: #1e222a;
                width: 8px;
                margin: 2px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #475569;
                min-height: 24px;
                border-radius: 4px;
            }
            QScrollBar::handle:vertical:hover {
                background: #64748b;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
            }
            QPushButton {
                background-color: #2b313c;
                color: #e6eaf0;
                border: 1px solid #3a4250;
                border-radius: 6px;
                padding: 6px 10px;
                font-weight: 500;
            }
            QPushButton:hover {
                background-color: #343b48;
                border-color: #4b5568;
            }
            QPushButton:pressed {
                background-color: #232832;
            }
            QPushButton:checked {
                background-color: #2563eb;
                border-color: #3b82f6;
                color: #ffffff;
            }
            QPushButton:disabled {
                background-color: #21252d;
                color: #646e7d;
                border-color: #2a2f38;
            }
            QPushButton#keypadBtn {
                background-color: #252a33;
                border: 1px solid #323946;
                border-radius: 8px;
                font-size: 15px;
                font-weight: 600;
                padding: 4px;
            }
            QPushButton#keypadBtn:hover {
                background-color: #303744;
                border-color: #4a5568;
            }
            QPushButton#keypadBtn:pressed {
                background-color: #3b82f6;
                color: #ffffff;
            }
            QPushButton#callBtn {
                background-color: #16a34a;
                border: none;
                color: #ffffff;
                font-weight: 700;
                font-size: 14px;
                border-radius: 8px;
            }
            QPushButton#callBtn:hover {
                background-color: #22c55e;
            }
            QPushButton#hangupBtn {
                background-color: #dc2626;
                border: none;
                color: #ffffff;
                font-weight: 700;
                font-size: 14px;
                border-radius: 8px;
            }
            QPushButton#hangupBtn:hover:!disabled {
                background-color: #ef4444;
            }
            QPushButton#hangupBtn:disabled {
                background-color: #2a2e37;
                color: #636b78;
            }
            QPushButton#primaryBtn {
                background-color: #2563eb;
                border: none;
                color: #ffffff;
                font-weight: 600;
            }
            QPushButton#primaryBtn:hover {
                background-color: #3b82f6;
            }
            QPushButton#statusPill {
                background-color: #262b35;
                border: 1px solid #363d4a;
                border-radius: 10px;
                padding: 2px 9px;
                font-size: 11px;
                font-weight: 700;
                color: #a6b0bf;
            }
            QPushButton#statusPill:hover {
                background-color: #323846;
            }
            QTableWidget, QListWidget {
                background-color: #20242c;
                alternate-background-color: #252a33;
                color: #e6eaf0;
                border: 1px solid #303642;
                border-radius: 6px;
                gridline-color: #2c323d;
            }
            QTableWidget::item:selected, QListWidget::item:selected {
                background-color: #2563eb;
                color: #ffffff;
            }
            QHeaderView::section {
                background-color: #262b34;
                color: #9aa5b4;
                padding: 6px;
                border: none;
                border-bottom: 1px solid #343b48;
                font-weight: 600;
            }
            QProgressBar {
                background-color: #262b34;
                border: none;
                border-radius: 3px;
            }
            QProgressBar::chunk {
                background-color: #22c55e;
                border-radius: 3px;
            }
            QSlider::groove:horizontal {
                height: 5px;
                background: #323946;
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: #3b82f6;
                width: 14px;
                height: 14px;
                margin: -5px 0;
                border-radius: 7px;
            }
            QSlider::sub-page:horizontal {
                background: #3b82f6;
                border-radius: 3px;
            }
            QStatusBar {
                background-color: #16191e;
                border-top: 1px solid #2b313c;
                color: #c5cedb;
            }
            QMenu {
                background-color: #252a33;
                color: #e6eaf0;
                border: 1px solid #3a4250;
                padding: 4px;
            }
            QMenu::item {
                padding: 6px 20px;
                border-radius: 4px;
            }
            QMenu::item:selected {
                background-color: #2563eb;
                color: #ffffff;
            }
            QMenu::separator {
                height: 1px;
                background: #363d4a;
                margin: 4px 8px;
            }
            QScrollArea {
                border: none;
                background-color: transparent;
            }
        )";
    }

    // Light modern theme
    return R"(
        QMainWindow, QDialog, QWidget {
            background-color: #f4f6f9;
            color: #1f2937;
            font-family: "Inter", "Noto Sans", "Segoe UI", sans-serif;
            font-size: 13px;
        }
        QFrame#headerBar {
            background-color: #ffffff;
            border-bottom: 1px solid #dce1e8;
        }
        QFrame#activeCallCard {
            background-color: #ffffff;
            border: 1px solid #d5dce6;
            border-radius: 10px;
        }
        QFrame#activeCallCard[inCall="true"] {
            background-color: #ecfdf5;
            border: 1px solid #16a34a;
        }
        QTabWidget::pane {
            border: none;
            background-color: #f4f6f9;
        }
        QTabBar::tab {
            background-color: #e9edf3;
            color: #4b5563;
            padding: 8px 14px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background-color: #f4f6f9;
            color: #111827;
            border-bottom: 2px solid #2563eb;
        }
        QLineEdit, QComboBox, QSpinBox, QTextBrowser, QPlainTextEdit {
            background-color: #ffffff;
            color: #111827;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 6px 8px;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
            border: 1px solid #2563eb;
        }
        QPushButton {
            background-color: #ffffff;
            color: #1f2937;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            padding: 6px 10px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #eef2f7;
            border-color: #94a3b8;
        }
        QPushButton:checked {
            background-color: #2563eb;
            border-color: #2563eb;
            color: #ffffff;
        }
        QPushButton:disabled {
            background-color: #e5e7eb;
            color: #9ca3af;
            border-color: #d1d5db;
        }
        QPushButton#keypadBtn {
            background-color: #ffffff;
            border: 1px solid #d1d5db;
            border-radius: 8px;
            font-size: 15px;
            font-weight: 600;
            padding: 4px;
        }
        QPushButton#keypadBtn:hover {
            background-color: #eef2ff;
            border-color: #93c5fd;
        }
        QPushButton#keypadBtn:pressed {
            background-color: #2563eb;
            color: #ffffff;
        }
        QPushButton#callBtn {
            background-color: #16a34a;
            border: none;
            color: #ffffff;
            font-weight: 700;
            font-size: 14px;
            border-radius: 8px;
        }
        QPushButton#callBtn:hover {
            background-color: #15803d;
        }
        QPushButton#hangupBtn {
            background-color: #dc2626;
            border: none;
            color: #ffffff;
            font-weight: 700;
            font-size: 14px;
            border-radius: 8px;
        }
        QPushButton#hangupBtn:hover:!disabled {
            background-color: #b91c1c;
        }
        QPushButton#hangupBtn:disabled {
            background-color: #e5e7eb;
            color: #9ca3af;
        }
        QPushButton#primaryBtn {
            background-color: #2563eb;
            border: none;
            color: #ffffff;
            font-weight: 600;
        }
        QPushButton#primaryBtn:hover {
            background-color: #1d4ed8;
        }
        QPushButton#statusPill {
            background-color: #e5e7eb;
            border: 1px solid #cbd5e1;
            border-radius: 10px;
            padding: 2px 9px;
            font-size: 11px;
            font-weight: 700;
            color: #374151;
        }
        QTableWidget, QListWidget {
            background-color: #ffffff;
            alternate-background-color: #f8fafc;
            color: #1f2937;
            border: 1px solid #dce1e8;
            border-radius: 6px;
        }
        QHeaderView::section {
            background-color: #eef2f6;
            color: #475569;
            padding: 6px;
            border: none;
            border-bottom: 1px solid #cbd5e1;
            font-weight: 600;
        }
        QProgressBar {
            background-color: #e2e8f0;
            border: none;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background-color: #16a34a;
            border-radius: 3px;
        }
        QSlider::groove:horizontal {
            height: 5px;
            background: #cbd5e1;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: #2563eb;
            width: 14px;
            height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #2563eb;
            border-radius: 3px;
        }
        QStatusBar {
            background-color: #ffffff;
            border-top: 1px solid #dce1e8;
            color: #334155;
        }
    )";
}

} // namespace msip
