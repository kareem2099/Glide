#include "clipboard_sync_manager.h"
#include <QApplication>
#include <QDebug>

ClipboardSyncManager::ClipboardSyncManager(QObject *parent)
    : QObject(parent),
      m_clipboard(QApplication::clipboard()),
      m_monitorTimer(new QTimer(this))
{
    connect(m_clipboard, &QClipboard::changed, this, &ClipboardSyncManager::onClipboardDataChanged);
    
    // Use a timer to periodically check clipboard, as QClipboard::changed might not always fire reliably
    // for all types of changes or on all platforms. This provides a fallback mechanism.
    m_monitorTimer->setInterval(1000); // Check every 1 second
    connect(m_monitorTimer, &QTimer::timeout, this, &ClipboardSyncManager::onClipboardDataChanged);
}

ClipboardSyncManager::~ClipboardSyncManager()
{
    stopMonitoring();
}

void ClipboardSyncManager::startMonitoring()
{
    m_monitorTimer->start();
    m_lastClipboardText = m_clipboard->text(); // Initialize with current clipboard content
    qDebug() << "Clipboard monitoring started.";
}

void ClipboardSyncManager::stopMonitoring()
{
    m_monitorTimer->stop();
    qDebug() << "Clipboard monitoring stopped.";
}

void ClipboardSyncManager::setClipboardText(const QString& text)
{
    // Temporarily disconnect to avoid re-triggering onClipboardDataChanged when setting clipboard
    disconnect(m_clipboard, &QClipboard::changed, this, &ClipboardSyncManager::onClipboardDataChanged);
    disconnect(m_monitorTimer, &QTimer::timeout, this, &ClipboardSyncManager::onClipboardDataChanged);

    m_clipboard->setText(text);
    m_lastClipboardText = text; // Update last known clipboard text

    // Reconnect after setting
    connect(m_clipboard, &QClipboard::changed, this, &ClipboardSyncManager::onClipboardDataChanged);
    connect(m_monitorTimer, &QTimer::timeout, this, &ClipboardSyncManager::onClipboardDataChanged);
    qDebug() << "Clipboard text set programmatically.";
}

void ClipboardSyncManager::onClipboardDataChanged()
{
    QString currentText = m_clipboard->text();
    if (currentText != m_lastClipboardText) {
        m_lastClipboardText = currentText;
        emit clipboardTextChanged(currentText);
        qDebug() << "Clipboard text changed:" << currentText;
    }
}
