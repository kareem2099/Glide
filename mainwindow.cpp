#include "mainwindow.h"
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include <QRandomGenerator> // Added for QRandomGenerator
#include <QFileDialog> // Added for file dialog
#include <thread> // Added for std::thread
#include "file_transfer.h" // Include for file transfer functions
#include "file_transfer_improvements.h" // Include enhanced file transfer
#include "constants.h"
#include <QIcon> // Added for QIcon
#include "settingsdialog.h" // Include the new settings dialog
#include <QTranslator> // Required for QTranslator
#include <QScrollBar> // Required for QScrollBar

GlideMainWindow::GlideMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_serverProcess(nullptr)
    , m_clientProcess(nullptr)
    , m_serverRunning(false)
    , m_clientConnected(false)
    , m_connectedClients(0)
    , m_settings(new QSettings(QString::fromStdString(Constants::APP_NAME), QString::fromStdString(Constants::SETTINGS_FILE_NAME), this))
    , m_translator(nullptr) // Initialize translator
    , m_clipboardSyncManager(new ClipboardSyncManager(this)) // Initialize ClipboardSyncManager
    , m_deviceDiscovery(DeviceDiscovery::instance()) // Initialize DeviceDiscovery
    , m_discoveredDevicesList(nullptr)
    , m_discoverDevicesBtn(nullptr)
    , m_connectionWizardBtn(nullptr)
    , m_discoveryStatusLabel(nullptr)
{
    setWindowTitle(tr(Constants::APP_NAME.c_str()) + " - " + tr("Cross-Device Input Sharing"));
    setWindowIcon(QIcon(":/icons/Glide.png")); // You'll need to add this resource
    resize(800, 600);
    
    createMenus(); // Call the new menu creation function
    setupUI();
    setupTrayIcon();
    loadSettings();
    updateUIBasedOnRole(); // Call to update UI based on loaded role
    
    // Setup timers
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &GlideMainWindow::updateConnectionStatus);
    m_statusTimer->start(1000); // Update every second
    
    m_latencyTimer = new QTimer(this);
    m_latencyTimer->start(2000); // Update latency every 2 seconds
    
    logMessage(tr("%1 initialized successfully").arg(QString::fromStdString(Constants::APP_NAME)));
    logMessage(tr("Local IP: %1").arg(getLocalIPAddress()));

    // Connect clipboard sync manager signal
    connect(m_clipboardSyncManager, &ClipboardSyncManager::clipboardTextChanged,
            this, &GlideMainWindow::onClipboardDataReceivedAndSend);
    m_clipboardSyncManager->startMonitoring(); // Start monitoring clipboard changes
}

GlideMainWindow::~GlideMainWindow()
{
    saveSettings();
    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        m_serverProcess->terminate();
        m_serverProcess->waitForFinished(3000);
    }
    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        m_clientProcess->terminate();
        m_clientProcess->waitForFinished(3000);
    }
}

void GlideMainWindow::setupUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(m_centralWidget);
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    setupServerGroupUI(contentLayout);
    setupClientGroupUI(contentLayout);
    setupFileTransferGroupUI(contentLayout);
    setupDeviceDiscoveryGroupUI(contentLayout);

    mainLayout->addLayout(contentLayout);

    setupLogGroupUI(mainLayout);
    
    // Status bar
    statusBar()->showMessage(tr("Ready"));
    
    // Local IP display
    m_localIPLabel = new QLabel(tr("Local IP: %1").arg(getLocalIPAddress()));
    m_localIPLabel->setStyleSheet("QLabel { font-weight: bold; color: blue; }");
    statusBar()->addPermanentWidget(m_localIPLabel);
    
    // Connect signals
    connect(m_startServerBtn, &QPushButton::clicked, this, &GlideMainWindow::startServer);
    connect(m_stopServerBtn, &QPushButton::clicked, this, &GlideMainWindow::stopServer);
    connect(m_connectBtn, &QPushButton::clicked, this, &GlideMainWindow::connectToServer);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &GlideMainWindow::disconnectFromServer);
    connect(m_browseFileBtn, &QPushButton::clicked, this, &GlideMainWindow::browseFile);
    connect(m_sendFileBtn, &QPushButton::clicked, this, &GlideMainWindow::sendFileEnhanced);
    connect(m_analyzeNetworkBtn, &QPushButton::clicked, this, &GlideMainWindow::analyzeNetwork);

    // Add a checkbox for enabling/disabling clipboard sync
    QCheckBox* clipboardSyncCheckBox = new QCheckBox(tr("Enable Clipboard Sync"));
    clipboardSyncCheckBox->setChecked(m_settings->value("clipboard/enabled", true).toBool());
    connect(clipboardSyncCheckBox, &QCheckBox::toggled, [this](bool checked) {
        m_settings->setValue("clipboard/enabled", checked);
        if (checked) {
            m_clipboardSyncManager->startMonitoring();
            logMessage(tr("Clipboard synchronization enabled."));
        } else {
            m_clipboardSyncManager->stopMonitoring();
            logMessage(tr("Clipboard synchronization disabled."));
        }
    });
    mainLayout->addWidget(clipboardSyncCheckBox);
}

