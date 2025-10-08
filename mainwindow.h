#ifndef GLIDEMAINWINDOW_H
#define GLIDEMAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QDateTime> // Added for QDateTime
#include <QMenuBar> // Added for QMenuBar
#include <QTranslator> // Added for QTranslator
#include <QClipboard> // Added for clipboard synchronization
#include <QComboBox> // Added for combo box
#include <QProgressBar> // Added for progress bar
#include "clipboard_sync_manager.h" // Added for clipboard synchronization
#include "device_discovery.h" // Added for device discovery
#include "connectionwizard.h" // Added for connection wizard
#include "file_transfer_improvements.h" // Added for enhanced file transfer

// Forward declarations for enhanced file transfer types
struct NetworkConditions;
struct FileTransferProgress;
enum class TransferMode;
class EnhancedFileTransferManager;

class GlideMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    GlideMainWindow(QWidget *parent = nullptr);
    ~GlideMainWindow();

private slots:
    void startServer();
    void stopServer();
    void connectToServer();
    void disconnectFromServer();
    void updateConnectionStatus();
    void onServerProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onClientProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void showAbout();
    void minimizeToTray();
    void restoreFromTray();
    void browseFile();
    void sendFile();
    void showActivityLog(); // New slot for Activity Log
    void showSettingsDialog(); // New slot for Settings Dialog
    void handleLanguageChange(const QString& langCode); // New slot to handle language changes
    void onClipboardDataReceivedAndSend(const QString& text); // New slot to handle clipboard changes from ClipboardSyncManager
    void onSettingsDialogAccepted(); // New slot to handle settings dialog acceptance
    void onDeviceDiscovered(const DiscoveredDevice& device); // New slot for device discovery
    void onDeviceLost(const QString& ipAddress); // New slot for device lost
    void showConnectionWizard(); // New slot to show connection wizard
    void startDeviceDiscovery(); // New slot to start device discovery
    void stopDeviceDiscovery(); // New slot to stop device discovery

    // Enhanced File Transfer slots
    void sendFileEnhanced();
    void analyzeNetwork();
    void onNetworkAnalysisComplete(const NetworkConditions& conditions);
    void updateTransferProgress(const FileTransferProgress& progress);
    void onTransferFinished(bool success, const QString& filePath);
    void updateTransferModeRecommendation(const NetworkConditions& conditions);
    void updateTransferStats();
    void cancelTransfer();

private:
    void setupUI();
    void retranslateUi(); // Declare retranslateUi
    void setupTrayIcon();
    void createMenus(); // New function to create menus
    void loadSettings();
    void saveSettings();
    void logMessage(const QString& message, const QString& type = "INFO");
    QString getLocalIPAddress();
    void updateUIBasedOnRole(); // New function to update UI based on device role

    // UI Setup Helpers
    void setupServerGroupUI(QHBoxLayout* parentLayout);
    void setupClientGroupUI(QHBoxLayout* parentLayout);
    void setupFileTransferGroupUI(QHBoxLayout* parentLayout);
    void setupLogGroupUI(QVBoxLayout* parentLayout);
    void setupDeviceDiscoveryGroupUI(QHBoxLayout* parentLayout);
    void setupTransferProgressUI(QVBoxLayout* parentLayout);
    
    // UI Elements
    QWidget* m_centralWidget;
    
    // Server Group
    QGroupBox* m_serverGroup;
    QPushButton* m_startServerBtn;
    QPushButton* m_stopServerBtn;
    QLabel* m_serverStatusLabel;
    QLabel* m_connectedClientsLabel;
    
    // Client Group
    QGroupBox* m_clientGroup;
    QLineEdit* m_serverIPEdit;
    QPushButton* m_connectBtn;
    QPushButton* m_disconnectBtn;
    QLabel* m_clientStatusLabel;
    QLabel* m_latencyLabel;
    
    // File Transfer Group
    QGroupBox* m_fileTransferGroup;
    QLineEdit* m_filePathEdit;
    QPushButton* m_browseFileBtn;
    QPushButton* m_sendFileBtn;
    QComboBox* m_transferModeCombo;
    QPushButton* m_analyzeNetworkBtn;
    QLabel* m_networkStatusLabel;
    QProgressBar* m_transferProgressBar;
    QLabel* m_speedLabel;
    QLabel* m_etaLabel;

    // Status and Log
    QTextEdit* m_logTextEdit;
    QLabel* m_localIPLabel;
    
    // System Tray
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // Processes
    QProcess* m_serverProcess;
    QProcess* m_clientProcess;
    
    // Status tracking
    bool m_serverRunning;
    bool m_clientConnected;
    int m_connectedClients;
    
    // Timer for updates
    QTimer* m_statusTimer;
    QTimer* m_latencyTimer;
    
    // Settings
    QSettings* m_settings;
    QTranslator* m_translator; // For dynamic language switching
    ClipboardSyncManager* m_clipboardSyncManager; // Manages clipboard synchronization
    QString m_deviceRole; // Stores the selected device role (Server/Client)

    // Enhanced File Transfer
    EnhancedFileTransferManager* m_enhancedTransferManager;

    // Device Discovery
    DeviceDiscovery* m_deviceDiscovery;
    QList<DiscoveredDevice> m_discoveredDevices;
    QGroupBox* m_deviceDiscoveryGroup;
    QListWidget* m_discoveredDevicesList;
    QPushButton* m_discoverDevicesBtn;
    QPushButton* m_connectionWizardBtn;
    QLabel* m_discoveryStatusLabel;
};

#endif // GLIDEMAINWINDOW_H
