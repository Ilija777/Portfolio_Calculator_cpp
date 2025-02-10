#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QThread>
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
// #include <QKeyEvent>

// Klasse zum Überprüfen der Verbindung
class ConnectionChecker : public QThread
{
    Q_OBJECT

public:
    explicit ConnectionChecker(QSerialPort *serial, QObject *parent = nullptr); // Konstruktor ohne Mutex
    void stopChecking();                                                        // Beendet den Thread
    void setPriority(bool highPriority);                                        // Setzt die Priorität für die Überprüfung
    void setProcessingFlag(bool processing);                                    // Setzt den "processing"-Status

protected:
    void run() override; // Hauptfunktion des Threads

signals:
    void connectionStatusChanged(bool isConnected); // Signal, wenn sich der Verbindungsstatus ändert

private:
    QSerialPort *serial; // Zeiger auf das serielle Gerät
    bool running;        // Kontrolliert den Thread
    bool highPriority;   // Bestimmt die Häufigkeit der Überprüfung
    bool processing;     // Gibt an, ob eine Berechnung aktiv ist
};

// Hauptklasse für die Anwendung
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static void writeErrorLog(const QString &message); // Funktion zum Schreiben von Fehlermeldungen in eine Datei
    bool isProcessing() const;                         // Getter für den "processing"-Status
private slots:
    void sendCalculation();                        // Funktion zum Senden von Berechnungen
    void saveLog();                                // Funktion zum Speichern des Logs
    void toggleConnection();                       // Verbindung herstellen oder trennen
    void exitApplication();                        // Beendet die Anwendung
    bool check_input(const QString &input);        // Überprüft die Eingabe auf Gültigkeit
    void refreshPorts();                           // Aktualisiert die Liste der verfügbaren Ports
    void updateConnectionStatus(bool isConnected); // Aktualisiert den Verbindungsstatus
    void handleEnterPressed();                     // Enter zum "Senden"
private:
    QSerialPort *serial;                  // Serielles Gerät
    bool processing = false;              // Status, ob aktuell eine Aktion ausgeführt wird
    ConnectionChecker *connectionChecker; // Zeiger auf den Verbindungsprüfer
    QPushButton *connectButton;           // Verbindungsbutton
    QPushButton *sendButton;              // Senden-Button
    QPushButton *saveLogButton;           // Log speichern
    QPushButton *exitButton;              // Exit-Button
    QPushButton *refreshPortsButton;      // Ports aktualisieren
    QLineEdit *inputField;                // Eingabefeld für Berechnungen
    QTextEdit *logOutput;                 // Anzeige des Logs
    QLabel *inputLabel;                   // Label für die Eingabe
    QLabel *statusLED;                    // LED-Statusanzeige
    QComboBox *portSelector;              // Auswahlfeld für die Ports
    QTimer *connectionTimer;              // Timer für die regelmäßige Überprüfung der Verbindung

    // Variablen zur Verbindungsverwaltung
    bool isConnected = false;               // Aktueller Verbindungsstatus
    bool manualDisconnection = false;       // Gibt an, ob die Trennung manuell war
    bool wasConnectedBefore = false;        // Gibt an, ob jemals eine Verbindung bestand
    bool connectionLostDialogShown = false; // Verhindert mehrfaches Öffnen des Verbindungsverlustdialogs

    void updateLED(bool isConnected); // Aktualisiert die LED-Anzeige je nach Verbindungsstatus
};

#endif // MAINWINDOW_H