void GlideMainWindow::setupServerGroupUI(QHBoxLayout* mainLayout)
{
    m_serverGroup = new QGroupBox(tr("🖥️ Server Mode"), this);
    m_serverGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* serverLayout = new QVBoxLayout(m_serverGroup);
    
    
    m_startServerBtn = new QPushButton(tr("Start Server"));
    m_startServerBtn->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    m_stopServerBtn = new QPushButton(tr("Stop Server"));
    m_stopServerBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_stopServerBtn->setEnabled(false);
    
    serverLayout->addWidget(m_startServerBtn);
    serverLayout->addWidget(m_stopServerBtn);
    
    serverLayout->addWidget(new QLabel(tr("Status:")));
    m_serverStatusLabel = new QLabel(tr("Stopped"));
    m_serverStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    serverLayout->addWidget(m_serverStatusLabel);
    
    serverLayout->addWidget(new QLabel(tr("Connected Clients:")));
    m_connectedClientsLabel = new QLabel(tr("0"));
    m_connectedClientsLabel->setStyleSheet("QLabel { font-weight: bold; }");
    serverLayout->addWidget(m_connectedClientsLabel);
    
    mainLayout->addWidget(m_serverGroup);
}

void GlideMainWindow::setupClientGroupUI(QHBoxLayout* mainLayout)
{
    m_clientGroup = new QGroupBox(tr("📱 Client Mode"), this);
    m_clientGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* clientLayout = new QVBoxLayout(m_clientGroup);
    
    clientLayout->addWidget(new QLabel(tr("Server IP Address:")));
    m_serverIPEdit = new QLineEdit();
    m_serverIPEdit->setPlaceholderText(tr("e.g., 192.168.1.100"));
    clientLayout->addWidget(m_serverIPEdit);
    
    
    m_connectBtn = new QPushButton(tr("Connect to Server"));
    m_connectBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    m_disconnectBtn = new QPushButton(tr("Disconnect"));
    m_disconnectBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    m_disconnectBtn->setEnabled(false);
    
    clientLayout->addWidget(m_connectBtn);
    clientLayout->addWidget(m_disconnectBtn);
    
    clientLayout->addWidget(new QLabel(tr("Status:")));
    m_clientStatusLabel = new QLabel(tr("Disconnected"));
    m_clientStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    clientLayout->addWidget(m_clientStatusLabel);
    
    clientLayout->addWidget(new QLabel(tr("Latency:")));
    m_latencyLabel = new QLabel(tr("- ms"));
    m_latencyLabel->setStyleSheet("QLabel { font-weight: bold; }");
    clientLayout->addWidget(m_latencyLabel);
    
    mainLayout->addWidget(m_clientGroup);
}

void GlideMainWindow::setupFileTransferGroupUI(QHBoxLayout* mainLayout)
{
    m_fileTransferGroup = new QGroupBox(tr("📁 Enhanced File Transfer"), this);
    m_fileTransferGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* fileTransferLayout = new QVBoxLayout(m_fileTransferGroup);

    // Transfer mode selection
    QHBoxLayout* modeLayout = new QHBoxLayout();
    modeLayout->addWidget(new QLabel(tr("Transfer Mode:")));
    m_transferModeCombo = new QComboBox();
    m_transferModeCombo->addItem(tr("Auto-Optimized"), static_cast<int>(TransferMode::HYBRID_ADAPTIVE));
    m_transferModeCombo->addItem(tr("Standard TCP"), static_cast<int>(TransferMode::TCP_STANDARD));
    m_transferModeCombo->addItem(tr("UDP Accelerated"), static_cast<int>(TransferMode::UDP_ACCELERATED));
    m_transferModeCombo->addItem(tr("Parallel TCP"), static_cast<int>(TransferMode::TCP_PARALLEL));
    m_transferModeCombo->setCurrentIndex(0); // Default to auto-optimized
    modeLayout->addWidget(m_transferModeCombo);

    m_analyzeNetworkBtn = new QPushButton(tr("Analyze Network"));
    m_analyzeNetworkBtn->setStyleSheet("QPushButton { background-color: #607D8B; color: white; font-weight: bold; padding: 6px; }");
    modeLayout->addWidget(m_analyzeNetworkBtn);
    fileTransferLayout->addLayout(modeLayout);

    // Network status display
    m_networkStatusLabel = new QLabel(tr("Network: Not analyzed"));
    m_networkStatusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    fileTransferLayout->addWidget(m_networkStatusLabel);

    // File selection
    QHBoxLayout* filePathLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText(tr("Path to file to send..."));
    filePathLayout->addWidget(m_filePathEdit);
    m_browseFileBtn = new QPushButton(tr("Browse"));
    filePathLayout->addWidget(m_browseFileBtn);
    fileTransferLayout->addLayout(filePathLayout);

    // Transfer controls
    QHBoxLayout* transferControls = new QHBoxLayout();
    m_sendFileBtn = new QPushButton(tr("🚀 Send File"));
    m_sendFileBtn->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    transferControls->addWidget(m_sendFileBtn);

    QPushButton* cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; font-weight: bold; padding: 8px; }");
    cancelBtn->setEnabled(false);
    transferControls->addWidget(cancelBtn);
    fileTransferLayout->addLayout(transferControls);

    // Enhanced progress display
    setupTransferProgressUI(fileTransferLayout);

    mainLayout->addWidget(m_fileTransferGroup);
}

