String sendMessage;
String receivedMessage;

void setup() {
  Serial.begin(9600);  //serielle Transferrate wird auf 9600 gesetzt
}

void loop() {
  // auf serielle Eingabe warten
  while (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if (receivedChar == '\n' || receivedChar == '\r') {  // Prüft auf beides!
      if (receivedMessage.length() > 0) {                // Nur verarbeiten, wenn wirklich etwas empfangen wurde
        String Result = GetResult(receivedMessage);
        Serial.println(Result);
        receivedMessage = "";  // Reset für die nächste Nachricht
      }
    } else if (receivedChar == "") {
      String Result = GetResult(receivedMessage) + "";
      Serial.println(Result);
      receivedMessage = "";  // Zurücksetzen der empfangenen Nachricht
    } else {
      receivedMessage += receivedChar;  // Anhängen von Zeichen an die empfangene Nachricht
    }
  }
}

String GetResult(String input_string) {
  char operation;
  double num1, num2;
  double result;
  int operator_index = -1;
  int string_len = input_string.length();  //Abfrage der Länge der Zeichenkette
  //Suche nach dem Index der Operation in der Zeichenfolge des arithmetischen Ausdrucks
  for (int i = 1; i <= string_len; i++) {
    operation = input_string[i];
    if ((operation == '+') || (operation == '-') || (operation == '*') || (operation == '/')) {
      operator_index = i;
      break;
    }
  }
  //Wenn der Operator nicht gefunden wurde, wird eine Fehlermeldung angezeigt
  if (operator_index == -1) { return "Error: operation not found"; }

  //Die Ausdrücke trennen und die Zahlen von String in Double umwandeln
  String s_num1 = input_string.substring(0, operator_index);
  String s_num2 = input_string.substring(operator_index + 1, string_len);
  num1 = s_num1.toDouble();
  num2 = s_num2.toDouble();

  //Die eigentliche Berechnung durchführen
  switch (operation) {
    case '+':
      result = num1 + num2;
      break;
    case '-':
      result = num1 - num2;
      break;
    case '*':
      result = num1 * num2;
      break;
    case '/':
      if (num2 == 0) { return "Error: divison by 0"; }
      result = num1 / num2;
      break;
  }
  String s_result = String(result, 4);  //Long Double in String umwandeln mit 4 Nachkommastellen
  return s_result;
}


