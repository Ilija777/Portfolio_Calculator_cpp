#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QSerialPort>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QFileDialog>

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

private:
    QSerialPort *serial;
    QPushButton *connectButton;
    QPushButton *sendButton;
    QLineEdit *inputField;
    QTextEdit *logOutput;
    QLabel *inputLabel;         // Label for the input field
    QPushButton *saveLogButton; // Neuer Button zum Speichern des Logs
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          serial(new QSerialPort(this))
{
    // Set the main widget and layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Create UI elements
    inputLabel = new QLabel("Please connect to the device first.", this); // Initial label
    connectButton = new QPushButton("Connect", this);
    sendButton = new QPushButton("Send", this);
    saveLogButton = new QPushButton("Save Log", this);
    inputField = new QLineEdit(this);
    logOutput = new QTextEdit(this);
    logOutput->setReadOnly(true); // Make the output field read-only

    // Add UI elements to the layout
    layout->addWidget(inputLabel); // Label above the input field
    layout->addWidget(inputField);
    layout->addWidget(connectButton);
    layout->addWidget(sendButton);
    layout->addWidget(saveLogButton);
    layout->addWidget(logOutput);

    // Set the layout as the central widget of the main window
    setCentralWidget(centralWidget);

    // Initially disable the input field and send button until connected
    inputField->setEnabled(false);
    sendButton->setEnabled(false);

    // Connect or disconnect when the button is clicked
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::toggleConnection);

    // Send the calculation when the send button is clicked
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::sendCalculation);

    // Save the log when the save log button is clicked
    connect(saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);
}

MainWindow::~MainWindow()
{
}

// Funktion um die Verbindung zu dem Mikrocontroller aufzubauen/trennen
void MainWindow::toggleConnection()
{
    try
    {
        if (serial->isOpen())
        {
            // Disconnect
            serial->close();
            connectButton->setText("Connect");
            logOutput->append("Disconnected.");

            // Update the label and disable the input field
            inputLabel->setText("Please connect to the device first.");
            inputField->setEnabled(false);
            sendButton->setEnabled(false);
        }
        else
        {
            // Attempt to connect
            serial->setPortName("COM3");                // Adjust the COM port to your system
            serial->setBaudRate(QSerialPort::Baud9600); // Make sure that Baudrate on the µC is the same
            serial->setDataBits(QSerialPort::Data8);
            serial->setParity(QSerialPort::NoParity);
            serial->setStopBits(QSerialPort::OneStop);
            serial->setFlowControl(QSerialPort::NoFlowControl);

            if (serial->open(QIODevice::ReadWrite))
            {
                connectButton->setText("Disconnect");
                logOutput->append("Successfully connected.");

                // Update the label and enable the input field
                inputLabel->setText("Please enter the operation to perform. Format: a+b | a-b | a*b | a/b");
                inputField->setEnabled(true);
                sendButton->setEnabled(true);
            }
            else
            {
                throw std::runtime_error("Could not connect! Check COM port and if it's already in use.");
            }
        }
    }
    catch (const std::exception &e)
    {
        // Handle exceptions and display an error message
        logOutput->append("Error: " + QString::fromStdString(e.what()));
    }
}
void MainWindow::sendCalculation()
{
    if (serial->isOpen() && serial->isWritable())
    {
        QString calculation = inputField->text() + "\n"; // Append a newline character for Arduino
        serial->write(calculation.toStdString().c_str());
        logOutput->append("Sent: " + calculation);

        // Wait for a response from the microcontroller
        QByteArray response;
        while (serial->waitForReadyRead(1000))
        {
            response += serial->readAll();
            if (response.endsWith("\n"))
            {
                break; // Response is complete when a newline character is received
            }
        }

        logOutput->append("Response: " + QString(response));
    }
    else
    {
        logOutput->append("Error: Not connected!"); // Error message if not connected
    }
}
// Funktion zum Speichern des Logs
void MainWindow::saveLog()
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
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.setWindowTitle("µC-Calculator");
    w.resize(400, 300);
    w.show();

    return app.exec();
}