void GlideMainWindow::setupLogGroupUI(QVBoxLayout* mainLayout)
{
    QGroupBox* logGroup = new QGroupBox(tr("📊 Activity Log"), this);
    logGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    m_logTextEdit = new QTextEdit();
    m_logTextEdit->setMaximumHeight(200);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setStyleSheet("QTextEdit { background-color: #f5f5f5; font-family: Consolas, monospace; }");
    m_logTextEdit->setPlaceholderText(tr("Activity log will appear here...")); // Added placeholder text
    logLayout->addWidget(m_logTextEdit);
    
    mainLayout->addWidget(logGroup);
}

void GlideMainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(this, tr("System Tray"),
                              tr("System tray is not available on this system."));
        return;
    }

    // Create tray icon menu
    m_trayMenu = new QMenu(this);
    
    QAction* showAction = m_trayMenu->addAction(tr("Show"));
    connect(showAction, &QAction::triggered, this, &GlideMainWindow::restoreFromTray);
    
    QAction* aboutAction = m_trayMenu->addAction(tr("About"));
    connect(aboutAction, &QAction::triggered, this, &GlideMainWindow::showAbout);
    
    m_trayMenu->addSeparator();
    
    QAction* quitAction = m_trayMenu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Create and setup tray icon
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setIcon(QIcon(":/icons/Glide.png"));
    m_trayIcon->setToolTip(tr(Constants::APP_NAME.c_str()));

    connect(m_trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            restoreFromTray();
        }
    });

    m_trayIcon->show();
}

void GlideMainWindow::startServer()
{
    if (m_deviceRole != "Server") {
        logMessage(tr("Cannot start server: Application is not in Server mode."), QString::fromStdString(Constants::LogTypes::WARNING));
        QMessageBox::warning(this, tr("Warning"), tr("Application is not in Server mode. Please change the device role in settings."));
        return;
    }

    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        return;
    }

    m_serverProcess = new QProcess(this);
    connect(m_serverProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GlideMainWindow::onServerProcessFinished);
    connect(m_serverProcess, &QProcess::readyReadStandardOutput, [this]() {
        logMessage("SERVER: " + m_serverProcess->readAllStandardOutput());
    });
    connect(m_serverProcess, &QProcess::readyReadStandardError, [this]() {
        logMessage("SERVER ERROR: " + m_serverProcess->readAllStandardError(), QString::fromStdString(Constants::LogTypes::ERROR));
    });

    QString program = "./build/server"; // Adjust path as needed
#ifdef _WIN32
    program = "build/server.exe";
#endif

    QStringList arguments;
    // The server needs to know the client's IP to send UDP packets.
    // For now, we'll assume the client IP is entered in the client's server IP field.
    // In a real scenario, this might be discovered or configured differently.
    QString clientIPForServer = m_serverIPEdit->text().trimmed(); 
    if (clientIPForServer.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter the Client IP Address in the Client Mode section to start the server!"));
        return;
    }
    arguments << clientIPForServer;

    int serverUdpPort = m_settings->value(Constants::SettingsKeys::SERVER_UDP_PORT, Constants::DEFAULT_UDP_PORT).toInt();
    logMessage(tr("Starting server, sending input to %1:%2...").arg(clientIPForServer).arg(serverUdpPort));
    
    m_serverProcess->start(program, arguments);
    
    if (m_serverProcess->waitForStarted()) {
        m_serverRunning = true;
        m_startServerBtn->setEnabled(false);
        m_stopServerBtn->setEnabled(true);
        m_serverStatusLabel->setText(tr("Running"));
        m_serverStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        logMessage(tr("Server started successfully!"), QString::fromStdString(Constants::LogTypes::SUCCESS));
        statusBar()->showMessage(tr("Server running, sending to %1").arg(clientIPForServer));
    } else {
        logMessage(tr("Failed to start server: %1").arg(m_serverProcess->errorString()), QString::fromStdString(Constants::LogTypes::ERROR));
        QMessageBox::critical(this, tr("Error"), tr("Failed to start server process!"));
    }
}

void GlideMainWindow::stopServer()
{
    if (m_deviceRole != "Server") {
        logMessage(tr("Cannot stop server: Application is not in Server mode."), QString::fromStdString(Constants::LogTypes::WARNING));
        return;
    }

    if (m_serverProcess && m_serverProcess->state() == QProcess::Running) {
        logMessage("Stopping server...");
        m_serverProcess->terminate();
        if (!m_serverProcess->waitForFinished(3000)) {
            m_serverProcess->kill();
        }
    }
    
    m_serverRunning = false;
    m_startServerBtn->setEnabled(true);
    m_stopServerBtn->setEnabled(false);
    m_serverStatusLabel->setText("Stopped");
    m_serverStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_connectedClientsLabel->setText("0");
    m_connectedClients = 0;
    logMessage(tr("Server stopped"), QString::fromStdString(Constants::LogTypes::SUCCESS));
    statusBar()->showMessage(tr("Server stopped"));
}

