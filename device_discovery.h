#ifndef DEVICE_DISCOVERY_H
#define DEVICE_DISCOVERY_H

#include <QString>
#include <QList>
#include <QTimer>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUdpSocket>
#include <QNetworkInterface>

struct DiscoveredDevice {
    QString name;
    QString ipAddress;
    QString deviceType; // "Server" or "Client"
    int port;
    QString status; // "Available", "Connected", "Busy"
    QHostAddress hostAddress;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["ipAddress"] = ipAddress;
        obj["deviceType"] = deviceType;
        obj["port"] = port;
        obj["status"] = status;
        return obj;
    }

    static DiscoveredDevice fromJson(const QJsonObject& obj) {
        DiscoveredDevice device;
        device.name = obj["name"].toString();
        device.ipAddress = obj["ipAddress"].toString();
        device.deviceType = obj["deviceType"].toString();
        device.port = obj["port"].toInt();
        device.status = obj["status"].toString();
        device.hostAddress = QHostAddress(device.ipAddress);
        return device;
    }
};

class DeviceDiscovery : public QObject {
    Q_OBJECT

public:
    static DeviceDiscovery* instance();
    ~DeviceDiscovery();

    void startDiscovery();
    void stopDiscovery();
    void broadcastPresence();
    QList<DiscoveredDevice> getDiscoveredDevices() const;

    QString getLocalDeviceName() const;
    void setLocalDeviceName(const QString& name);
    QString getLocalDeviceType() const;
    void setLocalDeviceType(const QString& type);

signals:
    void deviceDiscovered(const DiscoveredDevice& device);
    void deviceLost(const QString& ipAddress);
    void discoveryFinished();

private slots:
    void onDiscoveryTimeout();
    void onReadyRead();

private:
    explicit DeviceDiscovery(QObject* parent = nullptr);
    static DeviceDiscovery* m_instance;

    void sendBroadcastMessage();
    void processDiscoveryResponse(const QHostAddress& sender, const QString& message);
    QString getLocalIPAddress();

    QUdpSocket* m_broadcastSocket;
    QUdpSocket* m_listenSocket;
    QTimer* m_discoveryTimer;
    QTimer* m_broadcastTimer;

    QList<DiscoveredDevice> m_discoveredDevices;
    QString m_localDeviceName;
    QString m_localDeviceType;
    QString m_localIPAddress;

    static const int BROADCAST_PORT = 45455; // Different from main UDP port
    static const int DISCOVERY_INTERVAL = 5000; // 5 seconds
    static const int BROADCAST_INTERVAL = 30000; // 30 seconds
};

#endif // DEVICE_DISCOVERY_H
