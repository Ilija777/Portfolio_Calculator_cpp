#include "mainwindow.h"
#include <QKeyEvent> //Damit man die Berechnung auch mit der Enter-Taste senden kann!
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

// ConnectionChecker Implementation
ConnectionChecker::ConnectionChecker(QSerialPort *serial, QObject *parent)
    : QThread(parent), serial(serial), running(true), highPriority(false), processing(false) {}

// Stoppt den Prüfvorgang
void ConnectionChecker::stopChecking()
{
    running = false;
}

// Legt die Priorität für Verbindungsprüfungen fest
void ConnectionChecker::setPriority(bool highPriority)
{
    this->highPriority = highPriority;
}

// Setzt das Verarbeitungsflag
void ConnectionChecker::setProcessingFlag(bool processing)
{
    this->processing = processing;
}

// Die Hauptfunktion des Threads zur Überwachung der Verbindung
void ConnectionChecker::run()
{
    while (running)
    {
        QThread::msleep(highPriority ? 50 : 100);
        bool isOpen = false;

        try
        {
            MainWindow *mainWindow = static_cast<MainWindow *>(parent());
            if (!mainWindow->isProcessing())
            {
                isOpen = serial->isOpen() && serial->isWritable();
                if (isOpen)
                {
                    QByteArray test = serial->read(1);
                    if (test.isEmpty() && serial->error() != QSerialPort::NoError)
                    {
                        isOpen = false; // Verbindung als getrennt markieren
                    }
                }
            }
            else
            {
                isOpen = true; // Während der Verarbeitung Verbindung als offen annehmen
            }
        }
        catch (...)
        {
            isOpen = false; // Fehlerhafte Verbindung als getrennt markieren
        }

        emit connectionStatusChanged(isOpen); // Sende den Status
    }
}


// MainWindow Implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), serial(new QSerialPort(this)), processing(false), isConnected(false)
{
    // GUI setup
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

    connectionChecker = new ConnectionChecker(serial, this);
    connect(connectionChecker, &ConnectionChecker::connectionStatusChanged, this, &MainWindow::updateConnectionStatus);
    // **Enter-Taste soll senden**
    connect(inputField, &QLineEdit::returnPressed, this, &MainWindow::handleEnterPressed);

    refreshPorts();
    connectionChecker->start();
}


MainWindow::~MainWindow()
{
    connectionChecker->stopChecking();
    connectionChecker->wait();
    delete connectionChecker;
}

// Aktualisiert den Verbindungsstatus
void MainWindow::updateConnectionStatus(bool isConnected)
{
    if (this->isConnected != isConnected) // Nur wenn sich der Status ändert
    {
        this->isConnected = isConnected; // Verbindungsstatus aktualisieren
        updateLED(isConnected);          // LED entsprechend setzen

        if (!isConnected)
        {
            logOutput->append("<b>Warning:</b> Connection lost.");
            connectButton->setText("Connect");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
        }
        else
        {
            logOutput->append("<b>Info:</b> Connected.");
            connectButton->setText("Disconnect");
            inputField->setEnabled(true);
            sendButton->setEnabled(true);
        }
    }
}

bool MainWindow::isProcessing() const
{
    return processing;
}


// Die Eingabe an den µC Senden
void MainWindow::sendCalculation()
{
    QString calculation = inputField->text();
    if (calculation.isEmpty())
    {
        logOutput->append("<b>Error:</b> Input field is empty!");
        return;
    }
    // Eingabe validieren
    if (!check_input(calculation))
    {
        logOutput->append("<b>Error:</b> Invalid input format! Use a+b | a-b | a*b | a/b.");
        return;
    }

    calculation.replace(',', '.');
    calculation += '\n'; // **Newline für Arduino!**


    if (serial->isOpen() && serial->isWritable())
    {
        serial->write(calculation.toUtf8());
        serial->flush();
        logOutput->append("<b>Sent:</b> " + calculation);

        QByteArray response;
        while (serial->waitForReadyRead(1000))
        {
            response += serial->readAll();
            if (response.endsWith('\n') || response.endsWith('\r') || response.endsWith("\r\n"))
                break;
        }

        if (response.isEmpty())
        {
            logOutput->append("<b><font color='red'>Warning:</font></b> No response received!");
        }
        else
        {
            logOutput->append("<b>Response:</b> " + QString::fromUtf8(response.trimmed()));
        }
    }
    else
    {
        logOutput->append("<b>Error:</b> Serial port not available!");
        updateLED(false);
        connectButton->setText("Connect");
    }
}


//Damit man auch mit Enter-Taste die Berechnung senden kann
void MainWindow::handleEnterPressed() {
    if (!isConnected)
    {
        logOutput->append("<b>Error:</b> Please connect first!");
        return;
    }
    sendCalculation();
}

// Refresh available ports
void MainWindow::refreshPorts()
{
    portSelector->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports)
    {
        portSelector->addItem(port.portName());
    }
    logOutput->append(ports.isEmpty() ? "No COM ports available." : "COM ports refreshed.");
}

// Toggle Connection
void MainWindow::toggleConnection()
{
    if (serial->isOpen()) // Verbindung trennen wenn verbunden
    {
        manualDisconnection = true;
        serial->close();
        updateConnectionStatus(false); // Verbindung als getrennt melden
        return;
    }

    QString selectedPort = portSelector->currentText();
    if (selectedPort.isEmpty())
    {
        QMessageBox::warning(this, "Connection Error", "No COM port selected.");
        return;
    }

    serial->setPortName(selectedPort);
    serial->setBaudRate(QSerialPort::Baud9600);
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) // Verbindung herstellen
    {
        manualDisconnection = false;
        logOutput->append("Connected to " + selectedPort + ".");
        updateConnectionStatus(true);
    }
    else
    {
        QMessageBox::warning(this, "Connection Error", "Could not open the selected COM port.");
    }
}

// Update LED
void MainWindow::updateLED(bool isConnected)
{
    statusLED->setStyleSheet(isConnected ? "background-color: green; border-radius: 10px;" : "background-color: red; border-radius: 10px;");
}

// Speichert die Kommunikationhistorie in eine Text-Datei
void MainWindow::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log", "", "Text Files (*.txt);;All Files (*)");
    if (fileName.isEmpty())
        return;

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

// Überprüft die Eingabe des Benutzers
bool MainWindow::check_input(const QString &input)
{
    try
    {
        QString sanitizedInput = input;
        sanitizedInput.replace(',', '.'); // Ersetze Kommas durch Punkte

        // Überprüfe das Eingabeformat: <Zahl><Operator><Zahl>
        QRegularExpression regex("^([-+]?[0-9]*\\.?[0-9]+)\\s*([+\\-*/])\\s*([-+]?[0-9]*\\.?[0-9]+)$");
        QRegularExpressionMatch match = regex.match(sanitizedInput);

        if (match.hasMatch())
        {
            // Überprüfe Division durch Null
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
            return false; // Eingabe ungültig
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

//Schreibt in die Log-Datei falls Fehler auftreten
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

// Exit Application
void MainWindow::exitApplication()
{
    QApplication::quit();
}