void GlideMainWindow::connectToServer()
{
    if (m_deviceRole != "Client") {
        logMessage(tr("Cannot connect to server: Application is not in Client mode."), QString::fromStdString(Constants::LogTypes::WARNING));
        QMessageBox::warning(this, tr("Warning"), tr("Application is not in Client mode. Please change the device role in settings."));
        return;
    }

    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address!"));
        return;
    }
    
    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        return;
    }

    m_clientProcess = new QProcess(this);
    connect(m_clientProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GlideMainWindow::onClientProcessFinished);
    connect(m_clientProcess, &QProcess::readyReadStandardOutput, [this]() {
        logMessage("CLIENT: " + m_clientProcess->readAllStandardOutput());
    });
    connect(m_clientProcess, &QProcess::readyReadStandardError, [this]() {
        logMessage("CLIENT ERROR: " + m_clientProcess->readAllStandardError(), QString::fromStdString(Constants::LogTypes::ERROR));
    });

    QString program = "./build/client"; // Adjust path as needed
#ifdef _WIN32
    program = "build/client.exe";
#endif

    QStringList arguments;
    arguments << serverIP; // Client needs server IP to bind its UDP socket to listen for server's UDP packets

    int clientUdpPort = m_settings->value(Constants::SettingsKeys::CLIENT_UDP_PORT, Constants::DEFAULT_UDP_PORT).toInt();
    logMessage(tr("Connecting to server at %1:%2 (UDP)...").arg(serverIP).arg(clientUdpPort));
    
    m_clientProcess->start(program, arguments);
    
    if (m_clientProcess->waitForStarted()) {
        m_clientConnected = true;
        m_connectBtn->setEnabled(false);
        m_disconnectBtn->setEnabled(true);
        m_clientStatusLabel->setText(tr("Connected"));
        m_clientStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        logMessage(tr("Connected to server successfully!"), QString::fromStdString(Constants::LogTypes::SUCCESS));
        statusBar()->showMessage(tr("Connected to %1 (UDP)").arg(serverIP));
    } else {
        logMessage(tr("Failed to connect to server: %1").arg(m_clientProcess->errorString()), QString::fromStdString(Constants::LogTypes::ERROR));
        QMessageBox::critical(this, tr("Error"), tr("Failed to connect to server process!"));
    }
}

void GlideMainWindow::disconnectFromServer()
{
    if (m_deviceRole != "Client") {
        logMessage(tr("Cannot disconnect from server: Application is not in Client mode."), QString::fromStdString(Constants::LogTypes::WARNING));
        return;
    }

    if (m_clientProcess && m_clientProcess->state() == QProcess::Running) {
        logMessage("Disconnecting from server...");
        m_clientProcess->terminate();
        if (!m_clientProcess->waitForFinished(3000)) {
            m_clientProcess->kill();
        }
    }
    
    m_clientConnected = false;
    m_connectBtn->setEnabled(true);
    m_disconnectBtn->setEnabled(false);
    m_clientStatusLabel->setText("Disconnected");
    m_clientStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    m_latencyLabel->setText(tr("- ms"));
    logMessage(tr("Disconnected from server"), QString::fromStdString(Constants::LogTypes::SUCCESS));
    statusBar()->showMessage(tr("Disconnected"));
}

void GlideMainWindow::browseFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Select File to Send"));
    if (!filePath.isEmpty()) {
        m_filePathEdit->setText(filePath);
    }
}

void GlideMainWindow::sendFile()
{
    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address in the Client Mode section before sending a file!"));
        return;
    }

    QString filePath = m_filePathEdit->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a file to send!"));
        return;
    }

    logMessage(tr("Attempting to send file: %1 to %2 via TCP...").arg(filePath).arg(serverIP));
    
    // Run file sending in a separate thread to avoid blocking the GUI
    std::thread([serverIP, filePath, this]() {
#ifdef _WIN32
        send_file_tcp(serverIP.toStdString(), filePath.toStdString());
#else
        send_file_tcp_linux(serverIP.toStdString(), filePath.toStdString());
#endif
        // Log completion or error back on the main thread if needed
        // QMetaObject::invokeMethod(this, [this]() { logMessage("File transfer complete/failed."); });
    }).detach();
}

void GlideMainWindow::onClipboardDataReceivedAndSend(const QString& text)
{
    logMessage(tr("Clipboard data changed: %1").arg(text.left(50) + (text.length() > 50 ? "..." : "")));
    // Here, you would send the clipboard data over the network to other devices.
    // For now, we just log it.
    // Example: sendNetworkMessage(Constants::ClipboardMessages::CLIPBOARD_DATA + text.toStdString());
}


#include <QRandomGenerator> // Added for QRandomGenerator

void GlideMainWindow::updateConnectionStatus()
{
    // Update connected clients count (simulate for now)
    if (m_serverRunning) {
        // In a real implementation, you'd query the server process for this info
        static int lastCount = 0;
        if (QRandomGenerator::global()->bounded(10) == 0) { // Occasionally change the count
            lastCount = QRandomGenerator::global()->bounded(3);
            m_connectedClientsLabel->setText(QString::number(lastCount));
        }
    }
    
    // Update latency (simulate for now)
    if (m_clientConnected) {
        int latency = 5 + (QRandomGenerator::global()->bounded(20)); // 5-25ms
        m_latencyLabel->setText(QString::number(latency) + " ms");
    }
}

