#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    void sendCalculation();
    void saveLog();
    void toggleConnection();
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
    QLabel *statusLED;
    QComboBox *portSelector;
    QTimer *connectionTimer;

private:
    bool isConnected = false;
    bool wasConnectedOnce = false;
    void updateLED(bool isConnected);
};

#endif // MAINWINDOW_H
