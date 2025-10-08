#include "connectionwizard.h"
#include <QApplication>
#include <QStyle>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QTextEdit>
#include <QGroupBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QIcon>

// DeviceDiscoveryPage implementation
DeviceDiscoveryPage::DeviceDiscoveryPage(QWidget* parent)
    : QWizardPage(parent)
    , m_deviceList(nullptr)
    , m_refreshButton(nullptr)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_refreshTimer(nullptr)
{
    setTitle(tr("Device Discovery"));
    setSubTitle(tr("Searching for other Glide devices on your network..."));

    setupUI();
}

void DeviceDiscoveryPage::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Status label
    m_statusLabel = new QLabel(tr("Discovering devices..."), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0); // Indeterminate progress
    layout->addWidget(m_progressBar);

    // Device list
    m_deviceList = new QListWidget(this);
    m_deviceList->setMinimumHeight(200);
    layout->addWidget(m_deviceList);

    // Refresh button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    connect(m_refreshButton, &QPushButton::clicked, this, &DeviceDiscoveryPage::onRefreshClicked);
    buttonLayout->addWidget(m_refreshButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    setLayout(layout);
}

void DeviceDiscoveryPage::initializePage() {
    startDeviceDiscovery();
}

bool DeviceDiscoveryPage::isComplete() const {
    return m_deviceList->count() > 0;
}

void DeviceDiscoveryPage::startDeviceDiscovery() {
    m_deviceList->clear();
    m_statusLabel->setText(tr("Discovering devices..."));
    m_progressBar->setVisible(true);

    DeviceDiscovery* discovery = DeviceDiscovery::instance();
    connect(discovery, &DeviceDiscovery::deviceDiscovered,
            this, &DeviceDiscoveryPage::onDeviceDiscovered);
    connect(discovery, &DeviceDiscovery::discoveryFinished,
            this, &DeviceDiscoveryPage::onDiscoveryFinished);

    discovery->startDiscovery();
}

void DeviceDiscoveryPage::onDeviceDiscovered(const DiscoveredDevice& device) {
    QListWidgetItem* item = new QListWidgetItem(m_deviceList);
    item->setText(QString("%1 (%2) - %3")
                  .arg(device.name)
                  .arg(device.ipAddress)
                  .arg(device.status));
    item->setData(Qt::UserRole, QVariant::fromValue(device));
    item->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
}

void DeviceDiscoveryPage::onDiscoveryFinished() {
    m_progressBar->setVisible(false);
    m_statusLabel->setText(tr("Discovery finished. Found %1 device(s).")
                          .arg(m_deviceList->count()));

    if (m_deviceList->count() == 0) {
        m_statusLabel->setText(tr("No devices found. Make sure other devices are running Glide."));
    }

    emit completeChanged();
}

void DeviceDiscoveryPage::onRefreshClicked() {
    startDeviceDiscovery();
}

void DeviceDiscoveryPage::updateDeviceList() {
    // Update device statuses or refresh the list
    DeviceDiscovery* discovery = DeviceDiscovery::instance();
    QList<DiscoveredDevice> devices = discovery->getDiscoveredDevices();

    m_deviceList->clear();
    for (const DiscoveredDevice& device : devices) {
        onDeviceDiscovered(device);
    }
}

// DeviceSetupPage implementation
DeviceSetupPage::DeviceSetupPage(QWidget* parent)
    : QWizardPage(parent)
    , m_deviceList(nullptr)
    , m_serverButton(nullptr)
    , m_clientButton(nullptr)
    , m_instructionLabel(nullptr)
    , m_mainLayout(nullptr)
{
    setTitle(tr("Device Setup"));
    setSubTitle(tr("Select a device and choose your role"));

    setupUI();
}

void DeviceSetupPage::setupUI() {
    m_mainLayout = new QVBoxLayout(this);

    // Instructions
    m_instructionLabel = new QLabel(tr("Select a device from the list below:"), this);
    m_mainLayout->addWidget(m_instructionLabel);

    // Device list
    m_deviceList = new QListWidget(this);
    m_deviceList->setMinimumHeight(150);
    m_mainLayout->addWidget(m_deviceList);

    // Role selection
    QGroupBox* roleGroup = new QGroupBox(tr("Choose your role"), this);
    QVBoxLayout* roleLayout = new QVBoxLayout(roleGroup);

    m_serverButton = new QPushButton(tr("🖥️ Server Mode"), this);
    m_serverButton->setCheckable(true);
    m_serverButton->setStyleSheet("QPushButton { font-size: 16px; padding: 10px; text-align: left; }");
    connect(m_serverButton, &QPushButton::clicked, this, &DeviceSetupPage::onServerModeSelected);

    m_clientButton = new QPushButton(tr("📱 Client Mode"), this);
    m_clientButton->setCheckable(true);
    m_clientButton->setStyleSheet("QPushButton { font-size: 16px; padding: 10px; text-align: left; }");
    connect(m_clientButton, &QPushButton::clicked, this, &DeviceSetupPage::onClientModeSelected);

    roleLayout->addWidget(m_serverButton);
    roleLayout->addWidget(m_clientButton);
    m_mainLayout->addWidget(roleGroup);

    setLayout(m_mainLayout);
}