void GlideMainWindow::onServerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus); // Mark as unused to avoid warning if not used
    logMessage(tr("Server process finished with exit code: %1").arg(QString::number(exitCode)));
    if (exitCode != 0) {
        logMessage(tr("Server process ended unexpectedly"), QString::fromStdString(Constants::LogTypes::ERROR));
    }
    stopServer();
}

void GlideMainWindow::onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus); // Mark as unused to avoid warning if not used
    logMessage(tr("Client process finished with exit code: %1").arg(QString::number(exitCode)));
    if (exitCode != 0) {
        logMessage(tr("Client process ended unexpectedly"), QString::fromStdString(Constants::LogTypes::ERROR));
    }
    disconnectFromServer();
}

void GlideMainWindow::logMessage(const QString& message, const QString& type)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, type, message);
    
    QString color = "black";
    if (type == QString::fromStdString(Constants::LogTypes::ERROR)) color = "red";
    else if (type == QString::fromStdString(Constants::LogTypes::SUCCESS)) color = "green";
    else if (type == QString::fromStdString(Constants::LogTypes::WARNING)) color = "orange";
    
    m_logTextEdit->append(QString("<span style='color: %1'>%2</span>").arg(color, logEntry));
    
    // Keep log size manageable
    QTextDocument* doc = m_logTextEdit->document();
    if (doc->blockCount() > 100) {
        QTextCursor cursor = m_logTextEdit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, 10);
        cursor.removeSelectedText();
    }
}

QString GlideMainWindow::getLocalIPAddress()
{
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address != QHostAddress::LocalHost && 
            address.toIPv4Address() && 
            !address.isLoopback()) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

void GlideMainWindow::loadSettings()
{
    m_serverIPEdit->setText(m_settings->value(Constants::SettingsKeys::CLIENT_SERVER_IP, "").toString());
    m_deviceRole = m_settings->value(Constants::SettingsKeys::DEVICE_ROLE, "Client").toString(); // Load device role
    
    // Restore window geometry
    restoreGeometry(m_settings->value(Constants::SettingsKeys::WINDOW_GEOMETRY).toByteArray());
}

void GlideMainWindow::saveSettings()
{
    m_settings->setValue(Constants::SettingsKeys::CLIENT_SERVER_IP, m_serverIPEdit->text());
    m_settings->setValue(Constants::SettingsKeys::WINDOW_GEOMETRY, saveGeometry());
}

void GlideMainWindow::showAbout()
{
    QString aboutText = tr("%1 v1.0\n\n"
                           "Cross-platform input sharing application\n"
                           "Share mouse and keyboard across multiple devices seamlessly\n\n"
                           "Built with Qt and C++\n\n"
                           "Build Date: %2")
                            .arg(QString::fromStdString(Constants::APP_NAME))
                            .arg(QString(BUILD_DATETIME));

    QMessageBox::about(this, tr("About %1").arg(QString::fromStdString(Constants::APP_NAME)), aboutText);
}

void GlideMainWindow::minimizeToTray()
{
    if (m_trayIcon && m_trayIcon->isVisible()) {
        hide();
        m_trayIcon->showMessage(tr(Constants::APP_NAME.c_str()), 
                                tr("Application minimized to system tray"),
                                QSystemTrayIcon::Information, 2000);
    }
}

void GlideMainWindow::restoreFromTray()
{
    show();
    raise();
    activateWindow();
}

void GlideMainWindow::createMenus()
{
    QMenu* glideMenu = menuBar()->addMenu(tr("&Glide")); // New menu named "Glide"
    
    QAction* activityLogAction = glideMenu->addAction(tr("Activity &Log"));
    activityLogAction->setShortcut(QKeySequence("Ctrl+H"));
    connect(activityLogAction, &QAction::triggered, this, &GlideMainWindow::showActivityLog);

    QAction* settingsAction = glideMenu->addAction(tr("&Settings"));
    settingsAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(settingsAction, &QAction::triggered, this, &GlideMainWindow::showSettingsDialog);

    QAction* aboutAction = glideMenu->addAction(tr("&About Glide"));
    connect(aboutAction, &QAction::triggered, this, &GlideMainWindow::showAbout);
}

void GlideMainWindow::showActivityLog()
{
    // Ensure the main window is visible and brought to front
    if (isMinimized()) {
        showNormal();
    }
    raise();
    activateWindow();
    // Optionally, scroll the log to the bottom
    m_logTextEdit->verticalScrollBar()->setValue(m_logTextEdit->verticalScrollBar()->maximum());
}

void GlideMainWindow::showSettingsDialog()
{
    SettingsDialog settingsDialog(m_settings, this);
    connect(&settingsDialog, &SettingsDialog::languageChanged, this, &GlideMainWindow::handleLanguageChange);
    connect(&settingsDialog, &QDialog::accepted, this, &GlideMainWindow::onSettingsDialogAccepted); // Connect accepted signal
    settingsDialog.exec(); // Show the dialog modally
}

void GlideMainWindow::onSettingsDialogAccepted()
{
    loadSettings(); // Reload settings after dialog is accepted
    updateUIBasedOnRole(); // Update UI based on new settings
}

