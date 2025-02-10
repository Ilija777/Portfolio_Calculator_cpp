#include "mainwindow.h"
#include <QKeyEvent> //Damit man die Berechnung auch mit der Enter-Taste senden kann!
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
//#include <QProcess>

// ConnectionChecker Implementation

ConnectionChecker::ConnectionChecker(QSerialPort *serial, QObject *parent)
    : QThread(parent), serial(serial), running(true), highPriority(false), processing(false) {}

// Stops the checking process
void ConnectionChecker::stopChecking()
{
    running = false;
}

// Sets the priority for connection checks
void ConnectionChecker::setPriority(bool highPriority)
{
    this->highPriority = highPriority;
}

// Sets the processing flag
void ConnectionChecker::setProcessingFlag(bool processing)
{
    this->processing = processing;
}

// Thread's main function for monitoring connection
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
// **Erkennt die Enter-Taste und sendet die Berechnung**

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

// Updates the connection status
void MainWindow::updateConnectionStatus(bool isConnected)
{
    if (this->isConnected != isConnected) // Nur wenn sich der Status ändert
    {
        this->isConnected = isConnected; // Verbindungsstatus aktualisieren
        updateLED(isConnected);          // LED entsprechend setzen

        if (!isConnected)
        {
            logOutput->append("<b>Connection lost.</b> Button send disabled.");
            connectButton->setText("Connect");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
        }
        else
        {
            logOutput->append("<b>Connected.</b> Button send enabled.");
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

// Additional necessary functions like `sendCalculation`, `refreshPorts`, `toggleConnection`, etc., should be rechecked for any missing updates and called within the required context.

// Sending Calculation
void MainWindow::sendCalculation()
{
    //logOutput->append("<b>Debug:</b> sendCalculation() aufgerufen.");

    QString calculation = inputField->text();
    if (calculation.isEmpty())
    {
        logOutput->append("<b>Error:</b> Input field is empty!");
        return;
    }

    //logOutput->append("<b>Debug:</b> Eingabe erhalten: " + calculation);

    // Eingabe validieren
    if (!check_input(calculation))
    {
        logOutput->append("<b>Error:</b> Invalid input format! Use a+b | a-b | a*b | a/b.");
        return;
    }

    calculation.replace(',', '.');
    calculation += '\n'; // **Newline für Arduino!**
    //logOutput->append("<b>Debug:</b> Sende-Befehl formatiert: " + calculation);

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
void MainWindow::handleEnterPressed() {
//logOutput->append("Debug: Enter-Taste erkannt. Starte sendCalculation().");
    sendCalculation();
}

// void MainWindow::sendCalculation()
// {
//     if (processing)
//         return; // Verarbeitung läuft bereits

//     processing = true; // Verarbeitung starten
//     sendButton->setEnabled(false);
//     QString calculation = inputField->text();

//     if (!check_input(calculation))
//     {
//         logOutput->append("Error: Invalid input format! Use a+b | a-b | a*b | a/b.");
//         processing = false;
//         sendButton->setEnabled(true);
//         return;
//     }

//     calculation.replace(',', '.');
//     if (serial->isOpen() && serial->isWritable())
//     {
//         serial->write((calculation + '\r').toStdString().c_str());
//         serial->flush();
//         logOutput->append("Sent: " + calculation);

//         QByteArray response;
//         while (serial->waitForReadyRead(1000)) // Timeout von 1000ms
//         {
//             response += serial->readAll();
//             if (response.endsWith('\n') || response.endsWith('\r') || response.endsWith("\r\n"))
//                 break;
//         }
//         if (response.isEmpty())
//         {
//             logOutput->append("<b>Warning:</b> No response from µC. Connection might still be active.");
//             writeErrorLog("No response from µC after sending: " + calculation);
//         }
//         else
//         {
//             logOutput->append("Response: " + response);
//         }
//     }
//     else
//     {
//         logOutput->append("<b>Error:</b> Not connected!");
//         updateLED(false);
//         connectButton->setText("Connect");
//     }

//     inputField->clear(); // Eingabefeld leeren
//     processing = false;  // Verarbeitung abgeschlossen
//     sendButton->setEnabled(true);
// }

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
    if (serial->isOpen()) // Verbindung trennen
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

    if (serial->open(QIODevice::ReadWrite))
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

// Save log
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

// check the users input string
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
