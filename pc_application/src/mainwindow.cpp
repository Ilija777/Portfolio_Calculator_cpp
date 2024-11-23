#include "mainwindow.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ConnectionChecker Implementation

ConnectionChecker::ConnectionChecker(QSerialPort *serial, QMutex *mutex, QObject *parent)
    : QThread(parent), serial(serial), mutex(mutex), running(true), highPriority(false) {}

void ConnectionChecker::stopChecking()
{
    try
    {
        running = false;
    }
    catch (const std::exception &e)
    {
        MainWindow::writeErrorLog("ConnectionChecker::stopChecking error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        MainWindow::writeErrorLog("Unknown error in ConnectionChecker::stopChecking.");
    }
}

void ConnectionChecker::setPriority(bool highPriority)
{
    try
    {
        this->highPriority = highPriority;
    }
    catch (const std::exception &e)
    {
        MainWindow::writeErrorLog("ConnectionChecker::setPriority error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        MainWindow::writeErrorLog("Unknown error in ConnectionChecker::setPriority.");
    }
}
void ConnectionChecker::run()
{
    try
    {
        while (running)
        {
            QThread::msleep(100);
            bool isOpen = false;

            mutex->lock();
            try
            {
                isOpen = serial->isOpen() && serial->isWritable();
            }
            catch (const std::exception &e)
            {
                MainWindow::writeErrorLog("ConnectionChecker::run error: " + QString::fromStdString(e.what()));
            }
            catch (...)
            {
                MainWindow::writeErrorLog("Unknown error in ConnectionChecker::run.");
            }
            mutex->unlock();
            emit connectionStatusChanged(isOpen);
        }
    }
    catch (const std::exception &e)
    {
        MainWindow::writeErrorLog("ConnectionChecker::run error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        MainWindow::writeErrorLog("Unknown error in ConnectionChecker::run.");
    }
}

// MainWindow Implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), serial(new QSerialPort(this)), isConnected(false)
{
    try
    {
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
        QHBoxLayout *portLayout = new QHBoxLayout();
        QHBoxLayout *buttonLayout = new QHBoxLayout();

        inputLabel = new QLabel("Select COM Port and Connect:", this);
        portSelector = new QComboBox(this);
        refreshPortsButton = new QPushButton("Refresh Ports", this);
        portLayout->addWidget(portSelector);
        portLayout->addWidget(refreshPortsButton);
        logOutput = new QTextEdit(this);
        logOutput->setReadOnly(true);
        inputField = new QLineEdit(this);
        inputField->setPlaceholderText("Enter operation: a+b | a-b | a*b | a/b");
        inputField->setEnabled(false);

        connectButton = new QPushButton("Connect", this);
        sendButton = new QPushButton("Send", this);
        sendButton->setEnabled(false);
        saveLogButton = new QPushButton("Save Log", this);
        exitButton = new QPushButton("Exit", this);
        buttonLayout->addWidget(connectButton);
        buttonLayout->addWidget(sendButton);
        buttonLayout->addWidget(saveLogButton);
        buttonLayout->addWidget(exitButton);
        statusLED = new QLabel(this);
        statusLED->setFixedSize(20, 20);
        statusLED->setStyleSheet("background-color: red; border-radius: 10px;");

        mainLayout->addWidget(statusLED);
        mainLayout->addWidget(inputLabel);
        mainLayout->addLayout(portLayout);
        mainLayout->addWidget(logOutput);
        mainLayout->addWidget(inputField);
        mainLayout->addLayout(buttonLayout);

        setCentralWidget(centralWidget);

        connect(connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
        connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendCalculation);
        connect(saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);
        connect(exitButton, &QPushButton::clicked, this, &MainWindow::exitApplication);
        connect(refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);

        connectionChecker = new ConnectionChecker(serial, &mutex, this);
        connect(connectionChecker, &ConnectionChecker::connectionStatusChanged, this, &MainWindow::updateConnectionStatus);

        refreshPorts();
        connectionChecker->start();
    }
    catch (const std::exception &e)
    {
        writeErrorLog("Initialization error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error during initialization.");
    }
}

void MainWindow::writeErrorLog(const QString &message)
{
    QFile logFile(QDir::currentPath() + "/error_log.txt");
    if (logFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << " - " << message << "\n";
        logFile.close();
    }
}



MainWindow::~MainWindow()
{
    connectionChecker->stopChecking();
    connectionChecker->wait();
    delete connectionChecker;
}

bool MainWindow::check_input(const QString &input)
{
    try
    {
        QString sanitizedInput = input;
        sanitizedInput.replace(',', '.');

        QRegularExpression regex("^([-+]?[0-9]*\\.?[0-9]+)\\s*([+\\-*/])\\s*([-+]?[0-9]*\\.?[0-9]+)$");
        QRegularExpressionMatch match = regex.match(sanitizedInput);

        if (match.hasMatch())
        {
            double num2 = match.captured(3).toDouble();
            QString op = match.captured(2);

            if (op == "/" && num2 == 0)
            {
                logOutput->append("Error: Division by zero is not allowed!");
                return false;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    catch (const std::exception &e)
    {
        writeErrorLog("check_input error: " + QString::fromStdString(e.what()));
        return false;
    }
    catch (...)
    {
        writeErrorLog("Unknown error in check_input.");
        return false;
    }
}


// Ports aktualisieren
void MainWindow::refreshPorts()
{
    try
    {
        portSelector->clear();
        const auto ports = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &port : ports)
        {
            portSelector->addItem(port.portName());
        }

        logOutput->append(ports.isEmpty() ? "No COM ports available." : "COM ports refreshed.");

        // Falls vorher keine Verbindung war, keine unnötige Meldung über Verbindungsverlust
        if (ports.isEmpty() && wasConnectedOnce)
        {
            logOutput->append("<b>Connection lost or no COM ports available.</b>");
        }
    }
    catch (const std::exception &e)
    {
        writeErrorLog("refreshPorts error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error in refreshPorts.");
    }
}

// Verbindung herstellen oder trennen
void MainWindow::toggleConnection()
{
    try
    {
        if (serial->isOpen())
        {
            manualDisconnection = true;  // Mark as manual disconnection
            serial->close();
            connectButton->setText("Connect");
            logOutput->append("Disconnected.");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
            updateLED(false);
            connectionChecker->stopChecking();  // Stop the connection checker
            isConnected = false;
        }
        else
        {
            QString selectedPort = portSelector->currentText();
            if (selectedPort.isEmpty())
            {
                throw std::runtime_error("No COM port selected.");
            }

            serial->setPortName(selectedPort);
            serial->setBaudRate(QSerialPort::Baud9600);
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);

            if (serial->open(QIODevice::ReadWrite))
            {
                connectButton->setText("Disconnect");
                logOutput->append("Connected to " + selectedPort + ".");
                inputField->setEnabled(true);
                sendButton->setEnabled(true);
                updateLED(true);
                isConnected = true;
                manualDisconnection = false; // Reset manual disconnection flag

                // Start connection checker
                connectionChecker = new ConnectionChecker(serial, &mutex, this);
                connect(connectionChecker, &ConnectionChecker::connectionStatusChanged, this, &MainWindow::updateConnectionStatus);
                connectionChecker->start();
            }
            else
            {
                throw std::runtime_error("Could not connect! Check COM port and if it's already in use.");
            }
        }
    }
    catch (const std::exception &e)
    {
        writeErrorLog("toggleConnection error: " + QString::fromStdString(e.what()));
        QMessageBox::warning(this, "Connection Error", e.what());
        logOutput->append("Error: " + QString::fromStdString(e.what()));
    }
}

// Berechnung senden
void MainWindow::sendCalculation()
{
    try
    {
        QString calculation = inputField->text();
        // Prüfen, ob die Eingabe gültig ist
        if (!check_input(calculation)) 
        {
            logOutput->append("Error: Invalid input format! Use a+b | a-b | a*b | a/b.");
            return; // Sofort beenden, wenn die Eingabe ungültig ist
        }
        mutex.lock();
        calculation.replace(',', '.');

        if (serial->isOpen() && serial->isWritable())
        {
            serial->write((calculation + '\n').toStdString().c_str());
            logOutput->append("Sent: " + calculation);

            QByteArray response;
            while (serial->waitForReadyRead(1000))
            {
                response += serial->readAll();
                if (response.endsWith('\n'))
                    break;
            }

            if (response.isEmpty())
            {
                logOutput->append("Response: No response from µC.");
                writeErrorLog("No response from µC after sending: " + calculation);
            }
            else
            {
                logOutput->append("Response: " + response);
            }
        }
        else
        {
            logOutput->append("Error: Not connected!");
        }
        mutex.unlock();
    }
    catch (const std::exception &e)
    {
        writeErrorLog("sendCalculation error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error in sendCalculation.");
    }
}

// Speichere das Log
void MainWindow::saveLog()
{
    try
    {
        QString fileName = QFileDialog::getSaveFileName(this, "Save Log", "", "Text Files (*.txt);;All Files (*)");
        if (!fileName.isEmpty())
        {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&file);
                out << logOutput->toPlainText();
                file.close();
                logOutput->append("Log saved successfully.");
            }
            else
            {
                logOutput->append("Error: Could not save the log.");
            }
        }
    }
    catch (const std::exception &e)
    {
        writeErrorLog("saveLog error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error in saveLog.");
    }
}
// Aktualisiert den Status des LED-Indikators basierend auf dem Verbindungsstatus
void MainWindow::updateLED(bool isConnected)
{
    try
    {
        // Speichere den Verbindungsstatus intern, um ihn in der Anwendung zu verwenden
        this->isConnected = isConnected;

        // Wenn die Verbindung aktiv ist:
        if (isConnected)
        {
            // Setze die LED auf grün, um anzuzeigen, dass die Verbindung aktiv ist
            statusLED->setStyleSheet("background-color: green; border-radius: 10px;");
        }
        else
        {
            // Setze die LED auf rot, um anzuzeigen, dass die Verbindung unterbrochen wurde
            statusLED->setStyleSheet("background-color: red; border-radius: 10px;");

            // Log-Ausgabe: Verbindung verloren, Benutzer auffordern, das Gerät erneut zu verbinden
            
            // Wenn die Verbindung aktiv ist:
            if (isConnected)
            {
                logOutput->append("<b>Connection lost. Please reconnect the device.</b>");
            }
                
        }
    }
    catch (const std::exception &e)
    {
        // Fehler beim Aktualisieren des LED-Indikators: Schreibe eine Fehlerbeschreibung in die Log-Datei
        writeErrorLog("updateLED error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        // Unerwarteter Fehler beim Aktualisieren des LED-Indikators: Schreibe allgemeinen Fehler in die Log-Datei
        writeErrorLog("Unknown error in updateLED.");
    }
}


// Anwendung beenden
void MainWindow::exitApplication()
{
    try
    {
        QApplication::quit();
    }
    catch (const std::exception &e)
    {
        writeErrorLog("exitApplication error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error in exitApplication.");
    }
}


void MainWindow::updateConnectionStatus(bool isConnected)
{
    try
    {
        this->isConnected = isConnected;

        if (isConnected)
        {
            // Verbindung aktiv: Setze die LED auf grün und setze den Dialog-Status zurück
            updateLED(true);
            connectionLostDialogShown = false; // Reset, da die Verbindung wiederhergestellt wurde
        }
        else
        {
            // Verbindung verloren: Setze die LED auf rot
            updateLED(false);

            // Zeige nur einmal den Verbindungsverlust-Dialog und schreibe ins Log
            if (!manualDisconnection && !connectionLostDialogShown && wasConnectedOnce)
            {
                connectionLostDialogShown = true; // Markiere Dialog als gezeigt
                QMessageBox::critical(this, "Connection Error", "Connection lost. Please reconnect the device.");
                logOutput->append("Connection lost unexpectedly. Please reconnect the device."); // Schreibe ins Log
                writeErrorLog("Unexpected connection loss detected."); // Schreibe in die Log-Datei
            }
        }
    }
    catch (const std::exception &e)
    {
        writeErrorLog("updateConnectionStatus error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
        writeErrorLog("Unknown error in updateConnectionStatus.");
    }
}
