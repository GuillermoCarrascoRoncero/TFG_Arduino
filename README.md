# Código Arduino

En esta parte del sistema se ha desarrollado un programa en la plataforma Arduino IDE. La finalidad de este código es configurar el microcontrolador ESP8266MOD para poder manejar los servomotores del brazo robótico. Dado los requisitos del sistema, también se ha utilizado para el intercambio de mensajes MQTT con la página web y la base de datos. 

## Hardware requerido

Este proyecto solo puede ser ejecutado en una placa ESP8266. Para la correcta ejecución de este código es necesario utilizar un brazo robótico con seis servomotores cuyo accionamiento es mediante señales de ancho de pulso.

## Estructura del código

El código `ControlServoMQTT.ino` es el principal, encargado de controlar tanto los mensajes con el servidor MQTT y la actuación sobre los seis motores del brazo robótico. La clase `WiFiHandler` es la encargada de configurar las credenciales y la conexión a la red inalámbrica utilizada para el funcionamiento del sistema.



