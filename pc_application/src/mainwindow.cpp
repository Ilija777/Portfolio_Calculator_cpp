#include "mainwindow.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

// Hilfsfunktion zum Schreiben in die Log-Datei
void writeErrorLog(const QString &message)
{
    QFile logFile(QDir::currentPath() + "/error_log.txt");
    if (logFile.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << " - " << message << "\n";
        logFile.close();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      serial(new QSerialPort(this))
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

        refreshPorts();
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

MainWindow::~MainWindow() {}

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
            logOutput->append("Error: Invalid input format! Use a+b | a-b | a*b | a/b.");
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
void MainWindow::toggleConnection() {
    try {
        if (serial->isOpen()) {
            serial->close();
            connectButton->setText("Connect");
            logOutput->append("Disconnected.");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
            updateLED(false);
            isConnected = false;
        } else {
            QString selectedPort = portSelector->currentText();
            if (selectedPort.isEmpty()) {
                throw std::runtime_error("No COM port selected.");
            }

            serial->setPortName(selectedPort);
            serial->setBaudRate(QSerialPort::Baud9600);
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);

            if (serial->open(QIODevice::ReadWrite)) {
                connectButton->setText("Disconnect");
                logOutput->append("Connected to " + selectedPort + ".");
                inputField->setEnabled(true);
                sendButton->setEnabled(true);
                updateLED(true);
                isConnected = true;
                wasConnectedOnce = true;  // Verbindung wurde erfolgreich hergestellt
            } else {
                throw std::runtime_error("Could not connect! Check COM port and if it's already in use.");
            }
        }
    } catch (const std::exception &e) {
        writeErrorLog("toggleConnection error: " + QString::fromStdString(e.what()));
        QMessageBox::warning(this, "Connection Error", e.what());
        logOutput->append("Error: " + QString::fromStdString(e.what()));
    }
}

// Berechnung senden
void MainWindow::sendCalculation() {
    try {
        QString calculation = inputField->text();
        if (!check_input(calculation)) return;

        calculation.replace(',', '.');

        if (serial->isOpen() && serial->isWritable()) {
            serial->write((calculation + "\n").toStdString().c_str());
            logOutput->append("Sent: " + calculation);

            QByteArray response;
            while (serial->waitForReadyRead(1000)) {
                response += serial->readAll();
                if (response.endsWith("\n")) break;
            }

            if (response.isEmpty()) {
                logOutput->append("Response: No response from µC.");
                writeErrorLog("No response from µC after sending: " + calculation);
            } else {
                logOutput->append("Response: " + QString(response));
            }
        } else {
            logOutput->append("Error: Not connected!");
        }
    } catch (const std::exception &e) {
        writeErrorLog("sendCalculation error: " + QString::fromStdString(e.what()));
    } catch (...) {
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

// LED Indikator
void MainWindow::updateLED(bool isConnected)
{
    try
    {
        statusLED->setStyleSheet(isConnected ? "background-color: green; border-radius: 10px;"
                                             : "background-color: red; border-radius: 10px;");
    }
    catch (const std::exception &e)
    {
        writeErrorLog("updateLED error: " + QString::fromStdString(e.what()));
    }
    catch (...)
    {
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