void DeviceSetupPage::initializePage() {
    // Populate device list from wizard data
    QVariant deviceData = field("selectedDevice");
    if (deviceData.isValid()) {
        DiscoveredDevice device = deviceData.value<DiscoveredDevice>();
        QListWidgetItem* item = new QListWidgetItem(m_deviceList);
        item->setText(QString("%1 (%2)").arg(device.name).arg(device.ipAddress));
        item->setData(Qt::UserRole, deviceData);
        item->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    }
}

bool DeviceSetupPage::isComplete() const {
    return m_deviceList->currentItem() != nullptr && (m_serverButton->isChecked() || m_clientButton->isChecked());
}

int DeviceSetupPage::nextId() const {
    return ConnectionWizard::Page_ConnectionProgress;
}

DiscoveredDevice DeviceSetupPage::getSelectedDevice() const {
    if (m_deviceList->currentItem()) {
        return m_deviceList->currentItem()->data(Qt::UserRole).value<DiscoveredDevice>();
    }
    return DiscoveredDevice();
}

QString DeviceSetupPage::getDeviceRole() const {
    if (m_serverButton->isChecked()) return "Server";
    if (m_clientButton->isChecked()) return "Client";
    return "";
}

void DeviceSetupPage::onServerModeSelected() {
    m_clientButton->setChecked(false);
    emit completeChanged();
}

void DeviceSetupPage::onClientModeSelected() {
    m_serverButton->setChecked(false);
    emit completeChanged();
}

// ConnectionProgressPage implementation
ConnectionProgressPage::ConnectionProgressPage(QWidget* parent)
    : QWizardPage(parent)
    , m_progressBar(nullptr)
    , m_statusLabel(nullptr)
    , m_progressTimer(nullptr)
    , m_connectionSuccessful(false)
{
    setTitle(tr("Connection Progress"));
    setSubTitle(tr("Establishing connection..."));

    setupUI();
}

void ConnectionProgressPage::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(tr("Preparing connection..."), this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);

    setLayout(layout);

    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, &QTimer::timeout, this, &ConnectionProgressPage::updateProgress);
}

void ConnectionProgressPage::initializePage() {
    m_connectionSuccessful = false;
    m_progressBar->setValue(0);
    m_statusLabel->setText(tr("Establishing connection..."));

    // Start connection attempt
    QTimer::singleShot(100, this, &ConnectionProgressPage::attemptConnection);
}

bool ConnectionProgressPage::isComplete() const {
    return m_connectionSuccessful;
}

void ConnectionProgressPage::attemptConnection() {
    m_progressTimer->start(100); // Update progress every 100ms

    // Simulate connection attempt (replace with actual connection logic)
    QTimer::singleShot(3000, this, &ConnectionProgressPage::onConnectionAttemptFinished);
}

void ConnectionProgressPage::onConnectionAttemptFinished() {
    m_progressTimer->stop();
    m_progressBar->setValue(100);
    m_connectionSuccessful = true;
    m_statusLabel->setText(tr("Connection established successfully!"));

    emit completeChanged();
}

void ConnectionProgressPage::updateProgress() {
    int currentValue = m_progressBar->value();
    if (currentValue < 90) {
        m_progressBar->setValue(currentValue + 5);
    }
}

// ConnectionWizard implementation
ConnectionWizard::ConnectionWizard(QWidget* parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Glide Connection Wizard"));
    setWindowIcon(QIcon(":/icons/Glide.png"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::HaveHelpButton, false);
    setOption(QWizard::HaveCustomButton1, true);
    setButtonText(QWizard::CustomButton1, tr("Skip"));

    setupWizard();
    createPages();

    connect(this, &QWizard::customButtonClicked, [this](int which) {
        if (which == QWizard::CustomButton1) {
            reject(); // Skip wizard
        }
    });
}

void ConnectionWizard::setupWizard() {
    resize(600, 400);
}

void ConnectionWizard::createPages() {
    // Device Discovery Page
    DeviceDiscoveryPage* discoveryPage = new DeviceDiscoveryPage(this);
    setPage(Page_DeviceDiscovery, discoveryPage);

    // Device Setup Page
    DeviceSetupPage* setupPage = new DeviceSetupPage(this);
    setPage(Page_DeviceSetup, setupPage);

    // Connection Progress Page
    ConnectionProgressPage* progressPage = new ConnectionProgressPage(this);
    setPage(Page_ConnectionProgress, progressPage);

    // Finished Page
    QWizardPage* finishedPage = new QWizardPage(this);
    finishedPage->setTitle(tr("Connection Complete"));
    finishedPage->setSubTitle(tr("Your devices are now connected!"));

    QVBoxLayout* layout = new QVBoxLayout(finishedPage);
    QLabel* successLabel = new QLabel(tr("✅ Connection established successfully!\n\n"
                                         "You can now use Glide to share input between your devices."), finishedPage);
    successLabel->setAlignment(Qt::AlignCenter);
    successLabel->setStyleSheet("QLabel { font-size: 14px; }");
    layout->addWidget(successLabel);
    setPage(Page_Finished, finishedPage);
}

void ConnectionWizard::onWizardAccepted() {
    // Handle successful completion
    qDebug() << "Connection wizard completed successfully";
}

void ConnectionWizard::onDeviceSelected() {
    // Handle device selection
    qDebug() << "Device selected";
}
