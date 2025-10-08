#ifndef CONNECTIONWIZARD_H
#define CONNECTIONWIZARD_H

#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTimer>
#include "device_discovery.h"

class DeviceDiscoveryPage : public QWizardPage {
    Q_OBJECT

public:
    explicit DeviceDiscoveryPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;

private slots:
    void onDeviceDiscovered(const DiscoveredDevice& device);
    void onDiscoveryFinished();
    void onRefreshClicked();

private:
    QListWidget* m_deviceList;
    QPushButton* m_refreshButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QTimer* m_refreshTimer;

    void setupUI();
    void startDeviceDiscovery();
    void updateDeviceList();
};

class DeviceSetupPage : public QWizardPage {
    Q_OBJECT

public:
    explicit DeviceSetupPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;
    int nextId() const override;

    DiscoveredDevice getSelectedDevice() const;
    QString getDeviceRole() const;

private slots:
    void onServerModeSelected();
    void onClientModeSelected();

private:
    void setupUI();

    QListWidget* m_deviceList;
    QPushButton* m_serverButton;
    QPushButton* m_clientButton;
    QLabel* m_instructionLabel;
    QVBoxLayout* m_mainLayout;
};

class ConnectionProgressPage : public QWizardPage {
    Q_OBJECT

public:
    explicit ConnectionProgressPage(QWidget* parent = nullptr);

    void initializePage() override;
    bool isComplete() const override;

private slots:
    void onConnectionAttemptFinished();
    void updateProgress();

private:
    void setupUI();
    void attemptConnection();

    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    QTimer* m_progressTimer;
    bool m_connectionSuccessful;
};

class ConnectionWizard : public QWizard {
    Q_OBJECT

public:
    explicit ConnectionWizard(QWidget* parent = nullptr);

    enum {
        Page_DeviceDiscovery,
        Page_DeviceSetup,
        Page_ConnectionProgress,
        Page_Finished
    };

private slots:
    void onWizardAccepted();
    void onDeviceSelected();

private:
    void setupWizard();
    void createPages();

    DiscoveredDevice m_selectedDevice;
    QString m_deviceRole;
};

#endif // CONNECTIONWIZARD_H
