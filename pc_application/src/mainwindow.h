#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QThread>
#include <QMutex>
#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>
#include <QTimer>
#include <QRegularExpression>

class ConnectionChecker : public QThread
{
    Q_OBJECT

public:
    explicit ConnectionChecker(QSerialPort *serial, QMutex *mutex, QObject *parent = nullptr);
    void stopChecking();
    void setPriority(bool highPriority);

protected:
    void run() override;

signals:
    void connectionStatusChanged(bool isConnected);

private:
    QSerialPort *serial;
    QMutex *mutex;
    bool running;
    bool highPriority;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static void writeErrorLog(const QString &message); // Static function for logging.

private slots:
    void sendCalculation();
    void saveLog();
    void toggleConnection();
    void exitApplication();
    bool check_input(const QString &input);
    void refreshPorts();
    void updateConnectionStatus(bool isConnected);

private:
    QSerialPort *serial;
    QMutex mutex;
    ConnectionChecker *connectionChecker;
    QPushButton *connectButton;
    QPushButton *sendButton;
    QPushButton *saveLogButton;
    QPushButton *exitButton;
    QPushButton *refreshPortsButton;
    QLineEdit *inputField;
    QTextEdit *logOutput;
    QLabel *inputLabel;
    QLabel *statusLED;
    QComboBox *portSelector;
    QTimer *connectionTimer;

    // Connection management variables
    bool isConnected = false;          // Current connection status
    bool manualDisconnection = false;  // Tracks if disconnection is manual
    bool wasConnectedOnce = false;     // Tracks if connection was ever established
    bool connectionLostDialogShown = false; // Verhindert mehrfaches Öffnen
    void updateLED(bool isConnected);
};

#endif // MAINWINDOW_H