void GlideMainWindow::retranslateUi()
{
    // Retranslate main window title
    setWindowTitle(tr(Constants::APP_NAME.c_str()) + " - " + tr("Cross-Device Input Sharing"));

    // Retranslate menu items
    menuBar()->clear(); // Clear existing menus
    createMenus(); // Recreate menus with new translations

    // Retranslate group box titles
    m_serverGroup->setTitle(tr("🖥️ Server Mode"));
    m_clientGroup->setTitle(tr("📱 Client Mode"));
    m_fileTransferGroup->setTitle(tr("📁 File Transfer"));
    // Retranslate labels and buttons
    m_startServerBtn->setText(tr("Start Server"));
    m_stopServerBtn->setText(tr("Stop Server"));
    m_serverStatusLabel->setText(tr("Stopped")); // Initial state
    m_connectedClientsLabel->setText(tr("0")); // Initial state
    m_connectBtn->setText(tr("Connect to Server"));
    m_disconnectBtn->setText(tr("Disconnect"));
    m_clientStatusLabel->setText(tr("Disconnected")); // Initial state
    m_latencyLabel->setText(tr("- ms")); // Initial state
    m_browseFileBtn->setText(tr("Browse"));
    m_sendFileBtn->setText(tr("Send File (TCP)"));
    m_logTextEdit->setPlaceholderText(tr("Activity log will appear here..."));
    m_localIPLabel->setText(tr("Local IP: %1").arg(getLocalIPAddress()));
    statusBar()->showMessage(tr("Ready")); // Initial state

    // Retranslate other labels in setup functions if they are not dynamic
    // For simplicity, I'm only retranslating the main window elements here.
    // A more robust solution would involve a base class for translatable widgets
    // or iterating through all children and calling retranslateUi on them.
}

void GlideMainWindow::updateUIBasedOnRole()
{
    if (m_deviceRole == "Server") {
        m_serverGroup->setEnabled(true);
        m_clientGroup->setEnabled(false);
        m_fileTransferGroup->setEnabled(true); // Server can also initiate file transfers
        logMessage(tr("Application set to Server mode."));
        // Optionally start server automatically
        // startServer(); 
    } else { // Client mode
        m_serverGroup->setEnabled(false);
        m_clientGroup->setEnabled(true);
        m_fileTransferGroup->setEnabled(true); // Client can also initiate file transfers
        logMessage(tr("Application set to Client mode."));
        // Optionally connect to server automatically
        // connectToServer();
    }
}

void GlideMainWindow::handleLanguageChange(const QString& langCode)
{
    logMessage(tr("Language changed to: %1").arg(langCode), QString::fromStdString(Constants::LogTypes::INFO));

    if (m_translator) {
        QApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    m_translator = new QTranslator(this);
    QString translationsPath = QApplication::applicationDirPath() + "/translations"; // Translations are in a subdirectory
    if (m_translator->load("glide_" + langCode, translationsPath)) {
        QApplication::installTranslator(m_translator);
        logMessage(tr("Loaded translation for %1").arg(langCode), QString::fromStdString(Constants::LogTypes::SUCCESS));
    } else {
        logMessage(tr("Failed to load translation for %1").arg(langCode), QString::fromStdString(Constants::LogTypes::ERROR));
        delete m_translator;
        m_translator = nullptr;
    }

    // Retranslate the UI
    retranslateUi();
}

void GlideMainWindow::setupDeviceDiscoveryGroupUI(QHBoxLayout* mainLayout)
{
    m_deviceDiscoveryGroup = new QGroupBox(tr("🔍 Device Discovery"), this);
    m_deviceDiscoveryGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 14px; }");
    QVBoxLayout* discoveryLayout = new QVBoxLayout(m_deviceDiscoveryGroup);

    // Status label
    m_discoveryStatusLabel = new QLabel(tr("Ready to discover devices"), this);
    discoveryLayout->addWidget(m_discoveryStatusLabel);

    // Device list
    m_discoveredDevicesList = new QListWidget(this);
    m_discoveredDevicesList->setMaximumHeight(150);
    m_discoveredDevicesList->setMinimumHeight(100);
    discoveryLayout->addWidget(m_discoveredDevicesList);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    m_discoverDevicesBtn = new QPushButton(tr("Discover Devices"));
    m_discoverDevicesBtn->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; font-weight: bold; padding: 8px; }");
    connect(m_discoverDevicesBtn, &QPushButton::clicked, this, &GlideMainWindow::startDeviceDiscovery);
    buttonLayout->addWidget(m_discoverDevicesBtn);

    m_connectionWizardBtn = new QPushButton(tr("Connection Wizard"));
    m_connectionWizardBtn->setStyleSheet("QPushButton { background-color: #607D8B; color: white; font-weight: bold; padding: 8px; }");
    connect(m_connectionWizardBtn, &QPushButton::clicked, this, &GlideMainWindow::showConnectionWizard);
    buttonLayout->addWidget(m_connectionWizardBtn);

    discoveryLayout->addLayout(buttonLayout);

    mainLayout->addWidget(m_deviceDiscoveryGroup);

    // Connect device discovery signals
    connect(m_deviceDiscovery, &DeviceDiscovery::deviceDiscovered,
            this, &GlideMainWindow::onDeviceDiscovered);
    connect(m_deviceDiscovery, &DeviceDiscovery::deviceLost,
            this, &GlideMainWindow::onDeviceLost);
}

