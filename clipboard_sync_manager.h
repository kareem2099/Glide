#ifndef CLIPBOARD_SYNC_MANAGER_H
#define CLIPBOARD_SYNC_MANAGER_H

#include <QObject>
#include <QClipboard>
#include <QString>
#include <QTimer>

class ClipboardSyncManager : public QObject
{
    Q_OBJECT

public:
    explicit ClipboardSyncManager(QObject *parent = nullptr);
    ~ClipboardSyncManager();

    void startMonitoring();
    void stopMonitoring();
    void setClipboardText(const QString& text);

signals:
    void clipboardTextChanged(const QString& text);

private slots:
    void onClipboardDataChanged();

private:
    QClipboard* m_clipboard;
    QString m_lastClipboardText;
    QTimer* m_monitorTimer;
};

#endif // CLIPBOARD_SYNC_MANAGER_H
