#include <QApplication>
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void toggleConnection();
    void sendCalculation();
    void saveLog();
    void checkConnection();
    void exitApplication();
    bool check_input(const QString &input);
    void refreshPorts();

private:
    QSerialPort *serial;
    QPushButton *connectButton;
    QPushButton *sendButton;
    QPushButton *saveLogButton;
    QPushButton *exitButton;
    QPushButton *refreshPortsButton;
    QLineEdit *inputField;
    QTextEdit *logOutput;
    QLabel *inputLabel;
    QComboBox *portSelector;
    QTimer *connectionTimer;
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    serial(new QSerialPort(this))
{
    // Haupt-Widget
    QWidget *centralWidget = new QWidget(this);

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    QHBoxLayout *portLayout = new QHBoxLayout();
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    // Obere Sektion: COM-Port Auswahl
    inputLabel = new QLabel("Select COM Port and Connect:", this);
    portSelector = new QComboBox(this);
    refreshPortsButton = new QPushButton("Refresh Ports", this);
    portLayout->addWidget(portSelector);
    portLayout->addWidget(refreshPortsButton);

    // Log Output und Input Field
    logOutput = new QTextEdit(this);
    logOutput->setReadOnly(true);
    inputField = new QLineEdit(this);
    inputField->setPlaceholderText("Enter operation: a+b | a-b | a*b | a/b");
    inputField->setEnabled(false);

    // Buttons unten
    connectButton = new QPushButton("Connect", this);
    sendButton = new QPushButton("Send", this);
    sendButton->setEnabled(false);
    saveLogButton = new QPushButton("Save Log", this);
    exitButton = new QPushButton("Exit", this);

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addWidget(saveLogButton);
    buttonLayout->addWidget(exitButton);

    // Elemente zum Hauptlayout hinzufügen
    mainLayout->addWidget(inputLabel);
    mainLayout->addLayout(portLayout);
    mainLayout->addWidget(logOutput);
    mainLayout->addWidget(inputField);
    mainLayout->addLayout(buttonLayout);

    setCentralWidget(centralWidget);

    // Signale und Slots verbinden
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendCalculation);
    connect(saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);
    connect(exitButton, &QPushButton::clicked, this, &MainWindow::exitApplication);
    connect(refreshPortsButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);

    // Timer zur Verbindungskontrolle
    connectionTimer = new QTimer(this);
    connect(connectionTimer, &QTimer::timeout, this, &MainWindow::checkConnection);

    refreshPorts();
}

MainWindow::~MainWindow() {}

void MainWindow::refreshPorts()
{
    portSelector->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        portSelector->addItem(port.portName());
    }

    if (ports.isEmpty()) {
        logOutput->append("No COM ports available.");
    } else {
        logOutput->append("COM ports refreshed.");
    }
}

void MainWindow::toggleConnection()
{
    try {
        if (serial->isOpen()) {
            serial->close();
            connectButton->setText("Connect");
            logOutput->append("Disconnected.");
            inputLabel->setText("Please connect to the device first.");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
            connectionTimer->stop();
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
                connectionTimer->start(100);
            } else {
                throw std::runtime_error("Could not connect! Check COM port and if it's already in use.");
            }
        }
    } catch (const std::exception &e) {
        QMessageBox::warning(this, "Connection Error", e.what());
        logOutput->append("Error: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::checkConnection()
{
    if (!serial->isOpen()) {
        connectionTimer->stop();
        logOutput->append("<b>Connection lost. Please reconnect the device.</b>");
        inputField->setEnabled(false);
        sendButton->setEnabled(false);
        connectButton->setText("Connect");
    }
}

bool MainWindow::check_input(const QString &input)
{
    QString sanitizedInput = input;
    sanitizedInput.replace(',', '.');

    QRegularExpression regex("^([-+]?[0-9]*\\.?[0-9]+)\\s*([+\\-*/])\\s*([-+]?[0-9]*\\.?[0-9]+)$");
    QRegularExpressionMatch match = regex.match(sanitizedInput);

    if (match.hasMatch()) {
        double num2 = match.captured(3).toDouble();
        QString op = match.captured(2);

        if (op == "/" && num2 == 0) {
            logOutput->append("Error: Division by zero is not allowed!");
            return false;
        }
        return true;
    } else {
        logOutput->append("Error: Invalid input format! Use a+b | a-b | a*b | a/b.");
        return false;
    }
}

void MainWindow::sendCalculation()
{
    QString calculation = inputField->text();

    if (!check_input(calculation)) {
        return;
    }

    calculation.replace(',', '.');

    if (serial->isOpen() && serial->isWritable()) {
        serial->write((calculation + "\n").toStdString().c_str());
        logOutput->append("Sent: " + calculation);

        QByteArray response;
        while (serial->waitForReadyRead(1000)) {
            response += serial->readAll();
            if (response.endsWith("\n")) {
                break;
            }
        }
        logOutput->append("Response: " + QString(response));
    } else {
        logOutput->append("Error: Not connected!");
    }
}

void MainWindow::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log", "", "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << logOutput->toPlainText();
            file.close();
            logOutput->append("Log saved successfully.");
        } else {
            logOutput->append("Error: Could not save the log.");
        }
    }
}

void MainWindow::exitApplication()
{
    QApplication::quit();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle("µC-Calculator");
    w.resize(500, 400);
    w.show();

    return app.exec();
}