void GlideMainWindow::onDeviceDiscovered(const DiscoveredDevice& device)
{
    logMessage(tr("Discovered device: %1 at %2").arg(device.name).arg(device.ipAddress),
               QString::fromStdString(Constants::LogTypes::INFO));

    // Add to list if not already present
    for (int i = 0; i < m_discoveredDevicesList->count(); ++i) {
        QListWidgetItem* item = m_discoveredDevicesList->item(i);
        DiscoveredDevice existingDevice = item->data(Qt::UserRole).value<DiscoveredDevice>();
        if (existingDevice.ipAddress == device.ipAddress) {
            // Update existing item
            item->setText(QString("%1 (%2) - %3").arg(device.name).arg(device.ipAddress).arg(device.status));
            item->setData(Qt::UserRole, QVariant::fromValue(device));
            return;
        }
    }

    // Add new item
    QListWidgetItem* item = new QListWidgetItem(m_discoveredDevicesList);
    item->setText(QString("%1 (%2) - %3").arg(device.name).arg(device.ipAddress).arg(device.status));
    item->setData(Qt::UserRole, QVariant::fromValue(device));
    item->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
}

void GlideMainWindow::onDeviceLost(const QString& ipAddress)
{
    logMessage(tr("Device lost: %1").arg(ipAddress), QString::fromStdString(Constants::LogTypes::WARNING));

    // Remove from list
    for (int i = 0; i < m_discoveredDevicesList->count(); ++i) {
        QListWidgetItem* item = m_discoveredDevicesList->item(i);
        DiscoveredDevice device = item->data(Qt::UserRole).value<DiscoveredDevice>();
        if (device.ipAddress == ipAddress) {
            delete m_discoveredDevicesList->takeItem(i);
            break;
        }
    }
}

void GlideMainWindow::showConnectionWizard()
{
    ConnectionWizard wizard(this);
    wizard.exec();
}

void GlideMainWindow::startDeviceDiscovery()
{
    logMessage(tr("Starting device discovery..."), QString::fromStdString(Constants::LogTypes::INFO));

    m_discoverDevicesBtn->setEnabled(false);
    m_discoverDevicesBtn->setText(tr("Discovering..."));
    m_discoveryStatusLabel->setText(tr("Discovering devices on network..."));

    m_deviceDiscovery->startDiscovery();

    // Re-enable button after discovery
    QTimer::singleShot(6000, this, [this]() {
        m_discoverDevicesBtn->setEnabled(true);
        m_discoverDevicesBtn->setText(tr("Discover Devices"));
        m_discoveryStatusLabel->setText(tr("Discovery complete"));
    });
}

void GlideMainWindow::stopDeviceDiscovery()
{
    m_deviceDiscovery->stopDiscovery();
    logMessage(tr("Device discovery stopped"), QString::fromStdString(Constants::LogTypes::INFO));
}

// Enhanced File Transfer Methods
void GlideMainWindow::setupTransferProgressUI(QVBoxLayout* parentLayout)
{
    // Progress group
    QGroupBox* progressGroup = new QGroupBox(tr("Transfer Progress"));
    progressGroup->setStyleSheet("QGroupBox { font-weight: bold; font-size: 12px; }");
    QVBoxLayout* progressLayout = new QVBoxLayout(progressGroup);

    // Progress bar
    m_transferProgressBar = new QProgressBar();
    m_transferProgressBar->setRange(0, 100);
    m_transferProgressBar->setValue(0);
    m_transferProgressBar->setVisible(false);
    progressLayout->addWidget(m_transferProgressBar);

    // Progress details
    QHBoxLayout* detailsLayout = new QHBoxLayout();
    m_speedLabel = new QLabel(tr("Speed: -"));
    m_etaLabel = new QLabel(tr("ETA: -"));
    detailsLayout->addWidget(m_speedLabel);
    detailsLayout->addWidget(m_etaLabel);
    progressLayout->addLayout(detailsLayout);

    parentLayout->addWidget(progressGroup);
}

void GlideMainWindow::sendFileEnhanced()
{
    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address in the Client Mode section before sending a file!"));
        return;
    }

    QString filePath = m_filePathEdit->text().trimmed();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please select a file to send!"));
        return;
    }

    // Get selected transfer mode
    TransferMode mode = static_cast<TransferMode>(m_transferModeCombo->currentData().toInt());

    // Initialize enhanced transfer manager if needed
    if (!m_enhancedTransferManager) {
        m_enhancedTransferManager = new EnhancedFileTransferManager();
    }

    // Update UI for transfer in progress
    m_sendFileBtn->setEnabled(false);
    m_sendFileBtn->setText(tr("Sending..."));
    m_transferProgressBar->setVisible(true);
    m_transferProgressBar->setValue(0);

    logMessage(tr("Starting enhanced file transfer: %1 to %2").arg(filePath).arg(serverIP));

    // Enhanced progress callback
    auto progressCallback = [this](const FileTransferProgress& progress) {
        QMetaObject::invokeMethod(this, "updateTransferProgress",
            Qt::QueuedConnection, Q_ARG(FileTransferProgress, progress));
    };

    // Run transfer in separate thread
    std::thread([this, serverIP, filePath, mode, progressCallback]() {
        bool success = m_enhancedTransferManager->sendFileAdvanced(
            serverIP.toStdString(),
            filePath.toStdString(),
            progressCallback,
            mode,
            true, // enable encryption
            3    // max retries
        );

        QMetaObject::invokeMethod(this, "onTransferFinished",
            Qt::QueuedConnection, Q_ARG(bool, success), Q_ARG(QString, filePath));
    }).detach();
}

