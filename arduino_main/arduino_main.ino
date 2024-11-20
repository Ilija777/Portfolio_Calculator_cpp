String sendMessage;
String receivedMessage;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);       //serielle Transferrate wird auf 9600 gesetzt
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    //receivedMessage = Serial.readString();
    char receivedChar = Serial.read();
    if (receivedChar == '\n') {
      String Result = GetResult(receivedMessage);
      Serial.println(Result);
      receivedMessage = "";  // Reset the received message
    } else {
      receivedMessage += receivedChar;  // Append characters to the received message
    }
  } 
  }
String GetResult(String input_string)
{
  char operation;
  double num1, num2;
  double result;
  int operator_index = -1;
  int string_len = input_string.length(); //Get the length of the string
  //Find the index of the operation in the string of the arithmetic expression  
  for (int i=1; i<=string_len; i++) 
  {
    operation = input_string[i];
    if ((operation == '+') || (operation == '-') || (operation == '*') || (operation == '/'))
    {
      operator_index = i;
      break;
    }
  }
  
  //If the operator was not found, an error message is displayed
  if (operator_index==-1){return "Error: operation not found";}

  //separate the expressions and convert the numbers from string to double
  String s_num1= input_string.substring(0,operator_index);
  String s_num2= input_string.substring(operator_index+1, string_len);
  num1 = s_num1.toDouble();
  num2 = s_num2.toDouble();
  
  //Do the actual calculation
  switch (operation)
  {
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
      if (num2==0){return "Error: divison by 0";}
      result = num1 / num2;
      break;
  }
  String s_result =  String(result,4);  //Long Double in String umwandeln mit 4 Nachkommastellen
  return s_result;
}