#include "device_discovery.h"
#include <QNetworkInterface>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QHostInfo>
#include <QCoreApplication>

DeviceDiscovery* DeviceDiscovery::m_instance = nullptr;

DeviceDiscovery* DeviceDiscovery::instance() {
    if (!m_instance) {
        m_instance = new DeviceDiscovery();
    }
    return m_instance;
}

DeviceDiscovery::DeviceDiscovery(QObject* parent)
    : QObject(parent)
    , m_broadcastSocket(nullptr)
    , m_listenSocket(nullptr)
    , m_discoveryTimer(nullptr)
    , m_broadcastTimer(nullptr)
{
    m_localDeviceName = QHostInfo::localHostName();
    m_localIPAddress = getLocalIPAddress();
    m_localDeviceType = "Unknown";

    // Initialize UDP sockets
    m_broadcastSocket = new QUdpSocket(this);
    m_listenSocket = new QUdpSocket(this);

    // Setup listening socket
    if (!m_listenSocket->bind(BROADCAST_PORT, QUdpSocket::ShareAddress)) {
        qWarning() << "Failed to bind discovery socket to port" << BROADCAST_PORT;
    }

    connect(m_listenSocket, &QUdpSocket::readyRead, this, &DeviceDiscovery::onReadyRead);

    // Setup timers
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    connect(m_discoveryTimer, &QTimer::timeout, this, &DeviceDiscovery::onDiscoveryTimeout);

    m_broadcastTimer = new QTimer(this);
    m_broadcastTimer->setInterval(BROADCAST_INTERVAL);
    connect(m_broadcastTimer, &QTimer::timeout, this, &DeviceDiscovery::broadcastPresence);
}

DeviceDiscovery::~DeviceDiscovery() {
    stopDiscovery();
    if (m_broadcastTimer) {
        m_broadcastTimer->stop();
    }
}

void DeviceDiscovery::startDiscovery() {
    if (m_discoveryTimer->isActive()) {
        return; // Already discovering
    }

    // Clear previous discoveries
    m_discoveredDevices.clear();

    // Send initial broadcast
    sendBroadcastMessage();

    // Start discovery timer
    m_discoveryTimer->start(DISCOVERY_INTERVAL);

    // Start periodic broadcasting
    m_broadcastTimer->start();

    qDebug() << "Started device discovery";
}

void DeviceDiscovery::stopDiscovery() {
    m_discoveryTimer->stop();
    m_broadcastTimer->stop();
    m_discoveredDevices.clear();
    qDebug() << "Stopped device discovery";
}

void DeviceDiscovery::broadcastPresence() {
    sendBroadcastMessage();
}

void DeviceDiscovery::sendBroadcastMessage() {
    if (!m_broadcastSocket) return;

    QJsonObject broadcastData;
    broadcastData["type"] = "GLIDE_DISCOVERY";
    broadcastData["deviceName"] = m_localDeviceName;
    broadcastData["deviceType"] = m_localDeviceType;
    broadcastData["ipAddress"] = m_localIPAddress;
    broadcastData["port"] = BROADCAST_PORT;

    QJsonDocument doc(broadcastData);
    QByteArray data = doc.toJson();

    // Broadcast to all interfaces
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& interface : interfaces) {
        if (interface.flags() & QNetworkInterface::CanBroadcast) {
            QList<QNetworkAddressEntry> entries = interface.addressEntries();
            for (const QNetworkAddressEntry& entry : entries) {
                QHostAddress broadcastAddr = entry.broadcast();
                if (!broadcastAddr.isNull()) {
                    m_broadcastSocket->writeDatagram(data, broadcastAddr, BROADCAST_PORT);
                }
            }
        }
    }
}

void DeviceDiscovery::onReadyRead() {
    while (m_listenSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_listenSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        m_listenSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(datagram, &error);

        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj["type"].toString() == "GLIDE_DISCOVERY") {
                processDiscoveryResponse(sender, obj["deviceName"].toString());
            }
        }
    }
}

void DeviceDiscovery::processDiscoveryResponse(const QHostAddress& sender, const QString& deviceName) {
    // Check if we already know about this device
    for (const DiscoveredDevice& device : m_discoveredDevices) {
        if (device.ipAddress == sender.toString()) {
            return; // Already discovered
        }
    }

    // Add new device
    DiscoveredDevice newDevice;
    newDevice.name = deviceName;
    newDevice.ipAddress = sender.toString();
    newDevice.hostAddress = sender;
    newDevice.port = BROADCAST_PORT;
    newDevice.status = "Available";
    newDevice.deviceType = "Unknown"; // Will be updated when we know more

    m_discoveredDevices.append(newDevice);

    emit deviceDiscovered(newDevice);

    qDebug() << "Discovered device:" << deviceName << "at" << sender.toString();
}

void DeviceDiscovery::onDiscoveryTimeout() {
    emit discoveryFinished();
    qDebug() << "Device discovery finished. Found" << m_discoveredDevices.size() << "devices";
}

QList<DiscoveredDevice> DeviceDiscovery::getDiscoveredDevices() const {
    return m_discoveredDevices;
}

QString DeviceDiscovery::getLocalDeviceName() const {
    return m_localDeviceName;
}

void DeviceDiscovery::setLocalDeviceName(const QString& name) {
    m_localDeviceName = name;
}

QString DeviceDiscovery::getLocalDeviceType() const {
    return m_localDeviceType;
}

void DeviceDiscovery::setLocalDeviceType(const QString& type) {
    m_localDeviceType = type;
}

QString DeviceDiscovery::getLocalIPAddress() {
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address != QHostAddress::LocalHost &&
            address.toIPv4Address() &&
            !address.isLoopback()) {
            return address.toString();
        }
    }
    return QHostAddress(QHostAddress::LocalHost).toString();
}