void GlideMainWindow::updateTransferProgress(const FileTransferProgress& progress)
{
    m_transferProgressBar->setValue(progress.percentage);
    m_speedLabel->setText(tr("Speed: %1 KB/s").arg(progress.bytesPerSecond / 1024));

    if (progress.percentage > 0 && progress.percentage < 100) {
        // Calculate ETA
        int etaSeconds = (100 - progress.percentage) * progress.fileSize / progress.bytesPerSecond / 100;
        QString etaText = etaSeconds > 60 ?
            tr("%1m %2s").arg(etaSeconds / 60).arg(etaSeconds % 60) :
            tr("%1s").arg(etaSeconds);
        m_etaLabel->setText(tr("ETA: %1").arg(etaText));
    } else {
        m_etaLabel->setText(tr("ETA: -"));
    }

    if (progress.isComplete) {
        m_transferProgressBar->setValue(100);
        m_etaLabel->setText(tr("Complete"));
    }
}

void GlideMainWindow::onTransferFinished(bool success, const QString& filePath)
{
    m_sendFileBtn->setEnabled(true);
    m_sendFileBtn->setText(tr("🚀 Send File"));

    if (success) {
        logMessage(tr("File transfer completed successfully: %1").arg(filePath),
                   QString::fromStdString(Constants::LogTypes::SUCCESS));
        QMessageBox::information(this, tr("Success"),
            tr("File sent successfully!\n%1").arg(filePath));
    } else {
        logMessage(tr("File transfer failed: %1").arg(filePath),
                   QString::fromStdString(Constants::LogTypes::ERROR));
        QMessageBox::critical(this, tr("Error"),
            tr("File transfer failed!\n%1").arg(filePath));
    }
}

void GlideMainWindow::analyzeNetwork()
{
    QString serverIP = m_serverIPEdit->text().trimmed();
    if (serverIP.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Please enter a server IP address first!"));
        return;
    }

    m_analyzeNetworkBtn->setEnabled(false);
    m_analyzeNetworkBtn->setText(tr("Analyzing..."));
    m_networkStatusLabel->setText(tr("Analyzing network..."));

    logMessage(tr("Analyzing network conditions to %1...").arg(serverIP));

    // Run analysis in separate thread
    std::thread([this, serverIP]() {
        if (!m_enhancedTransferManager) {
            m_enhancedTransferManager = new EnhancedFileTransferManager();
        }

        NetworkConditions conditions = m_enhancedTransferManager->analyzeNetworkConditions(
            serverIP.toStdString());

        QMetaObject::invokeMethod(this, "onNetworkAnalysisComplete",
            Qt::QueuedConnection, Q_ARG(NetworkConditions, conditions));
    }).detach();
}

void GlideMainWindow::onNetworkAnalysisComplete(const NetworkConditions& conditions)
{
    m_analyzeNetworkBtn->setEnabled(true);
    m_analyzeNetworkBtn->setText(tr("Analyze Network"));

    QString statusText = tr("Network: %1").arg(QString::fromStdString(conditions.to_string()));
    m_networkStatusLabel->setText(statusText);

    // Color code based on network quality
    QString color = "green";
    if (conditions.is_congested) color = "red";
    else if (conditions.packet_loss_percent > 2) color = "orange";

    m_networkStatusLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }").arg(color));

    logMessage(tr("Network analysis complete: %1").arg(statusText));

    // Update transfer mode recommendation
    updateTransferModeRecommendation(conditions);
}

void GlideMainWindow::updateTransferModeRecommendation(const NetworkConditions& conditions)
{
    // Auto-select optimal transfer mode
    if (conditions.bandwidth_mbps > 10.0 && !conditions.is_congested) {
        m_transferModeCombo->setCurrentIndex(3); // Parallel TCP
        logMessage(tr("Recommended: Parallel TCP for high bandwidth"));
    } else if (conditions.latency_ms < 50.0 && conditions.packet_loss_percent < 2) {
        m_transferModeCombo->setCurrentIndex(2); // UDP Accelerated
        logMessage(tr("Recommended: UDP Accelerated for low latency"));
    } else {
        m_transferModeCombo->setCurrentIndex(1); // Standard TCP
        logMessage(tr("Recommended: Standard TCP for reliability"));
    }
}

// Update transfer statistics display
void GlideMainWindow::updateTransferStats()
{
    if (!m_enhancedTransferManager) return;

    // This is a placeholder - in a full implementation, we would get real stats
    logMessage(tr("Transfer statistics updated"));
}

// Cancel current transfer
void GlideMainWindow::cancelTransfer()
{
    if (m_enhancedTransferManager) {
        m_enhancedTransferManager->cancelTransfer();
        logMessage(tr("Transfer cancelled by user"));
    }

    m_sendFileBtn->setEnabled(true);
    m_sendFileBtn->setText(tr("🚀 Send File"));
    m_transferProgressBar->setVisible(false);
}
